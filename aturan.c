/* ========================================================================== *
 * AJUDAN 3.1 - Rulebase (aturan.c)                                           *
 * -------------------------------------------------------------------------- *
 * C89, POSIX, SQLITE3                                                        *
 * -------------------------------------------------------------------------- *
 * Modul rulebase: normalisasi, tokenisasi, fuzzy,                           *
 * prepared statement cache, SPOK, dan pipeline utama                        *
 * percakapan. Handler respon dipindahkan ke percakapan.c.                   *
 * ========================================================================== */

#include "ajudan.h"
#include <stdio.h>

/* ========================================================================== *
 *                          STATE & KONTEKS GLOBAL                            *
 * ========================================================================== */

static SesiPercakapan sesi_aktif[MAX_SESSIONS];
static int jumlah_sesi_aktif = 0;
int aj_log_enabled = 0;

/* ========================================================================== *
 *                          LOGGING IMPLEMENTATION                            *
 * ========================================================================== */

void aj_set_log_enabled(int enabled) {
    aj_log_enabled = enabled;
}

void aj_log(const char *fmt, ...) {
    va_list ap;
    if (!aj_log_enabled) return;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

int aj_parse_cli_flags(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0
            || strcmp(argv[i], "--verbose") == 0) {
            aj_log_enabled = 1;
        }
    }

    return 0;
}

/* ========================================================================== *
 *                      UTILITAS STRING PORTABEL (C89)                        *
 * ========================================================================== */

/* strcasecmp portabel C89 */
int ajudan_strcasecmp(const char *s1, const char *s2) {
    unsigned char c1, c2;
    if (!s1 || !s2) return (s1 == s2) ? 0 : (s1 ? 1 : -1);
    while (*s1 && *s2) {
        c1 = (unsigned char)tolower((unsigned char)*s1);
        c2 = (unsigned char)tolower((unsigned char)*s2);
        if (c1 != c2) return (int)c1 - (int)c2;
        s1++; s2++;
    }
    return (int)(unsigned char)tolower((unsigned char)*s1)
         - (int)(unsigned char)tolower((unsigned char)*s2);
}

/* Fungsi parser untuk KategoriPOS */
KategoriPOS parse_pos(const char *pos_str)
{
    if (!pos_str) return POS_UNKNOWN;

    if (ajudan_strcasecmp(pos_str, "nomina") == 0)
        return POS_NOMINA;
    if (ajudan_strcasecmp(pos_str, "verba") == 0)
        return POS_VERBA;
    if (ajudan_strcasecmp(pos_str, "adjektiva") == 0)
        return POS_ADJEKTIVA;
    if (ajudan_strcasecmp(pos_str, "adverbia") == 0)
        return POS_ADVERBIA;
    if (ajudan_strcasecmp(pos_str, "numeralia") == 0)
        return POS_NUMERALIA;
    if (ajudan_strcasecmp(pos_str, "interjeksi") == 0)
        return POS_INTERJEKSI;
    if (ajudan_strcasecmp(pos_str, "pronomina") == 0)
        return POS_PRONOMINA;
    if (ajudan_strcasecmp(pos_str, "preposisi") == 0)
        return POS_PREPOSISI;
    if (ajudan_strcasecmp(pos_str, "konjungsi") == 0)
        return POS_KONJUNGSI;
    if (ajudan_strcasecmp(pos_str, "partikel") == 0)
        return POS_PARTIKEL;
    if (ajudan_strcasecmp(pos_str, "determiner") == 0)
        return POS_DETERMINER;

    return POS_UNKNOWN;
}

/* Implementasi lokal untuk fungsi utilitas agar mandiri */
void trim(char *s) {
    char *p = s;
    size_t len;
    if (!s) return;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

void to_lower_case(char *s) {
    if (!s) return;
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

/* ========================================================================== *
 *                   PREPARED STATEMENT CACHE (PERFORMA)                      *
 * ========================================================================== */

int aj_prepare_stmt_cache(
    sqlite3 *db, AjStmtCache *cache)
{
    int rc;

    if (!db || !cache) return -1;

    memset(cache, 0, sizeof(AjStmtCache));

    /* stmt 1: kata -> POS lookup */
    rc = sqlite3_prepare_v2(db,
        "SELECT kk.nama FROM kelas_kata kk "
        "JOIN kata k ON kk.id = k.kelas_id "
        "WHERE lower(k.kata) = lower(?) "
        "LIMIT 1;",
        -1, &cache->stmt_kategori_kata, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 2: stopword check */
    rc = sqlite3_prepare_v2(db,
        "SELECT 1 FROM kata "
        "WHERE lower(kata) = lower(?) "
        "AND kelas_id IN (5,6,7,8,10,11) "
        "LIMIT 1;",
        -1, &cache->stmt_stopword, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 3: all kata list */
    rc = sqlite3_prepare_v2(db,
        "SELECT kata FROM kata ORDER BY kata;",
        -1, &cache->stmt_kata_list, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 4: phrase normalization */
    rc = sqlite3_prepare_v2(db,
        "SELECT frasa_asli, frasa_hasil "
        "FROM normalisasi_frasa "
        "ORDER BY length(frasa_asli) DESC;",
        -1, &cache->stmt_normalisasi_frasa, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 5: pattern detection */
    /*
     * PENTING: SQL ini mengambil SEMUA pola dari deteksi_pola.
     * Pencocokan substring dilakukan di C (strstr) oleh
     * fungsi deteksi_jenis_dan_sumber_dari_db.
     * Kolom: 0=pola_teks, 1=jenis_kalimat.nama, 2=id_hubungan.
     * id_hubungan adalah TEXT (bukan join ke tabel hubungan)
     * untuk menghindari masalah type mismatch.
     */
    rc = sqlite3_prepare_v2(db,
        "SELECT dp.pola_teks, jk.nama, dp.id_hubungan "
        "FROM deteksi_pola dp "
        "JOIN jenis_kalimat jk "
        "ON dp.id_jenis = jk.id "
        "ORDER BY dp.prioritas ASC;",
        -1, &cache->stmt_deteksi_pola, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 6: pengetahuan_umum */
    rc = sqlite3_prepare_v2(db,
        "SELECT judul, ringkasan, "
        "penjelasan, saran "
        "FROM pengetahuan_umum "
        "WHERE id_kata = ("
        "SELECT id FROM kata "
        "WHERE lower(kata) = lower(?)) "
        "LIMIT 1;",
        -1, &cache->stmt_pen_umum_kata, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 7: arti_kata */
    rc = sqlite3_prepare_v2(db,
        "SELECT arti FROM arti_kata "
        "WHERE id_kata = ("
        "SELECT id FROM kata "
        "WHERE lower(kata) = lower(?)) "
        "LIMIT 1;",
        -1, &cache->stmt_arti_kata, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 8: semantik_kata relations */
    rc = sqlite3_prepare_v2(db,
        "SELECT k2.kata, h.nama, s.nama "
        "FROM semantik_kata sk "
        "JOIN kata k1 ON sk.id_kata_1 = k1.id "
        "JOIN kata k2 ON sk.id_kata_2 = k2.id "
        "LEFT JOIN hubungan h "
        "ON sk.id_hubungan = h.id "
        "LEFT JOIN semantik s "
        "ON sk.id_semantik = s.id "
        "WHERE lower(k1.kata) = lower(?) "
        "LIMIT 5;",
        -1, &cache->stmt_rel_semantik, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 9: pengetahuan_bertingkat */
    rc = sqlite3_prepare_v2(db,
        "SELECT poin, penjelasan, urutan "
        "FROM pengetahuan_bertingkat "
        "WHERE id_kata = ("
        "SELECT id FROM kata "
        "WHERE lower(kata) = lower(?)) "
        "AND lower(topik) = lower(?) "
        "ORDER BY urutan;",
        -1, &cache->stmt_pen_bertingkat_list, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 10: jawaban_bawaan random (fallback) */
    rc = sqlite3_prepare_v2(db,
        "SELECT jawaban FROM jawaban_bawaan "
        "WHERE tipe = 'fallback' "
        "ORDER BY RANDOM() LIMIT 1;",
        -1, &cache->stmt_respon_default_acak, NULL);
    if (rc != SQLITE_OK) goto gagal;

    /* stmt 11: sapaan random */
    rc = sqlite3_prepare_v2(db,
        "SELECT jawaban FROM jawaban_bawaan "
        "WHERE tipe = 'sapaan' "
        "ORDER BY RANDOM() LIMIT 1;",
        -1, &cache->stmt_sapaan_acak, NULL);
    if (rc != SQLITE_OK) goto gagal;

    return 0;

gagal:
    aj_log("aj_prepare_stmt_cache: %s\n",
        sqlite3_errmsg(db));
    aj_finalize_stmt_cache(cache);
    return -1;
}

void aj_finalize_stmt_cache(AjStmtCache *cache)
{
    if (!cache) return;

    if (cache->stmt_kategori_kata) {
        sqlite3_finalize(cache->stmt_kategori_kata);
        cache->stmt_kategori_kata = NULL;
    }
    if (cache->stmt_stopword) {
        sqlite3_finalize(cache->stmt_stopword);
        cache->stmt_stopword = NULL;
    }
    if (cache->stmt_kata_list) {
        sqlite3_finalize(cache->stmt_kata_list);
        cache->stmt_kata_list = NULL;
    }
    if (cache->stmt_normalisasi_frasa) {
        sqlite3_finalize(
            cache->stmt_normalisasi_frasa);
        cache->stmt_normalisasi_frasa = NULL;
    }
    if (cache->stmt_deteksi_pola) {
        sqlite3_finalize(cache->stmt_deteksi_pola);
        cache->stmt_deteksi_pola = NULL;
    }
    if (cache->stmt_pen_umum_kata) {
        sqlite3_finalize(cache->stmt_pen_umum_kata);
        cache->stmt_pen_umum_kata = NULL;
    }
    if (cache->stmt_arti_kata) {
        sqlite3_finalize(cache->stmt_arti_kata);
        cache->stmt_arti_kata = NULL;
    }
    if (cache->stmt_rel_semantik) {
        sqlite3_finalize(cache->stmt_rel_semantik);
        cache->stmt_rel_semantik = NULL;
    }
    if (cache->stmt_pen_bertingkat_list) {
        sqlite3_finalize(
            cache->stmt_pen_bertingkat_list);
        cache->stmt_pen_bertingkat_list = NULL;
    }
    if (cache->stmt_respon_default_acak) {
        sqlite3_finalize(
            cache->stmt_respon_default_acak);
        cache->stmt_respon_default_acak = NULL;
    }
    if (cache->stmt_sapaan_acak) {
        sqlite3_finalize(
            cache->stmt_sapaan_acak);
        cache->stmt_sapaan_acak = NULL;
    }
}

/* ========================================================================== *
 *                        FUZZY MATCHING & NORMALISASI                        *
 * ========================================================================== */

/* Levenshtein */
int hitung_jarak_levenshtein(const char *a, const char *b) {
    int len_a, len_b, i, j, cost, del, ins, sub, minv, result;
    int *d;

    if (!a || !b) return -1;
    len_a = (int)strlen(a);
    len_b = (int)strlen(b);
    if (len_a < 0 || len_b < 0) return -1;

    d = (int*)malloc((size_t)(len_a + 1) * (size_t)(len_b + 1) * sizeof(int));
    if (!d) return -1;

    for (i = 0; i <= len_a; i++) d[i * (len_b + 1)] = i;
    for (j = 0; j <= len_b; j++) d[j] = j;

    for (i = 1; i <= len_a; i++) {
        for (j = 1; j <= len_b; j++) {
            cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            del = d[(i - 1) * (len_b + 1) + j] + 1;
            ins = d[i * (len_b + 1) + (j - 1)] + 1;
            sub = d[(i - 1) * (len_b + 1) + (j - 1)] + cost;
            minv = del;
            if (ins < minv) minv = ins;
            if (sub < minv) minv = sub;
            d[i * (len_b + 1) + j] = minv;
        }
    }

    result = d[len_a * (len_b + 1) + len_b];
    free(d);
    return result;
}

double hitung_skor_fuzzy(int jarak, int panjang_max) {
    double s;
    if (panjang_max <= 0) return 0.0;
    if (jarak <= 0) return 1.0;
    s = 1.0 - ((double)jarak / (double)panjang_max);
    if (s < 0.0) s = 0.0;
    if (s > 1.0) s = 1.0;
    return s;
}

int normalisasi_frasa_input_dari_db(
    sqlite3 *db, AjStmtCache *cache,
    const char *input_asli,
    char *input_norm, size_t ukuran_input)
{
    int rc;
    const char *fa, *fh;
    char *pos;
    char buf_lo[MAX_INPUT_USER];
    char fra_lo[MAX_PANJANG_STRING];
    int len_fa, len_fh, offset;
    size_t sisa_len;
    int batas_ok;

    if (!db || !cache || !input_asli
        || !input_norm || ukuran_input == 0) {
        return -1;
    }

    /* Mulai dengan salinan input asli */
    snprintf(input_norm, ukuran_input, "%s", input_asli);

    rc = sqlite3_reset(cache->stmt_normalisasi_frasa);
    if (rc != SQLITE_OK) return -1;

    while (sqlite3_step(
        cache->stmt_normalisasi_frasa) == SQLITE_ROW) {
        fa = (const char *)sqlite3_column_text(
            cache->stmt_normalisasi_frasa, 0);
        fh = (const char *)sqlite3_column_text(
            cache->stmt_normalisasi_frasa, 1);

        if (!fa || !fh || !fa[0]) continue;

        len_fa = (int)strlen(fa);
        len_fh = (int)strlen(fh);

        /* Buat salinan huruf kecil untuk pencarian */
        snprintf(buf_lo, sizeof(buf_lo), "%s",
            input_norm);
        to_lower_case(buf_lo);

        snprintf(fra_lo, sizeof(fra_lo), "%s", fa);
        to_lower_case(fra_lo);

        /* Cari dan ganti semua kemunculan */
        pos = strstr(buf_lo, fra_lo);
        while (pos != NULL) {
            offset = (int)(pos - buf_lo);

            /*
             * Pengecekan batas kata (word boundary):
             * frasa harus berdiri sendiri sebagai kata,
             * bukan bagian dari kata lain.
             * Contoh: "lo" harus cocok di "lo ya"
             * tapi TIDAK di "halo".
             */
            batas_ok = 1;
            /* Karakter sebelumnya harus awal string,
             * spasi, atau tanda baca */
            if (offset > 0) {
                if (isalnum((unsigned char)
                    buf_lo[offset - 1])) {
                    batas_ok = 0;
                }
            }
            /* Karakter sesudahnya harus akhir string,
             * spasi, atau tanda baca */
            if (batas_ok
                && (offset + len_fa)
                    < (int)strlen(buf_lo)) {
                if (isalnum((unsigned char)
                    buf_lo[offset + len_fa])) {
                    batas_ok = 0;
                }
            }

            if (!batas_ok) {
                /* Lanjutkan pencarian dari pos+1 */
                pos = strstr(pos + 1, fra_lo);
                continue;
            }

            sisa_len = strlen(
                input_norm + offset + len_fa) + 1;

            /* Cek ruang buffer */
            if ((size_t)(offset + len_fh) + sisa_len
                > ukuran_input) {
                break;
            }

            /* Geser sisa string */
            memmove(
                input_norm + offset + len_fh,
                input_norm + offset + len_fa,
                sisa_len);

            /* Salin pengganti */
            memcpy(input_norm + offset, fh,
                (size_t)len_fh);

            /* Perbarui buffer huruf kecil */
            snprintf(buf_lo, sizeof(buf_lo), "%s",
                input_norm);
            to_lower_case(buf_lo);

            /* Lanjut cari setelah pengganti */
            pos = strstr(
                buf_lo + offset + len_fh, fra_lo);
        }
    }

    sqlite3_reset(cache->stmt_normalisasi_frasa);
    return 0;
}

int cari_topik_dengan_fuzzy(
    sqlite3 *db, AjStmtCache *cache,
    const char *masukan,
    char *topik_norm, size_t ukuran,
    int jarak_maks, double skor_min,
    KandidatFuzzy kandidat[], int max_kandidat)
{
    int rc, count, idx, i, jarak, pl, pm, panjang_max;
    double skor;
    const char *kata;

    if (!db || !cache || !masukan || !topik_norm
        || ukuran == 0) {
        return -1;
    }

    rc = sqlite3_reset(cache->stmt_kata_list);
    if (rc != SQLITE_OK) return -1;

    count = 0;
    while (sqlite3_step(cache->stmt_kata_list)
        == SQLITE_ROW) {
        kata = (const char *)sqlite3_column_text(
            cache->stmt_kata_list, 0);
        if (!kata) continue;

        jarak = hitung_jarak_levenshtein(masukan, kata);
        pl = (int)strlen(kata);
        pm = (int)strlen(masukan);
        panjang_max = (pl > pm) ? pl : pm;
        skor = hitung_skor_fuzzy(jarak, panjang_max);

        if (jarak <= jarak_maks
            && skor >= skor_min
            && count < max_kandidat) {
            snprintf(kandidat[count].teks,
                sizeof(kandidat[count].teks),
                "%s", masukan);
            snprintf(kandidat[count].cocok,
                sizeof(kandidat[count].cocok),
                "%s", kata);
            kandidat[count].jarak = jarak;
            kandidat[count].skor = skor;
            count++;
        }
    }
    sqlite3_reset(cache->stmt_kata_list);

    if (count > 0) {
        idx = 0;
        for (i = 1; i < count; i++) {
            if (kandidat[i].skor
                > kandidat[idx].skor) {
                idx = i;
            } else if (kandidat[i].skor
                    == kandidat[idx].skor
                && kandidat[i].jarak
                    < kandidat[idx].jarak) {
                idx = i;
            }
        }
        snprintf(topik_norm, ukuran, "%s",
            kandidat[idx].cocok);
        return 0;
    }

    snprintf(topik_norm, ukuran, "%s", masukan);
    return -1;
}

/* ========================================================================== *
 *                     TOKENISASI & KATEGORI FALLBACK                          *
 * ========================================================================== */

int tokenisasi_dengan_stem(
    sqlite3 *db,
    const char *kalimat,
    TokenKalimat tokens[],
    int max_tokens)
{
    int jml, i;

    if (!db || !kalimat || !tokens || max_tokens <= 0) {
        return 0;
    }

    jml = tokenisasi_kalimat(kalimat, tokens, max_tokens);
    if (jml <= 0) return 0;

    for (i = 0; i < jml; i++) {
        if (tokens[i].teks[0] != '\0') {
            stem_kata(db, tokens[i].teks,
                tokens[i].lema,
                (int)sizeof(tokens[i].lema));
        }
    }

    return jml;
}

int isi_kategori_dengan_stem(
    sqlite3 *db,
    AjStmtCache *cache,
    TokenKalimat tokens[],
    int jml_token)
{
    int i, rc;
    const char *teks;
    const char *pos_str;
    char kata_stem[MAX_PANJANG_TOKEN];

    if (!db || !cache || !tokens || jml_token <= 0) {
        return -1;
    }

    rc = sqlite3_reset(cache->stmt_kategori_kata);
    if (rc != SQLITE_OK) return -1;

    for (i = 0; i < jml_token; i++) {
        teks = tokens[i].teks;

        sqlite3_bind_text(cache->stmt_kategori_kata,
            1, teks, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(cache->stmt_kategori_kata)
            == SQLITE_ROW) {
            pos_str = (const char *)sqlite3_column_text(
                cache->stmt_kategori_kata, 0);
            tokens[i].pos = parse_pos(pos_str);
            if (tokens[i].lema[0] == '\0') {
                snprintf(tokens[i].lema,
                    sizeof(tokens[i].lema),
                    "%s", teks);
            }
        } else {
            if (stem_kata(db, teks, kata_stem,
                (int)sizeof(kata_stem)) == 0
                && ajudan_strcasecmp(kata_stem, teks)
                    != 0) {
                sqlite3_reset(cache->stmt_kategori_kata);
                sqlite3_bind_text(
                    cache->stmt_kategori_kata,
                    1, kata_stem, -1,
                    SQLITE_TRANSIENT);

                if (sqlite3_step(
                    cache->stmt_kategori_kata)
                    == SQLITE_ROW) {
                    pos_str = (const char *)
                        sqlite3_column_text(
                            cache->stmt_kategori_kata,
                            0);
                    tokens[i].pos = parse_pos(pos_str);
                    snprintf(tokens[i].lema,
                        sizeof(tokens[i].lema),
                        "%s", kata_stem);
                } else {
                    tokens[i].pos =
                        cari_kategori_fallback_dari_db(
                            db, teks);
                    if (tokens[i].lema[0] == '\0') {
                        snprintf(
                            tokens[i].lema,
                            sizeof(tokens[i].lema),
                            "%s", teks);
                    }
                }
            } else {
                tokens[i].pos =
                    cari_kategori_fallback_dari_db(
                        db, teks);
                if (tokens[i].lema[0] == '\0') {
                    snprintf(tokens[i].lema,
                        sizeof(tokens[i].lema),
                        "%s", teks);
                }
            }
        }

        sqlite3_reset(cache->stmt_kategori_kata);
    }

    return 0;
}

int tokenisasi_kalimat(
    const char *kalimat,
    TokenKalimat tokens[], int max_tokens)
{
    char buf[MAX_INPUT_USER];
    char *tok;
    int count, i;

    if (!kalimat || !tokens || max_tokens <= 0)
        return 0;

    snprintf(buf, sizeof(buf), "%s", kalimat);
    for (i = 0; buf[i]; i++) {
        if (ispunct((unsigned char)buf[i])
            && buf[i] != '-') {
            buf[i] = ' ';
        }
    }

    tok = strtok(buf, " \t\n");
    count = 0;
    while (tok && count < max_tokens) {
        snprintf(tokens[count].teks,
            sizeof(tokens[count].teks), "%s", tok);
        tokens[count].lema[0] = '\0';
        tokens[count].pos = POS_UNKNOWN;
        tokens[count].id = count;
        count++;
        tok = strtok(NULL, " \t\n");
    }
    return count;
}

KategoriPOS cari_kategori_fallback_dari_db(
    sqlite3 *db, const char *kata)
{
    int len, i, all_digit;

    (void)db;
    if (!kata || !kata[0]) return POS_UNKNOWN;

    if (isupper((unsigned char)kata[0]))
        return POS_PROPER_NOUN;

    len = (int)strlen(kata);
    all_digit = 1;
    for (i = 0; i < len; i++) {
        if (!isdigit((unsigned char)kata[i])) {
            all_digit = 0;
            break;
        }
    }
    if (all_digit) return POS_NUMERALIA;

    if (strncmp(kata, "me", 2) == 0
        || strncmp(kata, "ber", 3) == 0
        || strncmp(kata, "di", 2) == 0
        || strncmp(kata, "ter", 3) == 0) {
        return POS_VERBA;
    }
    if (strncmp(kata, "pe", 2) == 0
        || strncmp(kata, "pen", 3) == 0
        || strncmp(kata, "peng", 4) == 0
        || strncmp(kata, "peny", 4) == 0) {
        return POS_NOMINA;
    }
    if (len > 2) {
        if (strcmp(kata + len - 2, "an") == 0)
            return POS_NOMINA;
        if (strcmp(kata + len - 1, "i") == 0
            || (len > 3
                && strcmp(kata + len - 3, "kan") == 0)) {
            return POS_VERBA;
        }
    }
    return POS_NOMINA;
}

int isi_kategori_dari_database(
    sqlite3 *db, AjStmtCache *cache,
    TokenKalimat tokens[], int jml_token)
{
    int i, rc;
    const char *teks;
    const char *pos_str;

    if (!db || !cache || !tokens || jml_token <= 0)
        return -1;

    rc = sqlite3_reset(cache->stmt_kategori_kata);
    if (rc != SQLITE_OK) return -1;

    for (i = 0; i < jml_token; i++) {
        teks = tokens[i].teks;
        sqlite3_bind_text(cache->stmt_kategori_kata,
            1, teks, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(cache->stmt_kategori_kata)
            == SQLITE_ROW) {
            pos_str = (const char *)
                sqlite3_column_text(
                    cache->stmt_kategori_kata, 0);
            tokens[i].pos = parse_pos(pos_str);
            snprintf(tokens[i].lema,
                sizeof(tokens[i].lema),
                "%s", teks);
        } else {
            tokens[i].pos =
                cari_kategori_fallback_dari_db(db, teks);
            snprintf(tokens[i].lema,
                sizeof(tokens[i].lema),
                "%s", teks);
        }
        sqlite3_reset(cache->stmt_kategori_kata);
    }
    return 0;
}

/* ========================================================================== *
 *                         DETEKSI JENIS KALIMAT                               *
 * ========================================================================== */

int deteksi_jenis_dan_sumber_dari_db(
    sqlite3 *db, AjStmtCache *cache,
    const char *kalimat_norm,
    char *jenis, size_t ukuran_jenis,
    char *sumber, size_t ukuran_sumber)
{
    int rc;
    const char *pola_teks;
    const char *jenis_kalimat;
    const char *id_hubungan_str;

    if (!db || !cache || !kalimat_norm
        || !jenis || ukuran_jenis == 0
        || !sumber || ukuran_sumber == 0) {
        return -1;
    }

    jenis[0] = '\0';
    sumber[0] = '\0';

    rc = sqlite3_reset(cache->stmt_deteksi_pola);
    if (rc != SQLITE_OK) return -1;

    while (sqlite3_step(cache->stmt_deteksi_pola)
        == SQLITE_ROW) {
        pola_teks = (const char *)sqlite3_column_text(
            cache->stmt_deteksi_pola, 0);
        jenis_kalimat = (const char *)
            sqlite3_column_text(
                cache->stmt_deteksi_pola, 1);
        id_hubungan_str = (const char *)
            sqlite3_column_text(
                cache->stmt_deteksi_pola, 2);

        if (!pola_teks || !jenis_kalimat) continue;

        /* Cocokkan substring menggunakan strstr */
        if (strstr(kalimat_norm, pola_teks) != NULL) {
            snprintf(jenis, ukuran_jenis, "%s",
                jenis_kalimat);
            if (id_hubungan_str
                && id_hubungan_str[0]) {
                snprintf(sumber, ukuran_sumber, "%s",
                    id_hubungan_str);
            } else {
                snprintf(sumber, ukuran_sumber, "%s",
                    "gabungan_spok");
            }
            sqlite3_reset(cache->stmt_deteksi_pola);
            return 0;
        }
    }

    sqlite3_reset(cache->stmt_deteksi_pola);
    return -1;
}

/* ========================================================================== *
 *                PENCARIAN ENTITAS TERKAIT DENGAN FILTER                      *
 * ========================================================================== */

int cari_entitas_terkait_dengan_filter(
    sqlite3 *db,
    const char *kata_kunci,
    const char *filter_kategori,
    const char *filter_bidang,
    const char *filter_gaya,
    EntitasTerkait hasil[],
    int max_hasil)
{
    sqlite3_stmt *stmt = NULL;
    int count = 0;
    const char *sql =
        "SELECT k.kata, ak.arti, kk.nama, b.nama, "
        "rk.nama "
        "FROM kata k "
        "LEFT JOIN arti_kata ak "
        "ON k.id = ak.id_kata "
        "LEFT JOIN kelas_kata kk "
        "ON k.kelas_id = kk.id "
        "LEFT JOIN bidang b "
        "ON k.bidang_id = b.id "
        "LEFT JOIN ragam_kata rk "
        "ON k.ragam_id = rk.id "
        "WHERE lower(k.kata) LIKE lower(?) "
        "AND (?='' OR kk.nama=? ) "
        "AND (?='' OR b.nama=? ) "
        "AND (?='' OR rk.nama=? ) "
        "LIMIT ?;";

    if (!db || !kata_kunci || !hasil || max_hasil <= 0)
        return -1;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)
        != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, kata_kunci, -1,
        SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2,
        filter_kategori ? filter_kategori : "",
        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3,
        filter_kategori ? filter_kategori : "",
        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4,
        filter_bidang ? filter_bidang : "",
        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5,
        filter_bidang ? filter_bidang : "",
        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6,
        filter_gaya ? filter_gaya : "",
        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7,
        filter_gaya ? filter_gaya : "",
        -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, max_hasil);

    while (sqlite3_step(stmt) == SQLITE_ROW
        && count < max_hasil) {
        const char *k = (const char *)
            sqlite3_column_text(stmt, 0);
        const char *a = (const char *)
            sqlite3_column_text(stmt, 1);
        const char *c = (const char *)
            sqlite3_column_text(stmt, 2);
        const char *b = (const char *)
            sqlite3_column_text(stmt, 3);
        const char *g = (const char *)
            sqlite3_column_text(stmt, 4);

        snprintf(hasil[count].kata,
            sizeof(hasil[count].kata), "%s",
            k ? k : "");
        snprintf(hasil[count].arti,
            sizeof(hasil[count].arti), "%s",
            a ? a : "");
        snprintf(hasil[count].kategori,
            sizeof(hasil[count].kategori), "%s",
            c ? c : "");
        snprintf(hasil[count].bidang,
            sizeof(hasil[count].bidang), "%s",
            b ? b : "");
        snprintf(hasil[count].gaya,
            sizeof(hasil[count].gaya), "%s",
            g ? g : "");
        hasil[count].skor_relevansi = 1.0;
        count++;
    }

    sqlite3_finalize(stmt);
    return count;
}

/* ========================================================================== *
 *              QUERY ISI: MAKNA, RELASI, PENGETAHUAN                         *
 * ========================================================================== */

int ambil_arti_kata(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *lema,
    char *definisi,
    size_t ukuran)
{
    int rc;
    const char *defi;
    char kata_stem[MAX_PANJANG_TOKEN];

    definisi[0] = '\0';
    if (!db || !cache || !lema || !definisi
        || ukuran == 0) {
        return -1;
    }

    rc = sqlite3_reset(cache->stmt_arti_kata);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(cache->stmt_arti_kata,
        1, lema, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(cache->stmt_arti_kata)
        == SQLITE_ROW) {
        defi = (const char *)sqlite3_column_text(
            cache->stmt_arti_kata, 0);
        if (defi) {
            snprintf(definisi, ukuran, "%s", defi);
            sqlite3_reset(cache->stmt_arti_kata);
            return 0;
        }
    }

    sqlite3_reset(cache->stmt_arti_kata);

    if (stem_kata(db, lema, kata_stem,
        (int)sizeof(kata_stem)) == 0
        && ajudan_strcasecmp(kata_stem, lema) != 0) {
        sqlite3_bind_text(cache->stmt_arti_kata,
            1, kata_stem, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(cache->stmt_arti_kata)
            == SQLITE_ROW) {
            defi = (const char *)sqlite3_column_text(
                cache->stmt_arti_kata, 0);
            if (defi) {
                snprintf(definisi, ukuran, "%s", defi);
                sqlite3_reset(cache->stmt_arti_kata);
                return 0;
            }
        }

        sqlite3_reset(cache->stmt_arti_kata);
    }

    return -1;
}

int ambil_pengetahuan_umum(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *topik,
    char *judul, size_t uj,
    char *ringkasan, size_t ur,
    char *penjelasan, size_t up,
    char *saran, size_t us)
{
    int rc;
    const char *j, *r, *p, *s;
    char kata_stem[MAX_PANJANG_TOKEN];

    if (!db || !cache || !topik || !judul
        || !ringkasan || !penjelasan || !saran
        || uj == 0 || ur == 0 || up == 0
        || us == 0) {
        return -1;
    }

    judul[0] = '\0';
    ringkasan[0] = '\0';
    penjelasan[0] = '\0';
    saran[0] = '\0';

    rc = sqlite3_reset(cache->stmt_pen_umum_kata);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(cache->stmt_pen_umum_kata,
        1, topik, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(cache->stmt_pen_umum_kata)
        == SQLITE_ROW) {
        j = (const char *)sqlite3_column_text(
            cache->stmt_pen_umum_kata, 0);
        r = (const char *)sqlite3_column_text(
            cache->stmt_pen_umum_kata, 1);
        p = (const char *)sqlite3_column_text(
            cache->stmt_pen_umum_kata, 2);
        s = (const char *)sqlite3_column_text(
            cache->stmt_pen_umum_kata, 3);
        if (j) snprintf(judul, uj, "%s", j);
        if (r) snprintf(ringkasan, ur, "%s", r);
        if (p) snprintf(penjelasan, up, "%s", p);
        if (s) snprintf(saran, us, "%s", s);
    }

    sqlite3_reset(cache->stmt_pen_umum_kata);

    if (judul[0] != '\0' || ringkasan[0] != '\0') {
        return 0;
    }

    if (stem_kata(db, topik, kata_stem,
        (int)sizeof(kata_stem)) == 0
        && ajudan_strcasecmp(kata_stem, topik) != 0) {
        sqlite3_bind_text(cache->stmt_pen_umum_kata,
            1, kata_stem, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(cache->stmt_pen_umum_kata)
            == SQLITE_ROW) {
            j = (const char *)sqlite3_column_text(
                cache->stmt_pen_umum_kata, 0);
            r = (const char *)sqlite3_column_text(
                cache->stmt_pen_umum_kata, 1);
            p = (const char *)sqlite3_column_text(
                cache->stmt_pen_umum_kata, 2);
            s = (const char *)sqlite3_column_text(
                cache->stmt_pen_umum_kata, 3);
            if (j) snprintf(judul, uj, "%s", j);
            if (r) snprintf(ringkasan, ur, "%s", r);
            if (p) snprintf(penjelasan, up, "%s", p);
            if (s) snprintf(saran, us, "%s", s);
        }

        sqlite3_reset(cache->stmt_pen_umum_kata);
    }

    if (judul[0] == '\0' && ringkasan[0] == '\0'
        && penjelasan[0] == '\0'
        && saran[0] == '\0') {
        return -1;
    }

    return 0;
}

int ambil_hubungan_semantik(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *topik,
    char *relasi,
    size_t ukuran)
{
    int rc, coba_stem;
    const char *kata2, *jenis_hub;
    char frag[256];
    char kata_stem[MAX_PANJANG_TOKEN];

    relasi[0] = '\0';
    if (!db || !cache || !topik || !relasi
        || ukuran == 0) {
        return -1;
    }

    coba_stem = 0;

    do {
        rc = sqlite3_reset(cache->stmt_rel_semantik);
        if (rc != SQLITE_OK) return -1;

        sqlite3_bind_text(cache->stmt_rel_semantik,
            1, coba_stem ? kata_stem : topik,
            -1, SQLITE_TRANSIENT);

        while (sqlite3_step(cache->stmt_rel_semantik)
            == SQLITE_ROW) {
            kata2 = (const char *)sqlite3_column_text(
                cache->stmt_rel_semantik, 0);
            jenis_hub = (const char *)
                sqlite3_column_text(
                    cache->stmt_rel_semantik, 1);
            if (kata2) {
                snprintf(frag, sizeof(frag), "%s %s",
                    (jenis_hub && jenis_hub[0])
                        ? jenis_hub : "terkait",
                    kata2);
                if (relasi[0] != '\0') {
                    strncat(relasi, "; ",
                        ukuran - strlen(relasi) - 1);
                }
                strncat(relasi, frag,
                    ukuran - strlen(relasi) - 1);
            }
        }

        sqlite3_reset(cache->stmt_rel_semantik);

        if (relasi[0] != '\0') return 0;

        if (!coba_stem) {
            if (stem_kata(db, topik, kata_stem,
                (int)sizeof(kata_stem)) == 0
                && ajudan_strcasecmp(kata_stem, topik)
                    != 0) {
                coba_stem = 1;
            } else {
                break;
            }
        } else {
            break;
        }
    } while (coba_stem > 0);

    return -1;
}

/* ========================================================================== *
 *                GENERATOR RESPONS: SPOK + BERTINGKAT                         *
 * ========================================================================== */

int format_respons_daftar(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *kata,
    const char *topik_bertingkat,
    int dengan_detail,
    char *output,
    size_t uo)
{
    int rc, i, ur;
    const char *p, *k;
    char header[256];
    char line[512];
    char kata_stem[MAX_PANJANG_TOKEN];

    if (!db || !cache || !kata || !topik_bertingkat
        || !output || uo == 0) {
        return -1;
    }

    snprintf(header, sizeof(header),
        "Berikut %s %s:\n",
        ajudan_strcasecmp(topik_bertingkat, "fakta")
            == 0
            ? "daftar"
            : (ajudan_strcasecmp(
                topik_bertingkat, "penyebab")
                == 0
                ? "hal terkait" : "langkah"),
        kata);
    snprintf(output, uo, "%s", header);

    rc = sqlite3_reset(cache->stmt_pen_bertingkat_list);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(
        cache->stmt_pen_bertingkat_list,
        1, kata, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(
        cache->stmt_pen_bertingkat_list,
        2, topik_bertingkat, -1, SQLITE_TRANSIENT);

    i = 0;
    while (sqlite3_step(
            cache->stmt_pen_bertingkat_list)
        == SQLITE_ROW) {
        p = (const char *)sqlite3_column_text(
            cache->stmt_pen_bertingkat_list, 0);
        k = (const char *)sqlite3_column_text(
            cache->stmt_pen_bertingkat_list, 1);
        ur = sqlite3_column_int(
            cache->stmt_pen_bertingkat_list, 2);

        snprintf(line, sizeof(line), "%d. %s",
            ur, p ? p : "");
        strncat(output, line,
            uo - strlen(output) - 1);
        if (dengan_detail && k && k[0]) {
            strncat(output, " \xe2\x80\x94 ",
                uo - strlen(output) - 1);
            strncat(output, k,
                uo - strlen(output) - 1);
        }
        strncat(output, "\n",
            uo - strlen(output) - 1);
        i++;
    }

    sqlite3_reset(cache->stmt_pen_bertingkat_list);

    if (i > 0) return 0;

    if (stem_kata(db, kata, kata_stem,
        (int)sizeof(kata_stem)) == 0
        && ajudan_strcasecmp(kata_stem, kata) != 0) {
        sqlite3_bind_text(
            cache->stmt_pen_bertingkat_list,
            1, kata_stem, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(
            cache->stmt_pen_bertingkat_list,
            2, topik_bertingkat, -1, SQLITE_TRANSIENT);

        i = 0;
        while (sqlite3_step(
                cache->stmt_pen_bertingkat_list)
            == SQLITE_ROW) {
            p = (const char *)sqlite3_column_text(
                cache->stmt_pen_bertingkat_list, 0);
            k = (const char *)sqlite3_column_text(
                cache->stmt_pen_bertingkat_list, 1);
            ur = sqlite3_column_int(
                cache->stmt_pen_bertingkat_list, 2);

            snprintf(line, sizeof(line), "%d. %s",
                ur, p ? p : "");
            strncat(output, line,
                uo - strlen(output) - 1);
            if (dengan_detail && k && k[0]) {
                strncat(output, " \xe2\x80\x94 ",
                    uo - strlen(output) - 1);
                strncat(output, k,
                    uo - strlen(output) - 1);
            }
            strncat(output, "\n",
                uo - strlen(output) - 1);
            i++;
        }

        sqlite3_reset(cache->stmt_pen_bertingkat_list);

        if (i > 0) return 0;
    }

    return -1;
}

/* ========================================================================== *
 *                    FITUR TAMBAHAN: FORMATTER ANALITIK                       *
 * ========================================================================== */

void format_jawaban_analitik(
    const char *topik,
    const char *analisis,
    const char *saran_verifikasi,
    char *output, size_t ukuran_output)
{
    const char *saran;

    if (!topik || !analisis || !output
        || ukuran_output == 0) {
        return;
    }

    saran = (saran_verifikasi && saran_verifikasi[0])
        ? saran_verifikasi
        : "sumber terpercaya";

    snprintf(output, ukuran_output,
        "Berdasarkan data yang saya pelajari, "
        "'%s' %s. Untuk kepastiannya, "
        "Anda bisa memeriksanya sendiri di %s.",
        topik, analisis, saran);
}

/* ========================================================================== *
 *            KERANGKA RESPON BERBASIS DATA (pola_kalimat)                     *
 * ========================================================================== */

int pilih_kerangka_respon(
    sqlite3 *db,
    const char *hubungan,
    char *kerangka,
    size_t ukuran_kerangka)
{
    sqlite3_stmt *stmt = NULL;
    int rc;
    const char *val;
    char nama_hubungan[256];

    if (!db || !hubungan || !kerangka
        || ukuran_kerangka == 0) {
        return -1;
    }
    kerangka[0] = '\0';

    memset(nama_hubungan, 0, sizeof(nama_hubungan));
    if (sqlite3_prepare_v2(db,
        "SELECT nama FROM hubungan "
        "WHERE id=? LIMIT 1;",
        -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, hubungan, -1,
            SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *name =
                (const char *)sqlite3_column_text(
                    stmt, 0);
            if (name) {
                snprintf(nama_hubungan,
                    sizeof(nama_hubungan), "%s",
                    name);
            }
        }
        sqlite3_finalize(stmt);
    }

    {
        const char *hub_key = nama_hubungan[0]
            ? nama_hubungan : hubungan;

        rc = sqlite3_prepare_v2(db,
            "SELECT kerangka_jawaban "
            "FROM pola_kalimat "
            "WHERE id_hubungan=? "
            "ORDER BY prioritas DESC LIMIT 1;",
            -1, &stmt, NULL);
        if (rc != SQLITE_OK) return -1;

        sqlite3_bind_text(stmt, 1, hub_key, -1,
            SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            val = (const char *)
                sqlite3_column_text(stmt, 0);
            if (val) {
                snprintf(kerangka,
                    ukuran_kerangka, "%s", val);
            }
        }
        sqlite3_finalize(stmt);
    }

    return (kerangka[0] == '\0') ? -1 : 0;
}

/* ========================================================================== *
 *                       FALLBACK RESPON DEFAULT                               *
 * ========================================================================== */

int ambil_respon_default_acak(
    sqlite3 *db, AjStmtCache *cache,
    char *output, size_t ukuran_output)
{
    int rc;
    const char *respon;

    if (!db || !cache || !output
        || ukuran_output == 0) {
        return -1;
    }

    rc = sqlite3_reset(cache->stmt_respon_default_acak);
    if (rc != SQLITE_OK) return -1;

    if (sqlite3_step(cache->stmt_respon_default_acak)
        == SQLITE_ROW) {
        respon = (const char *)sqlite3_column_text(
            cache->stmt_respon_default_acak, 0);
        if (respon) {
            snprintf(output, ukuran_output, "%s",
                respon);
            sqlite3_reset(
                cache->stmt_respon_default_acak);
            return 0;
        }
    }
    sqlite3_reset(cache->stmt_respon_default_acak);
    return -1;
}

/* ========================================================================== *
 *                          SESI PERCAKAPAN                                    *
 * ========================================================================== */

int cari_atau_buat_sesi_percakapan(const char *user_id)
{
    int i, id;
    time_t now;

    now = time(NULL);
    for (i = 0; i < jumlah_sesi_aktif; i++) {
        if (strcmp(sesi_aktif[i].user_id, user_id)
            == 0) {
            sesi_aktif[i].last_activity = now;
            return i;
        }
    }
    if (jumlah_sesi_aktif < MAX_SESSIONS) {
        id = jumlah_sesi_aktif++;
        sesi_aktif[id].id = id;
        snprintf(sesi_aktif[id].user_id,
            sizeof(sesi_aktif[id].user_id),
            "%s", user_id);
        sesi_aktif[id].topik_utama[0] = '\0';
        sesi_aktif[id].tipe_list[0] = '\0';
        snprintf(sesi_aktif[id].status_alur,
            sizeof(sesi_aktif[id].status_alur),
            "idle");
        sesi_aktif[id].pertanyaan_terakhir[0] = '\0';
        sesi_aktif[id].cache_alias_key[0] = '\0';
        sesi_aktif[id].cache_alias_val[0] = '\0';
        sesi_aktif[id].fuzzy_threshold = FUZZY_MIN_SKOR;
        sesi_aktif[id].jarak_maks = FUZZY_MAX_JARAK;
        sesi_aktif[id].last_activity = now;
        return id;
    }
    sesi_aktif[0].last_activity = now;
    return 0;
}

void perbarui_sesi_percakapan(
    int sesi_id,
    const char *topik_baru,
    const char *tipe_list,
    const char *status,
    const char *pertanyaan)
{
    time_t now;

    now = time(NULL);
    if (sesi_id < 0
        || sesi_id >= jumlah_sesi_aktif) {
        return;
    }

    if (topik_baru && strlen(topik_baru) > 0) {
        snprintf(sesi_aktif[sesi_id].topik_utama,
            sizeof(sesi_aktif[sesi_id].topik_utama),
            "%s", topik_baru);
    }
    if (tipe_list) {
        snprintf(sesi_aktif[sesi_id].tipe_list,
            sizeof(sesi_aktif[sesi_id].tipe_list),
            "%s", tipe_list);
    }
    if (status) {
        snprintf(sesi_aktif[sesi_id].status_alur,
            sizeof(sesi_aktif[sesi_id].status_alur),
            "%s", status);
    }
    if (pertanyaan) {
        snprintf(
            sesi_aktif[sesi_id].pertanyaan_terakhir,
            sizeof(
                sesi_aktif[sesi_id].pertanyaan_terakhir),
            "%s", pertanyaan);
    }

    sesi_aktif[sesi_id].last_activity = now;
}

int ambil_jumlah_sesi(void)
{
    return jumlah_sesi_aktif;
}

const char *ambil_topik_sesi(int sesi_id)
{
    if (sesi_id < 0
        || sesi_id >= jumlah_sesi_aktif) {
        return NULL;
    }
    return sesi_aktif[sesi_id].topik_utama;
}

void bersihkan_sesi_kadaluarsa(void)
{
    time_t now, batas;
    int i, j;

    now = time(NULL);
    batas = now - (24 * 60 * 60);
    for (i = 0; i < jumlah_sesi_aktif; i++) {
        if (sesi_aktif[i].last_activity < batas) {
            for (j = i;
                j < jumlah_sesi_aktif - 1; j++) {
                sesi_aktif[j] = sesi_aktif[j + 1];
            }
            jumlah_sesi_aktif--;
            i--;
        }
    }
}

/* ========================================================================== *
 *                   RESOLUSI REFERENSI KONTEKSTUAL                            *
 * ========================================================================== */

/*
 * Deteksi apakah topik yang diekstrak hanya berisi
 * kata-kata demonstratif ("itu", "hal itu", dll)
 * tanpa konten nyata, sehingga perlu diganti dengan
 * topik sesi sebelumnya.
 *
 * Return 1 jika topik kosong/berisi referensi,
 * 0 jika topik memiliki konten nyata.
 */
int adalah_referensi_kosong(const char *topik)
{
    char buf[MAX_PANJANG_STRING];
    char *t;
    int ada_konten = 0;

    if (!topik || topik[0] == '\0') return 1;

    snprintf(buf, sizeof(buf), "%s", topik);
    to_lower_case(buf);

    t = strtok(buf, " \t\n");
    while (t != NULL) {
        if (strcmp(t, "itu") != 0
            && strcmp(t, "ini") != 0
            && strcmp(t, "hal") != 0
            && strcmp(t, "masalah") != 0
            && strcmp(t, "yg") != 0
            && strcmp(t, "yang") != 0
            && strcmp(t, "dia") != 0
            && strcmp(t, "mereka") != 0
            && strlen(t) > 1) {
            ada_konten = 1;
        }
        t = strtok(NULL, " \t\n");
    }

    return ada_konten ? 0 : 1;
}

/*
 * Cek apakah input mengandung pola referensi
 * kontekstual yang menandakan topik sebenarnya
 * ada di kalimat lain atau sesi sebelumnya.
 *
 * Pola yang dideteksi: "tentang itu", "hal itu",
 * "hal tersebut", "tentang hal", "mengenai hal",
 * "soal itu", "lebih tentang", "lagi tentang".
 */
int mengandung_referensi(const char *input)
{
    char lo[MAX_INPUT_USER];
    int i;

    static const char *pola[] = {
        "tentang itu",
        "tentang hal itu",
        "tentang hal",
        "mengenai itu",
        "mengenai hal",
        "soal itu",
        "masalah itu",
        "hal tersebut",
        "hal yang sama",
        "lebih lanjut tentang",
        "lebih tentang",
        "lagi tentang",
        "tentang dia",
        "tentang mereka",
        NULL
    };

    if (!input || !input[0]) return 0;

    snprintf(lo, sizeof(lo), "%s", input);
    to_lower_case(lo);

    for (i = 0; pola[i] != NULL; i++) {
        if (strstr(lo, pola[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

/*
 * Cari topik nyata dari kalimat input yang berisi
 * referensi kontekstual ("tentang hal itu",
 * "sesuatu tentang itu", dll).
 *
 * Algoritma: hapus frasa referensi dan kata tanya
 * dari kalimat, sisanya adalah topik kontekstual.
 *
 * Contoh:
 * "saya sedang mencari tahu tentang jaringan,
 *  apakah kamu tahu sesuatu tentang hal itu?"
 * -> kalimat lengkap = "saya sedang mencari tahu
 *    tentang jaringan apakah kamu tahu sesuatu
 *    tentang hal itu"
 * -> hapus klausa tanya + referensi ->
 *    "saya sedang mencari tahu tentang jaringan"
 * -> ekstrak kata bermakna -> "jaringan"
 *
 * Return 1 jika topik ditemukan, 0 jika tidak.
 */
int cari_topik_dari_konteks(
    const char *input,
    char *topik_out,
    size_t ukuran)
{
    char buf[MAX_INPUT_USER];
    char cleaned[MAX_INPUT_USER];
    char *t;
    int pertama, i, j, is_sw;

    /*
     * Daftar frasa referensi yang menandakan
     * bagian belakang kalimat adalah referensi,
     * bukan konten.
     */
    static const char *frasa_referensi[] = {
        "tentang hal itu",
        "tentang hal tersebut",
        "tentang itu",
        "tentang hal",
        "mengenai hal itu",
        "mengenai hal tersebut",
        "mengenai itu",
        "mengenai hal",
        "soal itu",
        "masalah itu",
        "hal tersebut",
        "hal yang sama",
        NULL
    };

    char *pemotong;

    if (!input || !topik_out || ukuran == 0)
        return 0;

    snprintf(buf, sizeof(buf), "%s", input);
    to_lower_case(buf);
    trim(buf);

    /*
     * Langkah 1: Cari dan potong kalimat di
     * pemisah klausa (koma + konjungsi).
     * Ambil bagian SEBELUM pemisah yang
     * mengandung referensi.
     */
    {
        static const char *pemisah_ref[] = {
            ", apakah kamu tahu",
            ", kamu tahu",
            ", bisa jelaskan",
            ", jelaskan",
            ", tolong jelaskan",
            ", mohon jelaskan",
            ", apa itu",
            ", mengapa",
            ", kenapa",
            ", bagaimana",
            NULL
        };

        for (i = 0; pemisah_ref[i] != NULL; i++) {
            pemotong = strstr(buf, pemisah_ref[i]);
            if (pemotong != NULL) {
                *pemotong = '\0';
                trim(buf);
                break;
            }
        }
    }

    /*
     * Langkah 2: Hapus frasa pembuka yang bukan
     * konten (saya sedang mencari, saya ingin tahu,
     * dll).
     */
    {
        static const char *pembuka[] = {
            "saya sedang mencari",
            "saya sedang kesulitan",
            "saya sedang bingung",
            "saya sedang belajar",
            "saya ingin tahu",
            "saya mau tahu",
            "saya lagi cari",
            "saya lagi mencari",
            "saya sedang mempelajari",
            "saya sedang memahami",
            "saya sedang mengetahui",
            "saya penasaran",
            "saya butuh",
            "saya perlukan",
            "saya sedang memikirkan",
            "tolong jelaskan",
            "coba jelaskan",
            NULL
        };

        for (i = 0; pembuka[i] != NULL; i++) {
            if (strncmp(buf, pembuka[i],
                strlen(pembuka[i])) == 0) {
                size_t awal = strlen(pembuka[i]);
                size_t sisa_buf =
                    strlen(buf) - awal;
                memmove(buf, buf + awal,
                    sisa_buf + 1);
                trim(buf);
                break;
            }
        }
    }

    /*
     * Langkah 3: Hapus frasa referensi tersisa
     * dan kata lumpat sederhana.
     */
    for (i = 0; frasa_referensi[i] != NULL; i++) {
        char *pos = strstr(buf, frasa_referensi[i]);
        if (pos != NULL) {
            size_t len_ref =
                strlen(frasa_referensi[i]);
            size_t len_after = strlen(pos)
                - len_ref;
            memmove(pos, pos + len_ref,
                len_after + 1);
            trim(buf);
        }
    }

    /*
     * Langkah 4: Ambil kata bermakna dari sisa
     * teks. Gunakan daftar stopword sederhana.
     */
    {
        static const char *sw[] = {
            "saya", "kamu", "aku", "dia",
            "tidak", "bukan", "belum", "sudah",
            "sedang", "masih", "akan", "bisa",
            "dapat", "mau", "ingin", "hendak",
            "tentang", "untuk", "dari", "dengan",
            "pada", "di", "oleh", "ke", "dalam",
            "yang", "yg", "juga", "pun", "saja",
            "hanya", "lagi", "ini", "itu",
            "tersebut", "hal", "masalah",
            "apa", "siapa", "bagaimana",
            "mengapa", "kenapa", "kapan",
            "apakah", "sesuatu", "sebuah",
            "suatu", "seorang", "cari",
            "tahu", "paham", "mengerti",
            "mencari", "kesulitan",
            "mempelajari", "memahami",
            "menemukan", "mendapatkan",
            "informasi", "data",
            "mengenai", "soal",
            NULL
        };

        cleaned[0] = '\0';
        pertama = 1;
        t = strtok(buf, " \t\n");
        while (t != NULL) {
            is_sw = 0;
            for (j = 0; sw[j] != NULL; j++) {
                if (strcmp(t, sw[j]) == 0) {
                    is_sw = 1;
                    break;
                }
            }
            if (!is_sw && strlen(t) > 1) {
                if (!pertama) {
                    strncat(cleaned, " ",
                        sizeof(cleaned)
                            - strlen(cleaned) - 1);
                }
                strncat(cleaned, t,
                    sizeof(cleaned)
                        - strlen(cleaned) - 1);
                pertama = 0;
            }
            t = strtok(NULL, " \t\n");
        }
        trim(cleaned);
    }

    if (cleaned[0] != '\0') {
        snprintf(topik_out, ukuran, "%s", cleaned);
        return 1;
    }

    return 0;
}

/* ========================================================================== *
 *                      PROSES UTAMA PERCAKAPAN                               *
 * ========================================================================== */

int proses_percakapan(
    sqlite3 *koneksi_db,
    const char *id_pengguna,
    const char *input_pengguna,
    char *respon_bot,
    size_t ukuran_respon)
{
    char input_normal[MAX_INPUT_USER];
    TokenKalimat daftar_token[MAX_TOKEN_KALIMAH];
    SimpulKetergantungan
        pohon_ketergantungan[MAX_TOKEN_KALIMAH];
    HasilAnalisisSpok hasil_spok;
    HasilEkstraksiTopik hasil_ekstraksi;
    char jenis_kalimat[64];
    char sumber_data[64];
    char topik_bersih[MAX_PANJANG_STRING];
    char topik_norm[MAX_PANJANG_STRING];
    char topik_stem[MAX_PANJANG_STRING];
    char topik_klausa[MAX_PANJANG_STRING];
    KandidatFuzzy kandidat[1];
    AjStmtCache cache;
    int jml_token, sesi_id;
    int adalah_pertanyaan_lanjutan;

    if (!koneksi_db || !id_pengguna || !input_pengguna
        || !respon_bot || ukuran_respon == 0) {
        return -1;
    }

    bersihkan_sesi_kadaluarsa();
    sesi_id = cari_atau_buat_sesi_percakapan(id_pengguna);

    if (aj_prepare_stmt_cache(koneksi_db, &cache) != 0) {
        snprintf(respon_bot, ukuran_respon,
            "Maaf, sistem belum siap.");
        return 0;
    }

    inisialisasi_fts5_kata(koneksi_db);

    aj_log("Input: %s\n", input_pengguna);
    aj_log("\n");

    if (normalisasi_frasa_input_dari_db(
        koneksi_db, &cache, input_pengguna,
        input_normal, sizeof(input_normal)) != 0) {
        snprintf(respon_bot, ukuran_respon,
            "Maaf, input tidak dapat diproses.");
        aj_finalize_stmt_cache(&cache);
        return 0;
    }

    aj_log("Normalisasi: %s\n", input_normal);

    adalah_pertanyaan_lanjutan = 0;
    if (strstr(input_normal, "apa lagi")
        || strstr(input_normal, "contoh lain")
        || strstr(input_normal, "siapa lagi")) {
        adalah_pertanyaan_lanjutan = 1;
    }

    if (adalah_pertanyaan_lanjutan) {
        tangani_pertanyaan_lanjutan(
            koneksi_db, &cache, input_normal,
            sesi_id, respon_bot, ukuran_respon);
        aj_finalize_stmt_cache(&cache);
        return 0;
    }

    /*
     * Coba deteksi pola. JIKA GAGAL, JANGAN LANGSUNG
     * FALLBACK - tetap lanjutkan dengan default
     * "pengetahuan_umum" agar bot tetap mencoba mencari
     * informasi dari knowledge base.
     */
    if (deteksi_jenis_dan_sumber_dari_db(
        koneksi_db, &cache, input_normal,
        jenis_kalimat, sizeof(jenis_kalimat),
        sumber_data, sizeof(sumber_data)) == 0) {
        aj_log("Pola terdeteksi: jenis='%s' "
            "sumber='%s'\n",
            jenis_kalimat, sumber_data);
    } else {
        aj_log("Pola tidak terdeteksi, "
            "menggunakan default.\n");
        snprintf(jenis_kalimat,
            sizeof(jenis_kalimat), "lain");
        snprintf(sumber_data,
            sizeof(sumber_data), "");
    }

    /*
     * PIPELINE KLAUSA PERTANYAAN BARU (v3.1):
     *
     * 1. Ekstrak SEMUA klausa pertanyaan dari kalimat
     *    panjang (bukan hanya klausa terakhir)
     * 2. Untuk setiap klausa, klasifikasi intent
     *    berdasarkan pola SPOK
     * 3. Gunakan intent pertama yang berhasil
     *    diklasifikasi untuk menentukan sumber_data
     *
     * Keunggulan vs klasifikasi_pertanyaan_akhir():
     * - Mendeteksi klausa pertanyaan di AWAL/TENGAH
     * - Mendeteksi pertanyaan IMPLISIT (tanpa ?)
     * - Lebih banyak pola SPOK dikenali
     */
    topik_klausa[0] = '\0';
    {
        char klausa_arr[MAX_KLAUSA][MAX_INPUT_USER];
        int jml_klausa;
        int ki;
        TipeIntent intent_klausa;
        char tdk[MAX_PANJANG_STRING];

        jml_klausa = ekstrak_semua_klausa_pertanyaan(
            input_normal, klausa_arr, MAX_KLAUSA);

        aj_log("Klausa ditemukan: %d\n", jml_klausa);
        for (ki = 0; ki < jml_klausa; ki++) {
            aj_log("  klausa[%d]: '%s'\n", ki,
                klausa_arr[ki]);
        }

        tdk[0] = '\0';
        intent_klausa = INTENT_LAIN;

        {
            /*
             * Evaluasi SEMUA klausa dan pilih intent
             * TERBAIK, bukan yang pertama ditemukan.
             *
             * Kriteria pemilihan:
             * 1. Topik harus tidak kosong setelah
             *    difilter
             * 2. Intent PENJELASAN diutamakan
             *    (paling spesifik)
             * 3. Topik yang lebih panjang diutamakan
             *    (lebih spesifik)
             * 4. Jika masih imbang, klausa terakhir
             *    diutamakan (biasanya pertanyaan utama)
             */
            int best_ki = -1;
            TipeIntent best_intent = INTENT_LAIN;
            char best_tdk[MAX_PANJANG_STRING];
            int best_tdk_len = 0;

            best_tdk[0] = '\0';

            for (ki = 0; ki < jml_klausa; ki++) {
                TipeIntent cur;
                char cur_tdk[MAX_PANJANG_STRING];
                int cur_len;

                cur_tdk[0] = '\0';
                cur = klasifikasi_intent_klausa(
                    klausa_arr[ki],
                    cur_tdk, sizeof(cur_tdk));

                if (cur != INTENT_LAIN) {
                    /*
                     * Filter kata lumpat dari topik.
                     * Contoh:
                     * "saya tidak mengerti komputer"
                     * -> "komputer"
                     */
                    bersihkan_topik(cur_tdk);
                    cur_len = (int)strlen(cur_tdk);

                    aj_log("  Intent klausa[%d]: "
                        "%d, topik_raw='%s' "
                        "topik_filtered='%s'\n",
                        ki, cur,
                        klausa_arr[ki], cur_tdk);

                    if (cur_len > 0) {
                        int is_better = 0;

                        if (best_ki < 0) {
                            is_better = 1;
                        } else if (cur
                            == INTENT_PENJELASAN
                            && best_intent
                            != INTENT_PENJELASAN) {
                            /*
                             * PENJELASAN lebih
                             * spesifik dari
                             * DEFINISI/ARTI.
                             */
                            is_better = 1;
                        } else if (cur_len
                            > best_tdk_len) {
                            /*
                             * Topik lebih panjang =
                             * lebih spesifik.
                             */
                            is_better = 1;
                        } else if (cur_len
                            == best_tdk_len
                            && cur
                            > best_intent) {
                            /*
                             * Panjang sama,
                             * intent lebih tinggi.
                             */
                            is_better = 1;
                        }

                        if (is_better) {
                            best_ki = ki;
                            best_intent = cur;
                            snprintf(best_tdk,
                                sizeof(best_tdk),
                                "%s", cur_tdk);
                            best_tdk_len = cur_len;
                        }
                    }
                }
            }

            if (best_ki >= 0) {
                intent_klausa = best_intent;
                snprintf(tdk, sizeof(tdk),
                    "%s", best_tdk);
            }
        }

        if (intent_klausa == INTENT_DEFINISI
            || intent_klausa == INTENT_ARTI) {
            snprintf(sumber_data,
                sizeof(sumber_data), "definisi");
        } else if (intent_klausa == INTENT_JENIS) {
            snprintf(sumber_data,
                sizeof(sumber_data), "jenis");
        } else if (intent_klausa
            == INTENT_PENJELASAN) {
            snprintf(sumber_data,
                sizeof(sumber_data),
                "penjelasan_lengkap");
        } else if (intent_klausa == INTENT_ALASAN) {
            snprintf(sumber_data,
                sizeof(sumber_data), "alasan");
        } else if (intent_klausa == INTENT_CARA) {
            snprintf(sumber_data,
                sizeof(sumber_data), "cara");
        } else if (intent_klausa
            == INTENT_PERBANDINGAN) {
            snprintf(sumber_data,
                sizeof(sumber_data), "perbandingan");
        }

        if (tdk[0] != '\0') {
            snprintf(topik_klausa,
                sizeof(topik_klausa), "%s", tdk);
            aj_log("Topik dari klausa: '%s'\n",
                topik_klausa);
        }
    }

    aj_log("sumber_data final: '%s'\n", sumber_data);

    snprintf(hasil_spok.jenis_kalimat,
        sizeof(hasil_spok.jenis_kalimat), "%s",
        jenis_kalimat);

    aj_log("sumber_data=%s, input_normal='%s'\n",
        sumber_data, input_normal);

    /*
     * Tokenisasi dengan stemming.
     */
    jml_token = tokenisasi_dengan_stem(
        koneksi_db, input_normal,
        daftar_token, MAX_TOKEN_KALIMAH);

    if (jml_token <= 0) {
        snprintf(respon_bot, ukuran_respon,
            "Maaf, kalimat kosong.");
        aj_finalize_stmt_cache(&cache);
        return 0;
    }
    aj_log("Tokenisasi+Stem: %d token\n", jml_token);

    if (isi_kategori_dengan_stem(
        koneksi_db, &cache,
        daftar_token, jml_token) != 0) {
        int i;
        for (i = 0; i < jml_token; i++) {
            daftar_token[i].pos =
                cari_kategori_fallback_dari_db(
                    koneksi_db, daftar_token[i].teks);
            if (daftar_token[i].lema[0] == '\0') {
                snprintf(daftar_token[i].lema,
                    sizeof(daftar_token[i].lema),
                    "%s", daftar_token[i].teks);
            }
        }
    }
    aj_log("POS tagging: selesai\n");

    /*
     * Dependency parsing diperluas.
     */
    bangun_depensi_diperluas(
        daftar_token, jml_token,
        pohon_ketergantungan, 0);

    analisis_spok_diperluas(
        daftar_token, jml_token,
        pohon_ketergantungan, &hasil_spok);

    ekstrak_topik_diperluas(
        koneksi_db, daftar_token, jml_token,
        pohon_ketergantungan, &hasil_ekstraksi);

    aj_log("SPOK: S='%s' P='%s' O='%s' K='%s'\n",
        hasil_spok.subjek, hasil_spok.predikat,
        hasil_spok.objek, hasil_spok.keterangan);

    snprintf(topik_bersih, sizeof(topik_bersih), "%s",
        strlen(hasil_ekstraksi.topik_utama) > 0
            ? hasil_ekstraksi.topik_utama
            : input_normal);

    /*
     * Override topik_bersih jika pipeline klausa
     * menemukan topik yang lebih spesifik dari
     * kalimat panjang.
     */
    if (topik_klausa[0] != '\0') {
        aj_log("Override topik dengan klausa: "
            "'%s' -> '%s'\n",
            topik_bersih, topik_klausa);
        snprintf(topik_bersih, sizeof(topik_bersih),
            "%s", topik_klausa);
    }

    /*
     * RESOLUSI REFERENSI KONTEKSTUAL (pra-fuzzy):
     *
     * Jika topik_bersih kosong atau hanya berisi
     * kata demonstratif ("itu", "hal itu", dll),
     * langsung resolve sebelum fuzzy.
     */
    if (adalah_referensi_kosong(topik_bersih)) {
        char topik_ctx[MAX_PANJANG_STRING];

        aj_log("Topik kosong/referensi: '%s', "
            "mencari dari konteks...\n",
            topik_bersih);

        topik_ctx[0] = '\0';
        if (cari_topik_dari_konteks(
            input_normal, topik_ctx,
            sizeof(topik_ctx)) != 0) {
            snprintf(topik_bersih,
                sizeof(topik_bersih),
                "%s", topik_ctx);
            aj_log("Topik dari konteks: '%s'\n",
                topik_bersih);
        } else {
            const char *topik_sesi =
                sesi_aktif[sesi_id].topik_utama;
            if (topik_sesi && topik_sesi[0]) {
                snprintf(topik_bersih,
                    sizeof(topik_bersih),
                    "%s", topik_sesi);
                aj_log("Topik dari sesi: '%s'\n",
                    topik_bersih);
            }
        }
    }

    /*
     * Pencarian fuzzy optimal.
     * Jika fuzzy gagal DAN input mengandung pola
     * referensi kontekstual, coba resolve dari
     * kalimat atau sesi sebelumnya.
     */
    {
        int fuzzy_ok = 0;
        double fuzzy_skor = 0.0;

        if (cari_topik_optimal(
            koneksi_db, &cache, topik_bersih,
            topik_norm, (int)sizeof(topik_norm),
            kandidat, 1) == 0) {
            fuzzy_ok = 1;
            fuzzy_skor = kandidat[0].skor;
            aj_log("Topik norm: '%s' "
                "(skor=%.2f)\n",
                topik_norm, fuzzy_skor);
        }

        /*
         * Jika fuzzy gagal atau skor sangat rendah
         * dan input mengandung pola referensi,
         * coba resolve ulang dari konteks.
         */
        if ((!fuzzy_ok || fuzzy_skor < 0.7)
            && mengandung_referensi(input_normal)) {
            char topik_ctx[MAX_PANJANG_STRING];

            aj_log("Fuzzy gagal/skor rendah "
                "(%.2f), coba resolusi...\n",
                fuzzy_skor);

            topik_ctx[0] = '\0';
            if (cari_topik_dari_konteks(
                input_normal, topik_ctx,
                sizeof(topik_ctx)) != 0) {
                /*
                 * Retry fuzzy dengan topik
                 * dari konteks kalimat.
                 */
                if (cari_topik_optimal(
                    koneksi_db, &cache,
                    topik_ctx, topik_norm,
                    (int)sizeof(topik_norm),
                    kandidat, 1) == 0
                    && kandidat[0].skor >= 0.7) {
                    snprintf(topik_bersih,
                        sizeof(topik_bersih),
                        "%s", topik_ctx);
                    fuzzy_ok = 1;
                    aj_log("Resolve konteks: "
                        "'%s' -> '%s' "
                        "(skor=%.2f)\n",
                        topik_ctx, topik_norm,
                        kandidat[0].skor);
                }
            }

            /*
             * Jika masih gagal, coba topik sesi.
             */
            if (!fuzzy_ok) {
                const char *topik_sesi =
                    sesi_aktif[sesi_id]
                        .topik_utama;
                if (topik_sesi
                    && topik_sesi[0]) {
                    if (cari_topik_optimal(
                        koneksi_db, &cache,
                        topik_sesi, topik_norm,
                        (int)sizeof(
                            topik_norm),
                        kandidat, 1) == 0
                        && kandidat[0].skor
                            >= 0.7) {
                        snprintf(
                            topik_bersih,
                            sizeof(topik_bersih),
                            "%s", topik_sesi);
                        fuzzy_ok = 1;
                        aj_log("Resolve sesi: "
                            "'%s' -> '%s'\n",
                            topik_sesi,
                            topik_norm);
                    }
                }
            }
        }

        if (!fuzzy_ok) {
            snprintf(topik_norm,
                sizeof(topik_norm),
                "%s", topik_bersih);
            aj_log("Topik norm: '%s' (eksak)\n",
                topik_norm);
        }
    }

    topik_stem[0] = '\0';
    stem_kata(koneksi_db, topik_norm, topik_stem,
        (int)sizeof(topik_stem));

    snprintf(hasil_spok.topik_utama,
        sizeof(hasil_spok.topik_utama), "%s",
        topik_norm);

    aj_log("Topik: bersih='%s' norm='%s' stem='%s'",
        topik_bersih, topik_norm,
        topik_stem[0] ? topik_stem : "(sama)");
    aj_log(" sub='%s' qty='%s'\n",
        hasil_ekstraksi.sub_topik,
        hasil_ekstraksi.kuantitas);

    /* ============================================================ *
     *  DISPATCH: Handler untuk semua tipe sumber_data
     *  Setiap handler menerima HasilAnalisisSpok untuk
     *  menghasilkan respon natural berbasis SPOK.
     * ============================================================ */

    if (strcmp(sumber_data, "sapaan") == 0) {
        tangani_sapaan(koneksi_db, &cache,
            input_normal, respon_bot,
            ukuran_respon);
    } else if (strcmp(sumber_data, "definisi") == 0) {
        tangani_arti_singkat(koneksi_db, &cache,
            topik_norm, &hasil_spok,
            respon_bot, ukuran_respon);
    } else if (strcmp(sumber_data, "jenis") == 0) {
        tangani_jenis_benda(koneksi_db, &cache,
            topik_norm, &hasil_spok,
            respon_bot, ukuran_respon);
    } else if (strcmp(sumber_data,
            "penjelasan_lengkap") == 0
        || strcmp(sumber_data,
            "gabungan_spok") == 0) {
        tangani_permintaan_penjelasan(
            koneksi_db, &cache, topik_norm,
            &hasil_spok, respon_bot,
            ukuran_respon);
    } else if (strcmp(sumber_data, "sebab") == 0
        || strcmp(sumber_data, "dampak") == 0) {
        tangani_pertanyaan_sebab(
            koneksi_db, &cache, &hasil_spok,
            respon_bot, ukuran_respon);
    } else if (strcmp(sumber_data, "langkah") == 0) {
        if (strlen(hasil_ekstraksi.sub_topik) > 0) {
            snprintf(
                hasil_ekstraksi.sub_topik,
                sizeof(hasil_ekstraksi.sub_topik),
                "%s", "langkah");
        }
        tangani_permintaan_daftar(
            koneksi_db, &cache,
            &hasil_ekstraksi, &hasil_spok,
            respon_bot, ukuran_respon);
    } else if (strcmp(sumber_data, "fakta") == 0
        || strcmp(sumber_data, "sejarah") == 0
        || strcmp(sumber_data,
            "pengetahuan_umum") == 0) {
        tangani_permintaan_penjelasan(
            koneksi_db, &cache, topik_norm,
            &hasil_spok, respon_bot,
            ukuran_respon);
    } else if (strcmp(sumber_data, "manfaat") == 0
        || strcmp(sumber_data, "fungsi") == 0) {
        if (strlen(hasil_ekstraksi.sub_topik) > 0
            && strcmp(hasil_ekstraksi.sub_topik,
                "manfaat") != 0) {
            snprintf(
                hasil_ekstraksi.sub_topik,
                sizeof(hasil_ekstraksi.sub_topik),
                "%s", "manfaat");
        } else if (strlen(
                hasil_ekstraksi.sub_topik)
                == 0) {
            snprintf(
                hasil_ekstraksi.sub_topik,
                sizeof(hasil_ekstraksi.sub_topik),
                "%s", "manfaat");
        }
        tangani_permintaan_daftar(
            koneksi_db, &cache,
            &hasil_ekstraksi, &hasil_spok,
            respon_bot, ukuran_respon);
    } else if (strcmp(sumber_data, "perbedaan") == 0
        || strcmp(sumber_data, "analogi") == 0
        || strcmp(sumber_data, "ciri") == 0) {
        tangani_permintaan_penjelasan(
            koneksi_db, &cache, topik_norm,
            &hasil_spok, respon_bot,
            ukuran_respon);
    } else {
        tangani_arti_singkat(koneksi_db, &cache,
            topik_norm, &hasil_spok,
            respon_bot, ukuran_respon);
    }

    aj_log("Respon siap.\n");

    perbarui_sesi_percakapan(
        sesi_id, topik_norm, sumber_data,
        "idle", input_normal);
    aj_finalize_stmt_cache(&cache);
    return 0;
}
