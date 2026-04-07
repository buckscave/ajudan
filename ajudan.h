/*
 * ajudan.h - Header Terpadu AJUDAN 4.0
 *
 * File header ini memuat semua deklarasi tipe data dan fungsi
 * untuk modul-modul: basisdata, rulebase, stemming,
 * pencarian fuzzy, analisis kalimat, percakapan,
 * rangkaian respons, penalaran waktu, dan penalaran matematika.
 *
 * Standar: C89, POSIX, SQLite3
 */

#ifndef AJUDAN_H
#define AJUDAN_H

/* snprintf diperlukan — POSIX.1-2001 */
#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200112L
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include "sqlite3.h"

/* ================================================================== *
 *                           KONSTANTA                                  *
 * ================================================================== */

#define NAMA_BASISDATA "basisdata.db"
#define MAX_SESSIONS 100
#define MAX_BARIS_CSV 8192
#define MAX_KOLOM_CSV 64
#define MAX_PANJANG_STRING 256
#define MAX_PANJANG_TOKEN 128
#define MAX_TOKEN_KALIMAH 64
#define MAX_INPUT_USER 512
#define MAX_RESPONS 2048
#define MAX_KLAUSA 8
#define FUZZY_MAX_KANDIDAT 10
#define FUZZY_MIN_SKOR 0.6
#define FUZZY_MAX_JARAK 2

/* Batas cache linguistik */
#define MAX_CACHE_KATA_LUMPAT 256
#define MAX_CACHE_KATA_TANYA 32
#define MAX_CACHE_PEMISAH_KLAUSA 32
#define MAX_CACHE_MARKER_PENJELASAN 16
#define MAX_CACHE_MARKER_IMPLISIT 16
#define MAX_CACHE_POLA_REFERENSI 32
#define MAX_CACHE_FRASA_PEMBUKA 32
#define MAX_CACHE_PEMISAH_REF 16
#define MAX_CACHE_SAPAAN_WAKTU 16
#define MAX_CACHE_PENANDA_KALIMAT 64
#define MAX_CACHE_AFFIX_POS_RULE 16
#define MAX_CACHE_KATA_KOPULA 16
#define MAX_CACHE_KONJUNGSI_RESPON 32
#define MAX_CACHE_FALLBACK_RESPON 32
#define MAX_CACHE_REFERENSI_WAKTU 32
#define MAX_CACHE_HARI_MINGGU 16
#define MAX_CACHE_KATA_BILANGAN 32
#define MAX_CACHE_KATA_OPERASI_MAT 64
#define MAX_CACHE_POLA_RESPON 64
#define MAX_CACHE_KOMPONEN_RESPON 16
#define MAX_LANGKAH_MAT 16

/* ================================================================== *
 *                  ENUMERASI TIPE KALIMAT                              *
 * ================================================================== */

typedef enum {
    TIPE_KALIMAT_LAIN = 0,
    TIPE_KALIMAT_DECLARATIF,
    TIPE_KALIMAT_PERINTAH,
    TIPE_KALIMAT_TANYA_AKTIF,
    TIPE_KALIMAT_TANYA_PASIF,
    TIPE_KALIMAT_TANYA_SIFAT,
    TIPE_KALIMAT_DECLARATIF_SIFAT
} TipeKalimat;

/* ================================================================== *
 *                    ENUMERASI PERAN SPOK TOKEN                         *
 * ================================================================== */

typedef enum {
    PERAN_LAIN = 0,
    PERAN_SUBJEK,
    PERAN_FRASA_SUBJEK,
    PERAN_MODIF_SUBJEK,
    PERAN_PREDIKAT,
    PERAN_OBJEK,
    PERAN_FRASA_OBJEK,
    PERAN_KETERANGAN,
    PERAN_KUANTIFIKASI
} PeranSPOK;

/* ================================================================== *
 *                       ENUMERASI INTENT MATEMATIKA                     *
 * ================================================================== */

typedef enum {
    MAT_OP_TAMBAH = 0,
    MAT_OP_KURANG,
    MAT_OP_KALI,
    MAT_OP_BAGI,
    MAT_OP_MODULUS
} TipeOperasiMat;

/* ================================================================== *
 *                         TIPE DATA STRUKTUR                            *
 * ================================================================== */

typedef struct {
    int kode;
    char pesan[256];
} InfoKesalahan;

typedef enum {
    POS_UNKNOWN = 0,
    POS_NOMINA,
    POS_VERBA,
    POS_ADJEKTIVA,
    POS_ADVERBIA,
    POS_NUMERALIA,
    POS_INTERJEKSI,
    POS_PRONOMINA,
    POS_DETERMINER,
    POS_PREPOSISI,
    POS_KONJUNGSI,
    POS_PARTIKEL,
    POS_PROPER_NOUN
} KategoriPOS;

typedef enum {
    SPOK_TIDAK_DIKENAL = 0,
    SPOK_N_V,
    SPOK_N_V_N,
    SPOK_V_N,
    SPOK_V
} StrukturSpok;

typedef enum {
    INTENT_LAIN = 0,
    INTENT_DEFINISI,
    INTENT_JENIS,
    INTENT_PENJELASAN,
    INTENT_ALASAN,
    INTENT_CARA,
    INTENT_PERBANDINGAN,
    INTENT_ARTI,
    INTENT_MATEMATIKA,
    INTENT_WAKTU
} TipeIntent;

typedef struct {
    char teks[MAX_PANJANG_TOKEN];
    char lema[MAX_PANJANG_TOKEN];
    KategoriPOS pos;
    int id;
} TokenKalimat;

typedef struct DepNode {
    TokenKalimat *token;
    int head_id;
    char relasi[MAX_PANJANG_STRING];
    struct DepNode *children;
    struct DepNode *next;
} SimpulKetergantungan;

typedef struct {
    char teks[MAX_PANJANG_STRING];
    char cocok[MAX_PANJANG_STRING];
    int jarak;
    double skor;
} KandidatFuzzy;

typedef struct {
    int id;
    char user_id[64];
    char topik_utama[MAX_PANJANG_STRING];
    char tipe_list[64];
    char status_alur[64];
    char pertanyaan_terakhir[MAX_INPUT_USER];
    char cache_alias_key[MAX_PANJANG_STRING];
    char cache_alias_val[MAX_PANJANG_STRING];
    double fuzzy_threshold;
    int jarak_maks;
    time_t last_activity;
} SesiPercakapan;

typedef struct {
    char jenis_kalimat[64];
    char topik_utama[MAX_PANJANG_STRING];
    char subjek[MAX_PANJANG_STRING];
    char predikat[MAX_PANJANG_STRING];
    char objek[MAX_PANJANG_STRING];
    char keterangan[MAX_PANJANG_STRING];
    int jml_entitas_dikenal;
    int jml_entitas_otomatis;
    StrukturSpok struktur;
} HasilAnalisisSpok;

typedef struct {
    char topik_utama[MAX_PANJANG_STRING];
    char sub_topik[MAX_PANJANG_STRING];
    char kuantitas[MAX_PANJANG_STRING];
} HasilEkstraksiTopik;

typedef struct {
    char kata[MAX_PANJANG_STRING];
    char arti[MAX_RESPONS];
    char kategori[64];
    char bidang[64];
    char gaya[64];
    double skor_relevansi;
} EntitasTerkait;

typedef struct {
    sqlite3_stmt *stmt_kategori_kata;
    sqlite3_stmt *stmt_stopword;
    sqlite3_stmt *stmt_kata_list;
    sqlite3_stmt *stmt_normalisasi_frasa;
    sqlite3_stmt *stmt_deteksi_pola;
    sqlite3_stmt *stmt_pen_umum_kata;
    sqlite3_stmt *stmt_arti_kata;
    sqlite3_stmt *stmt_rel_semantik;
    sqlite3_stmt *stmt_pen_bertingkat_list;
    sqlite3_stmt *stmt_respon_default_acak;
    sqlite3_stmt *stmt_sapaan_acak;
} AjStmtCache;

/* ================================================================== *
 *           CACHE LINGUISTIK (memuat data dari DB ke memori)           *
 * ================================================================== */

typedef struct {
    char kata[MAX_PANJANG_STRING];
} CacheString;

typedef struct {
    char kata[MAX_PANJANG_STRING];
    char intent_default[32];
    char kategori[32];
} CacheKataTanya;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    char jenis[32];
    int prioritas;
} CachePemisahKlausa;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    char intent[32];
    int prioritas;
} CacheMarkerPenjelasan;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    int prioritas;
} CacheMarkerImplisit;

typedef struct {
    char kata[MAX_PANJANG_STRING];
    char kategori[32];
} CacheKataLumpat;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    int prioritas;
} CachePolaReferensi;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    int prioritas;
} CacheFrasaPembuka;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    int prioritas;
} CachePemisahRef;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    int jam_mulai;
    int jam_akhir;
    int prioritas;
} CacheSapaanWaktu;

typedef struct {
    char jenis[32];
    char kata[MAX_PANJANG_STRING];
    char keterangan[64];
    int prioritas;
} CachePenandaKalimat;

typedef struct {
    char jenis[16];
    char pola[16];
    int kelas_hasil;
    int panjang;
    int prioritas;
} CacheAffixPosRule;

typedef struct {
    char kata[MAX_PANJANG_STRING];
    char ragam[32];
    int prioritas;
} CacheKataKopula;

typedef struct {
    char kata[MAX_PANJANG_STRING];
    char jenis[32];
    int prioritas;
} CacheKonjungsiRespon;

typedef struct {
    char jenis[32];
    char pesan[MAX_RESPONS];
    int prioritas;
} CacheFallbackRespon;

typedef struct {
    char frasa[MAX_PANJANG_STRING];
    int offset_hari;
    char kategori[32];
    int prioritas;
} CacheReferensiWaktu;

typedef struct {
    char nama[MAX_PANJANG_STRING];
    int urutan;
    char ragam[32];
} CacheHariMinggu;

typedef struct {
    char kata[MAX_PANJANG_STRING];
    int nilai;
    char ragam[32];
} CacheKataBilangan;

typedef struct {
    char kata[MAX_PANJANG_STRING];
    char operasi[16];
    char kategori[32];
    int prioritas;
} CacheKataOperasiMat;

typedef struct {
    int id;
    char intent[32];
    char konteks[32];
    char ragam[32];
    int prioritas;
} CachePolaRespon;

typedef struct {
    int pola_id;
    int posisi;
    char peran_spok[32];
    char sumber_data[64];
    char konten_tetap[MAX_PANJANG_STRING];
    char bentuk[32];
    int spasi_sebelum;
    int spasi_sesudah;
} CacheKomponenRespon;

/* Struktur cache utama (singleton) */
typedef struct {
    /* Data linguistik */
    int n_kata_lumpat;
    CacheKataLumpat kata_lumpat[MAX_CACHE_KATA_LUMPAT];

    int n_kata_tanya;
    CacheKataTanya kata_tanya[MAX_CACHE_KATA_TANYA];

    int n_pemisah_klausa;
    CachePemisahKlausa pemisah_klausa[MAX_CACHE_PEMISAH_KLAUSA];

    int n_marker_penjelasan;
    CacheMarkerPenjelasan marker_penjelasan[
        MAX_CACHE_MARKER_PENJELASAN];

    int n_marker_implisit;
    CacheMarkerImplisit marker_implisit[
        MAX_CACHE_MARKER_IMPLISIT];

    int n_pola_referensi;
    CachePolaReferensi pola_referensi[
        MAX_CACHE_POLA_REFERENSI];

    int n_frasa_pembuka;
    CacheFrasaPembuka frasa_pembuka[
        MAX_CACHE_FRASA_PEMBUKA];

    int n_pemisah_ref;
    CachePemisahRef pemisah_ref[MAX_CACHE_PEMISAH_REF];

    int n_sapaan_waktu;
    CacheSapaanWaktu sapaan_waktu[MAX_CACHE_SAPAAN_WAKTU];

    int n_penanda_kalimat;
    CachePenandaKalimat penanda_kalimat[
        MAX_CACHE_PENANDA_KALIMAT];

    int n_affix_pos_rule;
    CacheAffixPosRule affix_pos_rule[
        MAX_CACHE_AFFIX_POS_RULE];

    int n_kata_kopula;
    CacheKataKopula kata_kopula[MAX_CACHE_KATA_KOPULA];

    int n_konjungsi_respon;
    CacheKonjungsiRespon konjungsi_respon[
        MAX_CACHE_KONJUNGSI_RESPON];

    int n_fallback_respon;
    CacheFallbackRespon fallback_respon[
        MAX_CACHE_FALLBACK_RESPON];

    /* Data penalaran */
    int n_referensi_waktu;
    CacheReferensiWaktu referensi_waktu[
        MAX_CACHE_REFERENSI_WAKTU];

    int n_hari_minggu;
    CacheHariMinggu hari_minggu[MAX_CACHE_HARI_MINGGU];

    int n_kata_bilangan;
    CacheKataBilangan kata_bilangan[MAX_CACHE_KATA_BILANGAN];

    int n_kata_operasi_mat;
    CacheKataOperasiMat kata_operasi_mat[
        MAX_CACHE_KATA_OPERASI_MAT];

    /* Data respons SPOK */
    int n_pola_respon;
    CachePolaRespon pola_respon[MAX_CACHE_POLA_RESPON];

    int n_komponen_respon;
    CacheKomponenRespon komponen_respon[
        MAX_CACHE_KOMPONEN_RESPON];

    /* Tanda bahwa cache sudah dimuat */
    int dimuat;
} CacheLinguistik;

/* Struktur langkah matematika */
typedef struct {
    char deskripsi[MAX_RESPONS];
    int nilai;
} LangkahMatematika;

typedef struct {
    int jumlah_langkah;
    LangkahMatematika langkah[MAX_LANGKAH_MAT];
    int hasil_akhir;
    char satuan[MAX_PANJANG_STRING];
} HasilPerhitunganMat;

/* ================================================================== *
 *                     FUNGSI UTILITAS UMUM                              *
 * ================================================================== */

void trim(char *s);
void to_lower_case(char *s);
void kapitalisasi_awal(char *s);
int parse_baris_csv_kuat(
    char *baris, char *kolom[], int max_kolom);
int ajudan_strcasecmp(const char *s1, const char *s2);

extern int g_log_on;
void ajudan_logf(
    const char *fmt,
    const char *a, const char *b, const char *c);

extern int aj_log_enabled;
void aj_set_log_enabled(int enabled);
void aj_log(const char *fmt, ...);
int aj_parse_cli_flags(int argc, char **argv);

/* ================================================================== *
 *              DEKLARASI MODUL DATABASE (data.c)                       *
 * ================================================================== */

int buka_basisdata(sqlite3 **db);
int tutup_basisdata(sqlite3 *db);
int eksekusi_sql(
    sqlite3 *db, const char *sql, InfoKesalahan *error);
void cetak_error(const InfoKesalahan *error);
int inisialisasi_basisdata(InfoKesalahan *error);

int impor_kata(
    const char *nama_berkas, InfoKesalahan *error);
int impor_arti_kata(
    const char *nama_berkas, InfoKesalahan *error);
int impor_semantik_kata(
    const char *nama_berkas, InfoKesalahan *error);
int impor_morfologi_kata(
    const char *nama_berkas, InfoKesalahan *error);
int impor_kata_fungsional(
    const char *nama_berkas, InfoKesalahan *error);
int impor_kelas_cadangan(
    const char *nama_berkas, InfoKesalahan *error);
int impor_jenis_kalimat(
    const char *nama_berkas, InfoKesalahan *error);
int impor_struktur_kalimat(
    const char *nama_berkas, InfoKesalahan *error);
int impor_pola_kalimat(
    const char *nama_berkas, InfoKesalahan *error);
int impor_deteksi_pola(
    const char *nama_berkas, InfoKesalahan *error);
int impor_normalisasi_frasa(
    const char *nama_berkas, InfoKesalahan *error);
int impor_jawaban_bawaan(
    const char *nama_berkas, InfoKesalahan *error);
int impor_pengetahuan_umum(
    const char *nama_berkas, InfoKesalahan *error);
int impor_pengetahuan_bertingkat(
    const char *nama_berkas, InfoKesalahan *error);
int impor_kelas_kata(
    const char *nama_berkas, InfoKesalahan *error);
int impor_ragam_kata(
    const char *nama_berkas, InfoKesalahan *error);
int impor_bidang(
    const char *nama_berkas, InfoKesalahan *error);
int impor_morfologi(
    const char *nama_berkas, InfoKesalahan *error);
int impor_semantik(
    const char *nama_berkas, InfoKesalahan *error);
int impor_hubungan(
    const char *nama_berkas, InfoKesalahan *error);
int impor_kategori_kata(
    const char *nama_berkas, InfoKesalahan *error);
int impor_kategori_kalimat(
    const char *nama_berkas, InfoKesalahan *error);
int impor_templat_respon(
    const char *nama_berkas, InfoKesalahan *error);

int muat_semua_cache(
    sqlite3 *db, CacheLinguistik *cache);
void bersihkan_cache(CacheLinguistik *cache);

/* ================================================================== *
 *             DEKLARASI MODUL RULEBASE (aturan.c)                       *
 * ================================================================== */

int proses_percakapan(
    sqlite3 *koneksi_db,
    CacheLinguistik *cache,
    const char *id_pengguna,
    const char *input_pengguna,
    char *respon_bot,
    size_t ukuran_respon);

int hitung_jarak_levenshtein(
    const char *a, const char *b);
double hitung_skor_fuzzy(
    int jarak, int panjang_max);
int normalisasi_frasa_input_dari_db(
    sqlite3 *db, AjStmtCache *cache,
    const char *input_asli,
    char *input_norm, size_t ukuran_input);
int cari_topik_dengan_fuzzy(
    sqlite3 *db, AjStmtCache *cache,
    const char *masukan,
    char *topik_norm, size_t ukuran,
    int jarak_maks, double skor_min,
    KandidatFuzzy kandidat[],
    int max_kandidat);
int tokenisasi_kalimat(
    const char *kalimat,
    TokenKalimat tokens[], int max_tokens);
KategoriPOS cari_kategori_fallback_dari_cache(
    CacheLinguistik *cache, const char *kata);
int isi_kategori_dari_database(
    sqlite3 *db, AjStmtCache *cache,
    TokenKalimat tokens[], int jml_token);
int deteksi_jenis_dan_sumber_dari_db(
    sqlite3 *db, AjStmtCache *cache,
    const char *kalimat_norm,
    char *jenis, size_t ukuran_jenis,
    char *sumber, size_t ukuran_sumber);
int ambil_arti_kata(
    sqlite3 *db, AjStmtCache *cache,
    const char *lema,
    char *definisi, size_t ukuran);
int ambil_pengetahuan_umum(
    sqlite3 *db, AjStmtCache *cache,
    const char *topik,
    char *judul, size_t uj,
    char *ringkasan, size_t ur,
    char *penjelasan, size_t up,
    char *saran, size_t us);
int ambil_hubungan_semantik(
    sqlite3 *db, AjStmtCache *cache,
    const char *topik,
    char *relasi, size_t ukuran);
int ambil_respon_default_acak(
    sqlite3 *db, AjStmtCache *cache,
    char *output, size_t ukuran_output);
int aj_prepare_stmt_cache(
    sqlite3 *db, AjStmtCache *cache);
void aj_finalize_stmt_cache(
    AjStmtCache *cache);
int cari_entitas_terkait_dengan_filter(
    sqlite3 *db,
    const char *kata_kunci,
    const char *filter_kategori,
    const char *filter_bidang,
    const char *filter_gaya,
    EntitasTerkait hasil[],
    int max_hasil);
int format_respons_daftar(
    sqlite3 *db, AjStmtCache *cache,
    const char *kata,
    const char *topik_bertingkat,
    int dengan_detail,
    char *output, size_t uo);
int pilih_kerangka_respon(
    sqlite3 *db,
    const char *hubungan,
    char *kerangka, size_t ukuran_kerangka);

int adalah_referensi_kosong(
    CacheLinguistik *cache, const char *topik);
int mengandung_referensi_cache(
    CacheLinguistik *cache, const char *input);
int cari_topik_dari_konteks_cache(
    CacheLinguistik *cache,
    const char *input,
    char *topik_out, size_t ukuran);
int tokenisasi_dengan_stem(
    sqlite3 *db,
    const char *kalimat,
    TokenKalimat tokens[], int max_tokens);
int isi_kategori_dengan_stem(
    sqlite3 *db, AjStmtCache *cache,
    TokenKalimat tokens[], int jml_token);
int cari_atau_buat_sesi_percakapan(
    const char *user_id);
void perbarui_sesi_percakapan(
    int sesi_id, const char *topik_baru,
    const char *tipe_list, const char *status,
    const char *pertanyaan);
void bersihkan_sesi_kadaluarsa(void);
int ambil_jumlah_sesi(void);
const char *ambil_topik_sesi(int sesi_id);

int adalah_kata_lumpat(
    CacheLinguistik *cache, const char *kata);
int cari_fallback_respon(
    CacheLinguistik *cache,
    const char *jenis,
    char *output, size_t ukuran_output,
    const char *arg1);

/* ================================================================== *
 *     DEKLARASI MODUL PERCAKAPAN (percakapan.c)                        *
 * ================================================================== */

void tangani_sapaan(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *input,
    char *output, size_t ukuran_output);
void tangani_arti_singkat(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *topik, HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output);
void tangani_permintaan_penjelasan(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *topik, HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output);
void tangani_permintaan_daftar(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    HasilEkstraksiTopik *ekstraksi,
    HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output);
void tangani_pertanyaan_sebab(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output);
void tangani_jenis_benda(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *topik, HasilAnalisisSpok *spok,
    char *output, size_t ukuran_output);
void tangani_pertanyaan_lanjutan(
    sqlite3 *db, AjStmtCache *stmt_cache,
    CacheLinguistik *cache,
    const char *input, int sesi_id,
    char *output, size_t ukuran_output);

/* ================================================================== *
 *           DEKLARASI MODUL STEMMING (potong.c)                        *
 * ================================================================== */

int stem_indonesia(
    const char *masukan,
    char *keluaran, int ukuran);
int stem_menggunakan_db(
    sqlite3 *db,
    const char *masukan,
    char *keluaran, int ukuran);
int stem_kata(
    sqlite3 *db,
    const char *masukan,
    char *keluaran, int ukuran);
void identifikasi_morfologi_kata(
    const char *kata,
    char *prefiks, int uk_prefiks,
    char *sufiks, int uk_sufiks,
    int *panjang_prefiks, int *panjang_sufiks);

/* ================================================================== *
 *          DEKLARASI MODUL PENCARIAN (cari.c)                          *
 * ================================================================== */

int cari_topik_optimal(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *masukan,
    char *topik_norm, int ukuran,
    KandidatFuzzy kandidat[], int max_kandidat);
int inisialisasi_fts5_kata(sqlite3 *db);

/* ================================================================== *
 *        DEKLARASI MODUL ANALISIS KALIMAT (kalimat.c)                  *
 * ================================================================== */

int kenali_tipe_kalimat(
    const CacheLinguistik *cache,
    const TokenKalimat tokens[], int jml_token);
int bangun_depensi_diperluas(
    const CacheLinguistik *cache,
    const TokenKalimat tokens[], int jml_token,
    SimpulKetergantungan nodes[], int verbose);
PeranSPOK klasifikasi_peran_token(
    const TokenKalimat tokens[], int jml_token,
    const SimpulKetergantungan nodes[], int indeks_token);
int analisis_spok_diperluas(
    const TokenKalimat tokens[], int jml_token,
    const SimpulKetergantungan nodes[],
    HasilAnalisisSpok *spok);
int ekstrak_topik_diperluas(
    sqlite3 *db,
    const TokenKalimat tokens[], int jml_token,
    const SimpulKetergantungan nodes[],
    HasilEkstraksiTopik *hasil);

/* ================================================================== *
 *     DEKLARASI MODUL KLAUSA & INTENT (klausa.c)                        *
 * ================================================================== */

int ekstrak_semua_klausa_pertanyaan(
    CacheLinguistik *cache,
    const char *input,
    char klausa_out[][MAX_INPUT_USER],
    int max_klausa);
int deteksi_pertanyaan_implisit_cache(
    CacheLinguistik *cache,
    const char *teks);
TipeIntent klasifikasi_intent_klausa(
    CacheLinguistik *cache,
    const char *klausa,
    char *topik_out, size_t ukuran_topik);
void bersihkan_topik_cache(
    CacheLinguistik *cache,
    char *topik);

/* ================================================================== *
 *     DEKLARASI MODUL RANGKAIAN RESPON (rangkaian.c)                   *
 * ================================================================== */

int rangkai_respon_spok(
    CacheLinguistik *cache,
    const char *intent,
    const char *konteks,
    const char *topik,
    const char *ringkasan,
    const char *penjelasan,
    const char *keterangan,
    char *output,
    size_t ukuran_output);

/* ================================================================== *
 *     DEKLARASI MODUL PENALARAN WAKTU (waktu.c)                        *
 * ================================================================== */

int deteksi_intent_waktu(
    CacheLinguistik *cache,
    const char *input);
int resolve_hari_dari_referensi(
    CacheLinguistik *cache,
    const char *frasa_waktu,
    int *hari_hasil,
    char *nama_hari,
    size_t ukuran_nama);
int hitung_hari_sekarang(int *urutan_hari);
int cari_referensi_waktu_cache(
    CacheLinguistik *cache,
    const char *kata,
    int *offset_hari);
int cari_nama_hari_cache(
    CacheLinguistik *cache,
    int urutan,
    char *nama, size_t ukuran_nama);
int cari_urutan_hari_cache(
    CacheLinguistik *cache,
    const char *nama);
int rangkai_respon_waktu(
    CacheLinguistik *cache,
    const char *frasa_waktu,
    const char *hari_tebakan,
    char *output,
    size_t ukuran_output);

/* ================================================================== *
 *   DEKLARASI MODUL PENALARAN MATEMATIKA (matematika.c)                *
 * ================================================================== */

int deteksi_intent_matematika(
    CacheLinguistik *cache,
    const char *input);
int konversi_kata_ke_angka(
    CacheLinguistik *cache,
    const char *kata,
    int *nilai);
int parse_soal_matematika(
    CacheLinguistik *cache,
    const char *input,
    HasilPerhitunganMat *hasil);
int rangkai_respon_matematika(
    CacheLinguistik *cache,
    const HasilPerhitunganMat *hasil,
    char *output,
    size_t ukuran_output);

#endif
