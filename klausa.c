/*
 * klausa.c - Pemisahan Klausa & Klasifikasi Intent Pertanyaan
 *
 * Modul untuk menangani kalimat panjang yang mengandung
 * banyak klausa, termasuk:
 * - Pemisahan kalimat menjadi klausa-klausa
 * - Deteksi pertanyaan implisit (tanpa tanda tanya)
 * - Klasifikasi intent berdasarkan pola SPOK
 *
 * Persyaratan: C89, ajudan.h
 */

#include "ajudan.h"

/* ================================================================== *
 *                    KONSTANTA INTERNAL                                 *
 * ================================================================== */

#define MAX_KLAUSA 8
#define MAX_PEMISAH 20

/* ================================================================== *
 *              DATA PEMISAH KLAUSA (KONJUNGSI & TANDA BACA)             *
 * ================================================================== */

/*
 * Pemisah klausa diurutkan dari terpanjang ke terpendek
 * agar pencocokan prioritas panjang duluan.
 */
static const char *DAFTAR_PEMISAH[] = {
    "sedangkan",
    "sambil",
    "sebelum",
    "setelah",
    "walaupun",
    "meskipun",
    "padahal",
    "saatnya",
    "kemudian",
    "maupun",
    "menurutmu",
    "menurut kamu",
    "menurutku",
    "namun",
    "lalu",
    "sedang",
    "atau",
    "tetapi",
    "dan",
    "ketika",
    NULL
};
static const int JML_PEMISAH = 21;

/* ================================================================== *
 *              DATA KATA TANYA BAHASA INDONESIA                         *
 * ================================================================== */

static const char *KATA_TANYA[] = {
    "apa", "siapa", "mengapa", "kenapa",
    "bagaimana", "kapan", "dimana", "di mana",
    "berapa", "mana", "macam apa",
    NULL
};
static const int JML_KATA_TANYA = 11;

/* ================================================================== *
 *          DATA MARKER PERMINTAAN PENJELASAN                           *
 * ================================================================== */

static const char *MARKER_PENJELASAN[] = {
    "jelaskan", "jabarkan",
    "beritahu lebih lanjut", "uraikan",
    "deskripsikan", "ceritakan",
    "tolong jelaskan", "jelaskan tentang",
    "jabarkan tentang", "beritahu tentang",
    "ceritakan tentang",
    NULL
};
static const int JML_MARKER_PENJELASAN = 11;

/* ================================================================== *
 *          DATA MARKER PERTANYAAN IMPLISIT                              *
 * ================================================================== */

static const char *MARKER_IMPLISIT[] = {
    "menurutmu", "menurut kamu", "menurutku",
    "kira-kira", "kirakira",
    "boleh tahu", "bolehkah",
    "mohon penjelasan",
    "bisa jelaskan", "bisa jelaskan",
    NULL
};
static const int JML_MARKER_IMPLISIT = 9;

/* ================================================================== *
 *          DAFTAR KATA LUMPAT (STOP WORDS UNTUK EKSTRAKSI TOPIK)         *
 * ================================================================== */

/*
 * Kata-kata yang harus diabaikan saat mengekstrak topik
 * dari klausa pertanyaan. Termasuk: pronomina, negasi,
 * modalitas, preposisi, konjungsi, kata tanya, imperatif,
 * partikel, dan verba umum yang bukan konten/entitas.
 *
 * Penggunaan: bersihkan_topik() menggunakan daftar ini
 * untuk menyaring kata bermakna dari noise.
 */
static const char *DAFTAR_KATA_LUMPAT[] = {
    /* Pronomina personal */
    "saya", "kamu", "aku", "dia", "mereka", "kita", "kami",
    "anda", "beliau", "ia", "nya", "mu", "ku",
    /* Pronomina posesif */
    "padaku", "padamu", "buatku", "buatmu",
    /* Negasi & aspek temporal */
    "tidak", "bukan", "belum", "sudah", "telah", "akan",
    "sedang", "masih", "pernah",
    /* Modalitas & keinginan */
    "bisa", "dapat", "mampu", "mau", "ingin", "hendak",
    /* Kognisi & keadaan */
    "mengerti", "tahu", "paham", "bingung",
    /* Preposisi & konjungsi umum */
    "tentang", "untuk", "dari", "dengan", "pada", "di",
    "oleh", "bagi", "kepada", "ke", "dalam",
    "agar", "supaya", "karena", "kalau", "jika",
    "waktu", "ketika", "saat", "sebelum", "setelah",
    "dimana", "kemana", "darimana",
    /* Partikel & kata tambahan */
    "juga", "pun", "saja", "hanya", "lagi",
    "yang", "yg",
    /* Penanda demonstratif */
    "ini", "itu", "tersebut", "hal", "masalah",
    /* Kata tanya (bukan konten) */
    "apa", "siapa", "bagaimana", "mengapa", "kenapa",
    "kapan", "dimana", "di mana", "berapa", "mana",
    "macam", "apakah",
    /* Imperatif/permintaan (bukan konten) */
    "jelaskan", "jabarkan", "beritahu", "tolong",
    "uraikan", "deskripsikan", "ceritakan",
    "mohon", "berikan",
    /* Kata tambahan/penegas */
    "lebih", "lanjut", "artinya", "maknanya",
    "kok", "ya", "yah", "dong", "deh", "sih", "lah",
    "punya", "punyaku", "punyamu",
    /* Verba umum yang bukan entitas */
    "mengajak", "menanyakan", "meminta", "memilih",
    "membeli", "berbelanja", "sampai",
    /* Verba pencarian/pemahaman (bukan konten) */
    "cari", "mencari", "kesulitan", "mendapatkan",
    "menemukan", "mengetahui", "memahami",
    /* Verba perujukan (bukan konten) */
    "disebut", "dimaksud", "dikatakan", "dinamakan",
    "disebutkan", "dimaksudkan", "diberi nama",
    "dikenal", "dipanggil", "dimaksudkan",
    /* Determiner */
    "seorang", "sesuatu", "sebuah", "suatu",
    NULL
};

/* ================================================================== *
 *                    FUNGSI UTILITAS INTERNAL                           *
 * ================================================================== */

/*
 * cek_kata_tanya - Cek apakah sebuah token adalah
 * kata tanya bahasa Indonesia.
 * Mengembalikan 1 jika ya, 0 jika tidak.
 */
static int cek_kata_tanya(const char *kata)
{
    int i;
    if (!kata || !kata[0]) return 0;

    for (i = 0; i < JML_KATA_TANYA; i++) {
        if (KATA_TANYA[i] == NULL) break;
        if (ajudan_strcasecmp(kata, KATA_TANYA[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * cek_marker_implisit - Cek apakah teks mengandung
 * marker pertanyaan implisit.
 */
static int cek_marker_implisit(const char *teks)
{
    int i;
    char lo[MAX_INPUT_USER];

    if (!teks || !teks[0]) return 0;

    snprintf(lo, sizeof(lo), "%s", teks);
    to_lower_case(lo);

    for (i = 0; i < JML_MARKER_IMPLISIT; i++) {
        if (MARKER_IMPLISIT[i] == NULL) break;
        if (strstr(lo, MARKER_IMPLISIT[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

/*
 * hapus_tanda_baca_akhir - Hapus tanda baca di akhir string.
 */
static void hapus_tanda_baca_akhir(char *s)
{
    int len;

    if (!s || !s[0]) return;

    len = (int)strlen(s);
    while (len > 0 && ispunct((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

/*
 * token_kata_tanya_dalam_teks - Cek apakah teks mengandung
 * kata tanya. Mengembalikan 1 jika ya.
 */
static int token_kata_tanya_dalam_teks(const char *teks)
{
    char buf[MAX_INPUT_USER];
    char *t;

    if (!teks || !teks[0]) return 0;

    snprintf(buf, sizeof(buf), "%s", teks);
    t = strtok(buf, " \t\n");
    while (t != NULL) {
        int tl = (int)strlen(t);
        while (tl > 0 && ispunct((unsigned char)t[tl - 1])) {
            t[tl - 1] = '\0';
            tl--;
        }
        if (cek_kata_tanya(t)) {
            return 1;
        }
        t = strtok(NULL, " \t\n");
    }
    return 0;
}

/*
 * cek_tanda_tanya_dalam_teks - Cek apakah teks mengandung '?'.
 */
static int cek_tanda_tanya_dalam_teks(const char *teks)
{
    int i;
    if (!teks) return 0;
    for (i = 0; teks[i]; i++) {
        if (teks[i] == '?') return 1;
    }
    return 0;
}

/*
 * cek_ada_pertanyaan - Cek apakah sebuah teks/klausa
 * mengandung indikasi pertanyaan (baik eksplisit maupun implisit).
 */
static int cek_ada_pertanyaan(const char *teks)
{
    if (!teks || !teks[0]) return 0;

    /* Eksplisit: tanda tanya */
    if (cek_tanda_tanya_dalam_teks(teks)) return 1;

    /* Eksplisit: kata tanya */
    if (token_kata_tanya_dalam_teks(teks)) return 1;

    /* Implisit: marker pertanyaan */
    if (cek_marker_implisit(teks)) return 1;

    return 0;
}

/* ================================================================== *
 *           BERSIHKAN TOPIK (HAPUS KATA LUMPAT)                        *
 * ================================================================== */

/*
 * is_kata_lumpat - Cek apakah kata termasuk daftar
 * kata lumpat (stop words) yang harus diabaikan
 * saat mengekstrak topik.
 * Mengembalikan 1 jika ya, 0 jika tidak.
 */
static int is_kata_lumpat(const char *kata)
{
    int i;

    if (!kata || !kata[0]) return 0;

    for (i = 0; DAFTAR_KATA_LUMPAT[i] != NULL; i++) {
        if (strcmp(kata, DAFTAR_KATA_LUMPAT[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * bersihkan_topik - Menghapus kata-kata lumpat (stop words)
 * dari string topik, menyisakan hanya kata bermakna
 * (entitas/konten).
 *
 * Contoh:
 *   "saya tidak mengerti komputer" -> "komputer"
 *   "apa itu komputer" -> "komputer"
 *   "jenis komputer yang bagus" -> "komputer bagus"
 *
 * Operasi dilakukan in-place pada string topik.
 */
void bersihkan_topik(char *topik)
{
    char buf[MAX_PANJANG_STRING];
    char *t;
    int pertama;

    if (!topik || !topik[0]) return;

    snprintf(buf, sizeof(buf), "%s", topik);
    to_lower_case(buf);

    topik[0] = '\0';
    pertama = 1;

    t = strtok(buf, " \t\n");
    while (t != NULL) {
        if (!is_kata_lumpat(t) && strlen(t) > 1) {
            if (!pertama) {
                strncat(topik, " ",
                    MAX_PANJANG_STRING
                    - strlen(topik) - 1);
            }
            strncat(topik, t,
                MAX_PANJANG_STRING
                - strlen(topik) - 1);
            pertama = 0;
        }
        t = strtok(NULL, " \t\n");
    }

    trim(topik);
}

/* ================================================================== *
 *             EKSTRAKSI SEMUA KLAUSA PERTANYAAN                         *
 * ================================================================== */

/*
 * pisah_kalimat - Memecah input menjadi kalimat-kalimat
 * berdasarkan tanda akhir kalimat (. ? !).
 * Mengembalikan jumlah kalimat.
 */
static int pisah_kalimat(
    const char *input,
    char kalimat_out[][MAX_INPUT_USER],
    int max_kalimat)
{
    char buf[MAX_INPUT_USER];
    int len, i, start, jml;
    int dalam_kalimat;

    if (!input || !kalimat_out || max_kalimat <= 0) return 0;

    snprintf(buf, sizeof(buf), "%s", input);
    len = (int)strlen(buf);
    jml = 0;
    start = 0;
    dalam_kalimat = 0;

    for (i = 0; i < len && jml < max_kalimat; i++) {
        if (buf[i] == '.' || buf[i] == '?' || buf[i] == '!') {
            if (dalam_kalimat) {
                char tmp[MAX_INPUT_USER];
                int tlen;

                tlen = i - start + 1;
                if (tlen > 0 && tlen < MAX_INPUT_USER) {
                    snprintf(tmp, sizeof(tmp), "%.*s",
                        tlen, buf + start);
                    trim(tmp);
                    hapus_tanda_baca_akhir(tmp);
                    if (tmp[0] != '\0') {
                        snprintf(
                            kalimat_out[jml],
                            MAX_INPUT_USER, "%s", tmp);
                        jml++;
                    }
                }
            }
            start = i + 1;
            dalam_kalimat = 0;
        } else {
            if (!dalam_kalimat && buf[i] != ' '
                && buf[i] != '\t' && buf[i] != '\n') {
                dalam_kalimat = 1;
            }
        }
    }

    /* Sisa teks setelah tanda baca terakhir */
    if (start < len && jml < max_kalimat) {
        char tmp[MAX_INPUT_USER];
        snprintf(tmp, sizeof(tmp), "%s", buf + start);
        trim(tmp);
        hapus_tanda_baca_akhir(tmp);
        if (tmp[0] != '\0') {
            snprintf(kalimat_out[jml],
                MAX_INPUT_USER, "%s", tmp);
            jml++;
        }
    }

    return jml;
}

/*
 * pisah_klausa_dalam_kalimat - Memecah satu kalimat
 * menjadi sub-klausa berdasarkan konjungsi dan koma.
 * Mengembalikan jumlah sub-klausa.
 *
 * Hanya memecah pada konjungsi yang terdeteksi
 * sebagai pemisah klausa, BUKAN pada setiap koma.
 */
static int pisah_klausa_dalam_kalimat(
    const char *kalimat,
    char klausa_out[][MAX_INPUT_USER],
    int max_klausa)
{
    char buf[MAX_INPUT_USER];
    char lo[MAX_INPUT_USER];
    int j;
    char *pos_pem;
    int panjang_pem;
    int ketemu;
    int jml;
    int coba_lagi;

    if (!kalimat || !klausa_out || max_klausa <= 0) return 0;

    snprintf(buf, sizeof(buf), "%s", kalimat);
    trim(buf);
    hapus_tanda_baca_akhir(buf);

    if (buf[0] == '\0') return 0;

    jml = 0;
    coba_lagi = 1;

    while (coba_lagi && jml < max_klausa - 1) {
        coba_lagi = 0;
        ketemu = 0;
        pos_pem = NULL;
        panjang_pem = 0;

        /* Buat salinan huruf kecil */
        snprintf(lo, sizeof(lo), "%s", buf);
        to_lower_case(lo);

        /* Cari pemisah klausa (konjungsi) */
        for (j = 0; j < JML_PEMISAH; j++) {
            if (DAFTAR_PEMISAH[j] == NULL) break;
            {
                char pp[64];
                int pl;
                char *found;
                char *found_lo;

                /* Coba dengan spasi di kedua sisi */
                snprintf(pp, sizeof(pp), " %s ",
                    DAFTAR_PEMISAH[j]);
                pl = (int)strlen(pp);

                found_lo = strstr(lo, pp);
                if (found_lo != NULL) {
                    int offset;
                    offset = (int)(found_lo - lo);
                    found = buf + offset;
                    if (pos_pem == NULL
                        || found > pos_pem) {
                        pos_pem = found;
                        panjang_pem = pl;
                        ketemu = 1;
                    }
                }

                /* Coba tanpa spasi awal */
                snprintf(pp, sizeof(pp), "%s ",
                    DAFTAR_PEMISAH[j]);
                pl = (int)strlen(pp);

                found_lo = strstr(lo, pp);
                if (found_lo != NULL) {
                    int offset;
                    offset = (int)(found_lo - lo);
                    found = buf + offset;
                    if (pos_pem == NULL
                        || found > pos_pem) {
                        pos_pem = found;
                        panjang_pem = pl;
                        ketemu = 1;
                    }
                }
            }
        }

        /* Juga cek koma sebagai pemisah terakhir */
        if (!ketemu || pos_pem == NULL) {
            char *cp;
            cp = strrchr(buf, ',');
            if (cp != NULL) {
                pos_pem = cp;
                panjang_pem = 1;
                ketemu = 1;
            }
        }

        if (!ketemu || pos_pem == NULL) break;

        /* Ambil bagian SETELAH pemisah sebagai klausa baru */
        {
            char *awal_sub;
            awal_sub = pos_pem + panjang_pem;
            while (*awal_sub
                && isspace((unsigned char)*awal_sub)) {
                awal_sub++;
            }

            if (awal_sub[0] != '\0'
                && jml < max_klausa) {
                /* Simpan bagian setelah pemisah */
                snprintf(klausa_out[jml],
                    MAX_INPUT_USER, "%s", awal_sub);
                jml++;

                /* Potong buffer, lanjut proses sisa */
                *pos_pem = '\0';
                coba_lagi = 1;
            }
        }
    }

    /* Sisa buffer adalah klausa pertama (terakhir yang tersisa) */
    if (buf[0] != '\0' && jml < max_klausa) {
        trim(buf);
        if (buf[0] != '\0') {
            /* Geser: sisipkan di awal array */
            int k;
            for (k = jml; k > 0; k--) {
                snprintf(klausa_out[k],
                    MAX_INPUT_USER, "%s",
                    klausa_out[k - 1]);
            }
            snprintf(klausa_out[0],
                MAX_INPUT_USER, "%s", buf);
            jml++;
        }
    }

    return jml;
}

/*
 * ekstrak_semua_klausa_pertanyaan - Fungsi utama untuk
 * mengekstrak SEMUA klausa yang mengandung pertanyaan
 * dari input kalimat panjang.
 *
 * Algoritma:
 * 1. Pecah input menjadi kalimat-kalimat (. ? !)
 * 2. Untuk setiap kalimat, pecah menjadi sub-klausa
 * 3. Kumpulkan semua sub-klausa yang mengandung
 *    indikasi pertanyaan (eksplisit atau implisit)
 *
 * Mengembalikan jumlah klausa pertanyaan yang ditemukan.
 *
 * Urutan: klausa_out[0] = pertanyaan pertama yang ditemukan,
 *         klausa_out[1] = pertanyaan kedua, dst.
 */
int ekstrak_semua_klausa_pertanyaan(
    const char *input,
    char klausa_out[][MAX_INPUT_USER],
    int max_klausa)
{
    char kalimat[MAX_KLAUSA][MAX_INPUT_USER];
    int jml_kalimat;
    int i, jml_hasil;

    if (!input || !klausa_out || max_klausa <= 0) return 0;

    jml_hasil = 0;

    /* Langkah 1: Pecah menjadi kalimat-kalimat */
    jml_kalimat = pisah_kalimat(
        input, kalimat, MAX_KLAUSA);

    /* Langkah 2: Untuk setiap kalimat, pecah dan cek */
    for (i = 0; i < jml_kalimat; i++) {
        char sub_klausa[MAX_KLAUSA][MAX_INPUT_USER];
        int jml_sub;
        int j;

        jml_sub = pisah_klausa_dalam_kalimat(
            kalimat[i], sub_klausa, MAX_KLAUSA);

        for (j = 0; j < jml_sub; j++) {
            if (jml_hasil >= max_klausa) break;

            if (cek_ada_pertanyaan(sub_klausa[j])) {
                trim(sub_klausa[j]);
                hapus_tanda_baca_akhir(sub_klausa[j]);
                if (sub_klausa[j][0] != '\0') {
                    snprintf(
                        klausa_out[jml_hasil],
                        MAX_INPUT_USER,
                        "%s", sub_klausa[j]);
                    jml_hasil++;
                }
            }
        }
    }

    /* Fallback: jika tidak ada klausa pertanyaan
     * terdeteksi, gunakan seluruh input */
    if (jml_hasil == 0) {
        char tmp[MAX_INPUT_USER];
        snprintf(tmp, sizeof(tmp), "%s", input);
        trim(tmp);
        hapus_tanda_baca_akhir(tmp);
        if (tmp[0] != '\0') {
            snprintf(klausa_out[0],
                MAX_INPUT_USER, "%s", tmp);
            jml_hasil = 1;
        }
    }

    return jml_hasil;
}

/* ================================================================== *
 *              DETEKSI PERTANYAAN IMPLISIT                              *
 * ================================================================== */

/*
 * deteksi_pertanyaan_implisit - Mendeteksi apakah
 * sebuah teks adalah pertanyaan meskipun tanpa
 * tanda tanya eksplisit (?).
 *
 * Kriteria pertanyaan implisit:
 * 1. Mengandung kata tanya (apa, siapa, bagaimana, dll)
 * 2. Mengandung marker implisit (menurutmu, kira-kira, dll)
 * 3. Mengandung kata kunci penjelasan (jelaskan, jabarkan, dll)
 * 4. Mengandung kata kunci arti/definisi (arti, makna, definisi dari)
 * 5. Pola "topik itu apa" atau "apa itu topik"
 *
 * Mengembalikan: 1 jika pertanyaan implisit, 0 jika bukan.
 */
int deteksi_pertanyaan_implisit(const char *teks)
{
    char lo[MAX_INPUT_USER];
    int i;

    if (!teks || !teks[0]) return 0;

    /* Jika ada tanda tanya eksplisit, bukan "implisit" */
    if (cek_tanda_tanya_dalam_teks(teks)) return 0;

    snprintf(lo, sizeof(lo), "%s", teks);
    to_lower_case(lo);

    /* 1. Kata tanya */
    if (token_kata_tanya_dalam_teks(teks)) return 1;

    /* 2. Marker implisit */
    if (cek_marker_implisit(teks)) return 1;

    /* 3. Marker penjelasan (imperatif pertanyaan) */
    for (i = 0; i < JML_MARKER_PENJELASAN; i++) {
        if (MARKER_PENJELASAN[i] == NULL) break;
        if (strstr(lo, MARKER_PENJELASAN[i]) != NULL) {
            return 1;
        }
    }

    /* 4. Kata kunci arti/definisi yang menandakan pertanyaan */
    if (strstr(lo, "arti dari") != NULL
        || strstr(lo, "makna dari") != NULL
        || strstr(lo, "definisi dari") != NULL
        || strstr(lo, "pengertian dari") != NULL) {
        return 1;
    }

    /* 5. Pola "X itu apa" atau "apa itu X"
     * (sudah tertangkap oleh kata taya "apa",
     *  tapi double-check untuk pola "itu apa"
     *  tanpa kata tanya eksplisit lainnya) */
    if (strstr(lo, "itu apa") != NULL
        || strstr(lo, "itu apaan") != NULL) {
        return 1;
    }

    return 0;
}

/* ================================================================== *
 *             KLASIFIKASI INTENT BERDASARKAN SPOK                       *
 * ================================================================== */

/*
 * klasifikasi_intent_klausa - Klasifikasi intent
 * dari satu klausa pertanyaan berdasarkan pola SPOK.
 *
 * Pola yang dikenali:
 *
 * DEFINISI:
 *   "apa itu X"         -> QW("apa") PRED("itu") SUBJ(X)
 *   "X itu apa"         -> SUBJ(X) PRED("itu") QW("apa")
 *   "apa X"             -> QW("apa") SUBJ(X)
 *   "X apa" (akhir)     -> SUBJ(X) QW("apa")
 *   "arti X"            -> keyword ARTI + SUBJ(X)
 *   "makna X"           -> keyword MAKNA + SUBJ(X)
 *
 * JENIS:
 *   "X apa itu"         -> SUBJ(X) QW("apa") PRED("itu")
 *   "jenis X"           -> keyword JENIS + SUBJ(X)
 *   "macam X"           -> keyword MACAM + SUBJ(X)
 *
 * PENJELASAN:
 *   "jelaskan X"        -> imperative + SUBJ(X)
 *   "jabarkan X"        -> imperative + SUBJ(X)
 *
 * ALASAN:
 *   "mengapa X"         -> QW("mengapa") + SUBJ(X)
 *   "kenapa X"          -> QW("kenapa") + SUBJ(X)
 *
 * CARA:
 *   "bagaimana X"       -> QW("bagaimana") + SUBJ(X)
 *   "bagaimana cara X"  -> QW + "cara" + SUBJ
 *
 * PERBANDINGAN:
 *   "perbedaan X dan Y" -> keyword + SUBJ
 *
 * Mengembalikan TipeIntent dan mengekstrak topik ke topik_out.
 */
TipeIntent klasifikasi_intent_klausa(
    const char *klausa,
    char *topik_out,
    size_t ukuran_topik)
{
    char buf[MAX_INPUT_USER];
    char tokens[MAX_TOKEN_KALIMAH][MAX_PANJANG_TOKEN];
    int n_tokens;
    int i;
    int pos_apa, pos_itu;
    int ada_apa, ada_itu;
    char lo[MAX_INPUT_USER];
    char *t;

    if (topik_out && ukuran_topik > 0) {
        topik_out[0] = '\0';
    }

    if (!klausa || !klausa[0]) return INTENT_LAIN;

    /* Salin dan bersihkan */
    snprintf(buf, sizeof(buf), "%s", klausa);
    hapus_tanda_baca_akhir(buf);

    /* Tokenisasi sederhana */
    n_tokens = 0;
    t = strtok(buf, " \t\n");
    while (t != NULL && n_tokens < MAX_TOKEN_KALIMAH) {
        snprintf(tokens[n_tokens],
            sizeof(tokens[n_tokens]), "%s", t);
        n_tokens++;
        t = strtok(NULL, " \t\n");
    }

    if (n_tokens <= 0) return INTENT_LAIN;

    /* Konversi semua token ke huruf kecil */
    for (i = 0; i < n_tokens; i++) {
        to_lower_case(tokens[i]);
    }

    /* Buat salinan huruf kecil dari keseluruhan klausa */
    snprintf(lo, sizeof(lo), "%s", klausa);
    to_lower_case(lo);

    /* ============================================================
     * PRIORITAS 1: Marker penjelasan (imperatif)
     * "jelaskan X", "jabarkan X", dll
     * ============================================================ */
    for (i = 0; i < JML_MARKER_PENJELASAN; i++) {
        if (MARKER_PENJELASAN[i] == NULL) break;
        if (strstr(lo, MARKER_PENJELASAN[i]) != NULL) {
            /* Ekstrak topik: semua kata setelah marker */
            if (topik_out && ukuran_topik > 0) {
                int k;
                topik_out[0] = '\0';
                for (k = 0; k < n_tokens; k++) {
                    int skip;
                    skip = 0;
                    if (ajudan_strcasecmp(
                        tokens[k], "jelaskan") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "jabarkan") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "uraikan") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "deskripsikan") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "ceritakan") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "beritahu") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "tolong") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "tentang") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "lebih") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "lanjut") == 0) {
                        skip = 1;
                    }
                    if (!skip) {
                        if (topik_out[0] != '\0') {
                            strncat(topik_out, " ",
                                ukuran_topik
                                - strlen(topik_out)
                                - 1);
                        }
                        strncat(topik_out, tokens[k],
                            ukuran_topik
                            - strlen(topik_out)
                            - 1);
                    }
                }
            }
            return INTENT_PENJELASAN;
        }
    }

    /* ============================================================
     * PRIORITAS 2: Kata kunci jenis/macam
     * "jenis X", "macam X", "tipe X", "varian X"
     * ============================================================ */
    if (strstr(lo, "jenis") != NULL
        || strstr(lo, "macam") != NULL
        || strstr(lo, "tipe") != NULL
        || strstr(lo, "varian") != NULL) {
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = 0; k < n_tokens; k++) {
                if (ajudan_strcasecmp(
                    tokens[k], "jenis") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "macam") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "tipe") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "varian") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "dari") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "yang") == 0) {
                    continue;
                }
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_JENIS;
    }

    /* ============================================================
     * PRIORITAS 3: Kata kunci arti/makna/definisi
     * "arti X", "makna X", "definisi X"
     * ============================================================ */
    if (strstr(lo, "arti") != NULL
        || strstr(lo, "makna") != NULL
        || strstr(lo, "definisi") != NULL
        || strstr(lo, "pengertian") != NULL) {
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = 0; k < n_tokens; k++) {
                if (ajudan_strcasecmp(
                    tokens[k], "arti") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "makna") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "definisi") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "pengertian") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "dari") == 0) {
                    continue;
                }
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_ARTI;
    }

    /* ============================================================
     * PRIORITAS 4: Kata tanya khusus -> intent spesifik
     * ============================================================ */

    /* "mengapa X" / "kenapa X" -> ALASAN */
    for (i = 0; i < n_tokens; i++) {
        if (ajudan_strcasecmp(tokens[i], "mengapa") == 0
            || ajudan_strcasecmp(tokens[i], "kenapa") == 0) {
            if (topik_out && ukuran_topik > 0) {
                int k;
                topik_out[0] = '\0';
                for (k = i + 1; k < n_tokens; k++) {
                    if (topik_out[0] != '\0') {
                        strncat(topik_out, " ",
                            ukuran_topik
                            - strlen(topik_out)
                            - 1);
                    }
                    strncat(topik_out, tokens[k],
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
            }
            return INTENT_ALASAN;
        }
    }

    /* "bagaimana X" -> CARA */
    for (i = 0; i < n_tokens; i++) {
        if (ajudan_strcasecmp(
            tokens[i], "bagaimana") == 0) {
            if (topik_out && ukuran_topik > 0) {
                int k;
                topik_out[0] = '\0';
                for (k = i + 1; k < n_tokens; k++) {
                    if (topik_out[0] != '\0') {
                        strncat(topik_out, " ",
                            ukuran_topik
                            - strlen(topik_out)
                            - 1);
                    }
                    strncat(topik_out, tokens[k],
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
            }
            return INTENT_CARA;
        }
    }

    /* "perbedaan X dan Y" -> PERBANDINGAN */
    if (strstr(lo, "perbedaan") != NULL
        || strstr(lo, "perbandingan") != NULL) {
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = 0; k < n_tokens; k++) {
                if (ajudan_strcasecmp(
                    tokens[k], "perbedaan") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "perbandingan") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "antara") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "dengan") == 0) {
                    continue;
                }
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_PERBANDINGAN;
    }

    /* ============================================================
     * PRIORITAS 5: Analisis pola SPOK "apa" dan "itu"
     * ============================================================ */

    ada_apa = 0;
    ada_itu = 0;
    pos_apa = -1;
    pos_itu = -1;

    for (i = 0; i < n_tokens; i++) {
        if (strcmp(tokens[i], "apa") == 0) {
            pos_apa = i;
            ada_apa = 1;
        }
        if (strcmp(tokens[i], "itu") == 0) {
            pos_itu = i;
            ada_itu = 1;
        }
    }

    if (!ada_apa && !ada_itu) {
        /* Tidak ada "apa" atau "itu":
         * cek kata tanya lainnya di awal */
        if (cek_kata_tanya(tokens[0])) {
            /* "siapa X", "kapan X", dll -> DEFINISI */
            if (topik_out && ukuran_topik > 0) {
                int k;
                topik_out[0] = '\0';
                for (k = 1; k < n_tokens; k++) {
                    if (topik_out[0] != '\0') {
                        strncat(topik_out, " ",
                            ukuran_topik
                            - strlen(topik_out)
                            - 1);
                    }
                    strncat(topik_out, tokens[k],
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
            }
            return INTENT_DEFINISI;
        }
        return INTENT_LAIN;
    }

    /* --- Hanya "itu" tanpa "apa" --- */
    if (!ada_apa && ada_itu) {
        if (pos_itu == n_tokens - 1) {
            /* "X itu" di akhir -> jenis */
            if (topik_out && ukuran_topik > 0) {
                int k;
                topik_out[0] = '\0';
                for (k = 0; k < pos_itu; k++) {
                    if (ajudan_strcasecmp(
                        tokens[k], "itu") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "yang") == 0
                        || ajudan_strcasecmp(
                            tokens[k], "yg") == 0) {
                        continue;
                    }
                    if (topik_out[0] != '\0') {
                        strncat(topik_out, " ",
                            ukuran_topik
                            - strlen(topik_out)
                            - 1);
                    }
                    strncat(topik_out, tokens[k],
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
            }
            return INTENT_JENIS;
        }
        return INTENT_LAIN;
    }

    /* --- Hanya "apa" tanpa "itu" --- */
    if (ada_apa && !ada_itu) {
        /* "apa" di akhir klausa -> DEFINISI
         * Contoh: "komputer itu apa" */
        if (pos_apa == n_tokens - 1) {
            if (topik_out && ukuran_topik > 0) {
                int k;
                topik_out[0] = '\0';
                for (k = 0; k < pos_apa; k++) {
                    if (topik_out[0] != '\0') {
                        strncat(topik_out, " ",
                            ukuran_topik
                            - strlen(topik_out)
                            - 1);
                    }
                    strncat(topik_out, tokens[k],
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
            }
            return INTENT_DEFINISI;
        }

        /* "apa" di awal -> DEFINISI
         * Contoh: "apa komputer" */
        if (pos_apa == 0 && n_tokens > 1) {
            if (topik_out && ukuran_topik > 0) {
                int k;
                topik_out[0] = '\0';
                for (k = 1; k < n_tokens; k++) {
                    if (topik_out[0] != '\0') {
                        strncat(topik_out, " ",
                            ukuran_topik
                            - strlen(topik_out)
                            - 1);
                    }
                    strncat(topik_out, tokens[k],
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
            }
            return INTENT_DEFINISI;
        }

        /* "apa" di tengah tanpa "itu"
         * -> fallback DEFINISI */
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = 0; k < n_tokens; k++) {
                if (k == pos_apa) continue;
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_DEFINISI;
    }

    /* ============================================================
     * KEDUA "apa" DAN "itu" ada -> analisis posisi relatif
     * ============================================================
     *
     * "apa itu X"  -> [apa][itu][topik]
     *   pos_apa(0) < pos_itu(1), ada kata setelah itu
     *   -> DEFINISI
     *
     * "X itu apa"  -> [topik][itu][apa]
     *   pos_itu < pos_apa, "apa" di akhir
     *   -> DEFINISI
     *
     * "X apa itu"  -> [topik][apa][itu]
     *   pos_apa < pos_itu, "itu" di akhir
     *   -> JENIS
     */

    /* Pola: [apa] [itu] [topik] -> DEFINISI */
    if (pos_apa < pos_itu
        && pos_itu < n_tokens - 1) {
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = pos_itu + 1; k < n_tokens; k++) {
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_DEFINISI;
    }

    /* Pola: [topik] [itu] [apa] -> DEFINISI */
    if (pos_itu < pos_apa
        && pos_apa == n_tokens - 1) {
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = 0; k < pos_itu; k++) {
                if (ajudan_strcasecmp(
                    tokens[k], "yang") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "yg") == 0) {
                    continue;
                }
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_DEFINISI;
    }

    /* Pola: [topik] [apa] [itu] -> JENIS */
    if (pos_apa < pos_itu
        && pos_itu == n_tokens - 1) {
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = 0; k < pos_apa; k++) {
                if (ajudan_strcasecmp(
                    tokens[k], "yang") == 0
                    || ajudan_strcasecmp(
                        tokens[k], "yg") == 0) {
                    continue;
                }
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_JENIS;
    }

    /* Fallback: pos_apa > pos_itu tapi bukan di akhir */
    if (pos_apa > pos_itu) {
        if (topik_out && ukuran_topik > 0) {
            int k;
            topik_out[0] = '\0';
            for (k = 0; k < n_tokens; k++) {
                if (k == pos_apa
                    || k == pos_itu) continue;
                if (topik_out[0] != '\0') {
                    strncat(topik_out, " ",
                        ukuran_topik
                        - strlen(topik_out) - 1);
                }
                strncat(topik_out, tokens[k],
                    ukuran_topik
                    - strlen(topik_out) - 1);
            }
        }
        return INTENT_DEFINISI;
    }

    return INTENT_LAIN;
}
