/*
 * ajudan.h - Header Terpadu AJUDAN 3.1
 *
 * File header ini memuat semua deklarasi tipe data dan fungsi
 * untuk modul-modul: basisdata, basisaturan, stemming,
 * pencarian fuzzy, dan analisis kalimat.
 *
 * Standar: C89, POSIX, SQLite3
 */

#ifndef AJUDAN_H
#define AJUDAN_H

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
#define FUZZY_MAX_KANDIDAT 10
#define FUZZY_MIN_SKOR 0.6
#define FUZZY_MAX_JARAK 2

/* ================================================================== *
 *                        ENUMERASI TIPE KALIMAT                         *
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
 *                         TIPE DATA STUKTUR                             *
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
} AjStmtCache;

/* ================================================================== *
 *                     FUNGSI UTILITAS UMUM                              *
 * ================================================================== */

void trim(char *s);
void to_lower_case(char *s);
int parse_baris_csv_kuat(
    char *baris, char *kolom[], int max_kolom);
int ajudan_strcasecmp(const char *s1, const char *s2);

/* Logging */
extern int g_log_on;
void ajudan_logf(
    const char *fmt,
    const char *a, const char *b, const char *c);

extern int aj_log_enabled;
void aj_set_log_enabled(int enabled);
void aj_log(const char *fmt, ...);
int aj_parse_cli_flags(int argc, char **argv);

/* ================================================================== *
 *              DEKLARASI MODUL DATABASE (basisdata.c)                   *
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

/* ================================================================== *
 *             DEKLARASI MODUL RULEBASE (basisaturan.c)                  *
 * ================================================================== */

int proses_percakapan(
    sqlite3 *koneksi_db,
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
KategoriPOS cari_kategori_fallback_dari_db(
    sqlite3 *db, const char *kata);
int isi_kategori_dari_database(
    sqlite3 *db, AjStmtCache *cache,
    TokenKalimat tokens[], int jml_token);
int bangun_pohon_ketergantungan(
    TokenKalimat tokens[], int jml_token,
    SimpulKetergantungan *nodes, int verbose);
int deteksi_jenis_dan_sumber_dari_db(
    sqlite3 *db, AjStmtCache *cache,
    const char *kalimat_norm,
    char *jenis, size_t ukuran_jenis,
    char *sumber, size_t ukuran_sumber);
int analisis_struktur_spok(
    sqlite3 *db,
    TokenKalimat tokens[], int jml_token,
    HasilAnalisisSpok *spok);
void ekstrak_topik_utama(
    sqlite3 *db,
    TokenKalimat tokens[], int jml_token,
    HasilEkstraksiTopik *hasil);
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
void format_jawaban_analitik(
    const char *topik,
    const char *analisis,
    const char *saran_verifikasi,
    char *output, size_t ukuran_output);
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

/* ================================================================== *
 *           DEKLARASI MODUL STEMMING (ajudan_stem.c)                    *
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
 *          DEKLARASI MODUL PENCARIAN (ajudan_cari.c)                    *
 * ================================================================== */

int cari_topik_optimal(
    sqlite3 *db,
    AjStmtCache *cache,
    const char *masukan,
    char *topik_norm, int ukuran,
    KandidatFuzzy kandidat[], int max_kandidat);
int inisialisasi_fts5_kata(sqlite3 *db);

/* ================================================================== *
 *        DEKLARASI MODUL ANALISIS KALIMAT (ajudan_kal.c)                *
 * ================================================================== */

int kenali_tipe_kalimat(
    const TokenKalimat tokens[], int jml_token);
int bangun_depensi_diperluas(
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

#endif
