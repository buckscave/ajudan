/*
 * kalimat.c - Analisis Dependency & Struktur Kalimat
 *
 * Modul untuk parsing struktur kalimat bahasa Indonesia
 * yang diperluas dengan pendeteksian prefiks verbal,
 * penanda kalimat pasif, pola predikat adjektiva,
 * dan strategi ekstraksi topik multi-token.
 *
 * Persyaratan: C89, SQLite3, ajudan.h
 */

#include "ajudan.h"

/* ================================================================== *
 *                         KONSTANTA INTERNAL                            *
 * ================================================================== */

#define JUMLAH_PREFIKS_VERBAL 10
#define JUMLAH_PENANDA_PASIF 3
#define PANJANG_PENANDA_PASIF_MAX 8
#define JUMLAH_PREDIKAT_SIFAT 4
#define PANJANG_PREDIKAT_SIFAT_MAX 8
#define JUMLAH_NOMINA_SUBJEK 5

/* ================================================================== *
 *                        DATA TABEL PREFIKS VERBAL                      *
 * ================================================================== */

static const char *PREFIKS_VERBAL[] = {
    "meng", "meny", "men", "mem", "me",
    "peng", "peny", "pen", "per", "pe"
};
static const int PANJANG_PREFIKS_VERBAL[] = {
    4, 4, 3, 3, 2, 4, 4, 3, 3, 2
};

/* ================================================================== *
 *                         DATA PENANDA PASIF                             *
 * ================================================================== */

static const char *PENANDA_PASIF[] = {
    "diper", "di", "terper", "ter"
};
static const int PANJANG_PENANDA_PASIF[] = {
    5, 2, 7, 3
};

/* ================================================================== *
 *                     DATA POLA PREDIKAT SIFAT                          *
 * ================================================================== */

static const char *POLA_PREDIKAT_SIFAT[] = {
    "sangat", "amat", "paling", "terlalu"
};
static const int PANJANG_POLA_SIFAT[] = {
    6, 4, 6, 7
};

/* ================================================================== *
 *                    DATA PENANDA AWAL SUBJEK                            *
 * ================================================================== */

static const char *PENANDA_AWAL_SUBJEK[] = {
    "saya", "kami", "kita", "dia", "mereka",
    "anda", "beliau", "ia", "aku"
};

/* ================================================================== *
 *                    FUNGSI UTILITAS PENGECEKAN                         *
 * ================================================================== */

static int mulai_dengan(const char *kata, const char *awalan)
{
    int la;

    if (!kata || !awalan) return 0;
    la = (int)strlen(awalan);
    if (strncmp(kata, awalan, (size_t)la) == 0) {
        return la;
    }
    return 0;
}

static int adalah_kata_benda_spesial(const char *kata)
{
    int i;
    static const char *KB_SPESIAL[] = {
        "ini", "itu", "tersebut", "hal", "masalah",
        NULL
    };

    if (!kata || !kata[0]) return 0;

    for (i = 0; KB_SPESIAL[i] != NULL; i++) {
        if (ajudan_strcasecmp(kata, KB_SPESIAL[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int adalah_penanda_awal_subjek(const char *kata)
{
    int i;
    static const int COUNT = 9;

    if (!kata || !kata[0]) return 0;

    for (i = 0; i < COUNT; i++) {
        if (ajudan_strcasecmp(kata, PENANDA_AWAL_SUBJEK[i])
            == 0) {
            return 1;
        }
    }
    return 0;
}

static int adalah_penanda_pasif(const char *kata)
{
    int i;

    if (!kata || !kata[0]) return 0;

    for (i = 0; i < JUMLAH_PENANDA_PASIF; i++) {
        if (mulai_dengan(kata, PENANDA_PASIF[i]) > 0) {
            return 1;
        }
    }
    return 0;
}

static int adalah_penanda_adjektiva(const char *kata)
{
    int i;

    if (!kata || !kata[0]) return 0;

    for (i = 0; i < JUMLAH_PREDIKAT_SIFAT; i++) {
        if (ajudan_strcasecmp(kata,
            POLA_PREDIKAT_SIFAT[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int adalah_nomina_setelah_predikat(
    const char *kata,
    KategoriPOS pos)
{
    if (!kata || !kata[0]) return 0;

    if (pos == POS_NOMINA || pos == POS_PROPER_NOUN) {
        return 1;
    }

    if (adalah_kata_benda_spesial(kata)) {
        return 1;
    }

    if (isupper((unsigned char)kata[0])) {
        return 1;
    }

    return 0;
}

/* ================================================================== *
 *                    PENGENALAN TIPE KALIMAT                            *
 * ================================================================== */

int kenali_tipe_kalimat(
    const TokenKalimat tokens[],
    int jml_token)
{
    int i, ada_tanda_tanya, ada_perintah;

    if (!tokens || jml_token <= 0) return TIPE_KALIMAT_LAIN;

    ada_tanda_tanya = 0;
    ada_perintah = 0;

    if (jml_token > 0) {
        const char *takhir;

        takhir = tokens[jml_token - 1].teks;
        if (strlen(takhir) > 0
            && takhir[strlen(takhir) - 1] == '?') {
            ada_tanda_tanya = 1;
        }
    }

    if (tokens[0].pos == POS_VERBA
        || adalah_penanda_pasif(tokens[0].teks)) {
        if (ada_tanda_tanya) return TIPE_KALIMAT_TANYA_PASIF;
        return TIPE_KALIMAT_PERINTAH;
    }

    for (i = 0; i < jml_token; i++) {
        if (tokens[i].pos == POS_VERBA
            || adalah_penanda_pasif(tokens[i].teks)) {
            if (ada_tanda_tanya) {
                return TIPE_KALIMAT_TANYA_AKTIF;
            }
            return TIPE_KALIMAT_DECLARATIF;
        }
    }

    for (i = 0; i < jml_token; i++) {
        if (tokens[i].pos == POS_ADJEKTIVA
            || adalah_penanda_adjektiva(tokens[i].teks)) {
            if (ada_tanda_tanya) {
                return TIPE_KALIMAT_TANYA_SIFAT;
            }
            return TIPE_KALIMAT_DECLARATIF_SIFAT;
        }
    }

    return TIPE_KALIMAT_LAIN;
}

/* ================================================================== *
 *                       PARSER DEPENDENSI                                *
 * ================================================================== */

int bangun_depensi_diperluas(
    const TokenKalimat tokens[],
    int jml_token,
    SimpulKetergantungan nodes[],
    int verbose)
{
    int i, head_pred, head_subj, head_obj;
    int head_ket, head_mod, j;

    if (!tokens || !nodes || jml_token <= 0) return -1;

    head_pred = -1;
    head_subj = -1;
    head_obj = -1;
    head_ket = -1;
    head_mod = -1;

    for (i = 0; i < jml_token; i++) {
        nodes[i].token = (TokenKalimat *)&tokens[i];
        nodes[i].head_id = -1;
        nodes[i].relasi[0] = '\0';
        nodes[i].children = NULL;
        nodes[i].next = (i + 1 < jml_token)
            ? &nodes[i + 1] : NULL;
    }

    for (i = 0; i < jml_token; i++) {
        if (tokens[i].pos == POS_VERBA) {
            head_pred = i;
            break;
        }
    }

    if (head_pred < 0) {
        for (i = 0; i < jml_token; i++) {
            if (tokens[i].pos == POS_ADJEKTIVA
                || adalah_penanda_adjektiva(tokens[i].teks)) {
                head_pred = i;
                break;
            }
        }
    }

    if (head_pred < 0) {
        head_pred = (jml_token > 0) ? 0 : -1;
    }

    if (head_pred >= 0) {
        nodes[head_pred].head_id = -1;

        if (adalah_penanda_pasif(tokens[head_pred].teks)) {
            snprintf(nodes[head_pred].relasi,
                sizeof(nodes[head_pred].relasi),
                "pred_pasif");
        } else if (tokens[head_pred].pos == POS_VERBA) {
            snprintf(nodes[head_pred].relasi,
                sizeof(nodes[head_pred].relasi),
                "predikat");
        } else if (tokens[head_pred].pos == POS_ADJEKTIVA) {
            snprintf(nodes[head_pred].relasi,
                sizeof(nodes[head_pred].relasi),
                "pred_sifat");
        } else {
            snprintf(nodes[head_pred].relasi,
                sizeof(nodes[head_pred].relasi),
                "akar");
        }
    }

    if (head_pred >= 0 && head_pred == 0) {
        for (i = 1; i < jml_token; i++) {
            if (adalah_nomina_setelah_predikat(
                tokens[i].teks, tokens[i].pos)) {
                nodes[i].head_id = head_pred;
                head_subj = i;
                snprintf(nodes[i].relasi,
                    sizeof(nodes[i].relasi), "subjek");
                break;
            }
        }

        for (i = 1; i < jml_token; i++) {
            if (i == head_subj) continue;
            if (isupper((unsigned char)tokens[i].teks[0])
                && tokens[i].pos == POS_PROPER_NOUN
                && !adalah_kata_benda_spesial(
                    tokens[i].teks)) {
                if (head_subj < 0) {
                    nodes[i].head_id = head_pred;
                    head_subj = i;
                    snprintf(nodes[i].relasi,
                        sizeof(nodes[i].relasi),
                        "subjek");
                } else {
                    nodes[i].head_id = head_subj;
                    snprintf(nodes[i].relasi,
                        sizeof(nodes[i].relasi),
                        "frasa_subjek");
                }
            }
        }
    } else if (head_pred > 0) {
        for (i = 0; i < head_pred; i++) {
            if (adalah_nomina_setelah_predikat(
                tokens[i].teks, tokens[i].pos)) {
                if (head_subj < 0) {
                    nodes[i].head_id = head_pred;
                    head_subj = i;
                    snprintf(nodes[i].relasi,
                        sizeof(nodes[i].relasi),
                        "subjek");
                } else {
                    nodes[i].head_id = head_subj;
                    snprintf(nodes[i].relasi,
                        sizeof(nodes[i].relasi),
                        "frasa_subjek");
                }
            } else if (tokens[i].pos == POS_ADJEKTIVA
                && head_subj >= 0) {
                nodes[i].head_id = head_subj;
                snprintf(nodes[i].relasi,
                    sizeof(nodes[i].relasi),
                    "modif_subjek");
            } else if (tokens[i].pos == POS_NUMERALIA
                && head_subj >= 0) {
                nodes[i].head_id = head_subj;
                snprintf(nodes[i].relasi,
                    sizeof(nodes[i].relasi),
                    "kuant_subjek");
            } else if (nodes[i].head_id == -1) {
                nodes[i].head_id = head_pred;
                snprintf(nodes[i].relasi,
                    sizeof(nodes[i].relasi),
                    "lain");
            }
        }
    }

    if (head_pred >= 0) {
        for (i = head_pred + 1; i < jml_token; i++) {
            if (adalah_nomina_setelah_predikat(
                tokens[i].teks, tokens[i].pos)) {
                if (head_obj < 0) {
                    nodes[i].head_id = head_pred;
                    head_obj = i;
                    snprintf(nodes[i].relasi,
                        sizeof(nodes[i].relasi),
                        "objek");
                } else {
                    nodes[i].head_id = head_obj;
                    snprintf(nodes[i].relasi,
                        sizeof(nodes[i].relasi),
                        "frasa_objek");
                }
            } else if (tokens[i].pos == POS_NUMERALIA
                && head_obj >= 0) {
                nodes[i].head_id = head_obj;
                snprintf(nodes[i].relasi,
                    sizeof(nodes[i].relasi),
                    "kuant_objek");
            } else if (nodes[i].head_id == -1) {
                nodes[i].head_id = head_pred;
                snprintf(nodes[i].relasi,
                    sizeof(nodes[i].relasi),
                    "keterangan");
            }
        }
    }

    for (i = 0; i < jml_token; i++) {
        if (nodes[i].head_id == -1 && i != head_pred) {
            nodes[i].head_id = (head_pred >= 0)
                ? head_pred : 0;
            snprintf(nodes[i].relasi,
                sizeof(nodes[i].relasi), "lain");
        }
    }

    (void)verbose;
    return 0;
}

/* ================================================================== *
 *               KLASIFIKASI TOKEN MENURUT PERAN SPOK                    *
 * ================================================================== */

PeranSPOK klasifikasi_peran_token(
    const TokenKalimat tokens[],
    int jml_token,
    const SimpulKetergantungan nodes[],
    int indeks_token)
{
    const char *relasi;
    PeranSPOK peran;

    if (!tokens || !nodes || indeks_token < 0
        || indeks_token >= jml_token) {
        return PERAN_LAIN;
    }

    peran = PERAN_LAIN;

    if (nodes[indeks_token].head_id == -1) {
        relasi = nodes[indeks_token].relasi;
        if (ajudan_strcasecmp(relasi, "predikat") == 0
            || ajudan_strcasecmp(relasi, "pred_pasif") == 0
            || ajudan_strcasecmp(relasi, "pred_sifat") == 0
            || ajudan_strcasecmp(relasi, "akar") == 0) {
            return PERAN_PREDIKAT;
        }
        return PERAN_LAIN;
    }

    relasi = nodes[indeks_token].relasi;

    if (ajudan_strcasecmp(relasi, "subjek") == 0) {
        peran = PERAN_SUBJEK;
    } else if (ajudan_strcasecmp(relasi,
        "frasa_subjek") == 0) {
        peran = PERAN_FRASA_SUBJEK;
    } else if (ajudan_strcasecmp(relasi,
        "modif_subjek") == 0) {
        peran = PERAN_MODIF_SUBJEK;
    } else if (ajudan_strcasecmp(relasi, "objek") == 0) {
        peran = PERAN_OBJEK;
    } else if (ajudan_strcasecmp(relasi,
        "frasa_objek") == 0) {
        peran = PERAN_FRASA_OBJEK;
    } else if (ajudan_strcasecmp(relasi,
        "keterangan") == 0) {
        peran = PERAN_KETERANGAN;
    } else if (ajudan_strcasecmp(relasi,
        "predikat") == 0) {
        peran = PERAN_PREDIKAT;
    } else if (ajudan_strcasecmp(relasi,
        "pred_pasif") == 0) {
        peran = PERAN_PREDIKAT;
    } else if (ajudan_strcasecmp(relasi,
        "pred_sifat") == 0) {
        peran = PERAN_PREDIKAT;
    } else if (ajudan_strcasecmp(relasi,
        "kuant_subjek") == 0) {
        peran = PERAN_KUANTIFIKASI;
    } else if (ajudan_strcasecmp(relasi,
        "kuant_objek") == 0) {
        peran = PERAN_KUANTIFIKASI;
    } else {
        peran = PERAN_LAIN;
    }

    return peran;
}

/* ================================================================== *
 *               ANALISIS SPOK DIPERLUAS                                  *
 * ================================================================== */

int analisis_spok_diperluas(
    const TokenKalimat tokens[],
    int jml_token,
    const SimpulKetergantungan nodes[],
    HasilAnalisisSpok *spok)
{
    char frasa_subjek[MAX_RESPONS];
    char frasa_objek[MAX_RESPONS];
    char frasa_ket[MAX_RESPONS];
    int pertama_subj, pertama_obj, pertama_ket;
    int i, head_pred;
    PeranSPOK peran;

    if (!tokens || !nodes || !spok || jml_token <= 0) {
        return -1;
    }

    spok->subjek[0] = '\0';
    spok->predikat[0] = '\0';
    spok->objek[0] = '\0';
    spok->keterangan[0] = '\0';
    spok->jml_entitas_dikenal = 0;
    spok->jml_entitas_otomatis = 0;
    spok->struktur = SPOK_TIDAK_DIKENAL;

    frasa_subjek[0] = '\0';
    frasa_objek[0] = '\0';
    frasa_ket[0] = '\0';
    pertama_subj = 1;
    pertama_obj = 1;
    pertama_ket = 1;
    head_pred = -1;

    for (i = 0; i < jml_token; i++) {
        if (nodes[i].head_id == -1
            && (ajudan_strcasecmp(nodes[i].relasi,
                "predikat") == 0
                || ajudan_strcasecmp(nodes[i].relasi,
                    "pred_pasif") == 0
                || ajudan_strcasecmp(nodes[i].relasi,
                    "pred_sifat") == 0
                || ajudan_strcasecmp(nodes[i].relasi,
                    "akar") == 0)) {
            head_pred = i;
            break;
        }
    }

    if (head_pred < 0) {
        for (i = 0; i < jml_token; i++) {
            if (ajudan_strcasecmp(nodes[i].relasi,
                "predikat") == 0
                || ajudan_strcasecmp(nodes[i].relasi,
                    "pred_pasif") == 0
                || ajudan_strcasecmp(nodes[i].relasi,
                    "pred_sifat") == 0) {
                head_pred = i;
                break;
            }
        }
    }

    for (i = 0; i < jml_token; i++) {
        peran = klasifikasi_peran_token(
            tokens, jml_token, nodes, i);

        if (peran == PERAN_PREDIKAT) {
            if (strlen(spok->predikat) > 0) {
                strncat(spok->predikat, " ",
                    sizeof(spok->predikat)
                    - strlen(spok->predikat) - 1);
            }
            strncat(spok->predikat, tokens[i].teks,
                sizeof(spok->predikat)
                - strlen(spok->predikat) - 1);
        } else if (peran == PERAN_SUBJEK
            || peran == PERAN_FRASA_SUBJEK) {
            if (pertama_subj) {
                frasa_subjek[0] = '\0';
                pertama_subj = 0;
            } else {
                strncat(frasa_subjek, " ",
                    sizeof(frasa_subjek)
                    - strlen(frasa_subjek) - 1);
            }
            strncat(frasa_subjek, tokens[i].teks,
                sizeof(frasa_subjek)
                - strlen(frasa_subjek) - 1);
        } else if (peran == PERAN_OBJEK
            || peran == PERAN_FRASA_OBJEK) {
            if (pertama_obj) {
                frasa_objek[0] = '\0';
                pertama_obj = 0;
            } else {
                strncat(frasa_objek, " ",
                    sizeof(frasa_objek)
                    - strlen(frasa_objek) - 1);
            }
            strncat(frasa_objek, tokens[i].teks,
                sizeof(frasa_objek)
                - strlen(frasa_objek) - 1);
        } else if (peran == PERAN_KETERANGAN) {
            if (pertama_ket) {
                frasa_ket[0] = '\0';
                pertama_ket = 0;
            } else {
                strncat(frasa_ket, " ",
                    sizeof(frasa_ket)
                    - strlen(frasa_ket) - 1);
            }
            strncat(frasa_ket, tokens[i].teks,
                sizeof(frasa_ket)
                - strlen(frasa_ket) - 1);
        }
    }

    if (strlen(frasa_subjek) > 0) {
        snprintf(spok->subjek, sizeof(spok->subjek),
            "%s", frasa_subjek);
    }
    if (strlen(frasa_objek) > 0) {
        snprintf(spok->objek, sizeof(spok->objek),
            "%s", frasa_objek);
    }
    if (strlen(frasa_ket) > 0) {
        snprintf(spok->keterangan,
            sizeof(spok->keterangan),
            "%s", frasa_ket);
    }

    if (strlen(spok->subjek) > 0
        && strlen(spok->predikat) > 0
        && strlen(spok->objek) > 0) {
        spok->struktur = SPOK_N_V_N;
    } else if (strlen(spok->subjek) > 0
        && strlen(spok->predikat) > 0) {
        spok->struktur = SPOK_N_V;
    } else if (strlen(spok->predikat) > 0
        && strlen(spok->objek) > 0) {
        spok->struktur = SPOK_V_N;
    } else if (strlen(spok->predikat) > 0) {
        spok->struktur = SPOK_V;
    }

    return 0;
}

/* ================================================================== *
 *                  EKSTRAKSI TOPIK DIPERLUAS                             *
 * ================================================================== */

int ekstrak_topik_diperluas(
    sqlite3 *db,
    const TokenKalimat tokens[],
    int jml_token,
    const SimpulKetergantungan nodes[],
    HasilEkstraksiTopik *hasil)
{
    int i, mulai_topik, j, max_len, ketemu;
    PeranSPOK peran;
    char buf[MAX_INPUT_USER];
    int bi;
    sqlite3_stmt *stmt;
    int rc;

    if (!tokens || jml_token <= 0 || !hasil) return -1;

    hasil->topik_utama[0] = '\0';
    hasil->sub_topik[0] = '\0';
    hasil->kuantitas[0] = '\0';

    buf[0] = '\0';
    bi = 0;

    for (i = 0; i < jml_token; i++) {
        const char *tok;
        int skip;

        tok = tokens[i].teks;

        if (ajudan_strcasecmp(tok, "salah") == 0
            && i + 1 < jml_token
            && ajudan_strcasecmp(tokens[i + 1].teks,
                "satu") == 0) {
            snprintf(hasil->kuantitas,
                sizeof(hasil->kuantitas),
                "salah satu");
            continue;
        }
        if (ajudan_strcasecmp(tok, "beberapa") == 0) {
            snprintf(hasil->kuantitas,
                sizeof(hasil->kuantitas),
                "%s", tok);
            continue;
        }
        if (ajudan_strcasecmp(tok, "semua") == 0
            || ajudan_strcasecmp(tok, "seluruh") == 0) {
            snprintf(hasil->kuantitas,
                sizeof(hasil->kuantitas),
                "%s", tok);
            continue;
        }

        skip = 0;
        if (strlen(hasil->kuantitas) > 0
            && ajudan_strcasecmp(tok,
                hasil->kuantitas) == 0) {
            skip = 1;
        }

        for (j = 0; tok[j]; j++) {
            if (isdigit((unsigned char)tok[j])) {
                skip = 1;
                break;
            }
        }

        if (!skip) {
            size_t needed;

            needed = strlen(tok)
                + ((buf[0] != '\0') ? 1U : 0U);
            if (strlen(buf) + needed < sizeof(buf)) {
                if (buf[0] != '\0') {
                    buf[bi++] = ' ';
                    buf[bi] = '\0';
                }
                if (bi + (int)strlen(tok)
                    < (int)sizeof(buf) - 1) {
                    snprintf(buf + bi,
                        sizeof(buf) - (size_t)bi,
                        "%s", tok);
                    bi += (int)strlen(tok);
                }
            }
        }
    }

    if (nodes) {
        buf[0] = '\0';
        bi = 0;

        mulai_topik = -1;
        for (i = 0; i < jml_token; i++) {
            peran = klasifikasi_peran_token(
                tokens, jml_token, nodes, i);
            if (peran == PERAN_OBJEK
                || peran == PERAN_FRASA_OBJEK) {
                mulai_topik = i;
                break;
            }
        }

        if (mulai_topik < 0) {
            for (i = 0; i < jml_token; i++) {
                peran = klasifikasi_peran_token(
                    tokens, jml_token, nodes, i);
                if (peran == PERAN_SUBJEK
                    || peran == PERAN_FRASA_SUBJEK) {
                    mulai_topik = i;
                    break;
                }
            }
        }

        if (mulai_topik >= 0) {
            for (i = mulai_topik; i < jml_token; i++) {
                peran = klasifikasi_peran_token(
                    tokens, jml_token, nodes, i);

                if (peran == PERAN_OBJEK
                    || peran == PERAN_FRASA_OBJEK
                    || peran == PERAN_SUBJEK
                    || peran == PERAN_FRASA_SUBJEK
                    || peran == PERAN_MODIF_SUBJEK
                    || peran == PERAN_KETERANGAN) {
                    if (bi > 0) {
                        buf[bi++] = ' ';
                        buf[bi] = '\0';
                    }
                    snprintf(buf + bi,
                        sizeof(buf) - (size_t)bi,
                        "%s", tokens[i].teks);
                    bi += (int)strlen(tokens[i].teks);
                } else if (peran == PERAN_PREDIKAT
                    || peran == PERAN_KUANTIFIKASI) {
                    if (bi > 0) break;
                }
            }
        }

        if (buf[0] == '\0') {
            for (i = jml_token - 1; i >= 0; i--) {
                peran = klasifikasi_peran_token(
                    tokens, jml_token, nodes, i);
                if (peran != PERAN_LAIN
                    && peran != PERAN_KUANTIFIKASI
                    && peran != PERAN_PREDIKAT) {
                    snprintf(buf, sizeof(buf),
                        "%s", tokens[i].teks);
                    break;
                }
            }
        }
    }

    if (buf[0] != '\0' && db) {
        char ngram[256];
        int n, ii, kk;
        int jml_cek;

        max_len = 6;
        jml_cek = 0;
        ketemu = 0;

        for (n = max_len; n >= 2 && !ketemu; n--) {
            for (ii = 0; ii + n <= jml_token
                && !ketemu; ii++) {
                ngram[0] = '\0';
                for (kk = 0; kk < n; kk++) {
                    if (kk > 0) {
                        strncat(ngram, " ",
                            sizeof(ngram)
                            - strlen(ngram) - 1);
                    }
                    strncat(ngram,
                        tokens[ii + kk].teks,
                        sizeof(ngram)
                        - strlen(ngram) - 1);
                }

                jml_cek++;
                if (jml_cek > 50) break;

                stmt = NULL;
                rc = sqlite3_prepare_v2(db,
                    "SELECT 1 FROM pengetahuan_umum pu "
                    "JOIN kata k ON pu.id_kata = k.id "
                    "WHERE lower(k.kata) = lower(?) "
                    "LIMIT 1;",
                    -1, &stmt, NULL);
                if (rc == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, ngram,
                        -1, SQLITE_TRANSIENT);
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        snprintf(buf, sizeof(buf),
                            "%s", ngram);
                        ketemu = 1;
                    }
                    sqlite3_finalize(stmt);
                }

                if (ketemu) break;
            }
        }
    }

    trim(buf);
    to_lower_case(buf);

    {
        char *spasi;
        int len_terakhir;

        spasi = strrchr(buf, ' ');
        len_terakhir = (int)strlen(buf);

        if (spasi && len_terakhir > 6) {
            int len_sub, len_top;

            *spasi = '\0';
            len_sub = (int)strlen(buf);
            len_top = (int)strlen(spasi + 1);

            if (len_sub > 1 && len_top >= 2) {
                snprintf(hasil->sub_topik,
                    sizeof(hasil->sub_topik),
                    "%s", buf);
                snprintf(hasil->topik_utama,
                    sizeof(hasil->topik_utama),
                    "%s", spasi + 1);
            } else {
                snprintf(hasil->topik_utama,
                    sizeof(hasil->topik_utama),
                    "%s", buf);
            }
        } else {
            snprintf(hasil->topik_utama,
                sizeof(hasil->topik_utama),
                "%s", buf);
        }
    }

    trim(hasil->topik_utama);
    trim(hasil->sub_topik);

    return 0;
}
