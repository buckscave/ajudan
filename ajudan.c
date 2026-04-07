/* ========================================================================== *
 * AJUDAN 4.0 - Program Utama                                                 *
 * -------------------------------------------------------------------------- *
 * Standar: C89, POSIX, SQLITE3                                                 *
 * ========================================================================== */

#include "ajudan.h"

/* ================================================================== *
 *                    VARIABEL GLOBAL                                          *
 * ================================================================== */

int g_log_on = 0;

/* Cache linguistik global (dimuat sekali saat chat) */
static CacheLinguistik g_cache;

/* ================================================================== *
 *                    IMPLEMENTASI LOG                                         *
 * ================================================================== */

void ajudan_logf(
    const char *fmt,
    const char *a,
    const char *b,
    const char *c)
{
    if (!g_log_on) return;
    fprintf(stderr, "[ajudan] ");
    if (fmt) {
        if (!a && !b && !c)
            fprintf(stderr, "%s\n", fmt);
        else if (a && !b && !c)
            fprintf(stderr, fmt, a);
        else if (a && b && !c)
            fprintf(stderr, fmt, a, b);
        else if (a && b && c)
            fprintf(stderr, fmt, a, b, c);
        else
            fprintf(stderr, "%s\n", fmt);
    }
    fprintf(stderr, "\n");
}

/* ================================================================== *
 *                  WRAPPER INISIALISASI                                  *
 * ================================================================== */

static int jalankan_inisialisasi(void)
{
    InfoKesalahan err;
    err.kode = 0;
    err.pesan[0] = '\0';
    printf("Inisialisasi database...\n");
    if (inisialisasi_basisdata(&err) != 0) {
        cetak_error(&err);
        return -1;
    }
    printf("Sukses.\n");
    return 0;
}

static int jalankan_impor_kata(const char *file)
{
    InfoKesalahan err;
    err.kode = 0;
    err.pesan[0] = '\0';
    printf("Impor kata dari: %s\n", file);
    if (impor_kata(file, &err) != 0) {
        cetak_error(&err);
        return -1;
    }
    printf("Sukses.\n");
    return 0;
}

static int jalankan_impor_kategori_kata(const char *file)
{
    InfoKesalahan err;
    err.kode = 0;
    err.pesan[0] = '\0';
    printf("Impor kategori kata dari: %s\n", file);
    if (impor_kategori_kata(file, &err) != 0) {
        cetak_error(&err);
        return -1;
    }
    printf("Sukses.\n");
    return 0;
}

static int jalankan_impor_seed_v4(const char *file)
{
    sqlite3 *db;
    char *sql = NULL;
    long ukuran;
    FILE *fp;
    char *err_msg = NULL;
    int rc;

    printf("Impor seed data v4 dari: %s\n", file);
    fp = fopen(file, "r");
    if (!fp) {
        fprintf(stderr,
            "Gagal membuka file: %s\n", file);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    ukuran = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (ukuran <= 0 || ukuran > 10 * 1024 * 1024) {
        fprintf(stderr,
            "Ukuran file tidak wajar: %ld\n", ukuran);
        fclose(fp);
        return -1;
    }

    sql = (char *)malloc((size_t)(ukuran + 1));
    if (!sql) {
        fprintf(stderr, "Gagal alokasi memori.\n");
        fclose(fp);
        return -1;
    }

    {
        size_t dibaca = fread(sql, 1,
            (size_t)ukuran, fp);
        sql[dibaca] = '\0';
    }
    fclose(fp);

    if (buka_basisdata(&db) != 0) {
        free(sql);
        return -1;
    }

    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr,
            "Error impor seed: %s\n",
            err_msg ? err_msg : "unknown");
        if (err_msg) sqlite3_free(err_msg);
        free(sql);
        tutup_basisdata(db);
        return -1;
    }

    if (err_msg) sqlite3_free(err_msg);
    free(sql);
    tutup_basisdata(db);
    printf("Sukses.\n");
    return 0;
}

/* ================================================================== *
 *                       MODE CHAT                                            *
 * ================================================================== */

static void jalankan_chat(void)
{
    sqlite3 *db;
    char input[MAX_INPUT_USER];
    char output[MAX_RESPONS];

    if (buka_basisdata(&db) != 0) {
        fprintf(stderr,
            "Gagal membuka database.\n");
        return;
    }

    /* Muat cache linguistik ke memori */
    if (muat_semua_cache(db, &g_cache) != 0) {
        fprintf(stderr,
            "Peringatan: cache linguistik "
            "gagal dimuat, bot "
            "akan berjalan tanpa cache.\n");
    }

    printf("=== AJUDAN 4.0 Chat ===\n");
    printf("Ketik 'keluar' untuk berhenti.\n");

    while (1) {
        printf("\nAnda: ");
        if (!fgets(input, sizeof(input), stdin))
            break;
        trim(input);
        if (strcmp(input, "keluar") == 0)
            break;

        if (proses_percakapan(
                db, &g_cache,
                "user_cli", input,
                output, sizeof(output)) == 0) {
            printf("\nBot: %s\n", output);
        } else {
            printf("\nBot: "
                "Maaf, terjadi kesalahan "
                "internal.\n");
        }
    }

    bersihkan_cache(&g_cache);
    tutup_basisdata(db);
}

/* ================================================================== *
 *                         PENGATURAN                                         *
 * ================================================================== */

static void cetak_penggunaan(void)
{
    printf("Penggunaan: ./ajudan [opsi] [argumen]\n");
    printf("Opsi:\n");
    printf("  -init"
        "           : Inisialisasi database\n");
    printf("  -impor-kata <file>"
        "   : Impor data kata\n");
    printf("  -impor-cat-kata <file>"
        " : Impor kategori kata\n");
    printf("  -seed-v4 <file>"
        "  : Impor seed data v4\n");
    printf("  -chat"
        "           : Mode interaktif\n");
    printf("  -log"
        "            : Aktifkan logging\n");
}

/* ================================================================== *
 *                          MAIN                                              *
 * ================================================================== */

int main(int argc, char **argv)
{
    int i;

    /* Cek flag log */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-log") == 0) {
            g_log_on = 1;
            break;
        }
    }

    if (argc < 2) {
        cetak_penggunaan();
        return 1;
    }

    if (strcmp(argv[1], "-init") == 0) {
        return jalankan_inisialisasi();
    }
    else if (strcmp(argv[1],
            "-impor-kata") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Butuh nama file.\n");
            return 1;
        }
        return jalankan_impor_kata(argv[2]);
    }
    else if (strcmp(argv[1],
            "-impor-cat-kata") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Butuh nama file.\n");
            return 1;
        }
        return jalankan_impor_kategori_kata(
            argv[2]);
    }
    else if (strcmp(argv[1],
            "-seed-v4") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Butuh nama file.\n");
            return 1;
        }
        return jalankan_impor_seed_v4(argv[2]);
    }
    else if (strcmp(argv[1], "-chat") == 0) {
        jalankan_chat();
        return 0;
    }
    else {
        cetak_penggunaan();
        return 1;
    }
}
