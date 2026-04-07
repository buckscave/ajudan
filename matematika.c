/* ================================================================== *
 * AJUDAN 4.0 - Modul Penalaran Matematika (matematika.c)               *
 * ------------------------------------------------------------------ *
 * Modul ini menangani penalaran aritmatika dasar berbasis database.   *
 * Bot dapat memahami soal cerita dalam bahasa Indonesia,              *
 * mengidentifikasi angka (teks dan digit), operasi, dan               *
 * menyusun langkah perhitungan.                                      *
 * Operasi: tambah, kurang, kali, bagi, modulus.                      *
 * ------------------------------------------------------------------ *
 * C89, POSIX                                                          *
 * ================================================================== */

#include "ajudan.h"

/* ================================================================== *
 *  KONVERSI KATA BILANGAN KE ANGKA                                    *
 * ================================================================== */

/*
 * konversi_kata_ke_angka - Mengubah kata bilangan Indonesia
 * menjadi nilai integer.
 * Contoh: "lima" -> 5, "dua belas" -> 12, "seratus" -> 100
 * Mengembalikan 0 jika tidak dikenali.
 */
int konversi_kata_ke_angka(
    CacheLinguistik *cache,
    const char *kata,
    int *nilai)
{
    int i;
    char lo[MAX_PANJANG_STRING];
    int angka_cari;
    int panjang;

    if (!cache || !kata || !nilai) return -1;
    if (!cache->dimuat) return -1;
    if (!kata[0]) return -1;

    *nilai = 0;

    /* Cek apakah kata sudah berupa digit */
    panjang = (int)strlen(kata);
    if (panjang > 0 && panjang <= 10) {
        int semua_digit = 1;
        int j;
        for (j = 0; j < panjang; j++) {
            if (kata[j] < '0' || kata[j] > '9') {
                semua_digit = 0;
                break;
            }
        }
        if (semua_digit) {
            *nilai = atoi(kata);
            return 0;
        }
    }

    snprintf(lo, sizeof(lo), "%s", kata);
    to_lower_case(lo);
    trim(lo);

    /* Pencarian langsung di cache */
    for (i = 0; i < cache->n_kata_bilangan; i++) {
        if (ajudan_strcasecmp(
            cache->kata_bilangan[i].kata, lo) == 0) {
            *nilai = cache->kata_bilangan[i].nilai;
            return 0;
        }
    }

    /* Cek pola "X belas" (11-19) */
    if (panjang > 5 && (ajudan_strcasecmp(
        lo + panjang - 5, "belas") == 0)) {
        char prefix[32];
        int nilai_prefix;
        int len_prefix;

        len_prefix = panjang - 5;
        if (len_prefix > 0 && len_prefix < 32) {
            strncpy(prefix, lo, len_prefix);
            prefix[len_prefix] = '\0';

            for (i = 0; i < cache->n_kata_bilangan; i++) {
                if (ajudan_strcasecmp(
                    cache->kata_bilangan[i].kata,
                    prefix) == 0) {
                    nilai_prefix =
                        cache->kata_bilangan[i].nilai;
                    *nilai = nilai_prefix + 10;
                    return 0;
                }
            }
        }
    }

    /* Cek pola "X puluh" (20, 30, ..., 90) */
    if (panjang > 5 && (ajudan_strcasecmp(
        lo + panjang - 5, "puluh") == 0)) {
        char prefix[32];
        int nilai_prefix;
        int len_prefix;

        len_prefix = panjang - 5;
        if (len_prefix > 0 && len_prefix < 32) {
            strncpy(prefix, lo, len_prefix);
            prefix[len_prefix] = '\0';

            for (i = 0; i < cache->n_kata_bilangan; i++) {
                if (ajudan_strcasecmp(
                    cache->kata_bilangan[i].kata,
                    prefix) == 0) {
                    nilai_prefix =
                        cache->kata_bilangan[i].nilai;
                    *nilai = nilai_prefix * 10;
                    return 0;
                }
            }
        }
    }

    /* Cek pola "X ratus" (100-900) */
    if (panjang > 5 && (ajudan_strcasecmp(
        lo + panjang - 5, "ratus") == 0)) {
        char prefix[32];
        int nilai_prefix;
        int len_prefix;

        len_prefix = panjang - 5;
        if (len_prefix > 0 && len_prefix < 32) {
            strncpy(prefix, lo, len_prefix);
            prefix[len_prefix] = '\0';

            if (ajudan_strcasecmp(prefix, "se") == 0) {
                *nilai = 100;
                return 0;
            }
            for (i = 0; i < cache->n_kata_bilangan; i++) {
                if (ajudan_strcasecmp(
                    cache->kata_bilangan[i].kata,
                    prefix) == 0) {
                    nilai_prefix =
                        cache->kata_bilangan[i].nilai;
                    *nilai = nilai_prefix * 100;
                    return 0;
                }
            }
        }
    }

    /* Cek pola "X ribu" (1000-9000) */
    if (panjang > 4 && (ajudan_strcasecmp(
        lo + panjang - 4, "ribu") == 0)) {
        char prefix[32];
        int nilai_prefix;
        int len_prefix;

        len_prefix = panjang - 4;
        if (len_prefix > 0 && len_prefix < 32) {
            strncpy(prefix, lo, len_prefix);
            prefix[len_prefix] = '\0';

            if (ajudan_strcasecmp(prefix, "se") == 0) {
                *nilai = 1000;
                return 0;
            }
            for (i = 0; i < cache->n_kata_bilangan; i++) {
                if (ajudan_strcasecmp(
                    cache->kata_bilangan[i].kata,
                    prefix) == 0) {
                    nilai_prefix =
                        cache->kata_bilangan[i].nilai;
                    *nilai = nilai_prefix * 1000;
                    return 0;
                }
            }
        }
    }

    return -1;
}

/* ================================================================== *
 *  DETEKSI INTENT MATEMATIKA                                          *
 * ================================================================== */

/*
 * deteksi_intent_matematika - Memeriksa apakah input
 * mengandung soal matematika.
 * Indikasi: adanya angka/digit + kata operasi + pertanyaan.
 */
int deteksi_intent_matematika(
    CacheLinguistik *cache,
    const char *input)
{
    int i, j;
    char lo[MAX_INPUT_USER];
    int ada_angka = 0;
    int ada_operasi = 0;
    int ada_tanya = 0;

    if (!cache || !input || !input[0]) return 0;
    if (!cache->dimuat) return 0;

    snprintf(lo, sizeof(lo), "%s", input);
    to_lower_case(lo);
    trim(lo);

    /* Cek keberadaan angka (digit atau kata bilangan) */
    for (i = 0; lo[i]; i++) {
        if (lo[i] >= '0' && lo[i] <= '9') {
            ada_angka = 1;
            break;
        }
    }

    if (!ada_angka) {
        /* Cek kata bilangan dalam cache */
        for (j = 0; j < cache->n_kata_bilangan; j++) {
            if (strstr(lo,
                cache->kata_bilangan[j].kata) != NULL) {
                ada_angka = 1;
                break;
            }
        }
    }

    /* Cek kata operasi */
    for (j = 0; j < cache->n_kata_operasi_mat; j++) {
        if (ajudan_strcasecmp(
            cache->kata_operasi_mat[j].operasi,
            "tanya") == 0) {
            continue;
        }
        if (strstr(lo,
            cache->kata_operasi_mat[j].kata) != NULL) {
            ada_operasi = 1;
            break;
        }
    }

    /* Cek kata tanya operasi */
    for (j = 0; j < cache->n_kata_operasi_mat; j++) {
        if (ajudan_strcasecmp(
            cache->kata_operasi_mat[j].operasi,
            "tanya") != 0) {
            continue;
        }
        if (strstr(lo,
            cache->kata_operasi_mat[j].kata) != NULL) {
            ada_tanya = 1;
            break;
        }
    }

    /* Butuh minimal angka + (operasi atau tanya) */
    if (ada_angka && (ada_operasi || ada_tanya))
        return 1;

    return 0;
}

/* ================================================================== *
 *  PARSING SOAL MATEMATIKA                                            *
 * ================================================================== */

/*
 * cari_angka_berikutnya - Scan string mulai dari posisi,
 * cari angka atau kata bilangan berikutnya.
 * Mengembalikan posisi setelah angka ditemukan,
 * atau -1 jika tidak ada.
 */
static int cari_angka_berikutnya(
    CacheLinguistik *cache,
    const char *teks,
    int mulai_dari,
    int *nilai_angka,
    int *panjang_kata)
{
    int i;
    char lo[MAX_INPUT_USER];

    if (!teks || !nilai_angka || !panjang_kata)
        return -1;
    *nilai_angka = -1;
    *panjang_kata = 0;

    for (i = mulai_dari; teks[i] != '\0'; i++) {
        /* Lewati spasi */
        if (teks[i] == ' ') continue;

        /* Cek digit langsung */
        if (teks[i] >= '0' && teks[i] <= '9') {
            int awal = i;
            while (teks[i] >= '0' && teks[i] <= '9')
                i++;
            {
                char buf[16];
                int len = i - awal;
                if (len > 15) len = 15;
                strncpy(buf, teks + awal, len);
                buf[len] = '\0';
                *nilai_angka = atoi(buf);
                *panjang_kata = len;
                return i;
            }
        }

        /* Cek kata bilangan multi-kata */
        if (isalpha((unsigned char)teks[i])) {
            int awal = i;
            char kata_coba[MAX_PANJANG_STRING];
            int j;

            while (teks[i] != '\0' &&
                teks[i] != ' ' &&
                teks[i] != ',' &&
                teks[i] != '.') {
                i++;
            }

            /* Coba ambil 1-3 kata berikutnya */
            {
                int panjang_pot = i - awal;
                if (panjang_pot >=
                    MAX_PANJANG_STRING - 1)
                    panjang_pot =
                        MAX_PANJANG_STRING - 2;
                strncpy(kata_coba, teks + awal,
                    panjang_pot);
                kata_coba[panjang_pot] = '\0';

                /* Coba langsung */
                if (konversi_kata_ke_angka(cache,
                    kata_coba, nilai_angka) == 0) {
                    *panjang_kata = panjang_pot;
                    return i;
                }

                /* Coba 2 kata pertama */
                for (j = 0; kata_coba[j]; j++) {
                    if (kata_coba[j] == ' ') {
                        char kata2[MAX_PANJANG_STRING];
                        int len2 = j;
                        if (len2 >
                            MAX_PANJANG_STRING - 1)
                            len2 =
                                MAX_PANJANG_STRING - 2;
                        strncpy(kata2, kata_coba, len2);
                        kata2[len2] = '\0';

                        if (konversi_kata_ke_angka(
                            cache, kata2,
                            nilai_angka) == 0) {
                            *panjang_kata = len2;
                            return awal + len2;
                        }
                        break;
                    }
                }
            }

            /* Lanjut scan dari posisi saat ini */
            i--;
        }
    }

    return -1;
}

/*
 * cari_operasi_berikutnya - Cari kata operasi matematika
 * dalam teks mulai dari posisi tertentu.
 * Mengembalikan tipe operasi atau -1.
 */
static int cari_operasi_berikutnya(
    CacheLinguistik *cache,
    const char *teks,
    int mulai_dari,
    int *panjang_kata)
{
    int i;
    char lo[MAX_INPUT_USER];
    int best_pos = -1;
    int best_len = 0;
    TipeOperasiMat op_terbaik = MAT_OP_TAMBAH;

    if (!cache || !teks || !panjang_kata) return -1;
    *panjang_kata = 0;

    for (i = 0; i < cache->n_kata_operasi_mat; i++) {
        const char *p;
        int j, len_kata;

        /* Lewati kata tanya saja */
        if (ajudan_strcasecmp(
            cache->kata_operasi_mat[i].operasi,
            "tanya") == 0)
            continue;

        /* Lewati inisial (konteks, bukan operasi) */
        if (ajudan_strcasecmp(
            cache->kata_operasi_mat[i].kategori,
            "konteks") == 0)
            continue;

        p = strstr(teks + mulai_dari,
            cache->kata_operasi_mat[i].kata);
        if (p == NULL) continue;

        /* Pastikan cocok paling awal */
        if (p - teks < mulai_dari) continue;

        /* Pastikan kecocokan kata utuh */
        len_kata = (int)strlen(
            cache->kata_operasi_mat[i].kata);
        if (p[len_kata] != '\0' &&
            p[len_kata] != ' ' &&
            p[len_kata] != ',' &&
            p[len_kata] != '.') {
            continue;
        }

        if (best_pos < 0 ||
            (int)(p - teks) < best_pos) {
            best_pos = (int)(p - teks);
            best_len = len_kata;

            if (ajudan_strcasecmp(
                cache->kata_operasi_mat[i].operasi,
                "tambah") == 0)
                op_terbaik = MAT_OP_TAMBAH;
            else if (ajudan_strcasecmp(
                cache->kata_operasi_mat[i].operasi,
                "kurang") == 0)
                op_terbaik = MAT_OP_KURANG;
            else if (ajudan_strcasecmp(
                cache->kata_operasi_mat[i].operasi,
                "kali") == 0)
                op_terbaik = MAT_OP_KALI;
            else if (ajudan_strcasecmp(
                cache->kata_operasi_mat[i].operasi,
                "bagi") == 0)
                op_terbaik = MAT_OP_BAGI;
            else if (ajudan_strcasecmp(
                cache->kata_operasi_mat[i].operasi,
                "modulus") == 0)
                op_terbaik = MAT_OP_MODULUS;
        }
    }

    if (best_pos >= 0) {
        *panjang_kata = best_len;
        return (int)op_terbaik;
    }

    return -1;
}

/*
 * konversi_angka_ke_kata - Mengubah angka menjadi
 * kata bilangan Indonesia sederhana.
 * Hanya mendukung 0-19, 20, 30, ..., 90, 100, 1000.
 */
static void konversi_angka_ke_kata(
    CacheLinguistik *cache,
    int angka,
    char *kata, size_t ukuran)
{
    int i;

    if (!kata || ukuran == 0) return;

    /* Cek langsung di cache */
    for (i = 0; i < cache->n_kata_bilangan; i++) {
        if (cache->kata_bilangan[i].nilai == angka) {
            snprintf(kata, ukuran, "%s",
                cache->kata_bilangan[i].kata);
            return;
        }
    }

    /* Fallback: gunakan digit */
    snprintf(kata, ukuran, "%d", angka);
}

/*
 * parse_soal_matematika - Parse soal cerita matematika
 * dari input bahasa Indonesia.
 * Mengekstrak: angka-angka, operasi, dan konteks.
 */
int parse_soal_matematika(
    CacheLinguistik *cache,
    const char *input,
    HasilPerhitunganMat *hasil)
{
    char lo[MAX_INPUT_USER];
    int pos;
    int angka[MAX_LANGKAH_MAT];
    int n_angka;
    int operasi[MAX_LANGKAH_MAT];
    int n_operasi;
    int i;
    int posisi_op;
    int panjang_op;
    int tipe_op;
    int nilai_a;
    int panjang_a;
    int j;
    int akumulasi;

    if (!cache || !input || !hasil) return -1;
    if (!cache->dimuat) return -1;

    memset(hasil, 0, sizeof(HasilPerhitunganMat));

    snprintf(lo, sizeof(lo), "%s", input);
    to_lower_case(lo);
    trim(lo);

    /* Ekstrak semua angka dari teks */
    n_angka = 0;
    pos = 0;
    while (pos < (int)strlen(lo) &&
        n_angka < MAX_LANGKAH_MAT) {
        int nilai;
        int panjang;

        nilai = 0;
        panjang = 0;
        pos = cari_angka_berikutnya(
            cache, lo, pos, &nilai, &panjang);

        if (pos < 0 || nilai < 0) break;

        angka[n_angka] = nilai;
        n_angka++;
        pos += (panjang > 0) ? panjang : 1;
    }

    if (n_angka < 2) {
        /* Hanya 1 angka, coba cari yang kedua */
        if (n_angka == 1) {
            snprintf(
                hasil->langkah[0].deskripsi,
                sizeof(hasil->langkah[0].deskripsi),
                "hanya ditemukan satu angka: %d",
                angka[0]);
            hasil->jumlah_langkah = 1;
            hasil->hasil_akhir = angka[0];
            return 0;
        }
        return -1;
    }

    /* Ekstrak operasi di antara angka */
    n_operasi = 0;
    posisi_op = 0;
    for (i = 0; i < n_angka - 1 &&
        n_operasi < MAX_LANGKAH_MAT - 1; i++) {
        tipe_op = cari_operasi_berikutnya(
            cache, lo, posisi_op,
            &panjang_op);

        if (tipe_op >= 0) {
            operasi[n_operasi] = tipe_op;
            n_operasi++;
            posisi_op += panjang_op + 1;
        } else {
            /* Default: kurang (paling umum) */
            operasi[n_operasi] = MAT_OP_KURANG;
            n_operasi++;
        }
    }

    /* Lakukan perhitungan langkah demi langkah */
    akumulasi = angka[0];
    snprintf(hasil->langkah[0].deskripsi,
        sizeof(hasil->langkah[0].deskripsi),
        "angka awal: %d", akumulasi);
    hasil->langkah[0].nilai = akumulasi;
    hasil->jumlah_langkah = 1;

    for (j = 0; j < n_operasi &&
        hasil->jumlah_langkah < MAX_LANGKAH_MAT;
        j++) {
        int angka_berikutnya = angka[j + 1];
        int hasil_op = 0;
        int idx = hasil->jumlah_langkah;
        char nama_op[32];

        switch (operasi[j]) {
        case MAT_OP_TAMBAH:
            hasil_op = akumulasi + angka_berikutnya;
            snprintf(nama_op, sizeof(nama_op),
                "ditambah");
            break;
        case MAT_OP_KURANG:
            hasil_op = akumulasi - angka_berikutnya;
            snprintf(nama_op, sizeof(nama_op),
                "dikurangi");
            break;
        case MAT_OP_KALI:
            hasil_op = akumulasi * angka_berikutnya;
            snprintf(nama_op, sizeof(nama_op),
                "dikali");
            break;
        case MAT_OP_BAGI:
            if (angka_berikutnya == 0) {
                snprintf(
                    hasil->langkah[idx].deskripsi,
                    sizeof(hasil->langkah[idx].deskripsi),
                    "pembagian dengan nol tidak bisa dilakukan"
                    );
                hasil->langkah[idx].nilai = 0;
                hasil->jumlah_langkah++;
                return -2;
            }
            hasil_op = akumulasi / angka_berikutnya;
            snprintf(nama_op, sizeof(nama_op),
                "dibagi");
            break;
        case MAT_OP_MODULUS:
            if (angka_berikutnya == 0) {
                snprintf(
                    hasil->langkah[idx].deskripsi,
                    sizeof(hasil->langkah[idx].deskripsi),
                    "modulus dengan nol tidak bisa dilakukan"
                    );
                hasil->langkah[idx].nilai = 0;
                hasil->jumlah_langkah++;
                return -2;
            }
            hasil_op = akumulasi % angka_berikutnya;
            snprintf(nama_op, sizeof(nama_op),
                "sisa bagi");
            break;
        default:
            hasil_op = akumulasi - angka_berikutnya;
            snprintf(nama_op, sizeof(nama_op),
                "dikurangi");
            break;
        }

        snprintf(hasil->langkah[idx].deskripsi,
            sizeof(hasil->langkah[idx].deskripsi),
            "%d %s %d = %d",
            akumulasi, nama_op,
            angka_berikutnya, hasil_op);
        hasil->langkah[idx].nilai = hasil_op;
        hasil->jumlah_langkah++;

        akumulasi = hasil_op;
    }

    hasil->hasil_akhir = akumulasi;
    return 0;
}

/* ================================================================== *
 *  RANGKAI RESPON MATEMATIKA                                          *
 * ================================================================== */

/*
 * rangkai_respon_matematika - Merangkai respons
 * penalaran matematika dalam kalimat natural Indonesia.
 */
int rangkai_respon_matematika(
    CacheLinguistik *cache,
    const HasilPerhitunganMat *hasil,
    char *output,
    size_t ukuran_output)
{
    int i;
    char hasil_kata[MAX_PANJANG_STRING];
    char langkah_buf[MAX_RESPONS];
    size_t sisa;
    const char *konj;
    const char *kata_konklusi;

    if (!cache || !hasil || !output
        || ukuran_output == 0)
        return -1;

    output[0] = '\0';

    /* Jika tidak ada langkah, gunakan fallback */
    if (hasil->jumlah_langkah <= 0) {
        if (cari_fallback_respon(cache,
            "matematika_gagal", output,
            ukuran_output, "") != 0) {
            output[0] = '\0';
        }
        return (output[0] != '\0') ? 0 : -1;
    }

    /* Konversi hasil akhir ke kata bilangan */
    konversi_angka_ke_kata(cache,
        hasil->hasil_akhir,
        hasil_kata, sizeof(hasil_kata));

    /* Cari konjungsi sekuensial dari cache */
    konj = NULL;
    for (i = 0; i < cache->n_konjungsi_respon; i++) {
        if (ajudan_strcasecmp(
            cache->konjungsi_respon[i].jenis,
            "sekuensial") == 0) {
            konj = cache->konjungsi_respon[i].kata;
            break;
        }
    }

    /* Cari kata konklusi dari cache */
    kata_konklusi = NULL;
    for (i = 0; i < cache->n_konjungsi_respon; i++) {
        if (ajudan_strcasecmp(
            cache->konjungsi_respon[i].jenis,
            "konklusi") == 0) {
            kata_konklusi =
                cache->konjungsi_respon[i].kata;
            break;
        }
    }
    if (!kata_konklusi || !kata_konklusi[0]) {
        for (i = 0; i < cache->n_konjungsi_respon; i++) {
            if (ajudan_strcasecmp(
                cache->konjungsi_respon[i].jenis,
                "penegas") == 0) {
                kata_konklusi =
                    cache->konjungsi_respon[i].kata;
                break;
            }
        }
    }

    /* Bangun teks langkah perhitungan */
    langkah_buf[0] = '\0';
    for (i = 0; i < hasil->jumlah_langkah; i++) {
        sisa = sizeof(langkah_buf)
            - strlen(langkah_buf) - 2;
        if (sisa < 20) break;

        if (i > 0 && konj && konj[0]) {
            strncat(langkah_buf, " ", sisa);
            sisa = sizeof(langkah_buf)
                - strlen(langkah_buf) - 2;
            strncat(langkah_buf, konj, sisa);
            sisa = sizeof(langkah_buf)
                - strlen(langkah_buf) - 2;
            strncat(langkah_buf, " ", sisa);
        }
        strncat(langkah_buf,
            hasil->langkah[i].deskripsi, sisa);
    }

    /* Tambahkan kata konklusi dan hasil akhir */
    if (kata_konklusi && kata_konklusi[0]
        && hasil_kata[0]) {
        sisa = sizeof(langkah_buf)
            - strlen(langkah_buf) - 2;
        if (sisa > 3) {
            strncat(langkah_buf, " ", sisa);
            sisa = sizeof(langkah_buf)
                - strlen(langkah_buf) - 2;
            strncat(langkah_buf,
                kata_konklusi, sisa);
            sisa = sizeof(langkah_buf)
                - strlen(langkah_buf) - 2;
            strncat(langkah_buf, " ", sisa);
            sisa = sizeof(langkah_buf)
                - strlen(langkah_buf) - 2;
            strncat(langkah_buf,
                hasil_kata, sisa);
        }
    }

    /* Rangkai via SPOK assembly */
    if (rangkai_respon_spok(cache,
        "matematika", "normal",
        "", langkah_buf, "", "",
        output, ukuran_output) == 0) {
        return 0;
    }

    /* Fallback via SPOK konteks fallback */
    if (rangkai_respon_spok(cache,
        "matematika", "fallback",
        "", langkah_buf, "", "",
        output, ukuran_output) == 0) {
        return 0;
    }

    /* Fallback langsung dari cache */
    if (cari_fallback_respon(cache,
        "matematika_gagal", output,
        ukuran_output, "") == 0
        && output[0] != '\0') {
        return 0;
    }

    output[0] = '\0';
    return -1;
}
