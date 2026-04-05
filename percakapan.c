/* ========================================================================== *
 * AJUDAN 3.1 - Modul Percakapan (percakapan.c)                                *
 * -------------------------------------------------------------------------- *
 * Berisi handler respon bot berbasis SPOK untuk                   *
 * Setiap handler menghasilkan kalimat natural langsung tanpa                  *
 * lapisan template tambahan. Data SPOK dari pipeline utama                    *
 * digunakan untuk memahami konteks dan mempersonalisasi jawaban.              *
 * -------------------------------------------------------------------------- *
 * C89, POSIX, SQLITE3                                                         *
 * ========================================================================== */

#include "ajudan.h"

/* Deklarasi maju untuk fungsi internal yang dipanggil
 * sebelum didefinisikan.
 */
static void tangani_permintaan_penjelasan_analitik(
    sqlite3 *db, AjStmtCache *cache,
    const char *topik, HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output);

/* ========================================================================== *
 *                          UTILITAS STRING                                   *
 * ========================================================================== */

/*
 * kapitalisasi_awal - Ubah huruf pertama string menjadi huruf
 * besar, sisanya huruf kecil.
 */
void kapitalisasi_awal(char *s)
{
    int i, len;

    if (!s || !s[0]) return;

    len = (int)strlen(s);
    s[0] = (char)toupper((unsigned char)s[0]);
    for (i = 1; i < len; i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
}

/* ========================================================================== *
 *                     DETEKSI & RESPONSE SAPAAN WAKTU (static)                *
 * ========================================================================== */

/*
 * deteksi_sapaan_waktu - Cek apakah input mengandung sapaan
 * berbasis waktu seperti "selamat pagi", "selamat siang", dll.
 * Mengembalikan pointer ke string waktu atau NULL.
 */
static const char *deteksi_sapaan_waktu(const char *input)
{
    char lo[MAX_INPUT_USER];
    int i;

    static const char *daftar_waktu[] = {
        "selamat pagi",
        "selamat siang",
        "selamat sore",
        "selamat malam",
        "selamat subuh",
        "selamat tengah hari",
        "selamat petang",
        NULL
    };

    if (!input || !input[0]) return NULL;

    snprintf(lo, sizeof(lo), "%s", input);
    to_lower_case(lo);
    trim(lo);

    for (i = 0; daftar_waktu[i] != NULL; i++) {
        if (strstr(lo, daftar_waktu[i]) != NULL) {
            return daftar_waktu[i];
        }
    }
    return NULL;
}

/*
 * buat_respon_sapaan_waktu - Buat respon sapaan kontekstual
 * berdasarkan waktu. Menggunakan kapitalisasi_awal() untuk
 * format natural "Selamat malam" (bukan "SELAMAT MALAM").
 * Mengembalikan 1 jika sapaan waktu terdeteksi, 0 jika bukan.
 */
static int buat_respon_sapaan_waktu(
    const char *waktu,
    const char *nama,
    char *output,
    size_t ukuran_output)
{
    char salam[64];
    char nama_kap[64];

    if (!waktu || !output || ukuran_output == 0)
        return 0;

    /* Salin dan kapitalisasi awal: "selamat malam" */
    snprintf(salam, sizeof(salam), "%s", waktu);
    kapitalisasi_awal(salam);

    nama_kap[0] = '\0';
    if (nama && nama[0]) {
        snprintf(nama_kap, sizeof(nama_kap), "%s", nama);
        kapitalisasi_awal(nama_kap);
    }

    if (nama_kap[0]) {
        snprintf(output, ukuran_output,
            "%s, %s! Ada yang bisa saya bantu?",
            salam, nama_kap);
    } else {
        snprintf(output, ukuran_output,
            "%s! Ada yang bisa saya bantu?",
            salam);
    }
    return 1;
}

/* ========================================================================== *
 *                  HANDLER SAPAAN (PUBLIC)                                    *
 * ========================================================================== */

/*
 * tangani_sapaan - Menangani input sapaan pengguna.
 *
 * Prioritas 1: Sapaan berbasis waktu ("selamat pagi", dll)
 *   menggunakan kapitalisasi_awal() untuk format natural.
 * Prioritas 2: Sapaan acak dari jawaban_bawaan (tipe=sapaan).
 * Fallback: "Halo! Saya AJUDAN, asisten virtual Anda."
 */
void tangani_sapaan(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *input,
    char *output,
    size_t ukuran_output)
{
    const char *waktu;
    const char *val;
    int rc;

    if (!db || !cache || !output || ukuran_output == 0)
        return;

    /* Prioritas 1: sapaan berbasis waktu */
    waktu = deteksi_sapaan_waktu(input);
    if (waktu) {
        buat_respon_sapaan_waktu(
            waktu, NULL, output, ukuran_output);
        return;
    }

    /* Prioritas 2: sapaan acak dari database */
    rc = sqlite3_reset(cache->stmt_sapaan_acak);
    if (rc != SQLITE_OK) {
        snprintf(output, ukuran_output,
            "Halo! Saya AJUDAN, "
            "asisten virtual Anda.");
        return;
    }

    if (sqlite3_step(cache->stmt_sapaan_acak)
        == SQLITE_ROW) {
        val = (const char *)sqlite3_column_text(
            cache->stmt_sapaan_acak, 0);
        if (val && val[0]) {
            snprintf(output, ukuran_output,
                "%s", val);
            sqlite3_reset(cache->stmt_sapaan_acak);
            return;
        }
    }

    sqlite3_reset(cache->stmt_sapaan_acak);
    snprintf(output, ukuran_output,
        "Halo! Saya AJUDAN, asisten virtual Anda.");
}

/* ========================================================================== *
 *              HANDLER ARTI SINGKAT (PUBLIC)                                  *
 * ========================================================================== */

/*
 * tangani_arti_singkat - Mengembalikan definisi/arti singkat.
 * Hanya ringkasan (tanpa penjelasan panjang).
 * Digunakan untuk pertanyaan "apa itu X", "X itu apa", dll.
 *
 * Integrasi SPOK: Jika spok->keterangan berisi aspek
 * spesifik, informasi tersebut digabungkan ke jawaban.
 */
void tangani_arti_singkat(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *topik,
    HasilAnalisisSpok *spok,
    char *output,
    size_t ukuran_output)
{
    char judul_buf[MAX_RESPONS];
    char ringkasan_buf[MAX_RESPONS];
    char penjelasan_buf[MAX_RESPONS];
    char saran_buf[MAX_RESPONS];
    char topik_kap[MAX_PANJANG_STRING];
    int pakai_keterangan;

    if (!db || !cache || !topik || !output
        || ukuran_output == 0) {
        return;
    }

    /* Siapkan topik dengan huruf kapital */
    snprintf(topik_kap, sizeof(topik_kap), "%s", topik);
    kapitalisasi_awal(topik_kap);

    pakai_keterangan = (spok != NULL
        && spok->keterangan[0] != '\0');

    /* Coba ambil dari pengetahuan_umum */
    if (ambil_pengetahuan_umum(db, cache, topik,
        judul_buf, sizeof(judul_buf),
        ringkasan_buf, sizeof(ringkasan_buf),
        penjelasan_buf, sizeof(penjelasan_buf),
        saran_buf, sizeof(saran_buf)) == 0) {

        if (ringkasan_buf[0] != '\0') {
            if (pakai_keterangan) {
                snprintf(output, ukuran_output,
                    "%s adalah %s %s.",
                    topik_kap, ringkasan_buf,
                    spok->keterangan);
            } else {
                snprintf(output, ukuran_output,
                    "%s adalah %s.",
                    topik_kap, ringkasan_buf);
            }
        } else if (judul_buf[0] != '\0') {
            if (pakai_keterangan) {
                snprintf(output, ukuran_output,
                    "%s adalah %s %s.",
                    topik_kap, judul_buf,
                    spok->keterangan);
            } else {
                snprintf(output, ukuran_output,
                    "%s adalah %s.",
                    topik_kap, judul_buf);
            }
        } else {
            if (ambil_respon_default_acak(db, cache,
                output, ukuran_output) != 0) {
                snprintf(output, ukuran_output,
                    "Maaf, saya belum tahu tentang "
                    "'%s'.", topik);
            }
        }
        return;
    }

    /* Coba ambil dari arti_kata */
    if (ambil_arti_kata(db, cache, topik,
        ringkasan_buf, sizeof(ringkasan_buf)) == 0) {
        if (pakai_keterangan) {
            snprintf(output, ukuran_output,
                "%s artinya %s %s.",
                topik_kap, ringkasan_buf,
                spok->keterangan);
        } else {
            snprintf(output, ukuran_output,
                "%s artinya %s.",
                topik_kap, ringkasan_buf);
        }
        return;
    }

    /* Fallback */
    if (ambil_respon_default_acak(db, cache,
        output, ukuran_output) != 0) {
        snprintf(output, ukuran_output,
            "Maaf, saya belum tahu tentang '%s'.",
            topik);
    }
}

/* ========================================================================== *
 *          HANDLER PERMINTAAN PENJELASAN (PUBLIC)                             *
 * ========================================================================== */

/*
 * tangani_permintaan_penjelasan - Menangani permintaan
 * penjelasan lengkap tentang suatu topik.
 *
 * Integrasi SPOK:
 * - spok->subjek "kamu"/"anda" = user tanya tentang bot
 * - spok->keterangan "singkat"/"sederhana" = ringkas saja
 * - spok->keterangan "lebih lanjut"/"detail" = penuh
 *
 * Format: "{Topik} adalah {ringkasan}\n\n{penjelasan}"
 */
void tangani_permintaan_penjelasan(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *topik,
    HasilAnalisisSpok *spok,
    char *output,
    size_t ukuran_output)
{
    char judul[MAX_RESPONS];
    char ringkasan[MAX_RESPONS];
    char penjelasan[MAX_RESPONS];
    char saran[MAX_RESPONS];
    char topik_kap[MAX_PANJANG_STRING];
    char sub_lo[MAX_PANJANG_STRING];
    int tampil_detail;

    if (!db || !cache || !topik || !output
        || ukuran_output == 0) return;

    /* Siapkan topik dengan huruf kapital */
    snprintf(topik_kap, sizeof(topik_kap), "%s", topik);
    kapitalisasi_awal(topik_kap);

    /*
     * SPOK: Tentukan level detail berdasarkan
     * kata kunci di keterangan.
     */
    tampil_detail = 1;
    if (spok && spok->keterangan[0] != '\0') {
        if (strstr(spok->keterangan, "singkat") != NULL
            || strstr(spok->keterangan, "sederhana")
                != NULL
            || strstr(spok->keterangan, "mudah")
                != NULL) {
            tampil_detail = 0;
        }
    }

    if (ambil_pengetahuan_umum(db, cache, topik,
        judul, sizeof(judul),
        ringkasan, sizeof(ringkasan),
        penjelasan, sizeof(penjelasan),
        saran, sizeof(saran)) == 0) {

        /*
         * SPOK: Jika user menanyakan tentang bot,
         * tambahkan intro kontekstual.
         */
        if (spok && spok->subjek[0] != '\0') {
            snprintf(sub_lo, sizeof(sub_lo),
                "%s", spok->subjek);
            to_lower_case(sub_lo);
            trim(sub_lo);

            if (strcmp(sub_lo, "kamu") == 0
                || strcmp(sub_lo, "anda") == 0) {
                if (tampil_detail
                    && penjelasan[0] != '\0'
                    && saran[0] != '\0') {
                    snprintf(output, ukuran_output,
                        "Sebagai AJUDAN, saya "
                        "bisa menjelaskan bahwa "
                        "%s adalah %s\n\n%s\n\n%s",
                        topik_kap, ringkasan,
                        penjelasan, saran);
                } else if (tampil_detail
                    && penjelasan[0] != '\0') {
                    snprintf(output, ukuran_output,
                        "Sebagai AJUDAN, saya "
                        "bisa menjelaskan bahwa "
                        "%s adalah %s\n\n%s",
                        topik_kap, ringkasan,
                        penjelasan);
                } else {
                    snprintf(output, ukuran_output,
                        "Sebagai AJUDAN, saya "
                        "bisa menjelaskan bahwa "
                        "%s adalah %s.",
                        topik_kap, ringkasan);
                }
                return;
            }
        }

        /* Format normal tanpa konteks bot */
        if (tampil_detail
            && penjelasan[0] != '\0'
            && saran[0] != '\0') {
            snprintf(output, ukuran_output,
                "%s adalah %s\n\n%s\n\n%s",
                topik_kap, ringkasan,
                penjelasan, saran);
        } else if (tampil_detail
            && penjelasan[0] != '\0') {
            snprintf(output, ukuran_output,
                "%s adalah %s\n\n%s",
                topik_kap, ringkasan,
                penjelasan);
        } else {
            snprintf(output, ukuran_output,
                "%s adalah %s.",
                topik_kap, ringkasan);
        }
    } else {
        aj_log("Fakta untuk '%s' tidak ditemukan. "
            "Mencoba analisis.\n", topik);
        tangani_permintaan_penjelasan_analitik(
            db, cache, topik, spok,
            output, ukuran_output);
    }
}

/* ========================================================================== *
 *         HANDLER PERMINTAAN ARTI KATA (PUBLIC)                               *
 * ========================================================================== */

/*
 * tangani_permintaan_arti_kata - Menangani permintaan
 * definisi kata dari kamus.
 *
 * Integrasi SPOK: spok->objek digunakan sebagai topik
 * alternatif jika topik utama kosong.
 *
 * Format: "{Topik} artinya {definisi}."
 */
void tangani_permintaan_arti_kata(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *topik,
    HasilAnalisisSpok *spok,
    char *output,
    size_t ukuran_output)
{
    char definisi[MAX_RESPONS];
    char topik_kap[MAX_PANJANG_STRING];
    const char *topik_aktif;

    if (!db || !cache || !output
        || ukuran_output == 0) return;

    /* SPOK: Gunakan objek sebagai alternatif */
    topik_aktif = topik;
    if ((!topik || !topik[0]) && spok
        && spok->objek[0] != '\0') {
        topik_aktif = spok->objek;
    }
    if (!topik_aktif || !topik_aktif[0]) return;

    snprintf(topik_kap, sizeof(topik_kap),
        "%s", topik_aktif);
    kapitalisasi_awal(topik_kap);

    if (ambil_arti_kata(db, cache, topik_aktif,
        definisi, sizeof(definisi)) == 0) {
        snprintf(output, ukuran_output,
            "%s artinya %s.",
            topik_kap, definisi);
    } else {
        if (ambil_hubungan_semantik(db, cache,
            topik_aktif, definisi,
            sizeof(definisi)) == 0) {
            snprintf(output, ukuran_output,
                "Terkait '%s': %s.",
                topik_kap, definisi);
        } else {
            if (ambil_respon_default_acak(db, cache,
                output, ukuran_output) != 0) {
                snprintf(output, ukuran_output,
                    "Maaf, saya tidak menemukan "
                    "informasi mengenai '%s'.",
                    topik_aktif);
            }
        }
    }
}

/* ========================================================================== *
 *      HANDLER PERMINTAAN PENJELASAN ANALITIK (PUBLIC)                       *
 * ========================================================================== */

/*
 * tangani_permintaan_penjelasan_analitik - Menangani
 * penjelasan topik berdasarkan analisis relasi semantik.
 *
 * Integrasi SPOK:
 * - spok->objek digunakan sebagai fallback topik
 * - spok->subjek untuk intro natural:
 *   "Berdasarkan pemahaman saya tentang {topik}..."
 */
void tangani_permintaan_penjelasan_analitik(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *topik,
    HasilAnalisisSpok *spok,
    char *output,
    size_t ukuran_output)
{
    char analisis_buffer[MAX_RESPONS];
    const char *topik_aktif;
    char topik_kap[MAX_PANJANG_STRING];
    char sub_lo[MAX_PANJANG_STRING];

    if (!db || !cache || !output
        || ukuran_output == 0) return;

    /* SPOK: Gunakan objek sebagai alternatif */
    topik_aktif = topik;
    if (spok && spok->objek[0] != '\0') {
        topik_aktif = spok->objek;
    }
    if (!topik_aktif || !topik_aktif[0]) return;

    snprintf(topik_kap, sizeof(topik_kap),
        "%s", topik_aktif);
    kapitalisasi_awal(topik_kap);

    if (ambil_hubungan_semantik(db, cache, topik_aktif,
        analisis_buffer,
        sizeof(analisis_buffer)) == 0) {

        /* SPOK: Intro kontekstual natural */
        if (spok && spok->subjek[0] != '\0') {
            snprintf(sub_lo, sizeof(sub_lo),
                "%s", spok->subjek);
            to_lower_case(sub_lo);
            trim(sub_lo);

            if (strcmp(sub_lo, "kamu") == 0
                || strcmp(sub_lo, "anda") == 0) {
                snprintf(output, ukuran_output,
                    "Berdasarkan pemahaman saya, "
                    "%s terkait dengan %s.",
                    topik_kap, analisis_buffer);
                return;
            }
        }

        snprintf(output, ukuran_output,
            "Berdasarkan pemahaman saya tentang "
            "%s, %s terkait dengan %s.",
            topik_kap, topik_kap, analisis_buffer);
    } else if (ambil_arti_kata(db, cache, topik_aktif,
        analisis_buffer,
        sizeof(analisis_buffer)) == 0) {
        snprintf(output, ukuran_output,
            "%s didefinisikan sebagai '%s'.",
            topik_kap, analisis_buffer);
    } else {
        if (ambil_respon_default_acak(db, cache,
            output, ukuran_output) != 0) {
            snprintf(output, ukuran_output,
                "Maaf, saya tidak memiliki cukup "
                "data untuk menjelaskan '%s'.",
                topik_aktif);
        }
    }
}

/* ========================================================================== *
 *        HANDLER PERMINTAAN DAFTAR (PUBLIC)                                  *
 * ========================================================================== */

/*
 * tangani_permintaan_daftar - Menangani permintaan daftar
 * item terkait topik (fakta, jenis, langkah, dll).
 *
 * Integrasi SPOK: Parameter spok tersedia untuk
 * penggunaan di masa depan.
 */
void tangani_permintaan_daftar(
    sqlite3 *db,
    AjStmtCache *cache,
    HasilEkstraksiTopik *ekstraksi,
    HasilAnalisisSpok *spok,
    char *output,
    size_t ukuran_output)
{
    int dengan_detail;

    (void)spok;

    if (!db || !cache || !ekstraksi || !output
        || ukuran_output == 0) return;

    dengan_detail = (strlen(ekstraksi->kuantitas) == 0)
        ? 1 : 0;

    if (format_respons_daftar(db, cache,
        ekstraksi->topik_utama,
        (strlen(ekstraksi->sub_topik) > 0)
            ? ekstraksi->sub_topik : "fakta",
        dengan_detail, output,
        ukuran_output) != 0) {
        snprintf(output, ukuran_output,
            "Maaf, daftar '%s%s%s' belum "
            "tersedia.",
            (strlen(ekstraksi->sub_topik) > 0)
                ? ekstraksi->sub_topik : "",
            (strlen(ekstraksi->sub_topik) > 0)
                ? " dari " : "",
            ekstraksi->topik_utama);
    }
}

/* ========================================================================== *
 *         HANDLER PERTANYAAN SEBAB (PUBLIC)                                  *
 * ========================================================================== */

/*
 * tangani_pertanyaan_sebab - Menangani pertanyaan
 * tentang sebab/alasan ("mengapa", "kenapa", dll).
 *
 * Integrasi SPOK:
 * - spok->objek digunakan sebagai topik utama
 * - Format natural: "Terkait {topik}, ..."
 * - Gunakan kerangka_respon dari pola_kalimat
 */
void tangani_pertanyaan_sebab(
    sqlite3 *db,
    AjStmtCache *cache,
    HasilAnalisisSpok *spok,
    char *output,
    size_t ukuran_output)
{
    EntitasTerkait entitas[10];
    int jml_entitas, i;
    char kerangka[256];
    char buffer[MAX_RESPONS];
    char topik_kap[MAX_PANJANG_STRING];
    int wrote;
    const char *topik_aktif;
    char *pos_topik, *pos_contoh;
    char tmp[MAX_RESPONS];
    char left_buf[MAX_RESPONS];
    char right_buf[MAX_RESPONS];
    size_t seg_len;

    if (!db || !cache || !spok || !output
        || ukuran_output == 0) return;

    /* SPOK: Gunakan objek sebagai topik utama */
    topik_aktif = spok->objek;
    if (!topik_aktif || !topik_aktif[0]) {
        topik_aktif = spok->topik_utama;
    }
    if (!topik_aktif || !topik_aktif[0]) return;

    snprintf(topik_kap, sizeof(topik_kap),
        "%s", topik_aktif);
    kapitalisasi_awal(topik_kap);

    if (pilih_kerangka_respon(db, "sebab",
        kerangka, sizeof(kerangka)) != 0) {
        snprintf(kerangka, sizeof(kerangka),
            "Terkait {topik}, "
            "alasan utama adalah karena "
            "{contoh}.");
    }

    jml_entitas = cari_entitas_terkait_dengan_filter(
        db, topik_aktif, "verba,nomina", "", "umum",
        entitas, 10);

    buffer[0] = '\0';
    wrote = 0;
    if (jml_entitas > 0) {
        for (i = 0; i < jml_entitas && i < 3; i++) {
            if (i > 0) {
                strncat(buffer, ", ",
                    sizeof(buffer)
                    - strlen(buffer) - 1);
            }
            strncat(buffer, entitas[i].kata,
                sizeof(buffer)
                - strlen(buffer) - 1);
            wrote++;
        }
    }

    /* Ganti placeholder {topik} */
    snprintf(tmp, sizeof(tmp), "%s", kerangka);
    pos_topik = strstr(tmp, "{topik}");
    if (pos_topik) {
        seg_len = (size_t)(pos_topik - tmp);
        snprintf(left_buf, sizeof(left_buf),
            "%.*s", (int)seg_len, tmp);
        snprintf(right_buf, sizeof(right_buf),
            "%s", pos_topik + 7);
        snprintf(tmp, sizeof(tmp), "%s%s%s",
            left_buf, topik_kap, right_buf);
    }

    /* Ganti placeholder {contoh} */
    pos_contoh = strstr(tmp, "{contoh}");
    if (pos_contoh) {
        seg_len = (size_t)(pos_contoh - tmp);
        snprintf(left_buf, sizeof(left_buf),
            "%.*s", (int)seg_len, tmp);
        snprintf(right_buf, sizeof(right_buf),
            "%s", pos_contoh + 8);
        snprintf(tmp, sizeof(tmp), "%s%s%s",
            left_buf,
            (wrote > 0) ? buffer : topik_kap,
            right_buf);
    }

    snprintf(output, ukuran_output, "%s", tmp);
}

/* ========================================================================== *
 *         HANDLER JENIS BENDA (PUBLIC)                                       *
 * ========================================================================== */

/*
 * tangani_jenis_benda - Menangani pertanyaan tentang JENIS
 * suatu benda atau konsep.
 *
 * Integrasi SPOK: Parameter spok tersedia untuk
 * konteks tambahan di masa depan.
 *
 * Sumber data (berurutan):
 * 1. pengetahuan_bertingkat topik 'jenis'
 * 2. Relasi semantik jenis_dari
 * 3. Stem + ulangi relasi semantik
 * 4. Fallback
 *
 * Format: "Beberapa jenis {topik} yang saya ketahui:"
 */
void tangani_jenis_benda(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *topik,
    HasilAnalisisSpok *spok,
    char *output,
    size_t ukuran_output)
{
    int rc, ada_hasil;
    const char *poin_val;
    const char *penjelasan_val;
    const char *hub_relasi;
    char buffer[MAX_RESPONS];
    char kata_stem[MAX_PANJANG_TOKEN];

    (void)spok;

    if (!db || !cache || !topik || !output
        || ukuran_output == 0) {
        return;
    }

    /* --- 1. Coba pengetahuan_bertingkat --- */
    rc = sqlite3_reset(cache->stmt_pen_bertingkat_list);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(
            cache->stmt_pen_bertingkat_list,
            1, topik, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(
            cache->stmt_pen_bertingkat_list,
            2, "jenis", -1, SQLITE_TRANSIENT);

        buffer[0] = '\0';
        ada_hasil = 0;

        while (sqlite3_step(
            cache->stmt_pen_bertingkat_list)
            == SQLITE_ROW) {
            poin_val = (const char *)
                sqlite3_column_text(
                cache->stmt_pen_bertingkat_list, 0);
            penjelasan_val = (const char *)
                sqlite3_column_text(
                cache->stmt_pen_bertingkat_list, 1);

            if (poin_val && poin_val[0]) {
                if (ada_hasil) {
                    strncat(buffer, "\n",
                        sizeof(buffer)
                        - strlen(buffer) - 1);
                }
                strncat(buffer, "- ",
                    sizeof(buffer)
                    - strlen(buffer) - 1);
                strncat(buffer, poin_val,
                    sizeof(buffer)
                    - strlen(buffer) - 1);
                if (penjelasan_val
                    && penjelasan_val[0]) {
                    strncat(buffer, ": ",
                        sizeof(buffer)
                        - strlen(buffer) - 1);
                    strncat(buffer, penjelasan_val,
                        sizeof(buffer)
                        - strlen(buffer) - 1);
                }
                ada_hasil = 1;
            }
        }

        sqlite3_reset(
            cache->stmt_pen_bertingkat_list);

        if (ada_hasil) {
            snprintf(output, ukuran_output,
                "Beberapa jenis %s yang saya "
                "ketahui:\n%s",
                topik, buffer);
            return;
        }
    }

    /* --- 2. Coba relasi semantik jenis_dari --- */
    rc = sqlite3_reset(cache->stmt_rel_semantik);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(
            cache->stmt_rel_semantik,
            1, topik, -1, SQLITE_TRANSIENT);

        buffer[0] = '\0';
        ada_hasil = 0;

        while (sqlite3_step(
            cache->stmt_rel_semantik) == SQLITE_ROW) {
            poin_val = (const char *)
                sqlite3_column_text(
                cache->stmt_rel_semantik, 0);
            hub_relasi = (const char *)
                sqlite3_column_text(
                cache->stmt_rel_semantik, 1);

            if (!poin_val || !poin_val[0]) continue;

            if (hub_relasi
                && strstr(hub_relasi, "jenis")
                    != NULL) {
                if (ada_hasil) {
                    strncat(buffer, ", ",
                        sizeof(buffer)
                        - strlen(buffer) - 1);
                }
                strncat(buffer, poin_val,
                    sizeof(buffer)
                    - strlen(buffer) - 1);
                ada_hasil = 1;
            }
        }

        sqlite3_reset(cache->stmt_rel_semantik);

        if (ada_hasil) {
            snprintf(output, ukuran_output,
                "Beberapa jenis %s yang saya "
                "ketahui: %s",
                topik, buffer);
            return;
        }
    }

    /* --- 3. Stem, lalu ulangi semantik --- */
    if (stem_kata(db, topik, kata_stem,
        (int)sizeof(kata_stem)) == 0
        && ajudan_strcasecmp(kata_stem, topik) != 0) {
        rc = sqlite3_reset(cache->stmt_rel_semantik);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(
                cache->stmt_rel_semantik,
                1, kata_stem, -1, SQLITE_TRANSIENT);

            buffer[0] = '\0';
            ada_hasil = 0;

            while (sqlite3_step(
                cache->stmt_rel_semantik)
                == SQLITE_ROW) {
                poin_val = (const char *)
                    sqlite3_column_text(
                    cache->stmt_rel_semantik, 0);
                hub_relasi = (const char *)
                    sqlite3_column_text(
                    cache->stmt_rel_semantik, 1);

                if (!poin_val || !poin_val[0])
                    continue;

                if (hub_relasi
                    && strstr(hub_relasi, "jenis")
                        != NULL) {
                    if (ada_hasil) {
                        strncat(buffer, ", ",
                            sizeof(buffer)
                            - strlen(buffer) - 1);
                    }
                    strncat(buffer, poin_val,
                        sizeof(buffer)
                        - strlen(buffer) - 1);
                    ada_hasil = 1;
                }
            }

            sqlite3_reset(
                cache->stmt_rel_semantik);

            if (ada_hasil) {
                snprintf(output, ukuran_output,
                    "Beberapa jenis %s yang saya "
                    "ketahui: %s",
                    topik, buffer);
                return;
            }
        }
    }

    /* --- 4. Fallback --- */
    snprintf(output, ukuran_output,
        "Maaf, saya belum memiliki data mengenai "
        "jenis-jenis '%s'. Coba tanyakan "
        "\"jelaskan %s\" untuk informasi lebih "
        "lengkap.",
        topik, topik);
}

/* ========================================================================== *
 *       HANDLER PERTANYAAN LANJUTAN (PUBLIC)                                 *
 * ========================================================================== */

/*
 * tangani_pertanyaan_lanjutan - Menangani pertanyaan
 * lanjutan yang merujuk ke topik percakapan sebelumnya.
 *
 * Menggunakan session context melalui
 * cari_atau_buat_sesi_percakapan dan accessor topik.
 * Format: "Kembali ke topik {topik}..."
 *
 * CATATAN: Fungsi ini memerlukan akses ke data sesi
 * yang dikelola oleh modul aturan.c. Mengakses via
 * fungsi publik cari_atau_buat_sesi_percakapan.
 */
void tangani_pertanyaan_lanjutan(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *input,
    int sesi_id,
    char *output,
    size_t ukuran_output)
{
    HasilEkstraksiTopik ekstraksi_kosong;
    char topik_kap[MAX_PANJANG_STRING];

    if (!db || !cache || !input || !output
        || ukuran_output == 0) return;

    /*
     * Ambil topik dari session melalui fungsi akses
     * yang disediakan aturan.c.
     */
    {
        const char *topik_sblm;

        if (sesi_id < 0
            || sesi_id >= ambil_jumlah_sesi()) {
            snprintf(output, ukuran_output,
                "Maaf, sesi percakapan tidak "
                "ditemukan.");
            return;
        }

        topik_sblm = ambil_topik_sesi(sesi_id);
        if (!topik_sblm || !topik_sblm[0]) {
            snprintf(output, ukuran_output,
                "Maksud Anda topik apa? Saya lupa "
                "topik pembicaraan kita "
                "sebelumnya.");
            return;
        }

        aj_log("Pertanyaan lanjutan untuk topik: "
            "'%s'\n", topik_sblm);

        if (strstr(input, "apa lagi")
            || strstr(input, "contoh lain")
            || strstr(input, "siapa lagi")) {
            memset(&ekstraksi_kosong, 0,
                sizeof(ekstraksi_kosong));
            tangani_permintaan_daftar(db, cache,
                &ekstraksi_kosong, NULL,
                output, ukuran_output);
        } else {
            snprintf(topik_kap,
                sizeof(topik_kap), "%s",
                topik_sblm);
            kapitalisasi_awal(topik_kap);

            snprintf(output, ukuran_output,
                "Kembali ke topik %s. "
                "Maaf, saya tidak yakin apa yang "
                "Anda maksud dengan '%s' dalam "
                "konteks ini.",
                topik_kap, input);
        }
    }
}
