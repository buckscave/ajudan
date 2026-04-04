/*
 * potong.c - Stemmer Bahasa Indonesia
 *
 * Implementasi algoritma stemming untuk bahasa Indonesia
 * berdasarkan aturan afiksasi: prefiks, sufiks, infiks,
 * konfiks, dan kombinasi partikel/possesif.
 *
 * Semua fungsi mematuhi standar C89 dan menggunakan konvensi
 * penanganan error yang konsisten: return 0 = sukses, -1 = gagal.
 */

#include "ajudan.h"

/* ================================================================== *
 *                     DATA TABEL PARTIKEL & POSSESIF                   *
 * ================================================================== */

static const char *DAFTAR_PARTIKEL[] = {
    "kah", "lah", "pun", "tah", "dong", "nya", "sih", "deh", NULL
};
static const int JUMLAH_PARTIKEL = 9;

static const char *DAFTAR_POSSESIF[] = {
    "ku", "mu", "nya", NULL
};
static const int JUMLAH_POSSESIF = 3;

/* ================================================================== *
 *                  DATA TABEL PREFIKS & SUFIKS                         *
 * ================================================================== */

static const char *DAFTAR_AWALAN_ME[] = {
    "meng", "meny", "men", "mem", "me", NULL
};
static const int PANJANG_AWALAN_ME[] = { 4, 4, 3, 3, 2 };

static const char *DAFTAR_AWALAN_BER[] = {
    "ber", NULL
};
static const int PANJANG_AWALAN_BER[] = { 3 };

static const char *DAFTAR_AWALAN_PER[] = {
    "peng", "peny", "pen", "per", "pe", NULL
};
static const int PANJANG_AWALAN_PER[] = { 4, 4, 3, 3, 2 };

static const char *DAFTAR_AWALAN_DI[] = {
    "di", NULL
};
static const int PANJANG_AWALAN_DI[] = { 2 };

static const char *DAFTAR_AWALAN_TER[] = {
    "ter", NULL
};
static const int PANJANG_AWALAN_TER[] = { 3 };

static const char *DAFTAR_AWALAN_KE[] = {
    "ke", NULL
};
static const int PANJANG_AWALAN_KE[] = { 2 };

static const char *DAFTAR_AKHIRAN_KB[] = {
    "kan", "an", "i", NULL
};
static const int PANJANG_AKHIRAN_KB[] = { 3, 2, 1 };

/* ================================================================== *
 *                        FUNGSI UTILITAS INTERNAL                       *
 * ================================================================== */

static int cek_akhir(const char *kata, int len, const char *akhiran)
{
    int la, lk, pos;

    if (!kata || !akhiran) return 0;
    la = (int)strlen(akhiran);
    if (len < la) return 0;

    lk = len - la;
    pos = lk;

    if (strncmp(kata + pos, akhiran, (size_t)la) == 0) {
        return pos;
    }
    return -1;
}

static int cek_awal(const char *kata, const char *awalan)
{
    int la;

    if (!kata || !awalan) return 0;
    la = (int)strlen(awalan);
    if (strncmp(kata, awalan, (size_t)la) == 0) {
        return la;
    }
    return 0;
}

static int huruf_vokal(char c)
{
    if (c == 'a' || c == 'e' || c == 'i'
        || c == 'o' || c == 'u') {
        return 1;
    }
    return 0;
}

static int cek_partikel(const char *kata, int len)
{
    int i;

    for (i = 0; i < JUMLAH_PARTIKEL; i++) {
        if (cek_akhir(kata, len, DAFTAR_PARTIKEL[i]) >= 0) {
            return i;
        }
    }
    return -1;
}

static int cek_possesif(const char *kata, int len)
{
    int i;

    for (i = 0; i < JUMLAH_POSSESIF; i++) {
        if (cek_akhir(kata, len, DAFTAR_POSSESIF[i]) >= 0) {
            return i;
        }
    }
    return -1;
}

/* ================================================================== *
 *                    TRANSFORMASI HURUF PREFIKS                         *
 * ================================================================== */

static void hapus_awalan(
    const char *kata, int len_awalan,
    char *keluaran, int *panjang_keluaran)
{
    int sisa, i;

    sisa = len_awalan;
    for (i = 0; kata[sisa + i] != '\0'; i++) {
        keluaran[i] = kata[sisa + i];
    }
    keluaran[i] = '\0';
    *panjang_keluaran = i;
}

static void terapkan_me_k(const char *kata, int len,
    char *keluaran, int *pk)
{
    int i;
    (void)len;

    for (i = 0; kata[2 + i] != '\0'; i++) {
        keluaran[i] = kata[2 + i];
    }
    keluaran[0] = 'k';
    keluaran[i] = '\0';
    *pk = i;
}

static void terapkan_me_s(const char *kata, int len,
    char *keluaran, int *pk)
{
    int i;
    (void)len;

    for (i = 0; kata[2 + i] != '\0'; i++) {
        keluaran[i] = kata[2 + i];
    }
    keluaran[0] = 's';
    keluaran[i] = '\0';
    *pk = i;
}

static void terapkan_me_mpw(const char *kata, int len,
    char *keluaran, int *pk)
{
    int i;
    (void)len;

    for (i = 0; kata[3 + i] != '\0'; i++) {
        keluaran[i] = kata[3 + i];
    }
    keluaran[0] = 'p';
    keluaran[i] = '\0';
    *pk = i;
}

static void terapkan_pe_k(const char *kata, int len,
    char *keluaran, int *pk)
{
    int i;
    (void)len;

    for (i = 0; kata[2 + i] != '\0'; i++) {
        keluaran[i] = kata[2 + i];
    }
    keluaran[0] = 'k';
    keluaran[i] = '\0';
    *pk = i;
}

static void terapkan_pe_s(const char *kata, int len,
    char *keluaran, int *pk)
{
    int i;
    (void)len;

    for (i = 0; kata[2 + i] != '\0'; i++) {
        keluaran[i] = kata[2 + i];
    }
    keluaran[0] = 's';
    keluaran[i] = '\0';
    *pk = i;
}

static void terapkan_pe_mpw(const char *kata, int len,
    char *keluaran, int *pk)
{
    int i;
    (void)len;

    for (i = 0; kata[3 + i] != '\0'; i++) {
        keluaran[i] = kata[3 + i];
    }
    keluaran[0] = 'p';
    keluaran[i] = '\0';
    *pk = i;
}

/* ================================================================== *
 *                          FUNGSI STEM AKTIF                           *
 * ================================================================== */

int stem_indonesia(const char *masukan, char *keluaran, int ukuran)
{
    char kata[128];
    char dasar[128];
    char tmp[128];
    int len, len_d, i, j, pos, punya_partikel, punya_possesif;

    if (!masukan || !keluaran || ukuran <= 1) return -1;

    len = (int)strlen(masukan);
    if (len <= 2 || len >= (int)sizeof(kata)) {
        snprintf(keluaran, (size_t)ukuran, "%s", masukan);
        return 0;
    }

    for (i = 0; masukan[i] != '\0' && i < (int)sizeof(kata) - 1; i++) {
        kata[i] = (char)tolower((unsigned char)masukan[i]);
    }
    kata[i] = '\0';
    len = i;

    snprintf(keluaran, (size_t)ukuran, "%s", kata);

    punya_partikel = 0;
    punya_possesif = 0;

    pos = cek_partikel(kata, len);
    if (pos > 2) {
        for (j = 0; j < pos; j++) {
            tmp[j] = kata[j];
        }
        tmp[j] = '\0';
        snprintf(kata, sizeof(kata), "%s", tmp);
        len = j;
        punya_partikel = 1;
    }

    pos = cek_possesif(kata, len);
    if (pos > 2) {
        for (j = 0; j < pos; j++) {
            tmp[j] = kata[j];
        }
        tmp[j] = '\0';
        snprintf(kata, sizeof(kata), "%s", tmp);
        len = j;
        punya_possesif = 1;
    }

    for (i = 0; i < 3 && len > 3; i++) {
        pos = cek_akhir(kata, len, DAFTAR_AKHIRAN_KB[i]);
        if (pos < 2) continue;

        for (j = 0; j < pos; j++) {
            dasar[j] = kata[j];
        }
        dasar[j] = '\0';
        len_d = j;

        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(kata, sizeof(kata), "%s", dasar);
            len = len_d;
            break;
        }

        if (strncmp(kata, "kan", 3) == 0 && pos >= 2) continue;
    }

    if (punya_possesif && punya_partikel && len >= 4) {
        pos = cek_possesif(kata, len);
        if (pos > 2) {
            for (j = 0; j < pos; j++) {
                tmp[j] = kata[j];
            }
            tmp[j] = '\0';
            snprintf(kata, sizeof(kata), "%s", tmp);
            len = j;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[0]) == 4 && len > 5
        && kata[4] != 'e' && kata[4] != 'i') {
        if (kata[4] == 'g') {
            terapkan_me_k(kata, len, dasar, &len_d);
        } else {
            hapus_awalan(kata, 4, dasar, &len_d);
        }
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[1]) == 4 && len > 5) {
        terapkan_me_s(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[2]) == 3 && len > 4
        && kata[3] == 'i') {
        terapkan_me_k(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[3]) == 3 && len > 4
        && (kata[3] == 'b' || kata[3] == 'f'
            || kata[3] == 'p' || kata[3] == 'w')) {
        terapkan_me_mpw(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[4]) == 2 && len > 3
        && kata[2] == 'l') {
        terapkan_me_k(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[4]) == 2 && len > 3
        && kata[2] == 'm' && huruf_vokal(kata[3])) {
        hapus_awalan(kata, 2, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[4]) == 2 && len > 3
        && kata[2] == 'n') {
        terapkan_me_k(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[4]) == 2 && len > 3
        && kata[2] == 'r' && huruf_vokal(kata[3])) {
        hapus_awalan(kata, 2, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[4]) == 2 && len > 3
        && kata[2] == 't' && huruf_vokal(kata[3])) {
        hapus_awalan(kata, 2, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_ME[4]) == 2 && len > 3
        && kata[2] == 'p' && kata[3] != '\0'
        && !huruf_vokal(kata[3])) {
        terapkan_me_mpw(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_BER[0]) == 3 && len > 4
        && huruf_vokal(kata[3])) {
        hapus_awalan(kata, 3, dasar, &len_d);
        if (len_d >= 3) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_PER[0]) == 4 && len > 6
        && kata[4] == 'g' && !huruf_vokal(kata[5])) {
        hapus_awalan(kata, 4, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_PER[1]) == 4 && len > 6) {
        terapkan_pe_s(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_PER[2]) == 3 && len > 5) {
        terapkan_pe_k(kata, len, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_PER[3]) == 3 && len > 5) {
        hapus_awalan(kata, 3, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_DI[0]) == 2 && len > 3) {
        hapus_awalan(kata, 2, dasar, &len_d);
        if (len_d >= 3 && huruf_vokal(dasar[0])) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_TER[0]) == 3 && len > 4
        && huruf_vokal(kata[3])) {
        hapus_awalan(kata, 3, dasar, &len_d);
        if (len_d >= 3) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_TER[0]) == 3 && len > 4
        && kata[3] == 'b' && huruf_vokal(kata[4])) {
        terapkan_me_mpw(kata, len, dasar, &len_d);
        if (len_d >= 3) {
            snprintf(keluaran, (size_t)ukuran, "%s", dasar);
            return 0;
        }
    }

    if (cek_awal(kata, DAFTAR_AWALAN_KE[0]) == 2 && len > 4
        && cek_akhir(kata, len, "an") > 2) {
        pos = len - 2;
        for (j = 0; j < pos && j < 2; j++) {
            tmp[j] = kata[j];
        }
        tmp[j] = '\0';
        len_d = j;

        if (len_d >= 2) {
            for (i = 0; kata[2 + i] != '\0' && i < 2; i++) {
                dasar[i] = kata[2 + i];
            }
            dasar[i] = '\0';
            len_d = i;
            if (len_d >= 2 && huruf_vokal(dasar[0])) {
                snprintf(keluaran, (size_t)ukuran, "%s", dasar);
                return 0;
            }
        }
    }

    snprintf(keluaran, (size_t)ukuran, "%s", kata);
    return 0;
}

/* ================================================================== *
 *                       LOOKUP MORFOLOGI DATABASE                      *
 * ================================================================== */

int stem_menggunakan_db(
    sqlite3 *db,
    const char *masukan,
    char *keluaran,
    int ukuran)
{
    sqlite3_stmt *stmt;
    int rc, len;

    if (!db || !masukan || !keluaran || ukuran <= 1) return -1;

    len = (int)strlen(masukan);
    if (len <= 2) {
        snprintf(keluaran, (size_t)ukuran, "%s", masukan);
        return 0;
    }

    stmt = NULL;
    rc = sqlite3_prepare_v2(db,
        "SELECT kd.kata FROM kata kd "
        "JOIN morfologi_kata mk ON mk.id_kata_bentuk = kd.id "
        "JOIN kata kb ON mk.id_kata_dasar = kb.id "
        "WHERE lower(kd.kata) = lower(?) "
        "LIMIT 1;",
        -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        keluaran[0] = '\0';
        return -1;
    }

    sqlite3_bind_text(stmt, 1, masukan, -1, SQLITE_TRANSIENT);

    rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *val;

        val = (const char *)sqlite3_column_text(stmt, 0);
        if (val) {
            snprintf(keluaran, (size_t)ukuran, "%s", val);
            rc = 0;
        }
    }

    sqlite3_finalize(stmt);

    if (rc != 0) {
        snprintf(keluaran, (size_t)ukuran, "%s", masukan);
    }

    return rc;
}

/* ================================================================== *
 *                       STEMMING DENGAN DB + FALLBACK                  *
 * ================================================================== */

int stem_kata(
    sqlite3 *db,
    const char *masukan,
    char *keluaran,
    int ukuran)
{
    char hasil_db[MAX_PANJANG_STRING];
    char hasil_algo[MAX_PANJANG_STRING];

    if (!masukan || !keluaran || ukuran <= 1) return -1;

    keluaran[0] = '\0';

    if (stem_menggunakan_db(db, masukan, hasil_db,
        (int)sizeof(hasil_db)) == 0) {
        if (ajudan_strcasecmp(hasil_db, masukan) != 0) {
            snprintf(keluaran, (size_t)ukuran, "%s", hasil_db);
            return 0;
        }
    }

    if (stem_indonesia(masukan, hasil_algo,
        (int)sizeof(hasil_algo)) == 0) {
        if (ajudan_strcasecmp(hasil_algo, masukan) != 0) {
            snprintf(keluaran, (size_t)ukuran, "%s", hasil_algo);
            return 0;
        }
    }

    snprintf(keluaran, (size_t)ukuran, "%s", masukan);
    return 0;
}

/* ================================================================== *
 *                     INFO MORFOLOGI BENTUK KATA                        *
 * ================================================================== */

void identifikasi_morfologi_kata(
    const char *kata,
    char *prefiks,
    int uk_prefiks,
    char *sufiks,
    int uk_sufiks,
    int *panjang_prefiks,
    int *panjang_sufiks)
{
    char tmp[128];
    int len, i, j, pos, idx_a, idx_s;

    if (prefiks && uk_prefiks > 0) prefiks[0] = '\0';
    if (sufiks && uk_sufiks > 0) sufiks[0] = '\0';
    if (panjang_prefiks) *panjang_prefiks = 0;
    if (panjang_sufiks) *panjang_sufiks = 0;

    if (!kata || !kata[0]) return;

    len = (int)strlen(kata);
    for (i = 0; kata[i] != '\0' && i < (int)sizeof(tmp) - 1; i++) {
        tmp[i] = (char)tolower((unsigned char)kata[i]);
    }
    tmp[i] = '\0';

    if (prefiks && uk_prefiks > 1) {
        idx_a = -1;
        if (strncmp(tmp, "meng", 4) == 0) idx_a = 4;
        else if (strncmp(tmp, "meny", 4) == 0) idx_a = 4;
        else if (strncmp(tmp, "men", 3) == 0) idx_a = 3;
        else if (strncmp(tmp, "mem", 3) == 0) idx_a = 3;
        else if (strncmp(tmp, "me", 2) == 0) idx_a = 2;
        else if (strncmp(tmp, "peng", 4) == 0) idx_a = 4;
        else if (strncmp(tmp, "peny", 4) == 0) idx_a = 4;
        else if (strncmp(tmp, "pen", 3) == 0) idx_a = 3;
        else if (strncmp(tmp, "per", 3) == 0) idx_a = 3;
        else if (strncmp(tmp, "pe", 2) == 0) idx_a = 2;
        else if (strncmp(tmp, "ber", 3) == 0) idx_a = 3;
        else if (strncmp(tmp, "di", 2) == 0) idx_a = 2;
        else if (strncmp(tmp, "ter", 3) == 0) idx_a = 3;
        else if (strncmp(tmp, "ke", 2) == 0) idx_a = 2;

        if (idx_a > 0) {
            for (j = 0; j < idx_a && j < uk_prefiks - 1; j++) {
                prefiks[j] = tmp[j];
            }
            prefiks[j] = '\0';
            if (panjang_prefiks) *panjang_prefiks = idx_a;
        }
    }

    if (sufiks && uk_sufiks > 1) {
        idx_s = -1;
        pos = cek_akhir(tmp, len, "kan");
        if (pos >= 2 && pos != len) idx_s = 3;
        if (idx_s < 0) {
            pos = cek_akhir(tmp, len, "an");
            if (pos >= 2 && pos != len) idx_s = 2;
        }
        if (idx_s < 0) {
            pos = cek_akhir(tmp, len, "i");
            if (pos >= 2 && pos != len) idx_s = 1;
        }

        if (idx_s > 0) {
            j = 0;
            for (i = len - idx_s; i < len
                && j < uk_sufiks - 1; i++, j++) {
                sufiks[j] = tmp[i];
            }
            sufiks[j] = '\0';
            if (panjang_sufiks) *panjang_sufiks = idx_s;
        }
    }
}
