/* ================================================================== *
 * AJUDAN 4.0 - Modul Rangkaian Respon (rangkaian.c)                   *
 * ------------------------------------------------------------------ *
 * Modul ini merangkai respons bot kata per kata berdasarkan            *
 * komponen SPOK yang dimuat dari database.                             *
 * Setiap slot (subjek, predikat, objek, keterangan) diambil           *
 * dari database, kemudian digabung menjadi kalimat utuh.              *
 * ------------------------------------------------------------------ *
 * C89, POSIX                                                          *
 * ================================================================== */

#include "ajudan.h"

/* ================================================================== *
 *  PENCARIAN KOPIULA                                                  *
 * ================================================================== */

/*
 * ambil_kata_kopula - Mengambil kata kopula dari cache
 * berdasarkan ragam (baku/tidak_baku).
 * Mengembalikan "adalah" sebagai default jika tidak ditemukan.
 */
static const char *ambil_kata_kopula(
    CacheLinguistik *cache,
    const char *ragam)
{
    int i;
    int gunakan_baku = 1;

    if (!cache || !cache->dimuat) return "adalah";

    if (ragam && ajudan_strcasecmp(ragam, "tidak_baku") == 0)
        gunakan_baku = 0;

    for (i = 0; i < cache->n_kata_kopula; i++) {
        int cocok_ragam = 0;
        if (gunakan_baku &&
            ajudan_strcasecmp(
                cache->kata_kopula[i].ragam,
                "baku") == 0)
            cocok_ragam = 1;
        if (!gunakan_baku &&
            ajudan_strcasecmp(
                cache->kata_kopula[i].ragam,
                "tidak_baku") == 0)
            cocok_ragam = 1;

        if (cocok_ragam)
            return cache->kata_kopula[i].kata;
    }

    return "adalah";
}

/* ================================================================== *
 *  RANGKAI RESPON SPOK                                                *
 * ================================================================== */

/*
 * rangkai_respon_spok - Merangkai respons dari komponen-komponen
 * SPOK yang dimuat dari database berdasarkan intent dan konteks.
 *
 * Algoritma:
 * 1. Cari pola_respon yang cocok (intent + konteks)
 * 2. Muat semua komponen_respon untuk pola tersebut
 * 3. Untuk setiap komponen, tentukan konten:
 *    - "tetap": gunakan konten_tetap dari DB
 *    - "input_topik": gunakan topik dari input user
 *    - "db_ringkasan": gunakan ringkasan dari DB
 *    - "kopula": ambil dari tabel kata_kopula
 *    - "tanda_baca": ambil dari konten_tetap
 * 4. Gabungkan semua komponen dengan aturan spasi
 */
int rangkai_respon_spok(
    CacheLinguistik *cache,
    const char *intent,
    const char *konteks,
    const char *topik,
    const char *ringkasan,
    const char *penjelasan,
    const char *keterangan,
    char *output,
    size_t ukuran_output)
{
    int i, j;
    int pola_id = -1;
    int offset = 0;

    if (!cache || !output || ukuran_output == 0)
        return -1;
    if (!cache->dimuat) return -1;

    output[0] = '\0';

    /* Langkah 1: Cari pola yang cocok */
    for (i = 0; i < cache->n_pola_respon; i++) {
        int cocok_intent = 0;
        int cocok_konteks = 0;

        if (intent && cache->pola_respon[i].intent[0]) {
            if (ajudan_strcasecmp(
                cache->pola_respon[i].intent,
                intent) == 0)
                cocok_intent = 1;
        }

        if (konteks &&
            cache->pola_respon[i].konteks[0]) {
            if (ajudan_strcasecmp(
                cache->pola_respon[i].konteks,
                konteks) == 0)
                cocok_konteks = 1;
        }

        if (cocok_intent && cocok_konteks) {
            pola_id = cache->pola_respon[i].id;
            break;
        }
    }

    /* Jika konteks tidak cocok, coba tanpa konteks */
    if (pola_id < 0) {
        for (i = 0; i < cache->n_pola_respon; i++) {
            if (intent &&
                cache->pola_respon[i].intent[0] &&
                ajudan_strcasecmp(
                    cache->pola_respon[i].intent,
                    intent) == 0) {
                pola_id = cache->pola_respon[i].id;
                break;
            }
        }
    }

    /* Jika tetap tidak ditemukan, buat default */
    if (pola_id < 0) {
        const char *kopula = ambil_kata_kopula(cache, "baku");
        const char *top = (topik && topik[0]) ? topik : "";
        const char *ring = (ringkasan && ringkasan[0])
            ? ringkasan : "";
        char top_kap[MAX_PANJANG_STRING];

        if (top[0]) {
            snprintf(top_kap, sizeof(top_kap),
                "%s", top);
            kapitalisasi_awal(top_kap);
        } else {
            top_kap[0] = '\0';
        }

        if (ring[0]) {
            snprintf(output, ukuran_output,
                "%s %s %s.",
                top_kap, kopula, ring);
        } else if (top[0]) {
            snprintf(output, ukuran_output,
                "%s.", top_kap);
        } else {
            return -1;
        }
        return 0;
    }

    /* Langkah 2: Muat komponen untuk pola_id */
    for (i = 0; i < cache->n_komponen_respon; i++) {
        const char *konten = NULL;
        int spasi_sblm = cache
            ->komponen_respon[i].spasi_sebelum;
        int spasi_ssdh = cache
            ->komponen_respon[i].spasi_sesudah;
        size_t sisa;
        char buf_kap[MAX_PANJANG_STRING];

        if (cache->komponen_respon[i].pola_id != pola_id)
            continue;

        buf_kap[0] = '\0';

        /* Langkah 3: Tentukan konten per komponen */
        if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "tetap") == 0) {
            konten = cache
                ->komponen_respon[i].konten_tetap;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "input_topik") == 0) {
            konten = topik;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "db_ringkasan") == 0) {
            konten = ringkasan;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "db_penjelasan") == 0) {
            konten = penjelasan;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "input_keterangan") == 0) {
            konten = keterangan;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "kopula") == 0) {
            konten = ambil_kata_kopula(cache, "baku");
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "tanda_baca") == 0) {
            konten = cache
                ->komponen_respon[i].konten_tetap;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "sapaan_waktu") == 0) {
            konten = topik;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "mat_langkah") == 0) {
            konten = ringkasan;
        } else if (ajudan_strcasecmp(
            cache->komponen_respon[i].sumber_data,
            "waktu_koreksi") == 0) {
            konten = ringkasan;
        }

        if (!konten || !konten[0]) continue;

        /* Kapitalisasi untuk posisi pertama */
        if (offset == 0) {
            snprintf(buf_kap, sizeof(buf_kap),
                "%s", konten);
            kapitalisasi_awal(buf_kap);
            konten = buf_kap;
        }

        /* Tambahkan spasi sebelum jika diperlukan */
        if (spasi_sblm && offset > 0) {
            sisa = ukuran_output - offset - 1;
            if (sisa > 1) {
                output[offset] = ' ';
                offset++;
            }
        }

        /* Tambahkan konten */
        {
            int len_konten = (int)strlen(konten);
            sisa = ukuran_output - offset - 1;
            if ((size_t)len_konten < sisa) {
                memcpy(output + offset,
                    konten, len_konten);
                offset += len_konten;
            } else if (sisa > 0) {
                memcpy(output + offset,
                    konten, sisa);
                offset += (int)sisa;
            }
        }
    }

    output[offset] = '\0';

    /* Pastikan diakhiri tanda baca kalimat */
    if (offset > 0 && output[offset - 1] != '.'
        && output[offset - 1] != '?'
        && output[offset - 1] != '!') {
        size_t sisa = ukuran_output - offset - 1;
        if (sisa > 0) {
            output[offset] = '.';
            offset++;
            output[offset] = '\0';
        }
    }

    return (offset > 0) ? 0 : -1;
}
