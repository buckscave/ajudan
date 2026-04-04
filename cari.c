/*
 * cari.c - Optimasi Pencarian Fuzzy
 *
 * Menggantikan fuzzy search O(n) linear dengan pendekatan bertahap:
 * 1. Persis (exact match, O(log n) via index)
 * 2. Awalan (LIKE prefix, O(log n) via index)
 * 3. Trigram (FTS5 jika tersedia)
 * 4. Levenshtein hanya pada kandidat terfilter
 *
 * Persyaratan: C89, SQLite3, ajudan.h
 */

#include "ajudan.h"

/* ================================================================== *
 *                         MAKRO & KONSTANTA                            *
 * ================================================================== */

#define CARI_MAKS_TRIGRAM 25
#define CARI_MAKS_FUZZY 10
#define CARI_MAKS_NGRAM 10
#define CARI_SKOR_TRIGRAM_MIN 0.3
#define CARI_SKOR_MIN_EFEKTIF(skor_sistem) \
    ((skor_sistem) > 0.8 ? 0.75 : 0.6)

/* ================================================================== *
 *                       PENCARIAN PERSIS                                *
 * ================================================================== */

static int cari_persis_dari_db(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *kata,
    char *hasil,
    int ukuran)
{
    int rc;
    const char *val;

    if (!db || !cache || !kata || !hasil || ukuran <= 0) return -1;
    hasil[0] = '\0';

    rc = sqlite3_reset(cache->stmt_kategori_kata);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(cache->stmt_kategori_kata, 1, kata,
        -1, SQLITE_TRANSIENT);

    if (sqlite3_step(cache->stmt_kategori_kata) == SQLITE_ROW) {
        val = (const char *)sqlite3_column_text(
            cache->stmt_kategori_kata, 0);
        if (val) {
            snprintf(hasil, (size_t)ukuran, "%s", kata);
            sqlite3_reset(cache->stmt_kategori_kata);
            return 0;
        }
    }

    sqlite3_reset(cache->stmt_kategori_kata);
    return -1;
}

/* ================================================================== *
 *                  PENCARIAN AWALAN (LIKE PREFIX)                       *
 * ================================================================== */

static int cari_awalan_dari_db(
    sqlite3 *db,
    const char *kata,
    char *hasil,
    int ukuran,
    int jarak_maks,
    double skor_min)
{
    sqlite3_stmt *stmt;
    int rc, count, found_idx, i, jarak, pm, pmax;
    double skor;
    const char *val;
    char kandidat[CARI_MAKS_FUZZY][MAX_PANJANG_TOKEN];
    double skor_arr[CARI_MAKS_FUZZY];
    int jarak_arr[CARI_MAKS_FUZZY];
    int len_kata;
    char pola_like[MAX_PANJANG_TOKEN + 4];

    if (!db || !kata || !hasil || ukuran <= 0) return -1;
    hasil[0] = '\0';

    len_kata = (int)strlen(kata);
    if (len_kata <= 2) return -1;

    snprintf(pola_like, sizeof(pola_like), "%s%%", kata);

    stmt = NULL;
    rc = sqlite3_prepare_v2(db,
        "SELECT kata FROM kata WHERE lower(kata) LIKE lower(?) "
        "AND length(kata) >= ? AND length(kata) <= ? "
        "ORDER BY length(kata) ASC LIMIT ?;",
        -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, pola_like, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2,
        len_kata > jarak_maks
        ? len_kata - jarak_maks : 1);
    sqlite3_bind_int(stmt, 3, len_kata + jarak_maks);
    sqlite3_bind_int(stmt, 4, CARI_MAKS_NGRAM);

    count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW
        && count < CARI_MAKS_FUZZY) {
        val = (const char *)sqlite3_column_text(stmt, 0);
        if (!val) continue;

        jarak = hitung_jarak_levenshtein(kata, val);
        if (jarak < 0) continue;

        pm = (int)strlen(val);
        pmax = (len_kata > pm) ? len_kata : pm;
        skor = hitung_skor_fuzzy(jarak, pmax);

        if (jarak <= jarak_maks && skor >= skor_min) {
            snprintf(kandidat[count],
                (size_t)sizeof(kandidat[count]),
                "%s", val);
            skor_arr[count] = skor;
            jarak_arr[count] = jarak;
            count++;
        }
    }

    sqlite3_finalize(stmt);

    if (count <= 0) return -1;

    found_idx = 0;
    for (i = 1; i < count; i++) {
        if (skor_arr[i] > skor_arr[found_idx]) {
            found_idx = i;
        } else if (skor_arr[i] == skor_arr[found_idx]
            && jarak_arr[i] < jarak_arr[found_idx]) {
            found_idx = i;
        }
    }

    snprintf(hasil, (size_t)ukuran, "%s", kandidat[found_idx]);
    return 0;
}

/* ================================================================== *
 *                       FUNGSI TRIGRAM                                 *
 * ================================================================== */

static int ekstrak_trigram(
    const char *kata,
    char trigram[][MAX_PANJANG_TOKEN + 1],
    int maks)
{
    int len, i, count;

    if (!kata) return 0;
    len = (int)strlen(kata);
    if (len < 3) return 0;

    count = 0;
    for (i = 0; i <= len - 3 && count < maks; i++) {
        trigram[count][0] = kata[i];
        trigram[count][1] = kata[i + 1];
        trigram[count][2] = kata[i + 2];
        trigram[count][3] = '\0';
        to_lower_case(trigram[count]);
        count++;
    }

    return count;
}

/* ================================================================== *
 *                  PENCARIAN TRIGRAM (FTS5 / LIKE)                      *
 * ================================================================== */

static int cari_dengan_fts5(
    sqlite3 *db,
    const char *kata,
    char *hasil,
    int ukuran,
    int jarak_maks,
    double skor_min)
{
    sqlite3_stmt *stmt;
    int rc, count, found_idx, i, jarak, pm, pmax;
    double skor;
    const char *val;
    char kandidat[CARI_MAKS_TRIGRAM][MAX_PANJANG_TOKEN];
    double skor_arr[CARI_MAKS_TRIGRAM];
    int jarak_arr[CARI_MAKS_TRIGRAM];
    int len_kata, jml_tri;
    char trigram[CARI_MAKS_NGRAM][MAX_PANJANG_TOKEN + 1];
    char kondisi[1024];
    int kondisi_len;
    char tmp[64];

    if (!db || !kata || !hasil || ukuran <= 0) return -1;
    hasil[0] = '\0';

    len_kata = (int)strlen(kata);
    if (len_kata < 3) return -1;

    jml_tri = ekstrak_trigram(kata, trigram, CARI_MAKS_NGRAM);
    if (jml_tri <= 0) return -1;

    stmt = NULL;
    rc = sqlite3_prepare_v2(db,
        "SELECT name FROM sqlite_master WHERE type='table' "
        "AND name='kata_fts';", -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rc = 0;
    }
    sqlite3_finalize(stmt);

    if (rc != 0) return -1;

    snprintf(kondisi, sizeof(kondisi), "(");
    kondisi_len = 1;
    for (i = 0; i < jml_tri; i++) {
        if (i > 0) {
            strncat(kondisi, " OR ",
                sizeof(kondisi) - strlen(kondisi) - 1);
            kondisi_len += 4;
        }
        strncat(kondisi, "kata MATCH '",
            sizeof(kondisi) - strlen(kondisi) - 1);
        kondisi_len += 13;
        strncat(kondisi, trigram[i],
            sizeof(kondisi) - strlen(kondisi) - 1);
        kondisi_len += 3;
        strncat(kondisi, "*'",
            sizeof(kondisi) - strlen(kondisi) - 1);
        kondisi_len += 2;
    }
    strncat(kondisi, ") AND length(kata) >= ? "
        "AND length(kata) <= ? LIMIT ?;",
        sizeof(kondisi) - strlen(kondisi) - 1);

    stmt = NULL;
    rc = sqlite3_prepare_v2(db,
        "SELECT kata FROM kata_fts WHERE "
        "AND length(kata) >= ? AND length(kata) <= ? LIMIT ?;",
        -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, kata, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2,
        len_kata > jarak_maks
        ? len_kata - jarak_maks : 1);
    sqlite3_bind_int(stmt, 3, len_kata + jarak_maks);
    sqlite3_bind_int(stmt, 4, CARI_MAKS_TRIGRAM);

    count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW
        && count < CARI_MAKS_TRIGRAM) {
        val = (const char *)sqlite3_column_text(stmt, 0);
        if (!val) continue;

        jarak = hitung_jarak_levenshtein(kata, val);
        if (jarak < 0) continue;

        pm = (int)strlen(val);
        pmax = (len_kata > pm) ? len_kata : pm;
        skor = hitung_skor_fuzzy(jarak, pmax);

        if (jarak <= jarak_maks && skor >= skor_min) {
            snprintf(kandidat[count],
                (size_t)sizeof(kandidat[count]),
                "%s", val);
            skor_arr[count] = skor;
            jarak_arr[count] = jarak;
            count++;
        }
    }

    sqlite3_finalize(stmt);

    if (count <= 0) return -1;

    found_idx = 0;
    for (i = 1; i < count; i++) {
        if (skor_arr[i] > skor_arr[found_idx]) {
            found_idx = i;
        } else if (skor_arr[i] == skor_arr[found_idx]
            && jarak_arr[i] < jarak_arr[found_idx]) {
            found_idx = i;
        }
    }

    snprintf(hasil, (size_t)ukuran, "%s", kandidat[found_idx]);
    return 0;
}

/* ================================================================== *
 *              PENCARIAN LEVENSHTEIN TERBATAS (FALLBACK)                *
 * ================================================================== */

static int cari_levenshtein_terbatas(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *kata,
    char *hasil,
    int ukuran,
    int jarak_maks,
    double skor_min)
{
    int rc, count, found_idx, i, jarak, pm, pmax;
    double skor;
    const char *val;
    char kandidat[CARI_MAKS_FUZZY][MAX_PANJANG_TOKEN];
    double skor_arr[CARI_MAKS_FUZZY];
    int jarak_arr[CARI_MAKS_FUZZY];
    int len_kata, len_val;

    if (!db || !cache || !kata || !hasil || ukuran <= 0) return -1;
    hasil[0] = '\0';

    len_kata = (int)strlen(kata);

    rc = sqlite3_reset(cache->stmt_kata_list);
    if (rc != SQLITE_OK) return -1;

    count = 0;
    while (sqlite3_step(cache->stmt_kata_list) == SQLITE_ROW
        && count < CARI_MAKS_FUZZY) {
        val = (const char *)sqlite3_column_text(
            cache->stmt_kata_list, 0);
        if (!val) continue;

        len_val = (int)strlen(val);
        pmax = (len_kata > len_val) ? len_kata : len_val;

        if (pmax > len_kata + jarak_maks) continue;
        if (pmax < len_kata - jarak_maks) continue;

        jarak = hitung_jarak_levenshtein(kata, val);
        if (jarak < 0) continue;

        pm = len_val;
        skor = hitung_skor_fuzzy(jarak, pmax);

        if (jarak <= jarak_maks && skor >= skor_min) {
            snprintf(kandidat[count],
                (size_t)sizeof(kandidat[count]),
                "%s", val);
            skor_arr[count] = skor;
            jarak_arr[count] = jarak;
            count++;
        }
    }

    sqlite3_reset(cache->stmt_kata_list);

    if (count <= 0) return -1;

    found_idx = 0;
    for (i = 1; i < count; i++) {
        if (skor_arr[i] > skor_arr[found_idx]) {
            found_idx = i;
        } else if (skor_arr[i] == skor_arr[found_idx]
            && jarak_arr[i] < jarak_arr[found_idx]) {
            found_idx = i;
        }
    }

    snprintf(hasil, (size_t)ukuran, "%s", kandidat[found_idx]);
    return 0;
}

/* ================================================================== *
 *                       SKOR JARAK PANJANG DINAMIS                     *
 * ================================================================== */

static double hitung_jarak_maks_rekomendasi(int panjang_kata)
{
    if (panjang_kata <= 3) return 1.0;
    if (panjang_kata <= 5) return 2.0;
    if (panjang_kata <= 7) return 3.0;
    if (panjang_kata <= 10) return 4.0;
    return 5.0;
}

/* ================================================================== *
 *                     FUNGSI PENCARIAN UTAMA                            *
 * ================================================================== */

int cari_topik_optimal(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *masukan,
    char *topik_norm,
    int ukuran,
    KandidatFuzzy kandidat[],
    int max_kandidat)
{
    char hasil_tmp[MAX_PANJANG_TOKEN];
    double skor_min_efektif;
    int len_masukan, jarak_maks_dinamis, rc;

    if (!db || !cache || !masukan || !topik_norm
        || ukuran <= 0) {
        return -1;
    }

    len_masukan = (int)strlen(masukan);
    if (len_masukan <= 1) {
        snprintf(topik_norm, (size_t)ukuran, "%s", masukan);
        return -1;
    }

    jarak_maks_dinamis = (int)hitung_jarak_maks_rekomendasi(
        len_masukan);
    skor_min_efektif = CARI_SKOR_MIN_EFEKTIF(
        (double)jarak_maks_dinamis / (double)len_masukan);

    if (cari_persis_dari_db(db, cache, masukan,
        hasil_tmp, (int)sizeof(hasil_tmp)) == 0) {
        if (kandidat && max_kandidat > 0) {
            kandidat[0].jarak = 0;
            kandidat[0].skor = 1.0;
            snprintf(kandidat[0].teks,
                sizeof(kandidat[0].teks), "%s", masukan);
            snprintf(kandidat[0].cocok,
                sizeof(kandidat[0].cocok), "%s", masukan);
        }
        snprintf(topik_norm, (size_t)ukuran, "%s", masukan);
        return 0;
    }

    if (cari_awalan_dari_db(db, masukan, hasil_tmp,
        (int)sizeof(hasil_tmp), jarak_maks_dinamis,
        skor_min_efektif) == 0) {
        if (kandidat && max_kandidat > 0) {
            int jr = hitung_jarak_levenshtein(
                masukan, hasil_tmp);
            int pm = (int)strlen(hasil_tmp);
            int pmax = (len_masukan > pm) ? len_masukan : pm;

            kandidat[0].jarak = jr;
            kandidat[0].skor = hitung_skor_fuzzy(jr, pmax);
            snprintf(kandidat[0].teks,
                sizeof(kandidat[0].teks), "%s", masukan);
            snprintf(kandidat[0].cocok,
                sizeof(kandidat[0].cocok), "%s", hasil_tmp);
        }
        snprintf(topik_norm, (size_t)ukuran, "%s", hasil_tmp);
        return 0;
    }

    rc = cari_dengan_fts5(db, masukan, hasil_tmp,
        (int)sizeof(hasil_tmp), jarak_maks_dinamis,
        skor_min_efektif);

    if (rc == 0) {
        if (kandidat && max_kandidat > 0) {
            int jr = hitung_jarak_levenshtein(
                masukan, hasil_tmp);
            int pm = (int)strlen(hasil_tmp);
            int pmax = (len_masukan > pm) ? len_masukan : pm;

            kandidat[0].jarak = jr;
            kandidat[0].skor = hitung_skor_fuzzy(jr, pmax);
            snprintf(kandidat[0].teks,
                sizeof(kandidat[0].teks), "%s", masukan);
            snprintf(kandidat[0].cocok,
                sizeof(kandidat[0].cocok), "%s", hasil_tmp);
        }
        snprintf(topik_norm, (size_t)ukuran, "%s", hasil_tmp);
        return 0;
    }

    if (cari_levenshtein_terbatas(db, cache, masukan,
        hasil_tmp, (int)sizeof(hasil_tmp),
        jarak_maks_dinamis, skor_min_efektif) == 0) {
        if (kandidat && max_kandidat > 0) {
            int jr = hitung_jarak_levenshtein(
                masukan, hasil_tmp);
            int pm = (int)strlen(hasil_tmp);
            int pmax = (len_masukan > pm) ? len_masukan : pm;

            kandidat[0].jarak = jr;
            kandidat[0].skor = hitung_skor_fuzzy(jr, pmax);
            snprintf(kandidat[0].teks,
                sizeof(kandidat[0].teks), "%s", masukan);
            snprintf(kandidat[0].cocok,
                sizeof(kandidat[0].cocok), "%s", hasil_tmp);
        }
        snprintf(topik_norm, (size_t)ukuran, "%s", hasil_tmp);
        return 0;
    }

    snprintf(topik_norm, (size_t)ukuran, "%s", masukan);
    return -1;
}

/* ================================================================== *
 *                    INISIALISASI FTS5 UNTUK KATA                       *
 * ================================================================== */

int inisialisasi_fts5_kata(sqlite3 *db)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!db) return -1;

    stmt = NULL;
    rc = sqlite3_prepare_v2(db,
        "SELECT name FROM sqlite_master WHERE type='table' "
        "AND name='kata_fts';",
        -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rc = 0;
    }
    sqlite3_finalize(stmt);

    if (rc == 0) {
        stmt = NULL;
        rc = sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO kata_fts(kata) "
            "SELECT kata FROM kata;",
            -1, &stmt, NULL);
        if (rc != SQLITE_OK) return -1;

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return 0;
    }

    rc = sqlite3_exec(db,
        "CREATE VIRTUAL TABLE IF NOT EXISTS kata_fts "
        "USING fts5(kata, content='kata', "
        "content_rowid='id');",
        NULL, NULL, NULL);

    if (rc != SQLITE_OK) return -1;

    stmt = NULL;
    rc = sqlite3_prepare_v2(db,
        "INSERT INTO kata_fts(rowid, kata) "
        "SELECT id, kata FROM kata;",
        -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    rc = sqlite3_exec(db,
        "CREATE TRIGGER IF NOT EXISTS kata_fts_ins "
        "AFTER INSERT ON kata BEGIN "
        "INSERT INTO kata_fts(rowid, kata) "
        "VALUES (new.id, new.kata); "
        "END;",
        NULL, NULL, NULL);

    if (rc != SQLITE_OK) return -1;

    rc = sqlite3_exec(db,
        "CREATE TRIGGER IF NOT EXISTS kata_fts_del "
        "AFTER DELETE ON kata BEGIN "
        "INSERT INTO kata_fts(kata_fts, rowid, kata) "
        "VALUES('delete', old.id, old.kata); "
        "END;",
        NULL, NULL, NULL);

    if (rc != SQLITE_OK) return -1;

    rc = sqlite3_exec(db,
        "CREATE TRIGGER IF NOT EXISTS kata_fts_upd "
        "AFTER UPDATE ON kata BEGIN "
        "INSERT INTO kata_fts(kata_fts, rowid, kata) "
        "VALUES('delete', old.id, old.kata); "
        "INSERT INTO kata_fts(rowid, kata) "
        "VALUES (new.id, new.kata); "
        "END;",
        NULL, NULL, NULL);

    if (rc != SQLITE_OK) return -1;

    return 0;
}
