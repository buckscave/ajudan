/* ========================================================================== *
 * AJUDAN 3.0 - Basisdata                                                     *
 * -------------------------------------------------------------------------- *
 * C89, POSIX, SQLITE3                                                        *
 * -------------------------------------------------------------------------- *
 * basisdata.c                                                                *
 * Penulis: Chandra Lesmana                                                   *
 * ========================================================================== */

#include "ajudan.h"

/* ========================================================================== *
 *                                KONNSTANTA                                  *
 * ========================================================================== */

#define NAMA_BASISDATA "basisdata.db"

/* ========================================================================== *
 *                          KONFIGURASI & LOG INTERNAL                        *
 * ========================================================================== */

static void log_init_env(void) {
    const char *env = getenv("BOT_LOG");
    g_log_on = (env && env[0] == '1') ? 1 : 0;
}

/* ========================================================================== *
 *                               UTILITAS UMUM                                *
 * ========================================================================== */

/* Parser CSV kuat: dukung kutip ganda, escape, dan koma di dalam teks. */
int parse_baris_csv_kuat(char *baris, char *kolom[], int max_kolom) {
    int count = 0, in_quotes = 0;
    char *start = baris, *p = baris, *dst = baris;
    if (!baris || !kolom || max_kolom <= 0) return 0;
    while (*p) {
        if (in_quotes) {
            if (*p == '"') {
                if (*(p+1) == '"') { *dst++ = '"'; p += 2; continue; }
                else { in_quotes = 0; p++; continue; }
            } else { *dst++ = *p++; continue; }
        } else {
            if (*p == '"') { in_quotes = 1; p++; continue; }
            else if (*p == ',') {
                *dst = '\0';
                if (count < max_kolom) kolom[count++] = start;
                dst++; p++; start = dst;
                continue;
            } else { *dst++ = *p++; }
        }
    }
    *dst = '\0';
    if (count < max_kolom) kolom[count++] = start;
    /* Bersihkan kutip & spasi */
    {
        int i;
        for (i = 0; i < count; i++) {
            int len = (int)strlen(kolom[i]);
            if (len > 1 && kolom[i][0] == '"' && kolom[i][len-1] == '"') {
                kolom[i][len-1] = '\0';
                kolom[i] = kolom[i] + 1;
            }
            trim(kolom[i]);
        }
    }
    return count;
}

/* ========================================================================== *
 *                            KONEKSI & EKSEKUSI DB                           *
 * ========================================================================== */

int buka_basisdata(sqlite3 **db) {
    int rc;
    log_init_env();
    rc = sqlite3_open(NAMA_BASISDATA, db);
    if (rc != SQLITE_OK) {
        if (*db) {
            fprintf(stderr, "Error buka DB: %s\n", sqlite3_errmsg(*db));
            sqlite3_close(*db);
        }
        return -1;
    }
    sqlite3_busy_timeout(*db, 2500);
    sqlite3_exec(*db, "PRAGMA journal_mode = DELETE;", NULL, NULL, NULL);
    sqlite3_exec(*db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(*db, "PRAGMA temp_store = MEMORY;", NULL, NULL, NULL);
    sqlite3_exec(*db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    sqlite3_exec(*db, "PRAGMA optimize;", NULL, NULL, NULL);
    ajudan_logf("Koneksi DB dibuka: %s", NAMA_BASISDATA, NULL, NULL);
    return 0;
}

int tutup_basisdata(sqlite3 *db) {
    int rc;
    if (!db) return 0;
    rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error tutup DB: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    ajudan_logf("Koneksi DB ditutup: %s", NAMA_BASISDATA, NULL, NULL);
    return 0;
}

int eksekusi_sql(sqlite3 *db, const char *sql, InfoKesalahan *error) {
    char *err = NULL;
    int rc;
    if (!db || !sql) return -1;
    rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        if (error) {
            error->kode = rc;
            snprintf(error->pesan, sizeof(error->pesan), 
                    "SQL error: %s", err ? err : "unknown");
        }
        if (err) sqlite3_free(err);
        return -1;
    }
    if (err) sqlite3_free(err);
    return 0;
}

void cetak_error(const InfoKesalahan *error) {
    if (!error) return;
    fprintf(stderr, "Error (kode: %d): %s\n", error->kode, error->pesan);
}

/* ========================================================================== *
 *                          SKEMA LENGKAP TERBARUKAN                          *
 * ========================================================================== */

/* Kelompok Kata */
int buat_tabel_kata(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS kata ("
        "id INTEGER PRIMARY KEY, "
        "kata TEXT NOT NULL UNIQUE, "
        "kelas_id INTEGER, "
        "ragam_id INTEGER, "
        "bidang_id INTEGER"
        ");";
    ajudan_logf("Buat tabel: %s", "kata", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_arti_kata(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS arti_kata ("
        "id INTEGER PRIMARY KEY, "
        "id_kata INTEGER NOT NULL, "
        "arti TEXT NOT NULL, "
        "FOREIGN KEY(id_kata) REFERENCES kata(id) ON DELETE CASCADE"
        ");";
    ajudan_logf("Buat tabel: %s", "arti_kata", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_semantik_kata(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS semantik_kata ("
        "id INTEGER PRIMARY KEY, "
        "id_kata_1 INTEGER NOT NULL, "
        "id_kata_2 INTEGER NOT NULL, "
        "id_semantik INTEGER, "
        "id_ragam INTEGER, "
        "id_bidang INTEGER, "
        "id_hubungan TEXT, "
        "FOREIGN KEY(id_kata_1) REFERENCES kata(id), "
        "FOREIGN KEY(id_kata_2) REFERENCES kata(id)"
        ");";
    ajudan_logf("Buat tabel: %s", "semantik_kata", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_morfologi_kata(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS morfologi_kata ("
        "id INTEGER PRIMARY KEY, "
        "id_kata_dasar INTEGER NOT NULL, "
        "id_kata_bentuk INTEGER NOT NULL, "
        "id_morfologi INTEGER, "
        "id_hubungan TEXT, "
        "FOREIGN KEY(id_kata_dasar) REFERENCES kata(id), "
        "FOREIGN KEY(id_kata_bentuk) REFERENCES kata(id)"
        ");";
    ajudan_logf("Buat tabel: %s", "morfologi_kata", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_kata_fungsional(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS kata_fungsional ("
        "id INTEGER PRIMARY KEY, "
        "id_kata INTEGER NOT NULL, "
        "tipe TEXT, "
        "id_hubungan TEXT, "
        "id_ragam INTEGER, "
        "FOREIGN KEY(id_kata) REFERENCES kata(id)"
        ");";
    ajudan_logf("Buat tabel: %s", "kata_fungsional", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_kelas_cadangan(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS kelas_cadangan ("
        "id INTEGER PRIMARY KEY, "
        "awalan TEXT, "
        "akhiran TEXT, "
        "kelas_hasil INTEGER"
        ");";
    ajudan_logf("Buat tabel: %s", "kelas_cadangan", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

/* Kelompok Kalimat */
int buat_tabel_jenis_kalimat(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS jenis_kalimat ("
        "id INTEGER PRIMARY KEY, "
        "nama TEXT NOT NULL, "
        "id_parent INTEGER, "
        "deskripsi TEXT"
        ");";
    ajudan_logf("Buat tabel: %s", "jenis_kalimat", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_struktur_kalimat(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS struktur_kalimat ("
        "id INTEGER PRIMARY KEY, "
        "pola_spok TEXT, "
        "pola_kategori TEXT, "
        "contoh TEXT, "
        "nilai INTEGER"
        ");";
    ajudan_logf("Buat tabel: %s", "struktur_kalimat", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_pola_kalimat(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS pola_kalimat ("
        "id INTEGER PRIMARY KEY, "
        "id_jenis INTEGER, "
        "kerangka_jawaban TEXT, "
        "id_hubungan TEXT, "
        "prioritas INTEGER DEFAULT 1, "
        "FOREIGN KEY(id_jenis) REFERENCES jenis_kalimat(id)"
        ");";
    ajudan_logf("Buat tabel: %s", "pola_kalimat", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_deteksi_pola(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS deteksi_pola ("
        "id INTEGER PRIMARY KEY, "
        "pola_teks TEXT NOT NULL, "
        "id_jenis INTEGER, "
        "id_hubungan TEXT, "
        "prioritas INTEGER DEFAULT 1, "
        "FOREIGN KEY(id_jenis) REFERENCES jenis_kalimat(id)"
        ");";
    ajudan_logf("Buat tabel: %s", "deteksi_pola", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_normalisasi_frasa(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS normalisasi_frasa ("
        "id INTEGER PRIMARY KEY, "
        "frasa_asli TEXT NOT NULL, "
        "frasa_hasil TEXT NOT NULL"
        ");";
    ajudan_logf("Buat tabel: %s", "normalisasi_frasa", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_jawaban_bawaan(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS jawaban_bawaan ("
        "id INTEGER PRIMARY KEY, "
        "jawaban TEXT NOT NULL, "
        "prioritas INTEGER DEFAULT 1"
        ");";
    ajudan_logf("Buat tabel: %s", "jawaban_bawaan", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

/* Kelompok Pengetahuan */
int buat_tabel_pengetahuan_umum(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS pengetahuan_umum ("
        "id INTEGER PRIMARY KEY, "
        "id_kata INTEGER, "
        "id_bidang INTEGER, "
        "judul TEXT, "
        "ringkasan TEXT, "
        "penjelasan TEXT, "
        "saran TEXT, "
        "sumber TEXT, "
        "id_hubungan TEXT, "
        "FOREIGN KEY(id_kata) REFERENCES kata(id)"
        ");";
    ajudan_logf("Buat tabel: %s", "pengetahuan_umum", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_pengetahuan_bertingkat(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS pengetahuan_bertingkat ("
        "id INTEGER PRIMARY KEY, "
        "id_kata INTEGER, "
        "id_bidang INTEGER, "
        "topik TEXT, "
        "urutan INTEGER, "
        "poin TEXT, "
        "penjelasan TEXT, "
        "id_hubungan TEXT, "
        "FOREIGN KEY(id_kata) REFERENCES kata(id)"
        ");";
    ajudan_logf("Buat tabel: %s", "pengetahuan_bertingkat", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

/* Master Referensi */
int buat_tabel_kelas_kata(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS kelas_kata ("
        "id INTEGER PRIMARY KEY, "
        "nama TEXT NOT NULL UNIQUE, "
        "deskripsi TEXT"
        ");";
    ajudan_logf("Buat tabel: %s", "kelas_kata", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_ragam_kata(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS ragam_kata ("
        "id INTEGER PRIMARY KEY, "
        "nama TEXT NOT NULL UNIQUE, "
        "deskripsi TEXT"
        ");";
    ajudan_logf("Buat tabel: %s", "ragam_kata", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_bidang(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS bidang ("
        "id INTEGER PRIMARY KEY, "
        "nama TEXT NOT NULL UNIQUE, "
        "deskripsi TEXT"
        ");";
    ajudan_logf("Buat tabel: %s", "bidang", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_morfologi(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS morfologi ("
        "id INTEGER PRIMARY KEY, "
        "nama TEXT NOT NULL UNIQUE, "
        "deskripsi TEXT"
        ");";
    ajudan_logf("Buat tabel: %s", "morfologi", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_semantik(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS semantik ("
        "id INTEGER PRIMARY KEY, "
        "nama TEXT NOT NULL UNIQUE, "
        "deskripsi TEXT"
        ");";
    ajudan_logf("Buat tabel: %s", "semantik", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

int buat_tabel_hubungan(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS hubungan ("
        "id INTEGER PRIMARY KEY, "
        "nama TEXT NOT NULL UNIQUE, "
        "kategori TEXT"
        ");";
    ajudan_logf("Buat tabel: %s", "hubungan", NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

/* ========================================================================== *
 *                               INDEKS PERFORMA                              *
 * ========================================================================== */

int buat_indeks(sqlite3 *db, InfoKesalahan *error) {
    const char *sql =
        /* Kata */
        "CREATE INDEX IF NOT EXISTS idx_kata_kata ON kata(kata);"
        "CREATE INDEX IF NOT EXISTS idx_kata_kelas ON kata(kelas_id);"
        "CREATE INDEX IF NOT EXISTS idx_kata_ragam ON kata(ragam_id);"
        "CREATE INDEX IF NOT EXISTS idx_kata_bidang ON kata(bidang_id);"
        /* Arti kata */
        "CREATE INDEX IF NOT EXISTS idx_arti_kata_id ON arti_kata(id_kata);"
        /* Semantik kata */
        "CREATE INDEX IF NOT EXISTS idx_semantik_kata1 ON \
        semantik_kata(id_kata_1);"
        "CREATE INDEX IF NOT EXISTS idx_semantik_kata2 ON \
        semantik_kata(id_kata_2);"
        /* Morfologi kata */
        "CREATE INDEX IF NOT EXISTS idx_morfologi_dasar ON \
        morfologi_kata(id_kata_dasar);"
        "CREATE INDEX IF NOT EXISTS idx_morfologi_bentuk ON \
        morfologi_kata(id_kata_bentuk);"
        /* Kata fungsional */
        "CREATE INDEX IF NOT EXISTS idx_kata_fungsional_kata ON \
        kata_fungsional(id_kata);"
        /* Jenis kalimat */
        "CREATE INDEX IF NOT EXISTS idx_jenis_parent ON \
        jenis_kalimat(id_parent);"
        /* Pola kalimat */
        "CREATE INDEX IF NOT EXISTS idx_pola_jenis ON \
        pola_kalimat(id_jenis);"
        /* Deteksi pola */
        "CREATE INDEX IF NOT EXISTS idx_deteksi_teks ON \
        deteksi_pola(pola_teks);"
        /* Pengetahuan umum */
        "CREATE INDEX IF NOT EXISTS idx_pengetahuan_kata ON \
        pengetahuan_umum(id_kata);"
        "CREATE INDEX IF NOT EXISTS idx_pengetahuan_bidang ON \
        pengetahuan_umum(id_bidang);"
        /* Pengetahuan bertingkat */
        "CREATE INDEX IF NOT EXISTS idx_pengetahuan_bertingkat_kata ON \
        pengetahuan_bertingkat(id_kata);"
        "CREATE INDEX IF NOT EXISTS idx_pengetahuan_bertingkat_bidang ON \
        pengetahuan_bertingkat(id_bidang);"
        "CREATE INDEX IF NOT EXISTS idx_pengetahuan_bertingkat_topik ON \
        pengetahuan_bertingkat(topik);";
    ajudan_logf("Membuat indeks performa", NULL, NULL, NULL);
    return eksekusi_sql(db, sql, error);
}

/* ========================================================================== *
 *                          INISIALISASI DATABASE                             *
 * ========================================================================== */

int inisialisasi_basisdata(InfoKesalahan *error) {
    sqlite3 *db = NULL;
    int ok = 0;
    if (buka_basisdata(&db) != 0) return -1;
    if (eksekusi_sql(db, "BEGIN;", NULL) != 0) ok = -1;

    /* Buat semua tabel */
    if (buat_tabel_kata(db, error) != 0) ok = -1;
    if (buat_tabel_arti_kata(db, error) != 0) ok = -1;
    if (buat_tabel_semantik_kata(db, error) != 0) ok = -1;
    if (buat_tabel_morfologi_kata(db, error) != 0) ok = -1;
    if (buat_tabel_kata_fungsional(db, error) != 0) ok = -1;
    if (buat_tabel_kelas_cadangan(db, error) != 0) ok = -1;
    if (buat_tabel_jenis_kalimat(db, error) != 0) ok = -1;
    if (buat_tabel_struktur_kalimat(db, error) != 0) ok = -1;
    if (buat_tabel_pola_kalimat(db, error) != 0) ok = -1;
    if (buat_tabel_deteksi_pola(db, error) != 0) ok = -1;
    if (buat_tabel_normalisasi_frasa(db, error) != 0) ok = -1;
    if (buat_tabel_jawaban_bawaan(db, error) != 0) ok = -1;
    if (buat_tabel_pengetahuan_umum(db, error) != 0) ok = -1;
    if (buat_tabel_pengetahuan_bertingkat(db, error) != 0) ok = -1;
    if (buat_tabel_kelas_kata(db, error) != 0) ok = -1;
    if (buat_tabel_ragam_kata(db, error) != 0) ok = -1;
    if (buat_tabel_bidang(db, error) != 0) ok = -1;
    if (buat_tabel_morfologi(db, error) != 0) ok = -1;
    if (buat_tabel_semantik(db, error) != 0) ok = -1;
    if (buat_tabel_hubungan(db, error) != 0) ok = -1;

    if (buat_indeks(db, error) != 0) ok = -1;

    eksekusi_sql(db, ok == 0 ? "COMMIT;" : "ROLLBACK;", NULL);
    sqlite3_exec(db, "PRAGMA optimize;", NULL, NULL, NULL);
    tutup_basisdata(db);
    return ok == 0 ? 0 : -1;
}

/* ========================================================================== *
 *                       IMPOR CSV PER-TABEL TERBARUKAN                       *
 * ========================================================================== */

/* Kata */
int impor_kata(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; 
    sqlite3 *db; 
    sqlite3_stmt *st;
    char baris[8192], *kolom[8]; 
    int rc, n, header = 1;
    fp = fopen(nama_berkas, "r"); 
    
    if (!fp) 
        return -1;
    if (buka_basisdata(&db) != 0) { 
        fclose(fp); 
        return -1; 
    }

    rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \
            kata(kata,kelas_id,ragam_id,bidang_id) \
            VALUES(?,?,?,?);", -1, &st, NULL);
    
    if (rc != SQLITE_OK) { 
        fclose(fp); 
        tutup_basisdata(db); 
        return -1; 
    }
    
    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while (fgets(baris, sizeof(baris), fp)) {
        n = parse_baris_csv_kuat(baris, kolom, 8);
        
        if (header) { 
            header = 0; 
            if (n > 0 && strcmp(kolom[0], "id") == 0) 
                continue; 
        }

        if (n < 5) 
            continue;

        sqlite3_bind_text(st, 1, kolom[1], -1, SQLITE_STATIC);
        sqlite3_bind_int(st, 2, atoi(kolom[2]));
        sqlite3_bind_int(st, 3, atoi(kolom[3]));
        sqlite3_bind_int(st, 4, atoi(kolom[4]));
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }
    
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Arti kata */
int impor_arti_kata(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; 
    sqlite3 *db; 
    sqlite3_stmt *st; 
    char baris[8192], *kolom[4]; 
    int rc, n, header = 1;
    fp = fopen(nama_berkas, "r"); 
    
    if (!fp) 
        return -1;
    if (buka_basisdata(&db) != 0) {
        fclose(fp);
        return -1;
    }
    
    rc=sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \
            arti_kata(id_kata,arti) \
            VALUES(?,?);", -1, &st, NULL);
    
    if (rc!=SQLITE_OK) {
        fclose(fp);
        tutup_basisdata(db);
        return -1;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while(fgets(baris, sizeof(baris), fp)) {
        n = parse_baris_csv_kuat(baris,kolom,4);
        
        if (header) {
            header = 0; 
            if (n > 0 && strcmp(kolom[0], "id") == 0) 
                continue;
        }

        if (n < 3) 
            continue;
        
        sqlite3_bind_int(st, 1, atoi(kolom[1]));
        sqlite3_bind_text(st, 2, kolom[2], -1, SQLITE_STATIC);
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }
    
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Semantik kata */
int impor_semantik_kata(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; 
    sqlite3 *db; 
    sqlite3_stmt *st; 
    char baris[8192], *kolom[10]; 
    int rc, n, header = 1;
    fp = fopen(nama_berkas, "r"); 

    if (!fp) 
        return -1;

    if (buka_basisdata(&db) != 0) {
        fclose(fp);
        return -1;
    }

    rc = sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO \
            semantik_kata(id_kata_1,id_kata_2,id_semantik,\
                id_ragam,id_bidang,id_hubungan) \
            VALUES(?,?,?,?,?,?);",-1,&st,NULL);

    if (rc != SQLITE_OK) {
        fclose(fp);
        tutup_basisdata(db);
        return -1;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while(fgets(baris, sizeof(baris), fp)) {
        n = parse_baris_csv_kuat(baris, kolom, 10);

        if (header) {
            header = 0; 
            if (n > 0 && strcmp(kolom[0], "id") == 0) 
                continue;
        }

        if (n < 7 ) 
            continue;
        
        sqlite3_bind_int(st, 1, atoi(kolom[1]));
        sqlite3_bind_int(st, 2, atoi(kolom[2]));
        sqlite3_bind_int(st, 3, atoi(kolom[3]));
        sqlite3_bind_int(st, 4, atoi(kolom[4]));
        sqlite3_bind_int(st, 5, atoi(kolom[5]));
        sqlite3_bind_text(st, 6, kolom[6], - 1, SQLITE_STATIC);
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Morfologi kata */
int impor_morfologi_kata(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; 
    sqlite3_stmt *st; 
    char baris[8192], *kolom[8]; 
    int rc, n, header=1;
    fp = fopen(nama_berkas, "r"); 
    
    if (!fp) 
        return -1;
    
    if (buka_basisdata(&db) != 0) {
        fclose(fp);
        return -1;
    }
    
    rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \
            morfologi_kata(id_kata_dasar,id_kata_bentuk,\
                id_morfologi,id_hubungan) \
            VALUES(?,?,?,?);",-1, &st, NULL);

    if (rc != SQLITE_OK) {
        fclose(fp);
        tutup_basisdata(db);
        return -1;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while(fgets(baris, sizeof(baris), fp)) {
        n=parse_baris_csv_kuat(baris,kolom,8);
        
        if (header) {
            header = 0; 
            if(n > 0 && strcmp(kolom[0], "id") == 0) 
                continue;
        }

        if (n < 5) 
            continue;
        
        sqlite3_bind_int(st, 1, atoi(kolom[1]));
        sqlite3_bind_int(st, 2, atoi(kolom[2]));
        sqlite3_bind_int(st, 3, atoi(kolom[3]));
        sqlite3_bind_text(st, 4, kolom[4], -1, SQLITE_STATIC);
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Kata fungsional */
int impor_kata_fungsional(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; 
    sqlite3 *db; 
    sqlite3_stmt *st; 
    char baris[8192], *kolom[8]; 
    int rc, n, header=1;
    fp = fopen(nama_berkas, "r"); 
    
    if (!fp) 
        return -1;

    if (buka_basisdata(&db) !=0) {
        fclose(fp);
        return -1;
    }
    
    rc = sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO \
            kata_fungsional(id_kata,tipe,id_hubungan,id_ragam) \
            VALUES(?,?,?,?);", -1, &st, NULL);

    if (rc != SQLITE_OK) {
        fclose(fp);
        tutup_basisdata(db);
        return -1;
    }

    sqlite3_exec(db,"BEGIN;", NULL, NULL, NULL);
    while(fgets(baris, sizeof(baris), fp)) {
        n = parse_baris_csv_kuat(baris, kolom, 8);
        
        if (header) {
            header = 0; 
            if(n > 0 && strcmp(kolom[0], "id") ==0) 
                continue;
        }

        if(n < 5) 
            continue;
        
        sqlite3_bind_int(st, 1, atoi(kolom[1]));
        sqlite3_bind_text(st, 2, kolom[2], -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 3, kolom[3], -1, SQLITE_STATIC);
        sqlite3_bind_int(st, 4, atoi(kolom[4]));
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }
    
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Kelas cadangan */
int impor_kelas_cadangan(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; 
    sqlite3 *db; 
    sqlite3_stmt *st; 
    char baris[8192], *kolom[6]; 
    int rc, n, header=1;
    fp = fopen(nama_berkas, "r"); 
    
    if (!fp) 
        return -1;
    
    if (buka_basisdata(&db) != 0) {
        fclose(fp);
        return -1;
    }
    
    rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \
            kelas_cadangan(awalan,akhiran,kelas_hasil) \
            VALUES(?,?,?);",-1,&st,NULL);
    
    if (rc != SQLITE_OK) {
        fclose(fp);
        tutup_basisdata(db);
        return -1;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while(fgets(baris, sizeof(baris), fp)) {
        n = parse_baris_csv_kuat(baris, kolom, 6);

        if (header) {
            header=0; 
            if(n > 0 && strcmp(kolom[0], "id") == 0) 
                continue;
        }

        if (n < 4) 
            continue;

        sqlite3_bind_text(st, 1, kolom[1], -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 2, kolom[2], -1, SQLITE_STATIC);
        sqlite3_bind_int(st, 3, atoi(kolom[3]));
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Jenis kalimat */
int impor_jenis_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; 
    sqlite3 *db; 
    sqlite3_stmt *st; 
    char baris[8192], *kolom[6]; 
    int rc, n, header=1;
    fp = fopen(nama_berkas, "r"); 
    
    if (!fp) 
        return -1;
    
    if (buka_basisdata(&db) != 0) {
        fclose(fp);
        return -1;
    }
    
    rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \
            jenis_kalimat(nama,id_parent,deskripsi) \
            VALUES(?,?,?);", -1, &st, NULL);

    if (rc != SQLITE_OK) {
        fclose(fp);
        tutup_basisdata(db);
        return -1;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while(fgets(baris, sizeof(baris), fp)) {
        n = parse_baris_csv_kuat(baris, kolom, 6);
        
        if (header) {
            header = 0; 
            if(n > 0 && strcmp(kolom[0], "id") == 0) 
                continue;
        }
        
        if(n < 4) 
            continue;
        
        sqlite3_bind_text(st, 1, kolom[1], -1, SQLITE_STATIC);
        sqlite3_bind_int(st, 2, atoi(kolom[2]));
        sqlite3_bind_text(st, 3, kolom[3], -1, SQLITE_STATIC);
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Struktur kalimat */
int impor_struktur_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; 
    sqlite3 *db; 
    sqlite3_stmt *st; 
    char baris[8192], *kolom[8]; 
    int rc, n, header = 1;
    fp = fopen(nama_berkas, "r"); 
    
    if (!fp) 
        return -1;
    
    if (buka_basisdata(&db) != 0) {
        fclose(fp);
        return -1;
    }
    
    rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \
            struktur_kalimat(pola_spok,pola_kategori,contoh,nilai) \
            VALUES(?,?,?,?);", -1, &st, NULL);

    if (rc != SQLITE_OK) {
        fclose(fp);
        tutup_basisdata(db);
        return -1;
    }
    
    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while(fgets(baris, sizeof(baris), fp)) {
        n = parse_baris_csv_kuat(baris, kolom, 8);
        if (header) {
            header = 0; 
            
        if (n > 0 && strcmp(kolom[0], "id") == 0) 
            continue;
        }

        if (n < 5) 
            continue;
        
        sqlite3_bind_text(st, 1, kolom[1], -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 2, kolom[2], -1, SQLITE_STATIC);
        sqlite3_bind_text(st, 3, kolom[3], -1, SQLITE_STATIC);
        sqlite3_bind_int(st, 4, atoi(kolom[4]));
        (void)sqlite3_step(st); 
        sqlite3_reset(st);
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(st); 
    fclose(fp); 
    tutup_basisdata(db); 
    return 0;
}

/* Pola kalimat */
int impor_pola_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[8]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO pola_kalimat(id_jenis,kerangka_jawaban,id_hubungan,prioritas) VALUES(?,?,?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,8);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<5) continue;
        sqlite3_bind_int(st,1,atoi(kolom[1]));
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,3,kolom[3],-1,SQLITE_STATIC); /* multi nilai */
        sqlite3_bind_int(st,4,atoi(kolom[4]));
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

/* Deteksi pola */
int impor_deteksi_pola(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[8]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO deteksi_pola(pola_teks,id_jenis,id_hubungan,prioritas) VALUES(?,?,?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,8);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<5) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_int(st,2,atoi(kolom[2]));
        sqlite3_bind_text(st,3,kolom[3],-1,SQLITE_STATIC); /* multi nilai */
        sqlite3_bind_int(st,4,atoi(kolom[4]));
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

/* Normalisasi frasa */
int impor_normalisasi_frasa(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO normalisasi_frasa(frasa_asli,frasa_hasil) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

/* Jawaban bawaan */
int impor_jawaban_bawaan(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO jawaban_bawaan(jawaban,prioritas) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_int(st,2,atoi(kolom[2]));
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

/* Pengetahuan umum */
int impor_pengetahuan_umum(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[16]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO pengetahuan_umum(id_kata,id_bidang,judul,ringkasan,penjelasan,saran,sumber,id_hubungan) VALUES(?,?,?,?,?,?,?,?);",
        -1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,16);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<9) continue;
        sqlite3_bind_int(st,1,atoi(kolom[1]));
        sqlite3_bind_int(st,2,atoi(kolom[2]));
        sqlite3_bind_text(st,3,kolom[3],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,4,kolom[4],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,5,kolom[5],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,6,kolom[6],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,7,kolom[7],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,8,kolom[8],-1,SQLITE_STATIC); /* multi nilai */
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

/* Pengetahuan bertingkat */
int impor_pengetahuan_bertingkat(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[16]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO pengetahuan_bertingkat(id_kata,id_bidang,topik,urutan,poin,penjelasan,id_hubungan) VALUES(?,?,?,?,?,?,?);",
        -1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,16);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<8) continue;
        sqlite3_bind_int(st,1,atoi(kolom[1]));
        sqlite3_bind_int(st,2,atoi(kolom[2]));
        sqlite3_bind_text(st,3,kolom[3],-1,SQLITE_STATIC);
        sqlite3_bind_int(st,4,atoi(kolom[4]));
        sqlite3_bind_text(st,5,kolom[5],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,6,kolom[6],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,7,kolom[7],-1,SQLITE_STATIC); /* multi nilai */
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

/* Master referensi: kelas_kata, ragam_kata, bidang, morfologi, semantik, hubungan */
int impor_kelas_kata(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO kelas_kata(nama,deskripsi) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

int impor_ragam_kata(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO ragam_kata(nama,deskripsi) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

int impor_bidang(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO bidang(nama,deskripsi) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

int impor_morfologi(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO morfologi(nama,deskripsi) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

int impor_semantik(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO semantik(nama,deskripsi) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

int impor_hubungan(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db; sqlite3_stmt *st; char baris[8192], *kolom[4]; int rc, n, header=1;
    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO hubungan(nama,kategori) VALUES(?,?);",-1,&st,NULL);
    if(rc!=SQLITE_OK){fclose(fp);tutup_basisdata(db);return -1;}
    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);
    while(fgets(baris,sizeof(baris),fp)){
        n=parse_baris_csv_kuat(baris,kolom,4);
        if(header){header=0; if(n>0 && strcmp(kolom[0],"id")==0) continue;}
        if(n<3) continue;
        sqlite3_bind_text(st,1,kolom[1],-1,SQLITE_STATIC);
        sqlite3_bind_text(st,2,kolom[2],-1,SQLITE_STATIC);
        (void)sqlite3_step(st); sqlite3_reset(st);
    }
    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st); fclose(fp); tutup_basisdata(db); return 0;
}

/* ========================================================================== *
 *                IMPOR KATEGORI: KATA / KALIMAT / PENGETAHUAN               *
 * ========================================================================== */

/* Impor kategori kata:
   Header dinamis; kolom yang dikenali akan diisi:
   kata, kelas_id, ragam_id, bidang_id, arti, tipe (kata_fungsional), id_hubungan_fungsional
*/
int impor_kategori_kata(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db;
    sqlite3_stmt *st_kata=NULL, *st_arti=NULL, *st_fungs=NULL;
    char baris[8192], *kolom[64], *hdr[64]; int rc, n, h=0;

    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}

    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);

    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO kata(kata,kelas_id,ragam_id,bidang_id) VALUES(?,?,?,?);",-1,&st_kata,NULL);
    if(rc!=SQLITE_OK){tutup_basisdata(db);fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO arti_kata(id_kata,arti) VALUES(?,?);",-1,&st_arti,NULL);
    if(rc!=SQLITE_OK){sqlite3_finalize(st_kata);tutup_basisdata(db);fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO kata_fungsional(id_kata,tipe,id_hubungan,id_ragam) VALUES(?,?,?,?);",-1,&st_fungs,NULL);
    if(rc!=SQLITE_OK){sqlite3_finalize(st_kata);sqlite3_finalize(st_arti);tutup_basisdata(db);fclose(fp);return -1;}

    /* Baca header */
    if (fgets(baris,sizeof(baris),fp)) {
        h=parse_baris_csv_kuat(baris,hdr,64);
    }

    /* Peta index header */
    /* default -1 jika tak ada kolom */
    #define FINDCOL(name, idx) int idx=-1; { int i; for(i=0;i<h;i++){ if(hdr[i]&&strcmp(hdr[i],name)==0){ idx=i; break; } } }
    FINDCOL("kata", i_kata)
    FINDCOL("kelas_id", i_kelas)
    FINDCOL("ragam_id", i_ragam)
    FINDCOL("bidang_id", i_bidang)
    FINDCOL("arti", i_arti)
    FINDCOL("tipe", i_tipe)
    FINDCOL("id_hubungan_fungsional", i_hub)

    while (fgets(baris,sizeof(baris),fp)) {
        n=parse_baris_csv_kuat(baris,kolom,64);
        if (n <= 0) continue;

        /* Insert kata */
        sqlite3_reset(st_kata); sqlite3_clear_bindings(st_kata);
        sqlite3_bind_text(st_kata,1,(i_kata>=0)?kolom[i_kata]:"", -1, SQLITE_STATIC);
        sqlite3_bind_int(st_kata,2,(i_kelas>=0 && kolom[i_kelas][0])?atoi(kolom[i_kelas]):0);
        sqlite3_bind_int(st_kata,3,(i_ragam>=0 && kolom[i_ragam][0])?atoi(kolom[i_ragam]):0);
        sqlite3_bind_int(st_kata,4,(i_bidang>=0 && kolom[i_bidang][0])?atoi(kolom[i_bidang]):0);
        (void)sqlite3_step(st_kata);
        sqlite3_int64 idk = sqlite3_last_insert_rowid(db);

        /* Insert arti jika ada */
        if (i_arti>=0 && kolom[i_arti][0]) {
            sqlite3_reset(st_arti); sqlite3_clear_bindings(st_arti);
            sqlite3_bind_int64(st_arti,1,idk);
            sqlite3_bind_text(st_arti,2,kolom[i_arti],-1,SQLITE_STATIC);
            (void)sqlite3_step(st_arti);
        }

        /* Insert kata_fungsional jika tipe ada */
        if (i_tipe>=0 && kolom[i_tipe][0]) {
            sqlite3_reset(st_fungs); sqlite3_clear_bindings(st_fungs);
            sqlite3_bind_int64(st_fungs,1,idk);
            sqlite3_bind_text(st_fungs,2,kolom[i_tipe],-1,SQLITE_STATIC);
            sqlite3_bind_text(st_fungs,3,(i_hub>=0)?kolom[i_hub]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_int(st_fungs,4,(i_ragam>=0 && kolom[i_ragam][0])?atoi(kolom[i_ragam]):0);
            (void)sqlite3_step(st_fungs);
        }
    }

    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st_kata); sqlite3_finalize(st_arti); sqlite3_finalize(st_fungs);
    fclose(fp); tutup_basisdata(db); return 0;
}

/* Impor kategori kalimat:
   Kolom: pola_teks, id_jenis, id_hubungan, prioritas, kerangka_jawaban, frasa_asli, frasa_hasil.
   Satu baris bisa mengisi deteksi_pola + pola_kalimat + normalisasi_frasa.
*/
int impor_kategori_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db;
    sqlite3_stmt *st_det=NULL, *st_pola=NULL, *st_norm=NULL;
    char baris[8192], *kolom[64], *hdr[64]; int rc, n, h=0;

    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}

    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);

    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO deteksi_pola(pola_teks,id_jenis,id_hubungan,prioritas) VALUES(?,?,?,?);",-1,&st_det,NULL);
    if(rc!=SQLITE_OK){tutup_basisdata(db);fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO pola_kalimat(id_jenis,kerangka_jawaban,id_hubungan,prioritas) VALUES(?,?,?,?);",-1,&st_pola,NULL);
    if(rc!=SQLITE_OK){sqlite3_finalize(st_det);tutup_basisdata(db);fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,"INSERT OR IGNORE INTO normalisasi_frasa(frasa_asli,frasa_hasil) VALUES(?,?);",-1,&st_norm,NULL);
    if(rc!=SQLITE_OK){sqlite3_finalize(st_det);sqlite3_finalize(st_pola);tutup_basisdata(db);fclose(fp);return -1;}

    if (fgets(baris,sizeof(baris),fp)) h=parse_baris_csv_kuat(baris,hdr,64);

    FINDCOL("pola_teks", i_teks)
    FINDCOL("id_jenis", i_jenis)
    FINDCOL("id_hubungan", i_hub)
    FINDCOL("prioritas", i_prio)
    FINDCOL("kerangka_jawaban", i_kerangka)
    FINDCOL("frasa_asli", i_fasli)
    FINDCOL("frasa_hasil", i_fhasil)

    while (fgets(baris,sizeof(baris),fp)) {
        n=parse_baris_csv_kuat(baris,kolom,64);
        if (n<=0) continue;

        /* deteksi_pola */
        if (i_teks>=0) {
            sqlite3_reset(st_det); sqlite3_clear_bindings(st_det);
            sqlite3_bind_text(st_det,1,(i_teks>=0)?kolom[i_teks]:"",-1,SQLITE_STATIC);
            sqlite3_bind_int(st_det,2,(i_jenis>=0 && kolom[i_jenis][0])?atoi(kolom[i_jenis]):0);
            sqlite3_bind_text(st_det,3,(i_hub>=0)?kolom[i_hub]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_int(st_det,4,(i_prio>=0 && kolom[i_prio][0])?atoi(kolom[i_prio]):1);
            (void)sqlite3_step(st_det);
        }

        /* pola_kalimat */
        if (i_kerangka>=0) {
            sqlite3_reset(st_pola); sqlite3_clear_bindings(st_pola);
            sqlite3_bind_int(st_pola,1,(i_jenis>=0 && kolom[i_jenis][0])?atoi(kolom[i_jenis]):0);
            sqlite3_bind_text(st_pola,2,(i_kerangka>=0)?kolom[i_kerangka]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_text(st_pola,3,(i_hub>=0)?kolom[i_hub]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_int(st_pola,4,(i_prio>=0 && kolom[i_prio][0])?atoi(kolom[i_prio]):1);
            (void)sqlite3_step(st_pola);
        }

        /* normalisasi_frasa */
        if (i_fasli>=0 || i_fhasil>=0) {
            sqlite3_reset(st_norm); sqlite3_clear_bindings(st_norm);
            sqlite3_bind_text(st_norm,1,(i_fasli>=0)?kolom[i_fasli]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_text(st_norm,2,(i_fhasil>=0)?kolom[i_fhasil]:NULL,-1,SQLITE_STATIC);
            (void)sqlite3_step(st_norm);
        }
    }

    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st_det); sqlite3_finalize(st_pola); sqlite3_finalize(st_norm);
    fclose(fp); tutup_basisdata(db); return 0;
}

/* Impor kategori pengetahuan:
   Kolom umum: id_kata,id_bidang,judul,ringkasan,penjelasan,saran,sumber,id_hubungan
   Kolom bertingkat: topik,urutan,poin,penjelasan_bertingkat,id_hubungan_bertingkat
*/
int impor_kategori_pengetahuan(const char *nama_berkas, InfoKesalahan *error) {
    FILE *fp; sqlite3 *db;
    sqlite3_stmt *st_umum=NULL, *st_bt=NULL;
    char baris[8192], *kolom[64], *hdr[64]; int rc, n, h=0;

    fp=fopen(nama_berkas,"r"); if(!fp) return -1;
    if(buka_basisdata(&db)!=0){fclose(fp);return -1;}

    sqlite3_exec(db,"BEGIN;",NULL,NULL,NULL);

    rc=sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO pengetahuan_umum(id_kata,id_bidang,judul,ringkasan,penjelasan,saran,sumber,id_hubungan) VALUES(?,?,?,?,?,?,?,?);",
        -1,&st_umum,NULL);
    if(rc!=SQLITE_OK){tutup_basisdata(db);fclose(fp);return -1;}
    rc=sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO pengetahuan_bertingkat(id_kata,id_bidang,topik,urutan,poin,penjelasan,id_hubungan) VALUES(?,?,?,?,?,?,?);",
        -1,&st_bt,NULL);
    if(rc!=SQLITE_OK){sqlite3_finalize(st_umum);tutup_basisdata(db);fclose(fp);return -1;}

    if (fgets(baris,sizeof(baris),fp)) h=parse_baris_csv_kuat(baris,hdr,64);

    FINDCOL("id_kata", i_kata)
    FINDCOL("id_bidang", i_bidang)
    FINDCOL("judul", i_judul)
    FINDCOL("ringkasan", i_ringkasan)
    FINDCOL("penjelasan", i_penjelasan)
    FINDCOL("saran", i_saran)
    FINDCOL("sumber", i_sumber)
    FINDCOL("id_hubungan", i_hub)
    /* bertingkat */
    FINDCOL("topik", i_topik)
    FINDCOL("urutan", i_urutan)
    FINDCOL("poin", i_poin)
    FINDCOL("penjelasan_bertingkat", i_pbt)
    FINDCOL("id_hubungan_bertingkat", i_hbt)

    while (fgets(baris,sizeof(baris),fp)) {
        n=parse_baris_csv_kuat(baris,kolom,64);
        if (n<=0) continue;

        /* pengetahuan umum */
        sqlite3_reset(st_umum); sqlite3_clear_bindings(st_umum);
        sqlite3_bind_int(st_umum,1,(i_kata>=0 && kolom[i_kata][0])?atoi(kolom[i_kata]):0);
        sqlite3_bind_int(st_umum,2,(i_bidang>=0 && kolom[i_bidang][0])?atoi(kolom[i_bidang]):0);
        sqlite3_bind_text(st_umum,3,(i_judul>=0)?kolom[i_judul]:NULL,-1,SQLITE_STATIC);
        sqlite3_bind_text(st_umum,4,(i_ringkasan>=0)?kolom[i_ringkasan]:NULL,-1,SQLITE_STATIC);
        sqlite3_bind_text(st_umum,5,(i_penjelasan>=0)?kolom[i_penjelasan]:NULL,-1,SQLITE_STATIC);
        sqlite3_bind_text(st_umum,6,(i_saran>=0)?kolom[i_saran]:NULL,-1,SQLITE_STATIC);
        sqlite3_bind_text(st_umum,7,(i_sumber>=0)?kolom[i_sumber]:NULL,-1,SQLITE_STATIC);
        sqlite3_bind_text(st_umum,8,(i_hub>=0)?kolom[i_hub]:NULL,-1,SQLITE_STATIC);
        (void)sqlite3_step(st_umum);

        /* pengetahuan bertingkat (opsional, diisi jika ada topik/poin/penjelasan_bertingkat/hubungan) */
        if ((i_topik>=0 && kolom[i_topik][0]) || (i_poin>=0 && kolom[i_poin][0]) || (i_pbt>=0 && kolom[i_pbt][0]) || (i_hbt>=0 && kolom[i_hbt][0])) {
            sqlite3_reset(st_bt); sqlite3_clear_bindings(st_bt);
            sqlite3_bind_int(st_bt,1,(i_kata>=0 && kolom[i_kata][0])?atoi(kolom[i_kata]):0);
            sqlite3_bind_int(st_bt,2,(i_bidang>=0 && kolom[i_bidang][0])?atoi(kolom[i_bidang]):0);
            sqlite3_bind_text(st_bt,3,(i_topik>=0)?kolom[i_topik]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_int(st_bt,4,(i_urutan>=0 && kolom[i_urutan][0])?atoi(kolom[i_urutan]):0);
            sqlite3_bind_text(st_bt,5,(i_poin>=0)?kolom[i_poin]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_text(st_bt,6,(i_pbt>=0)?kolom[i_pbt]:NULL,-1,SQLITE_STATIC);
            sqlite3_bind_text(st_bt,7,(i_hbt>=0)?kolom[i_hbt]:NULL,-1,SQLITE_STATIC);
            (void)sqlite3_step(st_bt);
        }
    }

    sqlite3_exec(db,"COMMIT;",NULL,NULL,NULL);
    sqlite3_finalize(st_umum); sqlite3_finalize(st_bt);
    fclose(fp); tutup_basisdata(db); return 0;
}

/* ========================================================================== *
 *                       EKSPOR CSV PER-TABEL TERBARUKAN                      *
 * ========================================================================== */

int ekspor_kata(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,kata,kelas_id,ragam_id,bidang_id FROM kata ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,kata,kelas_id,ragam_id,bidang_id\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",%d,%d,%d\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_int(st,3),
            sqlite3_column_int(st,4));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_arti_kata(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,id_kata,arti FROM arti_kata ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,id_kata,arti\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,%d,\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_semantik_kata(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,id_kata_1,id_kata_2,id_semantik,id_ragam,id_bidang,id_hubungan FROM semantik_kata ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,id_kata_1,id_kata_2,id_semantik,id_ragam,id_bidang,id_hubungan\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,%d,%d,%d,%d,%d,\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_int(st,3),
            sqlite3_column_int(st,4),
            sqlite3_column_int(st,5),
            sqlite3_column_text(st,6));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_morfologi_kata(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,id_kata_dasar,id_kata_bentuk,id_morfologi,id_hubungan FROM morfologi_kata ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,id_kata_dasar,id_kata_bentuk,id_morfologi,id_hubungan\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,%d,%d,%d,\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_int(st,3),
            sqlite3_column_text(st,4));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_kata_fungsional(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,id_kata,tipe,id_hubungan,id_ragam FROM kata_fungsional ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,id_kata,tipe,id_hubungan,id_ragam\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,%d,\"%s\",\"%s\",%d\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_text(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_int(st,4));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_kelas_cadangan(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,awalan,akhiran,kelas_hasil FROM kelas_cadangan ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,awalan,akhiran,kelas_hasil\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\",%d\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2),
            sqlite3_column_int(st,3));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_jenis_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,nama,id_parent,deskripsi FROM jenis_kalimat ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,nama,id_parent,deskripsi\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",%d,\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_text(st,3));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_struktur_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,pola_spok,pola_kategori,contoh,nilai FROM struktur_kalimat ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,pola_spok,pola_kategori,contoh,nilai\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\",\"%s\",%d\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_int(st,4));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_pola_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,id_jenis,kerangka_jawaban,id_hubungan,prioritas FROM pola_kalimat ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,id_jenis,kerangka_jawaban,id_hubungan,prioritas\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,%d,\"%s\",\"%s\",%d\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_text(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_int(st,4));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_deteksi_pola(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,pola_teks,id_jenis,id_hubungan,prioritas FROM deteksi_pola ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,pola_teks,id_jenis,id_hubungan,prioritas\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",%d,\"%s\",%d\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_int(st,4));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_normalisasi_frasa(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,frasa_asli,frasa_hasil FROM normalisasi_frasa ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,frasa_asli,frasa_hasil\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_jawaban_bawaan(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,jawaban,prioritas FROM jawaban_bawaan ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,jawaban,prioritas\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",%d\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_int(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_pengetahuan_umum(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,id_kata,id_bidang,judul,ringkasan,penjelasan,saran,sumber,id_hubungan FROM pengetahuan_umum ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,id_kata,id_bidang,judul,ringkasan,penjelasan,saran,sumber,id_hubungan\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,%d,%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_text(st,4),
            sqlite3_column_text(st,5),
            sqlite3_column_text(st,6),
            sqlite3_column_text(st,7),
            sqlite3_column_text(st,8));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_pengetahuan_bertingkat(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,id_kata,id_bidang,topik,urutan,poin,penjelasan,id_hubungan FROM pengetahuan_bertingkat ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,id_kata,id_bidang,topik,urutan,poin,penjelasan,id_hubungan\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,%d,%d,\"%s\",%d,\"%s\",\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_int(st,4),
            sqlite3_column_text(st,5),
            sqlite3_column_text(st,6),
            sqlite3_column_text(st,7));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_kelas_kata(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,nama,deskripsi FROM kelas_kata ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,nama,deskripsi\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_ragam_kata(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,nama,deskripsi FROM ragam_kata ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,nama,deskripsi\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_bidang(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,nama,deskripsi FROM bidang ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,nama,deskripsi\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_morfologi(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,nama,deskripsi FROM morfologi ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,nama,deskripsi\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_semantik(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,nama,deskripsi FROM semantik ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,nama,deskripsi\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_hubungan(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if(buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    if(sqlite3_prepare_v2(db,"SELECT id,nama,kategori FROM hubungan ORDER BY id;",-1,&st,NULL)!=SQLITE_OK){
        fclose(out); tutup_basisdata(db); return -1;
    }
    fprintf(out,"id,nama,kategori\n");
    while(sqlite3_step(st)==SQLITE_ROW){
        fprintf(out,"%d,\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_text(st,2));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

/* ========================================================================== *
 *                       EKSPOR KATEGORI: KATA/KALIMAT/PENGETAHUAN            *
 * ========================================================================== */

int ekspor_kategori_kata(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    const char *sql =
        "SELECT k.id,k.kata,k.kelas_id,k.ragam_id,k.bidang_id,"
        "(SELECT arti FROM arti_kata WHERE id_kata=k.id LIMIT 1) AS arti, "
        "(SELECT tipe FROM kata_fungsional WHERE id_kata=k.id LIMIT 1) AS tipe, "
        "(SELECT id_hubungan FROM kata_fungsional WHERE id_kata=k.id LIMIT 1) AS id_hubungan_fungsional "
        "FROM kata k ORDER BY k.id;";
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) { fclose(out); tutup_basisdata(db); return -1; }
    fprintf(out,"id,kata,kelas_id,ragam_id,bidang_id,arti,tipe,id_hubungan_fungsional\n");
    while (sqlite3_step(st)==SQLITE_ROW) {
        fprintf(out,"%d,\"%s\",%d,%d,%d,\"%s\",\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_int(st,3),
            sqlite3_column_int(st,4),
            sqlite3_column_text(st,5),
            sqlite3_column_text(st,6),
            sqlite3_column_text(st,7));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_kategori_kalimat(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    const char *sql =
        "SELECT d.id,d.pola_teks,d.id_jenis,d.id_hubungan,d.prioritas, "
        "(SELECT kerangka_jawaban FROM pola_kalimat WHERE id_jenis=d.id_jenis ORDER BY prioritas DESC LIMIT 1) AS kerangka_jawaban, "
        "(SELECT frasa_asli FROM normalisasi_frasa ORDER BY id LIMIT 1) AS frasa_asli, "
        "(SELECT frasa_hasil FROM normalisasi_frasa ORDER BY id LIMIT 1) AS frasa_hasil "
        "FROM deteksi_pola d ORDER BY d.id;";
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) { fclose(out); tutup_basisdata(db); return -1; }
    fprintf(out,"id,pola_teks,id_jenis,id_hubungan,prioritas,kerangka_jawaban,frasa_asli,frasa_hasil\n");
    while (sqlite3_step(st)==SQLITE_ROW) {
        fprintf(out,"%d,\"%s\",%d,\"%s\",%d,\"%s\",\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_text(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_int(st,4),
            sqlite3_column_text(st,5),
            sqlite3_column_text(st,6),
            sqlite3_column_text(st,7));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

int ekspor_kategori_pengetahuan(const char *nama_berkas, InfoKesalahan *error) {
    sqlite3 *db; FILE *out; sqlite3_stmt *st;
    if (buka_basisdata(&db)!=0) return -1;
    out=fopen(nama_berkas,"w"); if(!out){tutup_basisdata(db);return -1;}
    const char *sql =
        "SELECT u.id,u.id_kata,u.id_bidang,u.judul,u.ringkasan,u.penjelasan,u.saran,u.sumber,u.id_hubungan, "
        "(SELECT topik FROM pengetahuan_bertingkat WHERE id_kata=u.id_kata ORDER BY urutan LIMIT 1) AS topik, "
        "(SELECT urutan FROM pengetahuan_bertingkat WHERE id_kata=u.id_kata ORDER BY urutan LIMIT 1) AS urutan, "
        "(SELECT poin FROM pengetahuan_bertingkat WHERE id_kata=u.id_kata ORDER BY urutan LIMIT 1) AS poin, "
        "(SELECT penjelasan FROM pengetahuan_bertingkat WHERE id_kata=u.id_kata ORDER BY urutan LIMIT 1) AS penjelasan_bertingkat, "
        "(SELECT id_hubungan FROM pengetahuan_bertingkat WHERE id_kata=u.id_kata ORDER BY urutan LIMIT 1) AS id_hubungan_bertingkat "
        "FROM pengetahuan_umum u ORDER BY u.id;";
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) { fclose(out); tutup_basisdata(db); return -1; }
    fprintf(out,"id,id_kata,id_bidang,judul,ringkasan,penjelasan,saran,sumber,id_hubungan,topik,urutan,poin,penjelasan_bertingkat,id_hubungan_bertingkat\n");
    while (sqlite3_step(st)==SQLITE_ROW) {
        fprintf(out,"%d,%d,%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%d,\"%s\",\"%s\",\"%s\"\n",
            sqlite3_column_int(st,0),
            sqlite3_column_int(st,1),
            sqlite3_column_int(st,2),
            sqlite3_column_text(st,3),
            sqlite3_column_text(st,4),
            sqlite3_column_text(st,5),
            sqlite3_column_text(st,6),
            sqlite3_column_text(st,7),
            sqlite3_column_text(st,8),
            sqlite3_column_text(st,9),
            sqlite3_column_int(st,10),
            sqlite3_column_text(st,11),
            sqlite3_column_text(st,12),
            sqlite3_column_text(st,13));
    }
    sqlite3_finalize(st); fclose(out); tutup_basisdata(db); return 0;
}

/* ========================================================================== *
 *                               VACUUM & OPTIMIZE                            *
 * ========================================================================== */

int vacuum_basisdata(void) {
    sqlite3 *db;
    if (buka_basisdata(&db) != 0) return -1;
    sqlite3_exec(db, "VACUUM;", NULL, NULL, NULL);
    sqlite3_exec(db, "ANALYZE;", NULL, NULL, NULL);
    tutup_basisdata(db);
    return 0;
}

/* ========================================================================== *
 *                        AKHIR BERKAS BASISDATA.C                            *
 * ========================================================================== */
