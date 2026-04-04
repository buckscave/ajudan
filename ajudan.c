/* ========================================================================== *
 * AJUDAN 3.0 - Program Utama                                                 *
 * ========================================================================== */

#include "ajudan.h"

/* Logging Global Implementation */
int g_log_on = 0;

/* Implementasi ajudan_logf sederhana di sini jika tidak mau di header */
void ajudan_logf(const char *fmt, const char *a, const char *b, const char *c) {
    if (g_log_on) {
        fprintf(stderr, "[ajudan] ");
        if (fmt) {
            if (!a && !b && !c) fprintf(stderr, "%s\n", fmt);
            else if (a && !b && !c) fprintf(stderr, fmt, a);
            else if (a && b && !c) fprintf(stderr, fmt, a, b);
            else if (a && b && c) fprintf(stderr, fmt, a, b, c);
            else fprintf(stderr, "%s\n", fmt);
        }
        fprintf(stderr, "\n");
    }
}

/* Wrapper Inisialisasi */
static int jalankan_inisialisasi(void) {
    InfoKesalahan err;
    err.kode = 0; err.pesan[0] = '\0';
    printf("Inisialisasi database...\n");
    if (inisialisasi_basisdata(&err) != 0) {
        cetak_error(&err);
        return -1;
    }
    printf("Sukses.\n");
    return 0;
}

/* Wrapper Impor (Contoh untuk kata) */
static int jalankan_impor_kata(const char *file) {
    InfoKesalahan err;
    err.kode = 0; err.pesan[0] = '\0';
    printf("Impor kata dari: %s\n", file);
    if (impor_kata(file, &err) != 0) {
        cetak_error(&err);
        return -1;
    }
    printf("Sukses.\n");
    return 0;
}

/* Wrapper Impor Kategori (Dinamis) */
static int jalankan_impor_kategori_kata(const char *file) {
    InfoKesalahan err;
    err.kode = 0; err.pesan[0] = '\0';
    printf("Impor kategori kata dari: %s\n", file);
    if (impor_kategori_kata(file, &err) != 0) {
        cetak_error(&err);
        return -1;
    }
    printf("Sukses.\n");
    return 0;
}

static void cetak_penggunaan(void) {
    printf("Penggunaan: ./ajudan [opsi] [argumen]\n");
    printf("Opsi:\n");
    printf("  -init           : Inisialisasi database (buat tabel)\n");
    printf("  -impor-kata <file>   : Impor data kata\n");
    printf("  -impor-cat-kata <file> : Impor kategori kata (dinamis)\n");
    /* Tambahkan opsi impor lain sesuai kebutuhan */
    printf("  -chat           : Mode interaktif\n");
    printf("  -log            : Aktifkan logging\n");
}

/* Fungsi Chat */
static void jalankan_chat(void) {
    sqlite3 *db;
    char input[MAX_INPUT_USER];
    char output[MAX_RESPONS];

    if (buka_basisdata(&db) != 0) {
        fprintf(stderr, "Gagal membuka database.\n");
        return;
    }

    printf("=== AJUDAN 3.0 Chat ===\n");
    printf("Ketik 'keluar' untuk berhenti.\n");

    while (1) {
        printf("\nAnda: ");
        if (!fgets(input, sizeof(input), stdin)) break;
        trim(input);
        if (strcmp(input, "keluar") == 0) break;

        if (proses_percakapan(db, "user_cli", input, output, sizeof(output)) == 0) {
            printf("\nBot: %s\n", output);
        } else {
            printf("\nBot: Maaf, terjadi kesalahan internal.\n");
        }
    }

    tutup_basisdata(db);
}

int main(int argc, char **argv) {
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
    else if (strcmp(argv[1], "-impor-kata") == 0) {
        if (argc < 3) { fprintf(stderr, "Butuh nama file.\n"); return 1; }
        return jalankan_impor_kata(argv[2]);
    }
    else if (strcmp(argv[1], "-impor-cat-kata") == 0) {
        if (argc < 3) { fprintf(stderr, "Butuh nama file.\n"); return 1; }
        return jalankan_impor_kategori_kata(argv[2]);
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
