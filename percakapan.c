/* ================================================================== *
 * AJUDAN 4.0 - Modul Percakapan (percakapan.c)                       *
 * ------------------------------------------------------------------ *
 * Handler respon bot berbasis SPOK-assembly.                          *
 * Setiap handler merangkai respons melalui                           *
 * rangkai_respon_spok() menggunakan komponen dari                    *
 * CacheLinguistik. Jika SPOK assembly gagal, fallback ke            *
 * cari_fallback_respon() atau ambil_respon_default_acak().           *
 * ------------------------------------------------------------------ *
 * Tidak ada string respons Indonesia yang di-hardcode.               *
 * Semua respons berasal dari:                                        *
 *   1. rangkai_respon_spok() (SPOK-assembly)                        *
 *   2. cari_fallback_respon() (cache fallback)                      *
 *   3. Database queries (data dari basisdata)                       *
 * ------------------------------------------------------------------ *
 * C89, POSIX, SQLITE3                                                *
 * ================================================================== */

#include "ajudan.h"

/* ================================================================== *
 *                    UTILITAS STRING                                   *
 * ================================================================== */

/*
 * kapitalisasi_awal - Ubah huruf pertama string menjadi
 * huruf besar, sisanya huruf kecil.
 */
void kapitalisasi_awal(char *s)
{
    int i = 0, len = 0;

    if (!s || !s[0]) return;

    len = (int)strlen(s);
    s[0] = (char)toupper((unsigned char)s[0]);
    for (i = 1; i < len; i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
}

/* ================================================================== *
 *              HANDLER SAPAAN (PUBLIC)                                 *
 * ================================================================== */

/*
 * tangani_sapaan - Menangani input sapaan pengguna.
 *
 * Prioritas 1: Sapaan berbasis waktu via cache->sapaan_waktu
 *   (cocokkan frasa dalam input + validasi jam).
 *   Respons dirangkai via rangkai_respon_spok().
 * Prioritas 2: Fallback respon sapaan dari cache.
 * Prioritas 3: Respon default acak dari database.
 */
void tangani_sapaan(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *input,
    char *output, size_t ukuran_output)
{
    int i = 0, jam = 0, cocok = 0;
    time_t sekarang;
    struct tm *info_waktu;
    char lo_input[MAX_INPUT_USER];
    char lo_frasa[MAX_PANJANG_STRING];

    if (!db || !stmt_cache || !cache || !output
        || ukuran_output == 0)
        return;

    /* Prioritas 1: sapaan berbasis waktu via cache */
    cocok = 0;
    if (input && input[0]
        && cache->n_sapaan_waktu > 0) {
        time(&sekarang);
        info_waktu = localtime(&sekarang);
        jam = info_waktu->tm_hour;

        for (i = 0; i < cache->n_sapaan_waktu; i++) {
            snprintf(lo_input, sizeof(lo_input),
                "%s", input);
            to_lower_case(lo_input);
            trim(lo_input);

            snprintf(lo_frasa, sizeof(lo_frasa),
                "%s", cache->sapaan_waktu[i].frasa);
            to_lower_case(lo_frasa);
            trim(lo_frasa);

            if (strstr(lo_input, lo_frasa) != NULL
                && jam
                    >= cache->sapaan_waktu[i].jam_mulai
                && jam
                    <= cache->sapaan_waktu[i].jam_akhir) {
                if (rangkai_respon_spok(
                    cache, "sapaan", "waktu",
                    cache->sapaan_waktu[i].frasa,
                    "", "", "", output,
                    ukuran_output) == 0) {
                    return;
                }
                cocok = 1;
                break;
            }
        }
    }

    /* Prioritas 2: fallback respon sapaan */
    if (!cocok || output[0] == '\0') {
        if (cari_fallback_respon(
            cache, "sapaan", output,
            ukuran_output, NULL) == 0
            && output[0] != '\0') {
            return;
        }
    }

    /* Prioritas 3: respon default acak dari DB */
    if (ambil_respon_default_acak(db, stmt_cache,
        output, ukuran_output) == 0) {
        return;
    }

    output[0] = '\0';
}

/* ================================================================== *
 *          HANDLER ARTI SINGKAT (PUBLIC)                              *
 * ================================================================== */

/*
 * tangani_arti_singkat - Mengembalikan definisi/arti
 * singkat. Hanya ringkasan (tanpa penjelasan panjang).
 * Digunakan untuk pertanyaan "apa itu X", "X itu apa".
 *
 * Data diambil dari pengetahuan_umum atau arti_kata,
 * lalu dirangkai via rangkai_respon_spok() dengan
 * intent="definisi" dan konteks="singkat".
 */
void tangani_arti_singkat(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *topik, HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output)
{
    char judul_buf[MAX_RESPONS];
    char ringkasan_buf[MAX_RESPONS];
    char penjelasan_buf[MAX_RESPONS];
    char saran_buf[MAX_RESPONS];
    const char *ringkasan_aktif = NULL;
    const char *keterangan_aktif = NULL;
    int rc_spok = -1;

    if (!db || !stmt_cache || !cache || !topik
        || !output || ukuran_output == 0)
        return;

    /* Siapkan keterangan dari spok */
    keterangan_aktif = "";
    if (spok && spok->keterangan[0] != '\0')
        keterangan_aktif = spok->keterangan;

    /* Coba ambil dari pengetahuan_umum */
    if (ambil_pengetahuan_umum(db, stmt_cache, topik,
        judul_buf, sizeof(judul_buf),
        ringkasan_buf, sizeof(ringkasan_buf),
        penjelasan_buf, sizeof(penjelasan_buf),
        saran_buf, sizeof(saran_buf)) == 0) {

        ringkasan_aktif =
            ringkasan_buf[0] != '\0'
                ? ringkasan_buf : judul_buf;

        if (ringkasan_aktif[0] != '\0') {
            rc_spok = rangkai_respon_spok(
                cache, "definisi", "singkat",
                topik, ringkasan_aktif, "",
                keterangan_aktif, output,
                ukuran_output);
            if (rc_spok == 0)
                return;
        }
    }

    /* Coba ambil dari arti_kata */
    ringkasan_buf[0] = '\0';
    if (ambil_arti_kata(db, stmt_cache, topik,
        ringkasan_buf,
        sizeof(ringkasan_buf)) == 0) {
        if (ringkasan_buf[0] != '\0') {
            rc_spok = rangkai_respon_spok(
                cache, "definisi", "singkat",
                topik, ringkasan_buf, "",
                keterangan_aktif, output,
                ukuran_output);
            if (rc_spok == 0)
                return;
        }
    }

    /* Fallback via cache */
    if (cari_fallback_respon(cache, "tidak_tahu",
        output, ukuran_output, topik) == 0
        && output[0] != '\0') {
        return;
    }

    /* Fallback via DB */
    if (ambil_respon_default_acak(db, stmt_cache,
        output, ukuran_output) == 0) {
        return;
    }

    output[0] = '\0';
}

/* ================================================================== *
 *      HANDLER PERMINTAAN PENJELASAN (PUBLIC)                         *
 * ================================================================== */

/*
 * tangani_permintaan_penjelasan - Menangani permintaan
 * penjelasan lengkap tentang suatu topik.
 *
 * Konteks ditentukan dari spok->keterangan:
 *   "singkat"/"sederhana"/"mudah" -> konteks singkat
 *   Default -> konteks detail
 *
 * Jika spok->subjek "kamu"/"anda", gunakan konteks "bot"
 * untuk respons kontekstual tentang diri bot.
 *
 * Sumber data: pengetahuan_umum -> hubungan semantik
 * -> arti_kata -> fallback cache/DB.
 */
void tangani_permintaan_penjelasan(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *topik, HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output)
{
    char judul[MAX_RESPONS];
    char ringkasan[MAX_RESPONS];
    char penjelasan[MAX_RESPONS];
    char saran[MAX_RESPONS];
    char konteks[MAX_PANJANG_STRING];
    char sub_lo[MAX_PANJANG_STRING];
    char penjelasan_gabung[MAX_RESPONS];
    char relasi[MAX_RESPONS];
    char definisi[MAX_RESPONS];
    const char *konten_penjelasan = NULL;
    int rc_spok = -1;
    int gunakan_konteks_bot = 0;
    size_t sisa = 0;

    if (!db || !stmt_cache || !cache || !topik
        || !output || ukuran_output == 0)
        return;

    /* Tentukan konteks dari spok->keterangan */
    snprintf(konteks, sizeof(konteks), "%s", "detail");
    if (spok && spok->keterangan[0] != '\0') {
        if (strstr(spok->keterangan, "singkat")
            != NULL
            || strstr(spok->keterangan, "sederhana")
                != NULL
            || strstr(spok->keterangan, "mudah")
                != NULL) {
            snprintf(konteks, sizeof(konteks),
                "%s", "singkat");
        }
    }

    /* Cek apakah user menanyakan tentang bot */
    if (spok && spok->subjek[0] != '\0') {
        snprintf(sub_lo, sizeof(sub_lo),
            "%s", spok->subjek);
        to_lower_case(sub_lo);
        trim(sub_lo);
        if (strcmp(sub_lo, "kamu") == 0
            || strcmp(sub_lo, "anda") == 0) {
            gunakan_konteks_bot = 1;
        }
    }

    /* Coba ambil dari pengetahuan_umum */
    if (ambil_pengetahuan_umum(db, stmt_cache, topik,
        judul, sizeof(judul),
        ringkasan, sizeof(ringkasan),
        penjelasan, sizeof(penjelasan),
        saran, sizeof(saran)) == 0) {

        /* Gunakan judul jika ringkasan kosong */
        if (ringkasan[0] == '\0'
            && judul[0] != '\0') {
            snprintf(ringkasan,
                sizeof(ringkasan), "%s", judul);
        }

        if (ringkasan[0] != '\0') {
            /* Gabungkan penjelasan + saran */
            penjelasan_gabung[0] = '\0';
            if (penjelasan[0] != '\0') {
                snprintf(penjelasan_gabung,
                    sizeof(penjelasan_gabung),
                    "%s", penjelasan);
                if (saran[0] != '\0') {
                    sisa = sizeof(penjelasan_gabung)
                        - strlen(penjelasan_gabung)
                        - 1;
                    if (sisa > 4) {
                        strncat(
                            penjelasan_gabung,
                            "\n\n", sisa);
                        sisa = sizeof(
                            penjelasan_gabung)
                            - strlen(
                                penjelasan_gabung)
                            - 1;
                        strncat(
                            penjelasan_gabung,
                            saran, sisa);
                    }
                }
            }

            konten_penjelasan =
                penjelasan_gabung[0] != '\0'
                    ? penjelasan_gabung : "";

            if (gunakan_konteks_bot) {
                rc_spok = rangkai_respon_spok(
                    cache, "penjelasan", "bot",
                    topik, ringkasan,
                    konten_penjelasan, "",
                    output, ukuran_output);
            } else {
                rc_spok = rangkai_respon_spok(
                    cache, "penjelasan", konteks,
                    topik, ringkasan,
                    konten_penjelasan, "",
                    output, ukuran_output);
            }
            if (rc_spok == 0)
                return;
        }
    }

    /* Coba ambil dari hubungan semantik */
    relasi[0] = '\0';
    if (ambil_hubungan_semantik(db, stmt_cache,
        topik, relasi, sizeof(relasi)) == 0
        && relasi[0] != '\0') {
        rc_spok = rangkai_respon_spok(
            cache, "penjelasan", "analitik",
            topik, relasi, "", "",
            output, ukuran_output);
        if (rc_spok == 0)
            return;
    }

    /* Coba ambil dari arti_kata */
    definisi[0] = '\0';
    if (ambil_arti_kata(db, stmt_cache, topik,
        definisi, sizeof(definisi)) == 0
        && definisi[0] != '\0') {
        rc_spok = rangkai_respon_spok(
            cache, "penjelasan", "analitik",
            topik, definisi, "", "",
            output, ukuran_output);
        if (rc_spok == 0)
            return;
    }

    /* Fallback via cache */
    if (cari_fallback_respon(cache, "tidak_tahu",
        output, ukuran_output, topik) == 0
        && output[0] != '\0') {
        return;
    }

    /* Fallback via DB */
    if (ambil_respon_default_acak(db, stmt_cache,
        output, ukuran_output) == 0) {
        return;
    }

    output[0] = '\0';
}

/* ================================================================== *
 *       HANDLER PERMINTAAN DAFTAR (PUBLIC)                            *
 * ================================================================== */

/*
 * tangani_permintaan_daftar - Menangani permintaan
 * daftar item terkait topik (fakta, jenis, langkah).
 *
 * Menggunakan format_respons_daftar() untuk mengambil
 * data terformat dari DB, lalu merangkai via
 * rangkai_respon_spok() dengan intent="daftar".
 */
void tangani_permintaan_daftar(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    HasilEkstraksiTopik *ekstraksi,
    HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output)
{
    char daftar_buf[MAX_RESPONS];
    char topik_daftar[MAX_PANJANG_STRING];
    int dengan_detail = 0;
    int rc_spok = -1;

    (void)spok;

    if (!db || !stmt_cache || !cache || !ekstraksi
        || !output || ukuran_output == 0)
        return;

    dengan_detail = (strlen(ekstraksi->kuantitas) == 0)
        ? 1 : 0;

    /* Format daftar via fungsi DB */
    if (format_respons_daftar(db, stmt_cache,
        ekstraksi->topik_utama,
        (strlen(ekstraksi->sub_topik) > 0)
            ? ekstraksi->sub_topik : "fakta",
        dengan_detail, daftar_buf,
        sizeof(daftar_buf)) == 0
        && daftar_buf[0] != '\0') {

        snprintf(topik_daftar,
            sizeof(topik_daftar), "%s",
            ekstraksi->topik_utama);

        rc_spok = rangkai_respon_spok(
            cache, "daftar", "", topik_daftar,
            daftar_buf, "", "", output,
            ukuran_output);
        if (rc_spok == 0)
            return;
    }

    /* Fallback via cache */
    if (cari_fallback_respon(cache, "daftar", output,
        ukuran_output,
        ekstraksi->topik_utama) == 0
        && output[0] != '\0') {
        return;
    }

    /* Fallback via DB */
    if (ambil_respon_default_acak(db, stmt_cache,
        output, ukuran_output) == 0) {
        return;
    }

    output[0] = '\0';
}

/* ================================================================== *
 *       HANDLER PERTANYAAN SEBAB (PUBLIC)                             *
 * ================================================================== */

/*
 * tangani_pertanyaan_sebab - Menangani pertanyaan
 * tentang sebab/alasan ("mengapa", "kenapa", dll).
 *
 * Topik diambil dari spok->objek (fallback: topik_utama).
 * Entitas terkait dicari via cari_entitas_terkait_dengan_filter.
 * Respons dirangkai via rangkai_respon_spok()
 * dengan intent="alasan".
 */
void tangani_pertanyaan_sebab(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output)
{
    EntitasTerkait entitas[10];
    char entitas_buf[MAX_RESPONS];
    char topik_aktif[MAX_PANJANG_STRING];
    int jml_entitas = 0, i = 0;
    int rc_spok = -1;
    const char *topik_ptr = NULL;

    if (!db || !stmt_cache || !cache || !spok
        || !output || ukuran_output == 0)
        return;

    /* Tentukan topik dari spok */
    topik_ptr = spok->objek;
    if (!topik_ptr || !topik_ptr[0])
        topik_ptr = spok->topik_utama;
    if (!topik_ptr || !topik_ptr[0])
        return;

    snprintf(topik_aktif, sizeof(topik_aktif),
        "%s", topik_ptr);

    /* Cari entitas terkait */
    jml_entitas = cari_entitas_terkait_dengan_filter(
        db, topik_aktif, "verba,nomina", "",
        "umum", entitas, 10);

    /* Bangun string entitas */
    entitas_buf[0] = '\0';
    for (i = 0; i < jml_entitas && i < 3; i++) {
        if (i > 0) {
            strncat(entitas_buf, ", ",
                sizeof(entitas_buf)
                - strlen(entitas_buf) - 1);
        }
        strncat(entitas_buf, entitas[i].kata,
            sizeof(entitas_buf)
            - strlen(entitas_buf) - 1);
    }

    /* Rangkai via SPOK */
    rc_spok = rangkai_respon_spok(
        cache, "alasan", "", topik_aktif,
        (entitas_buf[0] != '\0')
            ? entitas_buf : topik_aktif,
        "", "", output, ukuran_output);
    if (rc_spok == 0)
        return;

    /* Fallback via cache */
    if (cari_fallback_respon(cache, "alasan", output,
        ukuran_output, topik_aktif) == 0
        && output[0] != '\0') {
        return;
    }

    /* Fallback via DB */
    if (ambil_respon_default_acak(db, stmt_cache,
        output, ukuran_output) == 0) {
        return;
    }

    output[0] = '\0';
}

/* ================================================================== *
 *       HANDLER JENIS BENDA (PUBLIC)                                  *
 * ================================================================== */

/*
 * tangani_jenis_benda - Menangani pertanyaan tentang
 * JENIS suatu benda atau konsep.
 *
 * Sumber data (berurutan):
 * 1. pengetahuan_bertingkat topik 'jenis'
 * 2. Relasi semantik jenis_dari
 * 3. Stem + ulangi relasi semantik
 * 4. Fallback via cache / DB
 *
 * Data yang ditemukan dirangkai via
 * rangkai_respon_spok() dengan intent="jenis".
 */
void tangani_jenis_benda(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *topik, HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output)
{
    int rc = 0, ada_hasil = 0;
    const char *poin_val = NULL;
    const char *penjelasan_val = NULL;
    const char *hub_relasi = NULL;
    char buffer[MAX_RESPONS];
    char kata_stem[MAX_PANJANG_TOKEN];
    int rc_spok = -1;

    (void)spok;

    if (!db || !stmt_cache || !cache || !topik
        || !output || ukuran_output == 0)
        return;

    /* --- 1. Coba pengetahuan_bertingkat --- */
    rc = sqlite3_reset(
        stmt_cache->stmt_pen_bertingkat_list);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(
            stmt_cache->stmt_pen_bertingkat_list,
            1, topik, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(
            stmt_cache->stmt_pen_bertingkat_list,
            2, "jenis", -1, SQLITE_TRANSIENT);

        buffer[0] = '\0';
        ada_hasil = 0;

        while (sqlite3_step(
            stmt_cache->stmt_pen_bertingkat_list)
            == SQLITE_ROW) {
            poin_val = (const char *)
                sqlite3_column_text(
                stmt_cache
                    ->stmt_pen_bertingkat_list,
                0);
            penjelasan_val = (const char *)
                sqlite3_column_text(
                stmt_cache
                    ->stmt_pen_bertingkat_list,
                1);

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
                    strncat(buffer,
                        penjelasan_val,
                        sizeof(buffer)
                        - strlen(buffer) - 1);
                }
                ada_hasil = 1;
            }
        }

        sqlite3_reset(
            stmt_cache->stmt_pen_bertingkat_list);

        if (ada_hasil) {
            rc_spok = rangkai_respon_spok(
                cache, "jenis", "", topik,
                buffer, "", "", output,
                ukuran_output);
            if (rc_spok == 0)
                return;
        }
    }

    /* --- 2. Coba relasi semantik --- */
    rc = sqlite3_reset(
        stmt_cache->stmt_rel_semantik);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(
            stmt_cache->stmt_rel_semantik,
            1, topik, -1, SQLITE_TRANSIENT);

        buffer[0] = '\0';
        ada_hasil = 0;

        while (sqlite3_step(
            stmt_cache->stmt_rel_semantik)
            == SQLITE_ROW) {
            poin_val = (const char *)
                sqlite3_column_text(
                stmt_cache->stmt_rel_semantik,
                0);
            hub_relasi = (const char *)
                sqlite3_column_text(
                stmt_cache->stmt_rel_semantik,
                1);

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
            stmt_cache->stmt_rel_semantik);

        if (ada_hasil) {
            rc_spok = rangkai_respon_spok(
                cache, "jenis", "", topik,
                buffer, "", "", output,
                ukuran_output);
            if (rc_spok == 0)
                return;
        }
    }

    /* --- 3. Stem, lalu ulangi semantik --- */
    if (stem_kata(db, topik, kata_stem,
        (int)sizeof(kata_stem)) == 0
        && ajudan_strcasecmp(kata_stem, topik) != 0) {
        rc = sqlite3_reset(
            stmt_cache->stmt_rel_semantik);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(
                stmt_cache->stmt_rel_semantik,
                1, kata_stem, -1,
                SQLITE_TRANSIENT);

            buffer[0] = '\0';
            ada_hasil = 0;

            while (sqlite3_step(
                stmt_cache->stmt_rel_semantik)
                == SQLITE_ROW) {
                poin_val = (const char *)
                    sqlite3_column_text(
                    stmt_cache
                        ->stmt_rel_semantik,
                    0);
                hub_relasi = (const char *)
                    sqlite3_column_text(
                    stmt_cache
                        ->stmt_rel_semantik,
                    1);

                if (!poin_val || !poin_val[0])
                    continue;

                if (hub_relasi
                    && strstr(hub_relasi,
                        "jenis") != NULL) {
                    if (ada_hasil) {
                        strncat(buffer,
                            ", ",
                            sizeof(buffer)
                            - strlen(buffer)
                            - 1);
                    }
                    strncat(buffer, poin_val,
                        sizeof(buffer)
                        - strlen(buffer)
                        - 1);
                    ada_hasil = 1;
                }
            }

            sqlite3_reset(
                stmt_cache->stmt_rel_semantik);

            if (ada_hasil) {
                rc_spok = rangkai_respon_spok(
                    cache, "jenis", "",
                    topik, buffer, "", "",
                    output, ukuran_output);
                if (rc_spok == 0)
                    return;
            }
        }
    }

    /* --- 4. Fallback via cache --- */
    if (cari_fallback_respon(cache, "jenis", output,
        ukuran_output, topik) == 0
        && output[0] != '\0') {
        return;
    }

    /* --- 5. Fallback via DB --- */
    if (ambil_respon_default_acak(db, stmt_cache,
        output, ukuran_output) == 0) {
        return;
    }

    output[0] = '\0';
}

/* ================================================================== *
 *     HANDLER PERTANYAAN LANJUTAN (PUBLIC)                            *
 * ================================================================== */

/*
 * tangani_pertanyaan_lanjutan - Menangani pertanyaan
 * lanjutan yang merujuk ke topik percakapan sebelumnya.
 *
 * Menggunakan session context via
 * cari_atau_buat_sesi_percakapan dan accessor topik.
 *
 * Jika input mengandung "apa lagi"/"contoh lain"/
 * "siapa lagi", delegasikan ke tangani_permintaan_daftar.
 * Jika tidak, rangkai via rangkai_respon_spok()
 * dengan intent="lanjutan".
 */
void tangani_pertanyaan_lanjutan(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *input, int sesi_id,
    char *output, size_t ukuran_output)
{
    HasilEkstraksiTopik ekstraksi_kosong;
    const char *topik_sblm = NULL;
    int rc_spok = -1;

    if (!db || !stmt_cache || !cache || !input
        || !output || ukuran_output == 0)
        return;

    /* Validasi sesi */
    if (sesi_id < 0
        || sesi_id >= ambil_jumlah_sesi()) {
        if (cari_fallback_respon(cache,
            "lanjutan", output,
            ukuran_output, NULL) == 0
            && output[0] != '\0') {
            return;
        }
        output[0] = '\0';
        return;
    }

    topik_sblm = ambil_topik_sesi(sesi_id);
    if (!topik_sblm || !topik_sblm[0]) {
        if (cari_fallback_respon(cache,
            "lanjutan", output,
            ukuran_output, NULL) == 0
            && output[0] != '\0') {
            return;
        }
        output[0] = '\0';
        return;
    }

    aj_log("Pertanyaan lanjutan untuk topik: "
        "'%s'\n", topik_sblm);

    /* Deteksi permintaan daftar lanjutan */
    if (strstr(input, "apa lagi") != NULL
        || strstr(input, "contoh lain") != NULL
        || strstr(input, "siapa lagi") != NULL) {
        memset(&ekstraksi_kosong, 0,
            sizeof(ekstraksi_kosong));
        snprintf(
            ekstraksi_kosong.topik_utama,
            sizeof(ekstraksi_kosong.topik_utama),
            "%s", topik_sblm);
        tangani_permintaan_daftar(db, stmt_cache,
            cache, &ekstraksi_kosong, NULL,
            output, ukuran_output);
        return;
    }

    /* Rangkai via SPOK untuk lanjutan */
    rc_spok = rangkai_respon_spok(
        cache, "lanjutan", "", topik_sblm,
        "", input, "", output, ukuran_output);
    if (rc_spok == 0)
        return;

    /* Fallback via cache */
    if (cari_fallback_respon(cache, "lanjutan",
        output, ukuran_output, topik_sblm) == 0
        && output[0] != '\0') {
        return;
    }

    /* Fallback via DB */
    if (ambil_respon_default_acak(db, stmt_cache,
        output, ukuran_output) == 0) {
        return;
    }

    output[0] = '\0';
}
