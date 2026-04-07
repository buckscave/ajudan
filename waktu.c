/* ================================================================== *
 * AJUDAN 4.0 - Modul Penalaran Waktu (waktu.c)                         *
 * ------------------------------------------------------------------ *
 * Modul ini menangani penalaran temporal berbasis database.            *
 * Bot dapat memahami referensi waktu relatif                          *
 * ("kemarin", "besok", "minggu depan") dan menghitung hari            *
 * secara otomatis berdasarkan tanggal sistem.                         *
 * ------------------------------------------------------------------ *
 * C89, POSIX                                                          *
 * ================================================================== */

#include "ajudan.h"

/* ================================================================== *
 *  FUNGSI BANTUAN: MENDAPATKAN HARI DARI SISTEM                       *
 * ================================================================== */

/*
 * hitung_hari_sekarang - Mendapatkan urutan hari (1=Senin s.d. 7=Minggu)
 * berdasarkan tanggal/waktu sistem saat ini.
 * Mengembalikan 0 jika gagal.
 */
int hitung_hari_sekarang(int *urutan_hari)
{
    time_t sekarang;
    struct tm *info;

    if (!urutan_hari) return -1;

    time(&sekarang);
    info = localtime(&sekarang);
    if (!info) return -1;

    /*
     * tm_wday (POSIX): 0=Minggu, 1=Senin, 2=Selasa,
     *                  3=Rabu, 4=Kamis, 5=Jumat, 6=Sabtu
     * Konversi ke sistem internal: 1=Senin, ..., 7=Minggu
     */
    if (info->tm_wday == 0) {
        *urutan_hari = 7;
    } else {
        *urutan_hari = info->tm_wday;
    }

    return 0;
}

/* ================================================================== *
 *  FUNGSI BANTUAN: PENCARIAN CACHE                                     *
 * ================================================================== */

/*
 * cari_referensi_waktu_cache - Mencari frasa waktu relatif
 * dalam cache dan mengembalikan offset hari.
 * Contoh: "kemarin" -> -1, "besok" -> 1
 */
int cari_referensi_waktu_cache(
    CacheLinguistik *cache,
    const char *kata,
    int *offset_hari)
{
    int i;
    char lo[MAX_PANJANG_STRING];

    if (!cache || !kata || !offset_hari) return -1;
    if (!cache->dimuat) return -1;

    snprintf(lo, sizeof(lo), "%s", kata);
    to_lower_case(lo);
    trim(lo);

    for (i = 0; i < cache->n_referensi_waktu; i++) {
        if (ajudan_strcasecmp(
            cache->referensi_waktu[i].frasa, lo) == 0) {
            *offset_hari = cache->referensi_waktu[i].offset_hari;
            return 0;
        }
    }

    return -1;
}

/*
 * cari_nama_hari_cache - Mencari nama hari berdasarkan urutan.
 * urutan: 1=Senin s.d. 7=Minggu
 */
int cari_nama_hari_cache(
    CacheLinguistik *cache,
    int urutan,
    char *nama, size_t ukuran_nama)
{
    int i;

    if (!cache || !nama || ukuran_nama == 0) return -1;
    if (!cache->dimuat) return -1;

    for (i = 0; i < cache->n_hari_minggu; i++) {
        if (cache->hari_minggu[i].urutan == urutan) {
            snprintf(nama, ukuran_nama, "%s",
                cache->hari_minggu[i].nama);
            return 0;
        }
    }

    return -1;
}

/*
 * cari_urutan_hari_cache - Mencari urutan hari dari nama.
 * Mengembalikan 0 jika tidak ditemukan.
 */
int cari_urutan_hari_cache(
    CacheLinguistik *cache,
    const char *nama)
{
    int i;
    char lo[MAX_PANJANG_STRING];

    if (!cache || !nama) return 0;
    if (!cache->dimuat) return 0;

    snprintf(lo, sizeof(lo), "%s", nama);
    to_lower_case(lo);
    trim(lo);

    for (i = 0; i < cache->n_hari_minggu; i++) {
        if (ajudan_strcasecmp(
            cache->hari_minggu[i].nama, lo) == 0) {
            return cache->hari_minggu[i].urutan;
        }
    }

    return 0;
}

/* ================================================================== *
 *  DETEKSI INTENT WAKTU                                               *
 * ================================================================== */

/*
 * deteksi_intent_waktu - Memeriksa apakah input mengandung
 * pertanyaan atau pernyataan terkait waktu/hari.
 * Mengembalikan 1 jika terdeteksi, 0 jika tidak.
 */
int deteksi_intent_waktu(
    CacheLinguistik *cache,
    const char *input)
{
    int i;
    char lo[MAX_INPUT_USER];
    char *p;

    if (!cache || !input || !input[0]) return 0;
    if (!cache->dimuat) return 0;

    snprintf(lo, sizeof(lo), "%s", input);
    to_lower_case(lo);
    trim(lo);

    /* Cek apakah input mengandung referensi waktu */
    for (i = 0; i < cache->n_referensi_waktu; i++) {
        if (strstr(lo,
            cache->referensi_waktu[i].frasa) != NULL) {
            return 1;
        }
    }

    /* Cek apakah input mengandung nama hari */
    for (i = 0; i < cache->n_hari_minggu; i++) {
        p = strstr(lo, cache->hari_minggu[i].nama);
        if (p != NULL) {
            /* Pastikan kecocokan kata utuh */
            int len_nama = (int)strlen(
                cache->hari_minggu[i].nama);
            int len_sisa = (int)strlen(p);
            if (len_sisa == len_nama ||
                !isalpha((unsigned char)
                    p[len_nama])) {
                return 1;
            }
        }
    }

    return 0;
}

/* ================================================================== *
 *  RESOLVE HARI DARI REFERENSI WAKTU                                   *
 * ================================================================== */

/*
 * resolve_hari_dari_referensi - Menghitung nama hari
 * berdasarkan frasa referensi waktu.
 * Contoh: "besok" -> mengembalikan "rabu" (jika hari ini selasa)
 */
int resolve_hari_dari_referensi(
    CacheLinguistik *cache,
    const char *frasa_waktu,
    int *hari_hasil,
    char *nama_hari,
    size_t ukuran_nama)
{
    int offset;
    int hari_sekarang;
    int hari_baru;

    if (!cache || !frasa_waktu || !hari_hasil
        || !nama_hari || ukuran_nama == 0)
        return -1;

    if (cari_referensi_waktu_cache(
        cache, frasa_waktu, &offset) != 0)
        return -1;

    if (hitung_hari_sekarang(&hari_sekarang) != 0)
        return -1;

    /* Hitung hari baru dengan wrap-around 1-7 */
    hari_baru = hari_sekarang + offset;
    while (hari_baru < 1) hari_baru += 7;
    while (hari_baru > 7) hari_baru -= 7;

    *hari_hasil = hari_baru;

    if (cari_nama_hari_cache(
        cache, hari_baru, nama_hari, ukuran_nama) != 0) {
        return -1;
    }

    return 0;
}

/* ================================================================== *
 *  FUNGSI BANTUAN: PENCARIAN TEMPLAT WAKTU                             *
 * ================================================================== */

/*
 * cari_templat_waktu - Mencari string templat respons
 * waktu dari cache fallback_respon berdasarkan jenis.
 * Jenis yang didukung: "waktu_konfirmasi", "waktu_koreksi",
 * "waktu_informasi".
 * Mengembalikan NULL jika tidak ditemukan.
 */
static const char *cari_templat_waktu(
    CacheLinguistik *cache,
    const char *jenis)
{
    int i;

    if (!cache || !jenis || !cache->dimuat)
        return NULL;

    for (i = 0; i < cache->n_fallback_respon; i++) {
        if (ajudan_strcasecmp(
            cache->fallback_respon[i].jenis,
            jenis) == 0) {
            return cache->fallback_respon[i].pesan;
        }
    }

    return NULL;
}

/* ================================================================== *
 *  RANGKAI RESPON WAKTU                                                *
 * ================================================================== */

/*
 * rangkai_respon_waktu - Merangkai respons temporal
 * berdasarkan frasa waktu dan hari tebakan user.
 * Semua templat respons diambil dari database (fallback_respon)
 * sehingga tidak ada string yang di-hardcode di kode C.
 *
 * Jenis templat (dari DB):
 *   waktu_konfirmasi : tebakan user benar
 *   waktu_koreksi    : tebakan user salah
 *   waktu_informasi  : tanpa tebakan / tebakan tidak dikenal
 *
 * Contoh input: frasa_waktu="besok", hari_tebakan="selasa"
 * Contoh output: "sekarang adalah hari selasa, jadi besok
 *                bukan selasa melainkan rabu."
 */
int rangkai_respon_waktu(
    CacheLinguistik *cache,
    const char *frasa_waktu,
    const char *hari_tebakan,
    char *output,
    size_t ukuran_output)
{
    int hari_sekarang;
    int hari_target;
    char nama_sekarang[MAX_PANJANG_STRING];
    char nama_target[MAX_PANJANG_STRING];
    char frasa_lo[MAX_PANJANG_STRING];
    char tebakan_lo[MAX_PANJANG_STRING];
    int urutan_tebakan;
    int offset_ref;
    const char *templat;

    if (!cache || !output || ukuran_output == 0)
        return -1;

    output[0] = '\0';

    /* Dapatkan hari ini dari sistem */
    if (hitung_hari_sekarang(&hari_sekarang) != 0)
        return -1;

    if (cari_nama_hari_cache(cache, hari_sekarang,
        nama_sekarang, sizeof(nama_sekarang)) != 0)
        return -1;

    /* Resolve frasa waktu ke hari target */
    if (frasa_waktu && frasa_waktu[0]) {
        snprintf(frasa_lo, sizeof(frasa_lo),
            "%s", frasa_waktu);
        to_lower_case(frasa_lo);
        trim(frasa_lo);

        if (cari_referensi_waktu_cache(
            cache, frasa_lo, &offset_ref) != 0)
            return -1;

        hari_target = hari_sekarang + offset_ref;
        while (hari_target < 1) hari_target += 7;
        while (hari_target > 7) hari_target -= 7;
    } else {
        return -1;
    }

    if (cari_nama_hari_cache(cache, hari_target,
        nama_target, sizeof(nama_target)) != 0)
        return -1;

    /* Cek kecocokan tebakan user */
    if (hari_tebakan && hari_tebakan[0]) {
        snprintf(tebakan_lo, sizeof(tebakan_lo),
            "%s", hari_tebakan);
        to_lower_case(tebakan_lo);
        trim(tebakan_lo);

        urutan_tebakan = cari_urutan_hari_cache(
            cache, tebakan_lo);

        if (urutan_tebakan > 0) {
            if (urutan_tebakan == hari_target) {
                /* Tebakan benar */
                templat = cari_templat_waktu(
                    cache, "waktu_konfirmasi");
                if (!templat)
                    templat = "%s memang %s.";
                snprintf(output, ukuran_output,
                    templat, frasa_lo, nama_target);
            } else {
                /* Tebakan salah - berikan koreksi */
                templat = cari_templat_waktu(
                    cache, "waktu_koreksi");
                if (!templat)
                    templat = "sekarang adalah hari %s, "
                        "jadi %s bukan %s melainkan %s.";
                snprintf(output, ukuran_output,
                    templat, nama_sekarang, frasa_lo,
                    tebakan_lo, nama_target);
            }
        } else {
            /* Tebakan tidak dikenal */
            templat = cari_templat_waktu(
                cache, "waktu_informasi");
            if (!templat)
                templat = "sekarang adalah hari %s, "
                    "jadi %s adalah hari %s.";
            snprintf(output, ukuran_output,
                templat, nama_sekarang, frasa_lo,
                nama_target);
        }
    } else {
        /* Tanpa tebakan user */
        templat = cari_templat_waktu(
            cache, "waktu_informasi");
        if (!templat)
            templat = "sekarang adalah hari %s, "
                "jadi %s adalah hari %s.";
        snprintf(output, ukuran_output,
            templat, nama_sekarang, frasa_lo,
            nama_target);
    }

    return 0;
}
