/* ================================================================== *
 * AJUDAN 4.0 - Skema & Data Baru (Database-Driven)                   *
 *                                                                    *
 * File ini berisi semua tabel dan data seed untuk                     *
 * menghapus seluruh hardcode string dari kode C.                      *
 *                                                                    *
 * Standar: SQLite3                                                   *
 * ================================================================== */

/* ================================================================== *
 * 1. KATA TANYA (menggantikan array di klausa.c)                     *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS kata_tanya (
    id INTEGER PRIMARY KEY,
    kata TEXT NOT NULL UNIQUE,
    intent_default TEXT,
    kategori TEXT,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO kata_tanya
    (kata, intent_default, kategori, prioritas) VALUES
('apa',         'definisi',  'umum',       1),
('siapa',       'definisi',  'orang',      2),
('mengapa',     'alasan',    'sebab',      1),
('kenapa',      'alasan',    'sebab',      2),
('bagaimana',   'cara',      'metode',     1),
('kapan',       'cara',      'waktu',      2),
('dimana',      'definisi',  'tempat',     2),
('di mana',     'definisi',  'tempat',     3),
('berapa',      'definisi',  'jumlah',     2),
('mana',        'definisi',  'pilihan',    3),
('macam apa',   'definisi',  'jenis',      2);

/* ================================================================== *
 * 2. PEMISAH KLAUSA (menggantikan DAFTAR_PEMISAH di klausa.c)       *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS pemisah_klausa (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    jenis TEXT,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO pemisah_klausa
    (frasa, jenis, prioritas) VALUES
('sedangkan',    'kontras',        10),
('sambil',       'temporal',        9),
('sebelum',      'temporal',        8),
('setelah',      'temporal',        8),
('walaupun',     'kontras',         9),
('meskipun',     'kontras',         9),
('padahal',      'kontras',         8),
('saatnya',      'temporal',        6),
('kemudian',     'sekuensial',      5),
('maupun',       'konjungsi',       5),
('menurutmu',    'opini',           7),
('menurut kamu', 'opini',           7),
('menurutku',    'opini',           7),
('namun',        'kontras',         6),
('lalu',         'sekuensial',      4),
('sedang',       'temporal',        5),
('atau',         'alternatif',      4),
('tetapi',       'kontras',         5),
('dan',          'koordinatif',     3),
('ketika',       'temporal',        6);

/* ================================================================== *
 * 3. MARKER PENJELASAN (imperatif penjelasan)                        *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS marker_penjelasan (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    intent TEXT DEFAULT 'penjelasan',
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO marker_penjelasan
    (frasa, intent, prioritas) VALUES
('jelaskan',            'penjelasan',  5),
('jabarkan',            'penjelasan',  5),
('beritahu lebih lanjut', 'penjelasan', 4),
('uraikan',             'penjelasan',  5),
('deskripsikan',        'penjelasan',  5),
('ceritakan',           'penjelasan',  4),
('tolong jelaskan',     'penjelasan',  6),
('jelaskan tentang',    'penjelasan',  6),
('jabarkan tentang',    'penjelasan',  6),
('beritahu tentang',    'penjelasan',  5),
('ceritakan tentang',   'penjelasan',  5);

/* ================================================================== *
 * 4. MARKER IMPLISIT (pertanyaan tanpa tanda ?)                      *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS marker_implisit (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO marker_implisit
    (frasa, prioritas) VALUES
('menurutmu',     3),
('menurut kamu',  3),
('menurutku',     3),
('kira-kira',     2),
('kirakira',      2),
('boleh tahu',    4),
('bolehkah',      3),
('mohon penjelasan', 5),
('bisa jelaskan', 4);

/* ================================================================== *
 * 5. KATA LUMPAT / STOP WORDS (gabungan klausa + aturan)             *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS kata_lumpat (
    id INTEGER PRIMARY KEY,
    kata TEXT NOT NULL UNIQUE,
    kategori TEXT,
    keterangan TEXT
);

INSERT OR IGNORE INTO kata_lumpat
    (kata, kategori, keterangan) VALUES
('saya',        'pronomina',    'orang pertama tunggal'),
('kamu',        'pronomina',    'orang kedua tunggal'),
('aku',         'pronomina',    'orang pertama tunggal informal'),
('dia',         'pronomina',    'orang ketiga tunggal'),
('mereka',      'pronomina',    'orang ketiga jamak'),
('kita',        'pronomina',    'orang pertama jamak inklusif'),
('kami',        'pronomina',    'orang pertama jamak eksklusif'),
('anda',        'pronomina',    'orang kedua formal'),
('beliau',      'pronomina',    'orang ketiga hormat'),
('ia',          'pronomina',    'orang ketiga tunggal singkat'),
('nya',         'pronomina',    'klitik posesif'),
('mu',          'pronomina',    'klitik orang kedua'),
('ku',          'pronomina',    'klitik orang pertama'),
('padaku',      'pronomina',    'kepadaku'),
('padamu',      'pronomina',    'kepadamu'),
('buatku',      'pronomina',    'untukku'),
('buatmu',      'pronomina',    'untukmu'),
('tidak',       'negasi',       'penolakan'),
('bukan',       'negasi',       'penolakan predikat'),
('belum',       'negasi',       'belum terjadi'),
('sudah',       'temporal',     'telah terjadi'),
('telah',       'temporal',     'telah terjadi formal'),
('akan',        'temporal',     'akan terjadi'),
('sedang',      'temporal',     'sedang berlangsung'),
('masih',       'temporal',     'berkelanjutan'),
('pernah',      'temporal',     'pengalaman masa lalu'),
('bisa',        'modalitas',    'kemampuan'),
('dapat',       'modalitas',    'kemampuan formal'),
('mampu',       'modalitas',    'kemampuan kuat'),
('mau',         'modalitas',    'keinginan informal'),
('ingin',       'modalitas',    'keinginan'),
('hendak',      'modalitas',    'keinginan formal'),
('mengerti',    'kognisi',      'pemahaman'),
('tahu',        'kognisi',      'pengetahuan'),
('paham',       'kognisi',      'pemahaman'),
('bingung',     'kognisi',      'ketidakpahaman'),
('tentang',     'preposisi',    'topik'),
('untuk',       'preposisi',    'tujuan'),
('dari',        'preposisi',    'asal'),
('dengan',      'preposisi',    'instrumen'),
('pada',        'preposisi',    'lokasi abstrak'),
('di',          'preposisi',    'lokasi konkret'),
('oleh',        'preposisi',    'agen pasif'),
('bagi',        'preposisi',    'penerima'),
('kepada',      'preposisi',    'tujuan orang'),
('ke',          'preposisi',    'arah'),
('dalam',       'preposisi',    'kontainer'),
('agar',        'konjungsi',    'tujuan'),
('supaya',      'konjungsi',    'tujuan'),
('karena',      'konjungsi',    'sebab'),
('kalau',       'konjungsi',    'syarat informal'),
('jika',        'konjungsi',    'syarat formal'),
('waktu',       'temporal',     'waktu umum'),
('saat',        'temporal',     'saat tertentu'),
('dimana',      'temporal',     'lokasi pertanyaan'),
('kemana',      'temporal',     'arah pertanyaan'),
('darimana',    'temporal',     'asal pertanyaan'),
('juga',        'partikel',     'penambahan'),
('pun',         'partikel',     'penekanan'),
('saja',        'partikel',     'pembatasan'),
('hanya',       'partikel',     'pembatasan'),
('lagi',        'partikel',     'kelanjutan'),
('yang',        'partikel',     'relatif'),
('yg',          'partikel',     'relatif singkat'),
('ini',         'demonstratif', 'deiktik dekat'),
('itu',         'demonstratif', 'deiktik jauh'),
('tersebut',    'demonstratif', 'referensi sebelumnya'),
('hal',         'demonstratif', 'perkara'),
('masalah',     'demonstratif', 'permasalahan'),
('apa',         'tanya',        'pertanyaan umum'),
('siapa',       'tanya',        'pertanyaan orang'),
('bagaimana',   'tanya',        'pertanyaan cara'),
('mengapa',     'tanya',        'pertanyaan sebab'),
('kenapa',      'tanya',        'pertanyaan sebab informal'),
('kapan',       'tanya',        'pertanyaan waktu'),
('di mana',     'tanya',        'pertanyaan tempat'),
('berapa',      'tanya',        'pertanyaan jumlah'),
('mana',        'tanya',        'pertanyaan pilihan'),
('macam',       'tanya',        'pertanyaan jenis'),
('apakah',      'tanya',        'pertanyaan ya/tidak'),
('jelaskan',    'imperatif',    'perintah jelaskan'),
('jabarkan',    'imperatif',    'perintah jabarkan'),
('beritahu',    'imperatif',    'perintah beritahu'),
('tolong',      'imperatif',    'permintaan tolong'),
('uraikan',     'imperatif',    'perintah uraikan'),
('deskripsikan','imperatif',    'perintah deskripsikan'),
('ceritakan',   'imperatif',    'perintah ceritakan'),
('mohon',       'imperatif',    'permintaan hormat'),
('berikan',     'imperatif',    'perintah berikan'),
('lebih',       'penegas',      'perbandingan'),
('lanjut',      'penegas',      'kelanjutan'),
('artinya',     'penegas',      'penjelasan makna'),
('maknanya',    'penegas',      'penjelasan makna'),
('kok',         'penegas',      'rasa ingin tahu'),
('ya',          'penegas',      'persetujuan'),
('yah',         'penegas',      'persetujuan informal'),
('dong',        'penegas',      'tekanan informal'),
('deh',         'penegas',      'penutup informal'),
('sih',         'penegas',      'penekanan'),
('lah',         'penegas',      'penekanan akhir'),
('punya',       'posesif',      ' kepemilikan informal'),
('punyaku',     'posesif',      'kepemilikan orang pertama'),
('punyamu',     'posesif',      'kepemilikan orang kedua'),
('mengajak',    'verba_nonentitas','ajakan'),
('menanyakan',  'verba_nonentitas','tanya'),
('meminta',     'verba_nonentitas','permintaan'),
('memilih',     'verba_nonentitas','pemilihan'),
('membeli',     'verba_nonentitas','pembelian'),
('berbelanja',  'verba_nonentitas','pembelian'),
('cari',        'verba_nonentitas','pencarian'),
('mencari',     'verba_nonentitas','pencarian'),
('kesulitan',   'verba_nonentitas','masalah'),
('mendapatkan', 'verba_nonentitas','perolehan'),
('menemukan',   'verba_nonentitas','penemuan'),
('mengetahui',  'verba_nonentitas','pengetahuan'),
('memahami',    'verba_nonentitas','pemahaman'),
('disebut',     'verba_perujukan','referensi pasif'),
('dimaksud',    'verba_perujukan','referensi pasif'),
('dikatakan',   'verba_perujukan','referensi pasif'),
('dinamakan',   'verba_perujukan','referensi pasif'),
('disebutkan',  'verba_perujukan','referensi pasif'),
('dimaksudkan', 'verba_perujukan','referensi pasif'),
('diberi nama', 'verba_perujukan','referensi pasif'),
('dikenal',     'verba_perujukan','referensi pasif'),
('dipanggil',   'verba_perujukan','referensi pasif'),
('seorang',     'determiner',   'penanda orang'),
('sesuatu',     'determiner',   'penanda benda umum'),
('sebuah',      'determiner',   'penanda benda'),
('suatu',       'determiner',   'penanda benda formal'),
('informasi',   'abstrak',      'non-entitas'),
('data',        'abstrak',      'non-entitas'),
('mengenai',    'preposisi',    'tentang formal'),
('soal',        'demonstratif', 'topik informal');

/* ================================================================== *
 * 6. POLA REFERENSI (frasa "tentang itu" di aturan.c)                *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS pola_referensi (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO pola_referensi
    (frasa, prioritas) VALUES
('tentang itu',          5),
('tentang hal itu',      6),
('tentang hal',          4),
('mengenai itu',         5),
('mengenai hal',         4),
('mengenai hal itu',     6),
('soal itu',             5),
('masalah itu',          5),
('hal tersebut',         4),
('hal yang sama',        3),
('lebih lanjut tentang', 4),
('lebih tentang',        3),
('lagi tentang',         3),
('tentang dia',          3),
('tentang mereka',       3);

/* ================================================================== *
 * 7. FRASA PEMBUKA (kalimat pembuka konteks)                         *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS frasa_pembuka (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO frasa_pembuka
    (frasa, prioritas) VALUES
('saya sedang mencari',       3),
('saya sedang kesulitan',     3),
('saya sedang bingung',       3),
('saya sedang belajar',       3),
('saya ingin tahu',           4),
('saya mau tahu',             4),
('saya lagi cari',            3),
('saya lagi mencari',         3),
('saya sedang mempelajari',   3),
('saya sedang memahami',      3),
('saya sedang mengetahui',    3),
('saya penasaran',            4),
('saya butuh',                4),
('saya perlukan',             4),
('saya sedang memikirkan',    3),
('tolong jelaskan',           5),
('coba jelaskan',             5);

/* ================================================================== *
 * 8. PEMISAH REFERENSI (pemisah klausa konteks)                      *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS pemisah_ref (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO pemisah_ref
    (frasa, prioritas) VALUES
(', apakah kamu tahu',    6),
(', kamu tahu',           5),
(', bisa jelaskan',       5),
(', jelaskan',            5),
(', tolong jelaskan',     6),
(', mohon jelaskan',      6),
(', apa itu',             4),
(', mengapa',             4),
(', kenapa',              4),
(', bagaimana',           4);

/* ================================================================== *
 * 9. FALLBACK RESPON (pesan fallback)                                *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS fallback_respon (
    id INTEGER PRIMARY KEY,
    jenis TEXT NOT NULL,
    pesan TEXT NOT NULL,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO fallback_respon
    (jenis, pesan, prioritas) VALUES
('tidak_tahu',
 'Maaf, saya belum tahu tentang ''%s''.',                  1),
('tidak_ditemukan',
 'Maaf, saya tidak menemukan informasi mengenai ''%s''.',  1),
('data_kurang',
 'Maaf, saya tidak memiliki cukup data untuk menjelaskan ''%s''.', 1),
('daftar_kosong',
 'Maaf, daftar ''%s%s%s'' belum tersedia.',                 1),
('jenis_kosong',
 'Maaf, saya belum memiliki data mengenai jenis-jenis ''%s''. Coba tanyakan "jelaskan %s" untuk informasi lebih lengkap.', 1),
('sesi_tidak_ada',
 'Maaf, sesi percakapan tidak ditemukan.',                 1),
('topik_lupa',
 'Maksud Anda topik apa? Saya lupa topik pembicaraan kita sebelumnya.', 1),
('konteks_tidak_jelas',
 'Maaf, saya tidak yakin apa yang Anda maksud dengan ''%s'' dalam konteks ini.', 1),
('kalimat_kosong',
 'Maaf, kalimat kosong.',                                  1),
('sistem_belum_siap',
 'Maaf, sistem belum siap.',                               1),
('input_gagal',
 'Maaf, input tidak dapat diproses.',                      1),
('perintah_darab',
 'Perintah tidak dikenali.',                                1),
('matematika_gagal',
 'Maaf, saya tidak bisa menghitung dari informasi yang diberikan.', 1),
('matematika_nol',
 'Maaf, tidak dapat melakukan pembagian dengan nol.',      1),
('waktu_konfirmasi',
 '%s memang %s.',                                          1),
('waktu_koreksi',
 'sekarang adalah hari %s, jadi %s bukan %s melainkan %s.', 1),
('waktu_informasi',
 'sekarang adalah hari %s, jadi %s adalah hari %s.', 1);

/* ================================================================== *
 * 10. SAPAAN WAKTU (sapaan berbasis waktu)                           *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS sapaan_waktu (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    jam_mulai INTEGER DEFAULT 0,
    jam_akhir INTEGER DEFAULT 24,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO sapaan_waktu
    (frasa, jam_mulai, jam_akhir, prioritas) VALUES
('selamat pagi',       5,  10,  1),
('selamat siang',     10,  15,  1),
('selamat sore',      15,  17,  1),
('selamat malam',     18,   5,  2),
('selamat subuh',      4,   6,  3),
('selamat tengah hari',11, 13,  2),
('selamat petang',    16,  18,  2);

/* ================================================================== *
 * 11. PENANDA KALIMAT (dari kalimat.c + aturan.c)                   *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS penanda_kalimat (
    id INTEGER PRIMARY KEY,
    jenis TEXT NOT NULL,
    kata TEXT NOT NULL,
    keterangan TEXT,
    prioritas INTEGER DEFAULT 1,
    UNIQUE(jenis, kata)
);

INSERT OR IGNORE INTO penanda_kalimat
    (jenis, kata, keterangan, prioritas) VALUES
('prefiks_verbal',  'meng',   'awalan verba aktif',    1),
('prefiks_verbal',  'meny',   'awalan verba aktif',    1),
('prefiks_verbal',  'men',    'awalan verba aktif',    1),
('prefiks_verbal',  'mem',    'awalan verba aktif',    1),
('prefiks_verbal',  'me',     'awalan verba aktif',    1),
('prefiks_verbal',  'peng',   'awalan nomina agen',    2),
('prefiks_verbal',  'peny',   'awalan nomina agen',    2),
('prefiks_verbal',  'pen',    'awalan nomina agen',    2),
('prefiks_verbal',  'per',    'awalan nomina agen',    2),
('prefiks_verbal',  'pe',     'awalan nomina agen',    2),
('penanda_pasif',   'diper',  'penanda kalimat pasif', 1),
('penanda_pasif',   'di',     'penanda kalimat pasif', 1),
('penanda_pasif',   'terper', 'penanda kalimat pasif', 1),
('penanda_pasif',   'ter',    'penanda kalimat pasif', 1),
('predikat_sifat',  'sangat', 'penguat adjektiva',     1),
('predikat_sifat',  'amat',   'penguat adjektiva',     1),
('predikat_sifat',  'paling', 'superlatif',            1),
('predikat_sifat',  'terlalu','berlebihan',            1),
('penanda_subjek',  'saya',   'subjek orang pertama',  1),
('penanda_subjek',  'kami',   'subjek orang pertama jamak', 1),
('penanda_subjek',  'kita',   'subjek inklusif',      1),
('penanda_subjek',  'dia',    'subjek orang ketiga',  1),
('penanda_subjek',  'mereka', 'subjek orang ketiga jamak', 1),
('penanda_subjek',  'anda',   'subjek orang kedua formal', 1),
('penanda_subjek',  'beliau', 'subjek orang ketiga hormat', 1),
('penanda_subjek',  'ia',     'subjek orang ketiga singkat', 1),
('penanda_subjek',  'aku',    'subjek orang pertama informal', 1),
('kata_benda_spesial', 'ini',     'referensi dekat', 1),
('kata_benda_spesial', 'itu',     'referensi jauh',  1),
('kata_benda_spesial', 'tersebut','referensi sebelumnya', 1),
('kata_benda_spesial', 'hal',     'perkara',        1),
('kata_benda_spesial', 'masalah', 'permasalahan',    1);

/* ================================================================== *
 * 12. AFFIX POS RULE (fallback kelas kata dari afiks)                *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS affix_pos_rule (
    id INTEGER PRIMARY KEY,
    jenis TEXT NOT NULL,
    pola TEXT NOT NULL,
    kelas_hasil INTEGER,
    panjang INTEGER DEFAULT 2,
    prioritas INTEGER DEFAULT 1,
    UNIQUE(jenis, pola)
);

INSERT OR IGNORE INTO affix_pos_rule
    (jenis, pola, kelas_hasil, panjang, prioritas) VALUES
('awalan', 'me',   2, 2, 1),
('awalan', 'ber',  2, 3, 1),
('awalan', 'di',   2, 2, 1),
('awalan', 'ter',  2, 3, 1),
('awalan', 'pe',   1, 2, 2),
('awalan', 'pen',  1, 3, 2),
('awalan', 'peng', 1, 4, 2),
('awalan', 'peny', 1, 4, 2),
('akhiran', 'an',  1, 2, 2),
('akhiran', 'i',   2, 1, 2),
('akhiran', 'kan', 2, 3, 2);

/* ================================================================== *
 * 13. KATA KOPULA (kata hubung predikat)                             *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS kata_kopula (
    id INTEGER PRIMARY KEY,
    kata TEXT NOT NULL UNIQUE,
    ragam TEXT DEFAULT 'baku',
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO kata_kopula
    (kata, ragam, prioritas) VALUES
('adalah',      'baku',    1),
('merupakan',   'baku',    2),
('yakni',       'baku',    3),
('ialah',       'baku',    2),
('yaitu',       'baku',    3),
('artinya',     'baku',    4),
('maknanya',    'baku',    5),
('sama dengan', 'baku',    4),
('barupa',      'tidak_baku', 6),
('berupa',      'baku',    5),
('terdiri dari','baku',    5),
('sebagai',     'baku',    6);

/* ================================================================== *
 * 14. KONJUNGSI RESPON (kata sambung respons)                       *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS konjungsi_respon (
    id INTEGER PRIMARY KEY,
    kata TEXT NOT NULL UNIQUE,
    jenis TEXT,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO konjungsi_respon
    (kata, jenis, prioritas) VALUES
('dan',         'koordinatif',  1),
('serta',       'koordinatif',  2),
('atau',        'alternatif',   2),
('tetapi',      'kontras',      2),
('namun',       'kontras',      2),
('meskipun',    'kontras',      3),
('walaupun',    'kontras',      3),
('karena',      'kausal',       1),
('oleh karena', 'kausal',       2),
('sehingga',    'konsekutif',   2),
('maka',        'konsekutif',   1),
('jika',        'kondisional',  1),
('kalau',       'kondisional',  2),
('untuk',       'final',        1),
('supaya',      'final',        2),
('agar',        'final',        2),
('selain itu',  'adisi',        3),
('di samping itu', 'adisi',     3);

/* ================================================================== *
 * 15. TANDA BACA (untuk penyusunan kalimat)                         *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS tanda_baca (
    id INTEGER PRIMARY KEY,
    tanda TEXT NOT NULL UNIQUE,
    konteks TEXT,
    spasi_sebelum INTEGER DEFAULT 0,
    spasi_sesudah INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO tanda_baca
    (tanda, konteks, spasi_sebelum, spasi_sesudah) VALUES
('.',  'akhiran kalimat',  0, 0),
(',',  'pemisah klausa',   0, 1),
('?',  'akhiran tanya',     0, 0),
('!',  'akhiran seru',      0, 0),
(':',  'pendahuluman',      0, 1),
(';',  'pemisah kalimat',   0, 1),
('-',  'daftar item',       1, 1),
('(',  'kurung buka',       0, 0),
(')',  'kurung tutup',      0, 1),
('"',  'kutip',             0, 0);

/* ================================================================== *
 * 16. REFERENSI WAKTU (penalaran temporal)                           *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS referensi_waktu (
    id INTEGER PRIMARY KEY,
    frasa TEXT NOT NULL UNIQUE,
    offset_hari INTEGER,
    kategori TEXT,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO referensi_waktu
    (frasa, offset_hari, kategori, prioritas) VALUES
('kemarin',            -1, 'relatif_hari',  1),
('hari ini',            0, 'relatif_hari',  1),
('besok',               1, 'relatif_hari',  1),
('lusa',                2, 'relatif_hari',  2),
('minggu lalu',        -7, 'relatif_minggu',2),
('minggu depan',        7, 'relatif_minggu',2),
('bulan lalu',         -30,'relatif_bulan', 3),
('bulan depan',         30,'relatif_bulan', 3),
('tahun lalu',        -365,'relatif_tahun', 3),
('tahun depan',        365,'relatif_tahun', 3),
('tadinya',            -1, 'implisit',      4),
('sekarang',            0, 'absolut',        1),
('tadi',               -1, 'implisit',      3);

/* ================================================================== *
 * 17. HARI MINGGU (nama hari)                                       *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS hari_minggu (
    id INTEGER PRIMARY KEY,
    nama TEXT NOT NULL UNIQUE,
    urutan INTEGER NOT NULL,
    ragam TEXT DEFAULT 'baku'
);

INSERT OR IGNORE INTO hari_minggu
    (nama, urutan, ragam) VALUES
('senin',   1, 'baku'),
('selasa',  2, 'baku'),
('rabu',    3, 'baku'),
('kamis',   4, 'baku'),
('jumat',   5, 'baku'),
('sabtu',   6, 'baku'),
('minggu',  7, 'baku'),
('ahad',    7, 'baku');

/* ================================================================== *
 * 18. KATA BILANGAN (angka Indonesia)                                *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS kata_bilangan (
    id INTEGER PRIMARY KEY,
    kata TEXT NOT NULL UNIQUE,
    nilai INTEGER NOT NULL,
    ragam TEXT DEFAULT 'baku'
);

INSERT OR IGNORE INTO kata_bilangan
    (kata, nilai, ragam) VALUES
('nol',      0,  'baku'),
('kosong',   0,  'tidak_baku'),
('satu',     1,  'baku'),
('se',       1,  'tidak_baku'),
('dua',      2,  'baku'),
('tiga',     3,  'baku'),
('empat',    4,  'baku'),
('lima',     5,  'baku'),
('enam',     6,  'baku'),
('tujuh',    7,  'baku'),
('delapan',  8,  'baku'),
('sembilan', 9,  'baku'),
('sepuluh', 10,  'baku'),
('sebelas', 11,  'baku'),
('belas',   10,  'imbuhan'),
('puluh',   10,  'imbuhan'),
('ratus',  100,  'imbuhan'),
('ribu',  1000,  'imbuhan'),
('juta',1000000,'imbuhan');

/* ================================================================== *
 * 19. KATA OPERASI MATEMATIKA                                        *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS kata_operasi_mat (
    id INTEGER PRIMARY KEY,
    kata TEXT NOT NULL UNIQUE,
    operasi TEXT NOT NULL,
    kategori TEXT,
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO kata_operasi_mat
    (kata, operasi, kategori, prioritas) VALUES
('tambah',     'tambah',  'aritmatika',  1),
('ditambah',   'tambah',  'aritmatika',  1),
('plus',       'tambah',  'aritmatika',  2),
('kurang',     'kurang',  'aritmatika',  1),
('dikurangi',  'kurang',  'aritmatika',  1),
('dikurang',   'kurang',  'aritmatika',  2),
('dipotong',   'kurang',  'aritmatika',  3),
('berkurang',  'kurang',  'aritmatika',  3),
('hilang',     'kurang',  'aritmatika',  4),
('jatuh',      'kurang',  'aritmatika',  4),
('pecah',      'kurang',  'aritmatika',  4),
('rusak',      'kurang',  'aritmatika',  4),
('mati',       'kurang',  'aritmatika',  5),
('kali',       'kali',    'aritmatika',  1),
('dikali',     'kali',    'aritmatika',  1),
('dikalikan',  'kali',    'aritmatika',  2),
('bagi',       'bagi',    'aritmatika',  1),
('dibagi',     'bagi',    'aritmatika',  1),
('dibagikan',  'bagi',    'aritmatika',  2),
('sisa',       'modulus', 'aritmatika',  1),
('mod',        'modulus', 'aritmatika',  2),
('sisa bagi',  'modulus', 'aritmatika',  1),
('berapa',     'tanya',   'aritmatika',  1),
('berapa',     'tanya',   'aritmatika',  1),
('hasilnya',   'tanya',   'aritmatika',  2),
('total',      'tanya',   'aritmatika',  2),
('jumlah',     'tanya',   'aritmatika',  2),
('membeli',    'inisial', 'konteks',     3),
('memiliki',   'inisial', 'konteks',     3),
('punya',      'inisial', 'konteks',     3),
('ada',        'inisial', 'konteks',     4),
('sisanya',    'tanya',   'aritmatika',  3),
('tersisa',    'tanya',   'aritmatika',  3),
('masih',      'tanya',   'aritmatika',  4);

/* ================================================================== *
 * 20. POLA RESPON (blueprint SPOK untuk merangkai jawaban)          *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS pola_respon (
    id INTEGER PRIMARY KEY,
    intent TEXT NOT NULL,
    konteks TEXT NOT NULL,
    ragam TEXT DEFAULT 'baku',
    prioritas INTEGER DEFAULT 1
);

INSERT OR IGNORE INTO pola_respon
    (intent, konteks, ragam, prioritas) VALUES
('definisi', 'normal',      'baku',  1),
('definisi', 'dengan_ket',  'baku',  2),
('definisi', 'dengan_judul','baku',  3),
('definisi', 'fallback',    'baku',  4),
('penjelasan', 'lengkap',   'baku',  1),
('penjelasan', 'ringkas',   'baku',  2),
('penjelasan', 'bot',       'baku',  3),
('penjelasan', 'fallback',  'baku',  4),
('arti',      'normal',     'baku',  1),
('arti',      'dengan_ket', 'baku',  2),
('arti',      'fallback',   'baku',  3),
('sebab',     'normal',     'baku',  1),
('sebab',     'fallback',   'baku',  2),
('jenis',     'bertingkat', 'baku',  1),
('jenis',     'semantik',   'baku',  2),
('jenis',     'fallback',   'baku',  3),
('daftar',    'normal',     'baku',  1),
('daftar',    'fallback',   'baku',  2),
('analitik',  'normal',     'baku',  1),
('analitik',  'bot',        'baku',  2),
('analitik',  'fallback',   'baku',  3),
('sapaan',    'waktu',      'baku',  1),
('sapaan',    'normal',     'baku',  2),
('sapaan',    'fallback',   'baku',  3),
('lanjutan',  'normal',     'baku',  1),
('lanjutan',  'fallback',   'baku',  2),
('matematika','normal',     'baku',  1),
('matematika','fallback',   'baku',  2),
('waktu',     'normal',     'baku',  1),
('waktu',     'koreksi',    'baku',  2),
('waktu',     'fallback',   'baku',  3);

/* ================================================================== *
 * 21. KOMPONEN RESPON (slot kata dalam pola)                        *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS komponen_respon (
    id INTEGER PRIMARY KEY,
    pola_id INTEGER NOT NULL,
    posisi INTEGER NOT NULL,
    peran_spok TEXT NOT NULL,
    sumber_data TEXT,
    konten_tetap TEXT,
    bentuk TEXT DEFAULT 'dasar',
    spasi_sebelum INTEGER DEFAULT 1,
    spasi_sesudah INTEGER DEFAULT 1,
    FOREIGN KEY(pola_id) REFERENCES pola_respon(id)
);

/* Komponen untuk definisi normal (pola_id=1) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(1, 1, 'objek',  'input_topik', NULL, 'dasar', 0, 1),
(1, 2, 'predikat','tetap',      'adalah','kopula',1, 1),
(1, 3, 'objek',  'db_ringkasan',NULL, 'dasar',1, 0),
(1, 4, 'keterangan','tetap',    '.',     'tanda_baca',0, 0);

/* Komponen untuk definisi dengan keterangan (pola_id=2) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(2, 1, 'objek',     'input_topik', NULL,    'dasar', 0, 1),
(2, 2, 'predikat',  'tetap',       'adalah','kopula',1, 1),
(2, 3, 'objek',     'db_ringkasan',NULL,    'dasar', 1, 1),
(2, 4, 'keterangan','input_keterangan',NULL, 'dasar',1, 0),
(2, 5, 'keterangan','tetap',       '.',     'tanda_baca',0, 0);

/* Komponen untuk sapaan waktu (pola_id=24) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(24, 1, 'predikat', 'sapaan_waktu', NULL,   'dasar', 0, 1),
(24, 2, 'keterangan','tetap',      '! Ada yang bisa saya bantu?',
     'dasar', 0, 0);

/* Komponen untuk sapaan normal (pola_id=25) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(25, 1, 'predikat','tetap', 'Halo! Saya AJUDAN, asisten virtual Anda.',
     'dasar', 0, 0);

/* Komponen untuk matematika normal (pola_id=29) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(29, 1, 'keterangan','mat_langkah', NULL,  'dasar', 0, 0);

/* Komponen untuk waktu koreksi (pola_id=31) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(31, 1, 'keterangan','waktu_koreksi', NULL,  'dasar', 0, 0);

/* ================================================================== *
 * KOMPONEN RESPON TAMBAHAN (Task 2-a)                                *
 * pola_id yang sebelumnya belum punya komponen:                     *
 * 3-23, 26-28, 30                                                    *
 * ================================================================== */

/* Komponen untuk definisi/dengan_judul (pola_id=3) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(3, 1, 'subjek',  'input_topik', NULL,    'judul', 0, 1),
(3, 2, 'predikat','tetap',       'adalah','kopula',1, 1),
(3, 3, 'objek',  'db_ringkasan', NULL,    'dasar', 1, 0),
(3, 4, 'keterangan','tetap',     '.',     'tanda_baca',0, 0);

/* Komponen untuk definisi/fallback (pola_id=4) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(4, 1, 'predikat','tetap',
     'Maaf, saya belum tahu tentang','dasar', 0, 1),
(4, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(4, 3, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk penjelasan/lengkap (pola_id=5) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(5, 1, 'subjek',  'input_topik', NULL,    'dasar', 0, 1),
(5, 2, 'predikat','tetap',       'adalah','kopula',1, 1),
(5, 3, 'objek',  'db_ringkasan', NULL,    'dasar', 1, 1),
(5, 4, 'keterangan','db_penjelasan', NULL,'dasar', 1, 0),
(5, 5, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk penjelasan/ringkas (pola_id=6) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(6, 1, 'subjek',  'input_topik', NULL,    'dasar', 0, 1),
(6, 2, 'predikat','tetap',       'adalah','kopula',1, 1),
(6, 3, 'objek',  'db_ringkasan', NULL,    'dasar', 1, 0),
(6, 4, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk penjelasan/bot (pola_id=7) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(7, 1, 'subjek',  'tetap', 'Saya', 'dasar', 0, 1),
(7, 2, 'predikat','tetap', 'adalah','kopula', 1, 1),
(7, 3, 'objek',  'db_ringkasan', NULL, 'dasar', 1, 0),
(7, 4, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk penjelasan/fallback (pola_id=8) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(8, 1, 'predikat','tetap',
     'Maaf, saya belum bisa menjelaskan','dasar', 0, 1),
(8, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(8, 3, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk arti/normal (pola_id=9) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(9, 1, 'subjek',  'input_topik', NULL,    'dasar', 0, 1),
(9, 2, 'predikat','tetap',       'berarti','kopula',1, 1),
(9, 3, 'objek',  'db_ringkasan', NULL,    'dasar', 1, 0),
(9, 4, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk arti/dengan_ket (pola_id=10) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(10, 1, 'subjek',  'input_topik', NULL,    'dasar', 0, 1),
(10, 2, 'predikat','tetap',       'berarti','kopula',1, 1),
(10, 3, 'objek',  'db_ringkasan', NULL,    'dasar', 1, 1),
(10, 4, 'keterangan','input_keterangan',NULL,'dasar',1, 0),
(10, 5, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk arti/fallback (pola_id=11) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(11, 1, 'predikat','tetap',
     'Maaf, saya belum tahu arti','dasar', 0, 1),
(11, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(11, 3, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk sebab/normal (pola_id=12) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(12, 1, 'subjek',  'input_topik', NULL, 'dasar', 0, 1),
(12, 2, 'keterangan','tetap',
     'terjadi karena', 'dasar', 1, 1),
(12, 3, 'objek',  'db_penjelasan', NULL, 'dasar', 1, 0),
(12, 4, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk sebab/fallback (pola_id=13) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(13, 1, 'predikat','tetap',
     'Maaf, saya belum tahu penyebab','dasar', 0, 1),
(13, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(13, 3, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk jenis/bertingkat (pola_id=14) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(14, 1, 'subjek',  'tetap', 'Jenis-jenis','dasar',0, 1),
(14, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(14, 3, 'keterangan','tetap', ':', 'dasar', 0, 1),
(14, 4, 'keterangan','db_ringkasan', NULL, 'dasar',1, 0),
(14, 5, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk jenis/semantik (pola_id=15) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(15, 1, 'subjek',  'tetap','Beberapa jenis','dasar',0, 1),
(15, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 1),
(15, 3, 'predikat','tetap', 'adalah','kopula', 1, 1),
(15, 4, 'keterangan','db_ringkasan',NULL, 'dasar', 1, 0),
(15, 5, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk jenis/fallback (pola_id=16) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(16, 1, 'predikat','tetap',
     'Maaf, saya belum memiliki data jenis-jenis',
     'dasar', 0, 1),
(16, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(16, 3, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk daftar/normal (pola_id=17) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(17, 1, 'subjek',  'input_topik', NULL, 'dasar', 0, 0),
(17, 2, 'keterangan','tetap', ':', 'dasar', 0, 1),
(17, 3, 'keterangan','db_ringkasan', NULL, 'dasar', 1, 0),
(17, 4, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk daftar/fallback (pola_id=18) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(18, 1, 'predikat','tetap','Maaf, daftar','dasar',0, 1),
(18, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 1),
(18, 3, 'keterangan','tetap',
     'belum tersedia', 'dasar', 1, 0),
(18, 4, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk sapaan/waktu (pola_id=22) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(22, 1, 'predikat', 'sapaan_waktu', NULL,
     'dasar', 0, 1),
(22, 2, 'keterangan','tetap',
     '! Ada yang bisa saya bantu?',
     'dasar', 0, 0);

/* Komponen untuk sapaan/normal (pola_id=23) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(23, 1, 'predikat','tetap',
     'Halo! Saya AJUDAN, asisten virtual Anda.',
     'dasar', 0, 0);

/* Komponen untuk analitik/normal (pola_id=19) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(19, 1, 'subjek',  'tetap','Analisis', 'dasar', 0, 1),
(19, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(19, 3, 'keterangan','tetap', ':', 'dasar', 0, 1),
(19, 4, 'keterangan','db_penjelasan',NULL, 'dasar',1, 0),
(19, 5, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk analitik/bot (pola_id=20) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(20, 1, 'subjek',  'tetap', 'Saya adalah','dasar',0, 1),
(20, 2, 'predikat','db_ringkasan', NULL, 'dasar', 1, 0),
(20, 3, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk analitik/fallback (pola_id=21) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(21, 1, 'predikat','tetap',
     'Maaf, saya belum bisa menganalisis','dasar',0, 1),
(21, 2, 'objek',  'input_topik', NULL, 'dasar', 1, 0),
(21, 3, 'keterangan','tetap', '.', 'tanda_baca', 0, 0);

/* Komponen untuk lanjutan/fallback (pola_id=26) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(26, 1, 'predikat','tetap',
     'Maaf, saya tidak yakin topik yang dimaksud.',
     'dasar', 0, 0);

/* Komponen untuk matematika/normal (pola_id=27) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(27, 1, 'keterangan','mat_langkah', NULL,  'dasar', 0, 0);

/* Komponen untuk matematika/fallback (pola_id=28) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(28, 1, 'predikat','tetap',
     'Maaf, saya tidak bisa menghitung dari informasi yang diberikan.', 'dasar', 0, 0);

/* Komponen untuk waktu/koreksi (pola_id=30) */
INSERT OR IGNORE INTO komponen_respon
    (pola_id, posisi, peran_spok, sumber_data,
     konten_tetap, bentuk, spasi_sebelum, spasi_sesudah)
VALUES
(30, 1, 'keterangan','waktu_koreksi', NULL,  'dasar', 0, 0);

/* ================================================================== *
 * 22. INDEKS UNTUK TABEL BARU                                        *
 * ================================================================== */

CREATE INDEX IF NOT EXISTS idx_kata_tanya_kata
    ON kata_tanya(kata);
CREATE INDEX IF NOT EXISTS idx_pemisah_klausa_frasa
    ON pemisah_klausa(frasa);
CREATE INDEX IF NOT EXISTS idx_kata_lumpat_kata
    ON kata_lumpat(kata);
CREATE INDEX IF NOT EXISTS idx_kata_lumpat_kategori
    ON kata_lumpat(kategori);
CREATE INDEX IF NOT EXISTS idx_pola_referensi_frasa
    ON pola_referensi(frasa);
CREATE INDEX IF NOT EXISTS idx_frasa_pembuka_frasa
    ON frasa_pembuka(frasa);
CREATE INDEX IF NOT EXISTS idx_sapaan_waktu_frasa
    ON sapaan_waktu(frasa);
CREATE INDEX IF NOT EXISTS idx_penanda_kalimat_jenis
    ON penanda_kalimat(jenis);
CREATE INDEX IF NOT EXISTS idx_referensi_waktu_frasa
    ON referensi_waktu(frasa);
CREATE INDEX IF NOT EXISTS idx_hari_minggu_nama
    ON hari_minggu(nama);
CREATE INDEX IF NOT EXISTS idx_kata_bilangan_kata
    ON kata_bilangan(kata);
CREATE INDEX IF NOT EXISTS idx_kata_operasi_mat_kata
    ON kata_operasi_mat(kata);
CREATE INDEX IF NOT EXISTS idx_kata_operasi_mat_operasi
    ON kata_operasi_mat(operasi);
CREATE INDEX IF NOT EXISTS idx_pola_respon_intent
    ON pola_respon(intent);
CREATE INDEX IF NOT EXISTS idx_komponen_respon_pola
    ON komponen_respon(pola_id);
CREATE INDEX IF NOT EXISTS idx_fallback_respon_jenis
    ON fallback_respon(jenis);
CREATE INDEX IF NOT EXISTS idx_kata_kopula_ragam
    ON kata_kopula(ragam);
CREATE INDEX IF NOT EXISTS idx_konjungsi_respon_jenis
    ON konjungsi_respon(jenis);
CREATE INDEX IF NOT EXISTS idx_marker_penjelasan_frasa
    ON marker_penjelasan(frasa);
CREATE INDEX IF NOT EXISTS idx_marker_implisit_frasa
    ON marker_implisit(frasa);
CREATE INDEX IF NOT EXISTS idx_affix_pos_rule_jenis
    ON affix_pos_rule(jenis);
CREATE INDEX IF NOT EXISTS idx_penanda_kalimat_kata
    ON penanda_kalimat(kata);

/* ================================================================== *
 * 22. DATA KATA (kosakata untuk pengetahuan)                           *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS kata (
    id INTEGER PRIMARY KEY,
    kata TEXT NOT NULL UNIQUE,
    kelas_id INTEGER,
    ragam_id INTEGER,
    bidang_id INTEGER
);

INSERT OR IGNORE INTO kata (id, kata, kelas_id, ragam_id, bidang_id) VALUES
(1, 'komputer', 1, 1, 1),
(2, 'internet', 1, 1, 1),
(3, 'algoritma', 1, 5, 3),
(4, 'program', 1, 1, 1),
(5, 'database', 1, 2, 1),
(6, 'robot', 1, 1, 1),
(7, 'kecerdasan', 1, 1, 1),
(8, 'sistem', 1, 1, 1),
(9, 'aplikasi', 1, 1, 1),
(10, 'jaringan', 1, 1, 1),
(11, 'data', 1, 1, 1),
(12, 'informasi', 1, 1, 1),
(13, 'air', 1, 1, 2),
(14, 'bumi', 1, 1, 6),
(15, 'matahari', 1, 1, 2),
(16, 'bulan', 1, 1, 2),
(17, 'gravitasi', 1, 1, 2),
(18, 'energi', 1, 1, 2),
(19, 'cahaya', 1, 1, 2),
(20, 'fotosintesis', 1, 1, 2),
(21, 'evolusi', 1, 1, 2),
(22, 'ekosistem', 1, 1, 16),
(23, 'atom', 1, 1, 2),
(24, 'oksigen', 1, 1, 2),
(25, 'manusia', 1, 1, 20),
(26, 'sel', 1, 1, 20),
(27, 'hewan', 1, 1, 20),
(28, 'tumbuhan', 1, 1, 20),
(29, 'vitamin', 1, 1, 11),
(30, 'otak', 1, 1, 20),
(31, 'jantung', 1, 1, 20),
(32, 'bahasa', 1, 1, 7),
(33, 'kata', 1, 1, 7),
(34, 'kalimat', 1, 1, 7),
(35, 'sinonim', 1, 1, 7),
(36, 'antonim', 1, 1, 7),
(37, 'imbuhan', 1, 1, 7),
(38, 'indonesia', 1, 1, 4),
(39, 'batik', 1, 1, 4),
(40, 'budaya', 1, 1, 4),
(41, 'sejarah', 1, 1, 5),
(42, 'matematika', 1, 1, 3),
(43, 'angka', 1, 1, 3),
(44, 'aljabar', 1, 1, 3),
(45, 'gunung', 1, 1, 6),
(46, 'laut', 1, 1, 6),
(47, 'hutan', 1, 1, 16),
(48, 'pengetahuan', 1, 1, 12),
(49, 'pendidikan', 1, 1, 12),
(50, 'kesehatan', 1, 1, 11),
(51, 'ekonomi', 1, 1, 13),
(52, 'uang', 1, 1, 13),
(53, 'obat', 1, 1, 11),
(54, 'olahraga', 1, 1, 10),
(55, 'musik', 1, 1, 19),
(56, 'makanan', 1, 1, 18),
(57, 'rumah', 1, 1, NULL),
(58, 'laptop', 1, 1, 1),
(59, 'telepon', 1, 1, 1),
(60, 'website', 1, 2, 1),
(61, 'server', 1, 2, 1),
(62, 'prosesor', 1, 1, 1),
(63, 'memori', 1, 1, 1),
(64, 'kode', 1, 1, 1);

/* ================================================================== *
 * 23. PENGETAHUAN UMUM (data ensiklopedia untuk bot)                    *
 * ================================================================== */

CREATE TABLE IF NOT EXISTS pengetahuan_umum (
    id INTEGER PRIMARY KEY,
    id_kata INTEGER,
    id_bidang INTEGER,
    judul TEXT,
    ringkasan TEXT,
    penjelasan TEXT,
    saran TEXT,
    sumber TEXT,
    id_hubungan TEXT,
    FOREIGN KEY(id_kata) REFERENCES kata(id)
);

INSERT OR IGNORE INTO pengetahuan_umum (id_kata, id_bidang, judul, ringkasan, penjelasan, saran, sumber, id_hubungan) VALUES
(2, 1, 'Internet', 'Jaringan komputer global yang saling terhubung menggunakan protokol standar untuk bertukar data dan informasi di seluruh dunia', 'Internet adalah sistem jaringan komputer global yang menghubungkan miliaran perangkat di seluruh dunia. Internet menggunakan protokol TCP/IP sebagai standar komunikasi utama, memungkinkan pertukaran data, informasi, dan layanan secara real-time. Internet telah mengubah cara manusia berkomunikasi, bekerja, belajar, dan mengakses hiburan secara fundamental.', 'Pelajari lebih lanjut tentang protokol jaringan dan keamanan internet', 'umum', 'definisi'),
(10, 1, 'Jaringan Komputer', 'Sistem yang menghubungkan dua atau lebih perangkat komputer agar dapat saling bertukar data dan berbagi sumber daya', 'Jaringan komputer adalah infrastruktur yang memungkinkan perangkat-perangkat komputer untuk saling berkomunikasi dan berbagi sumber daya seperti printer, file, dan koneksi internet. Jenis jaringan meliputi LAN (Local Area Network), WAN (Wide Area Network), dan wireless network. Setiap perangkat dalam jaringan memiliki alamat IP unik sebagai identifikasi.', 'Pelajari topologi jaringan dan protokol komunikasi data', 'umum', 'definisi'),
(1, 1, 'Komputer', 'Mesin elektronik yang mampu menerima, memproses, menyimpan, dan menghasilkan data sesuai instruksi program yang diberikan oleh pengguna', 'Komputer adalah perangkat elektronik yang terdiri dari perangkat keras (hardware) dan perangkat lunak (software). Perangkat keras utama meliputi CPU, memori (RAM), penyimpanan (hard disk/SSD), dan perangkat input/output. Komputer modern mampu melakukan miliaran kalkulasi per detik dan digunakan di hampir semua bidang kehidupan.', 'Pelajari arsitektur komputer dan sistem operasi', 'umum', 'definisi'),
(12, 1, 'Sistem', 'Sekumpulan komponen yang saling berhubungan dan bekerja bersama untuk mencapai tujuan tertentu secara terintegrasi', 'Sistem dalam teknologi merujuk pada kumpulan komponen yang terorganisir untuk menjalankan fungsi tertentu. Contoh sistem meliputi sistem operasi, sistem basis data, sistem jaringan, dan sistem informasi. Setiap sistem memiliki komponen input, proses, output, dan umpan balik yang bekerja secara terkoordinasi.', 'Pelajari tentang sistem informasi dan analisis sistem', 'umum', 'definisi'),
(9, 1, 'Aplikasi', 'Perangkat lunak yang dirancang untuk membantu pengguna menyelesaikan tugas tertentu pada perangkat komputer atau mobile', 'Aplikasi adalah program komputer yang dibuat untuk tujuan spesifik, seperti pengolah kata, browser web, aplikasi pesan, atau game. Aplikasi dapat berjalan di berbagai platform termasuk desktop, web, dan perangkat mobile. Proses pengembangan aplikasi melibatkan analisis kebutuhan, desain, pemrograman, pengujian, dan deployment.', 'Pelajari tentang pengembangan perangkat lunak dan user experience', 'umum', 'definisi'),
(11, 1, 'Data', 'Fakta atau informasi mentah yang dikumpulkan dan dapat diolah menjadi bentuk yang bermakna untuk pengambilan keputusan', 'Data dalam konteks teknologi adalah representasi fakta, konsep, atau instruksi dalam bentuk yang dapat diproses oleh komputer. Data dapat berupa teks, angka, gambar, audio, atau video. Proses transformasi data menjadi informasi berguna melalui pengolahan, analisis, dan visualisasi adalah inti dari sistem informasi modern.', 'Pelajari tentang ilmu data dan analisis data', 'umum', 'definisi'),
(58, 1, 'Laptop', 'Komputer portabel yang menggabungkan komponen komputer desktop dalam satu perangkat ringan yang dapat dibawa ke mana-mana', 'Laptop adalah komputer personal yang dirancang agar mudah dibawa dan digunakan di berbagai lokasi. Laptop modern memiliki layar, keyboard, touchpad, baterai, speaker, kamera, dan konektivitas nirkabel terintegrasi. Spesifikasi laptop bervariasi dari penggunaan dasar hingga kebutuhan profesional seperti desain grafis dan pengembangan perangkat lunak.', 'Pelajari tentang spesifikasi laptop dan perawatan baterai', 'umum', 'definisi'),
(60, 1, 'Website', 'Kumpulan halaman web yang dapat diakses melalui internet menggunakan browser dan memiliki alamat URL unik', 'Website adalah kumpulan halaman berformat HTML yang ditautkan bersama dan dapat diakses melalui internet. Website dapat bersifat statis (konten tetap) atau dinamis (konten berubah sesuai interaksi pengguna). Teknologi pengembangan website meliputi HTML, CSS, JavaScript untuk frontend, serta berbagai bahasa pemrograman untuk backend.', 'Pelajari tentang pengembangan web dan desain responsif', 'umum', 'definisi'),
(3, 3, 'Algoritma', 'Sekumpulan langkah-langkah terstruktur dan logis yang digunakan untuk menyelesaikan suatu masalah komputasi secara sistematis', 'Algoritma adalah prosedur komputasi yang mendefinisikan serangkaian langkah untuk menyelesaikan masalah tertentu. Setiap algoritma memiliki input, proses, dan output yang terdefinisi dengan jelas. Efisiensi algoritma diukur dengan notasi Big-O yang menggambarkan kompleksitas waktu dan ruang. Contoh algoritma populer meliputi binary search, quicksort, dan Dijkstra.', 'Pelajari tentang struktur data dan analisis kompleksitas algoritma', 'umum', 'definisi'),
(5, 1, 'Database', 'Sistem penyimpanan data terstruktur yang terorganisir sehingga memudahkan pengelolaan, pencarian, dan pengambilan informasi', 'Database adalah kumpulan data yang terorganisir dan disimpan secara sistematis sehingga mudah diakses, dikelola, dan diperbarui. Sistem Manajemen Database (DBMS) seperti MySQL, PostgreSQL, dan SQLite menyediakan antarmuka untuk operasi CRUD (Create, Read, Update, Delete). Database relasional menyimpan data dalam tabel yang berhubungan melalui kunci primer dan kunci asing.', 'Pelajari tentang desain database dan SQL', 'umum', 'definisi'),
(6, 1, 'Robot', 'Mesin yang dapat diprogram untuk melakukan tugas secara otomatis, baik secara otonom maupun dengan kendali jarak jauh', 'Robot adalah mesin atau perangkat yang dapat diprogram untuk melakukan tugas-tugas tertentu secara otomatis. Robot modern menggunakan sensor, aktuator, dan sistem kontrol yang canggih. Bidang robotika mencakup robot industri untuk manufaktur, robot medis untuk pembedahan, robot eksplorasi untuk ruang angkasa, dan robot humanoid untuk riset.', 'Pelajari tentang robotika dan kecerdasan buatan', 'umum', 'definisi'),
(7, 1, 'Kecerdasan', 'Kemampuan untuk belajar, memahami, dan menerapkan pengetahuan dalam menghadapi situasi baru serta memecahkan masalah', 'Kecerdasan dalam konteks teknologi sering merujuk pada kecerdasan buatan (AI), yaitu simulasi kecerdasan manusia oleh mesin. AI mencakup pembelajaran mesin (machine learning), pemrosesan bahasa alami (NLP), penglihatan komputer (computer vision), dan sistem pakar. Kecerdasan manusia sendiri meliputi kecerdasan logis-matematis, linguistik, spasial, dan interpersonal.', 'Pelajari tentang kecerdasan buatan dan machine learning', 'umum', 'definisi'),
(4, 1, 'Program', 'Rangkaian instruksi tertulis dalam bahasa pemrograman yang dapat dieksekusi oleh komputer untuk melakukan tugas tertentu', 'Program komputer adalah kumpulan instruksi yang ditulis dalam bahasa pemrograman untuk menyelesaikan tugas spesifik. Proses pembuatan program meliputi menulis kode (coding), pengujian (testing), dan debugging. Bahasa pemrograman populer meliputi Python, JavaScript, Java, C++, dan banyak lagi, masing-masing dengan keunggulan di bidang tertentu.', 'Pelajari tentang pemrograman dan algoritma', 'umum', 'definisi'),
(13, 2, 'Air', 'Zat cair jernih tanpa warna dan rasa yang tersusun dari molekul hidrogen dan oksigen, merupakan kebutuhan esensial bagi seluruh makhluk hidup', 'Air (H2O) adalah senyawa kimia yang terdiri dari dua atom hidrogen dan satu atom oksigen. Air menutupi sekitar 71% permukaan bumi dan merupakan pelarut universal yang sangat penting bagi kehidupan. Siklus air meliputi evaporasi, kondensasi, dan presipitasi yang mengatur distribusi air di bumi. Manusia membutuhkan air minum bersih minimal 2 liter per hari.', 'Pelajari tentang siklus air dan konservasi sumber daya air', 'umum', 'definisi'),
(14, 2, 'Bumi', 'Planet ketiga dari Matahari dan satu-satunya planet yang diketahui memiliki kehidupan, dengan permukaan yang sebagian besar tertutup air', 'Bumi adalah planet terestrial dengan diameter sekitar 12.742 km. Bumi memiliki atmosfer yang melindungi kehidupan dari radiasi matahari berbahaya, medan magnet yang mengarahkan kompas, dan satu satelit alami yaitu bulan. Bumi terbagi menjadi beberapa lapisan: inti dalam, inti luar, mantel, dan kerak bumi. Rotasi bumi menyebabkan siang dan malam, sedangkan revolusi menyebabkan musim.', 'Pelajari tentang geologi dan ilmu bumi', 'umum', 'definisi'),
(15, 2, 'Matahari', 'Bintang pusat tata surya yang menghasilkan energi melalui reaksi fusi nuklir, menjadi sumber cahaya dan panas utama bagi planet-planet di sekitarnya', 'Matahari adalah bintang berjenis G2V yang memiliki diameter sekitar 1,4 juta km, sekitar 109 kali diameter bumi. Suhu permukaan matahari mencapai sekitar 5.500 derajat Celsius, sementara suhu intinya mencapai 15 juta derajat Celsius. Matahari menghasilkan energi melalui fusi nuklir hidrogen menjadi helium, dan merupakan sumber energi utama bagi kehidupan di bumi.', 'Pelajari tentang astrofisika dan tata surya', 'umum', 'definisi'),
(20, 2, 'Evolusi', 'Proses perubahan bertahap pada sifat-sifat populasi makhluk hidup dari generasi ke generasi melalui seleksi alam dan variasi genetik', 'Teori evolusi yang dikemukakan oleh Charles Darwin menjelaskan bahwa spesies makhluk hidup berubah secara bertahap dari generasi ke generasi. Mekanisme utama evolusi adalah seleksi alam (natural selection), di mana individu yang lebih cocok dengan lingkungan memiliki peluang lebih besar untuk bertahan hidup dan berkembang biak. Bukti evolusi ditemukan dalam rekaman fosil, anatomi perbandingan, dan genetika molekuler.', 'Pelajari tentang genetika populasi dan spesiasi', 'umum', 'definisi'),
(25, 20, 'Manusia', 'Spesies Homo sapiens yang merupakan makhluk hidup paling maju dengan kemampuan berpikir abstrak, berbahasa, dan membuat alat', 'Manusia (Homo sapiens) adalah spesies primata yang memiliki kemampuan kognitif tinggi termasuk berpikir abstrak, menggunakan bahasa kompleks, dan membuat alat. Manusia modern telah ada sekitar 300.000 tahun dan telah mengembangkan peradaban, teknologi, dan budaya yang sangat beragam. Tubuh manusia terdiri dari triliunan sel yang terorganisir menjadi berbagai sistem organ.', 'Pelajari tentang antropologi dan anatomi manusia', 'umum', 'definisi'),
(30, 20, 'Otak', 'Organ pusat sistem saraf pada vertebrata yang mengendalikan seluruh fungsi tubuh, pemikiran, ingatan, emosi, dan kesadaran', 'Otak manusia terdiri dari sekitar 86 miliar neuron yang saling terhubung melalui triliunan sinapsis. Otak dibagi menjadi beberapa bagian: otak besar (serebrum) untuk fungsi kognitif tinggi, otak kecil (serebelum) untuk koordinasi gerakan, batang otak untuk fungsi vital, dan sistem limbik untuk emosi. Otak menggunakan sekitar 20% dari total energi tubuh meskipun hanya berat sekitar 1,4 kg.', 'Pelajari tentang neurosains dan psikologi kognitif', 'umum', 'definisi'),
(32, 7, 'Bahasa', 'Sistem lambang bunyi yang digunakan oleh suatu kelompok masyarakat untuk berkomunikasi, menyampaikan gagasan, dan berekspresi', 'Bahasa adalah sistem komunikasi yang menggunakan simbol bunyi, tulisan, atau gerakan. Bahasa Indonesia sebagai bahasa nasional Indonesia berakar dari bahasa Melayu dan telah berkembang menjadi bahasa yang kaya dengan penyerapan dari berbagai bahasa lain. Komponen bahasa meliputi fonologi (bunyi), morfologi (kata), sintaksis (kalimat), dan semantik (makna).', 'Pelajari tentang linguistik dan kaidah bahasa Indonesia', 'umum', 'definisi'),
(42, 3, 'Matematika', 'Ilmu yang mempelajari bilangan, struktur, ruang, dan perubahan melalui penalaran logis dan notasi simbolis yang terstruktur', 'Matematika adalah ilmu fundamental yang menjadi dasar bagi banyak bidang ilmu lain. Cabang utama matematika meliputi aritmatika, aljabar, geometri, kalkulus, statistika, dan teori bilangan. Matematika digunakan secara luas dalam sains, teknologi, ekonomi, dan kehidupan sehari-hari. Penalaran matematis membantu mengembangkan kemampuan berpikir logis dan analitis.', 'Pelajari tentang cabang-cabang matematika dan penerapannya', 'umum', 'definisi'),
(48, 12, 'Pengetahuan', 'Segala sesuatu yang diketahui atau dipelajari manusia melalui pengalaman, pendidikan, dan penyelidikan ilmiah', 'Pengetahuan adalah hasil dari proses belajar dan pengalaman yang meliputi fakta, informasi, keterampilan, dan pemahaman. Menurut Bloom, pengetahuan merupakan ranah kognitif paling dasar yang menjadi fondasi untuk kemampuan berpikir tingkat tinggi. Sumber pengetahuan meliputi pengalaman langsung, pembelajaran formal, riset ilmiah, dan tradisi kebudayaan.', 'Pelajari tentang epistemologi dan teori pengetahuan', 'umum', 'definisi'),
(50, 11, 'Kesehatan', 'Keadaan sejahtera secara fisik, mental, dan sosial yang tidak hanya berarti bebas dari penyakit tetapi juga mencakup kualitas hidup', 'Kesehatan menurut WHO adalah keadaan sempurna secara fisik, mental, dan sosial, bukan hanya bebas dari penyakit atau kecacatan. Faktor yang mempengaruhi kesehatan meliputi pola makan, olahraga, tidur, manajemen stres, dan akses layanan kesehatan. Pencegahan penyakit melalui gaya hidup sehat merupakan strategi kesehatan yang paling efektif dan ekonomis.', 'Pelajari tentang gaya hidup sehat dan pencegahan penyakit', 'umum', 'definisi'),
(51, 13, 'Ekonomi', 'Ilmu yang mempelajari produksi, distribusi, dan konsumsi barang serta jasa serta interaksi antara individu, bisnis, dan pemerintah', 'Ekonomi adalah ilmu sosial yang mempelajari bagaimana masyarakat mengalokasikan sumber daya yang terbatas untuk memenuhi kebutuhan yang tidak terbatas. Cabang utama ekonomi meliputi mikroekonomi (perilaku individu dan perusahaan) dan makroekonomi (perekonomian secara keseluruhan). Indikator ekonomi penting meliputi PDB, inflasi, pengangguran, dan neraca perdagangan.', 'Pelajari tentang prinsip ekonomi dan keuangan', 'umum', 'definisi'),
(38, 4, 'Indonesia', 'Negara kepulauan terbesar di dunia yang terletak di Asia Tenggara dengan lebih dari 17.000 pulau dan lebih dari 270 juta penduduk', 'Indonesia adalah negara kesatuan berbentuk republik dengan ibu kota Jakarta. Indonesia terdiri dari lebih dari 17.000 pulau yang membentang di khatulistiwa, menjadikannya memiliki keanekaragaman hayati dan budaya yang sangat tinggi. Indonesia memiliki lebih dari 1.300 suku bangsa dan 700 bahasa daerah. Bahasa Indonesia sebagai bahasa persatuan berperan penting dalam menyatukan keberagaman bangsa.', 'Pelajari tentang sejarah Indonesia dan kebudayaan nusantara', 'umum', 'definisi'),
(41, 5, 'Sejarah', 'Ilmu yang mempelajari dan mencatat peristiwa-peristiwa masa lampau serta perkembangan peradaban manusia dari waktu ke waktu', 'Sejarah adalah ilmu yang menyelidiki dan mengkaji peristiwa masa lampau untuk memahami proses perkembangan masyarakat manusia. Sumber sejarah meliputi artefak, dokumen tertulis, tradisi lisan, dan bukti arkeologis. Periode sejarah Indonesia dimulai dari era prasejarah, kerajaan Hindu-Buddha, kerajaan Islam, kolonialisme Eropa, hingga kemerdekaan dan pembangunan modern.', 'Pelajari tentang metode penelitian sejarah dan historiografi', 'umum', 'definisi'),
(31, 20, 'Jantung', 'Otot berongga yang berkontraksi secara ritmis untuk memompa darah ke seluruh tubuh melalui pembuluh darah arteri dan vena', 'Jantung manusia berukuran sekitar sekepalan tangan dan berfungsi memompa darah ke seluruh tubuh. Jantung memiliki empat ruangan: dua atrium dan dua ventrikel. Jantung berkontraksi rata-rata 100.000 kali per hari dan memompa sekitar 7.500 liter darah setiap hari. Penyakit jantung merupakan salah satu penyebab kematian tertinggi di dunia yang dapat dicegah dengan pola hidup sehat.', 'Pelajari tentang sistem kardiovaskular dan pencegahan penyakit jantung', 'umum', 'definisi'),
(29, 11, 'Vitamin', 'Senyawa organik yang dibutuhkan tubuh dalam jumlah kecil untuk menjalankan berbagai fungsi metabolisme dan menjaga kesehatan secara optimal', 'Vitamin adalah nutrisi esensial yang dibutuhkan tubuh dalam jumlah kecil tetapi vital untuk kesehatan. Vitamin dibagi menjadi dua jenis: larut air (vitamin B kompleks dan C) dan larut lemak (vitamin A, D, E, dan K). Kekurangan vitamin dapat menyebabkan berbagai gangguan kesehatan seperti skorbuto (kekurangan vitamin C), rabun senja (kekurangan vitamin A), dan riketsia (kekurangan vitamin D).', 'Pelajari tentang nutrisi dan gizi seimbang', 'umum', 'definisi'),
(16, 2, 'Bulan', 'Satelit alami Bumi yang mengorbit pada jarak rata-rata 384.400 kilometer, mempengaruhi pasang surut laut dan pencahayaan malam', 'Bulan adalah satu-satunya satelit alami bumi dengan diameter sekitar 3.474 km. Permukaan bulan dipenuhi kawah yang terbentuk akibat tumbukan asteroid dan komet miliaran tahun lalu. Bulan tidak memiliki atmosfer yang berarti, menyebabkan suhu permukaannya sangat bervariasi dari -173 derajat Celsius di malam hari hingga 127 derajat Celsius di siang hari. Gravitasi bulan menyebabkan fenomena pasang surut di bumi.', 'Pelajari tentang astronomi dan eksplorasi ruang angkasa', 'umum', 'definisi'),
(17, 2, 'Gravitasi', 'Gaya tarik-menarik antarmassa yang menyebabkan benda saling menarik, menjaga planet pada orbitnya, dan membuat benda jatuh ke permukaan Bumi', 'Gravitasi adalah salah satu dari empat gaya fundamental alam yang ditemukan oleh Isaac Newton dan kemudian dijelaskan lebih mendalam oleh Albert Einstein melalui teori relativitas umum. Gaya gravitasi bumi memberikan percepatan 9,8 m/s persegi yang kita rasakan sebagai berat badan. Semakin besar massa suatu benda, semakin besar gaya gravitasinya.', 'Pelajari tentang mekanika klasik dan relativitas umum', 'umum', 'definisi'),
(24, 2, 'Oksigen', 'Unsur kimia gas tidak berwarna dan tidak berbau yang sangat penting bagi kehidupan, merupakan sekitar 21 persen dari atmosfer Bumi', 'Oksigen (O2) adalah unsur kimia dengan nomor atom 8 yang sangat penting bagi kehidupan. Oksigen menyusun sekitar 21% dari atmosfer bumi dan merupakan komponen utama dalam air (H2O). Oksigen dihasilkan melalui fotosintesis oleh tumbuhan dan alga. Proses respirasi seluler menggunakan oksigen untuk menghasilkan energi (ATP) dari glukosa. Oksigen juga digunakan dalam banyak proses industri seperti pembakaran dan pengelasan.', 'Pelajari tentang kimia dan proses respirasi seluler', 'umum', 'definisi'),
(23, 2, 'Atom', 'Satuan dasar materi yang terdiri dari inti atom berisi proton dan neutron serta elektron yang mengelilinginya', 'Atom adalah partikel terkecil dari suatu unsur yang masih memiliki sifat unsur tersebut. Atom terdiri dari inti atom (nukleus) yang berisi proton dan neutron, serta elektron yang mengorbit inti. Jumlah proton menentukan jenis unsur, sedangkan jumlah neutron menentukan isotop. Teori atom modern dikembangkan oleh para ilmuwan seperti Dalton, Thomson, Rutherford, dan Bohr.', 'Pelajari tentang fisika kuantum dan struktur atom', 'umum', 'definisi'),
(18, 2, 'Energi', 'Kemampuan untuk melakukan usaha atau menggerakkan benda, yang dapat berwujud panas, cahaya, gerak, listrik, kimia, atau nuklir', 'Energi adalah besaran fisika yang tidak dapat diciptakan atau dimusnahkan, hanya dapat diubah dari satu bentuk ke bentuk lain (hukum kekekalan energi). Bentuk-bentuk energi meliputi energi kinetik (gerak), energi potensial (posisi), energi panas, energi listrik, energi kimia, energi nuklir, dan energi cahaya. Satuan energi dalam SI adalah Joule (J). Sumber energi utama di bumi adalah matahari.', 'Pelajari tentang termodinamika dan sumber energi terbarukan', 'umum', 'definisi'),
(19, 2, 'Cahaya', 'Bentuk radiasi elektromagnetik yang dapat dilihat oleh mata manusia, merambat dengan kecepatan 299.792 kilometer per detik dalam ruang hampa', 'Cahaya adalah gelombang elektromagnetik dengan panjang gelombang antara 380-700 nanometer yang dapat dideteksi oleh mata manusia. Kecepatan cahaya dalam ruang hampa adalah konstanta fisika terpenting sebesar 299.792.458 meter per detik. Cahaya memiliki sifat gelombang (difraksi, interferensi) sekaligus sifat partikel (efek fotolistrik). Spektrum cahaya terlihat membentuk pelangi dari merah hingga ungu.', 'Pelajari tentang optika dan fisika gelombang', 'umum', 'definisi'),
(21, 2, 'Fotosintesis', 'Proses biokimia yang dilakukan tumbuhan hijau untuk mengubah karbon dioksida dan air menjadi glukosa dan oksigen menggunakan energi cahaya matahari', 'Fotosintesis adalah proses yang dilakukan tumbuhan, alga, dan beberapa bakteri untuk mengubah energi cahaya matahari menjadi energi kimia berupa glukosa. Reaksi ini terjadi di kloroplas menggunakan pigmen klorofil. Persamaan fotosintesis: 6CO2 + 6H2O + cahaya menghasilkan C6H12O6 + 6O2. Fotosintesis merupakan sumber oksigen dan basis rantai makanan di bumi.', 'Pelajari tentang botani dan biokimia tumbuhan', 'umum', 'definisi'),
(22, 16, 'Ekosistem', 'Satuan ekologi yang terbentuk dari interaksi antara makhluk hidup dengan lingkungan fisiknya dalam suatu area tertentu', 'Ekosistem adalah sistem ekologi yang terdiri dari komponen biotik (makhluk hidup) dan abiotik (lingkungan non-hidup) yang saling berinteraksi. Ekosistem memiliki komponen produsen (tumbuhan), konsumen (hewan), dan dekomposer (bakteri, jamur). Contoh ekosistem meliputi hutan hujan tropis, terumbu karang, padang rumput, dan danau. Keseimbangan ekosistem sangat penting untuk keberlangsungan kehidupan.', 'Pelajari tentang ekologi dan konservasi lingkungan', 'umum', 'definisi'),
(26, 20, 'Sel', 'Satuan struktural dan fungsional terkecil dari makhluk hidup yang mampu melakukan metabolisme, pertumbuhan, dan reproduksi secara mandiri', 'Sel adalah unit dasar kehidupan yang ditemukan pertama kali oleh Robert Hooke. Sel terdiri dari membran sel, sitoplasma, dan inti sel (nukleus) yang berisi materi genetik (DNA). Organisme uniseluler seperti bakteri terdiri dari satu sel, sedangkan manusia terdiri dari triliunan sel yang terdifferensiasi menjadi berbagai jenis jaringan dan organ.', 'Pelajari tentang biologi sel dan biologi molekuler', 'umum', 'definisi'),
(27, 20, 'Hewan', 'Organisme eukariotik multiseluler yang tidak mampu melakukan fotosintesis, mendapatkan energi dengan mengonsumsi bahan organik lain', 'Hewan adalah organisme multiseluler yang termasuk dalam kingdom Animalia. Ciri-ciri hewan meliputi kemampuan bergerak secara aktif, tidak memiliki dinding sel, dan memperoleh nutrisi dengan cara mengonsumsi organisme lain. Klasifikasi hewan sangat beragam mulai dari invertebrata (serangga, cacing) hingga vertebrata (ikan, reptil, burung, mamalia). Hewan berperan penting dalam ekosistem sebagai konsumen.', 'Pelajari tentang zoologi dan perilaku hewan', 'umum', 'definisi'),
(28, 20, 'Tumbuhan', 'Organisme multiseluler eukariotik yang mengandung klorofil dan mampu melakukan fotosintesis untuk menghasilkan makanannya sendiri', 'Tumbuhan adalah organisme autotrof yang menghasilkan makanannya sendiri melalui fotosintesis. Tumbuhan memiliki struktur yang terdiri dari akar, batang, daun, bunga, dan buah. Tumbuhan berperan sangat penting di bumi sebagai produsen oksigen, sumber pangan, bahan bangunan, dan obat-obatan. Tumbuhan juga membantu menjaga keseimbangan ekosistem dan mencegah erosi tanah.', 'Pelajari tentang botani dan fisiologi tumbuhan', 'umum', 'definisi'),
(44, 3, 'Aljabar', 'Cabang matematika yang menggunakan simbol dan huruf untuk merepresentasikan bilangan dalam persamaan dan memecahkan masalah abstrak', 'Aljabar adalah cabang matematika yang menggunakan variabel, konstanta, dan operasi matematika untuk memecahkan masalah. Aljabar dasar meliputi operasi dengan variabel, persamaan linear, persamaan kuadrat, dan pertidaksamaan. Aljabar linear mempelajari vektor, matriks, dan ruang vektor, sedangkan aljabar abstrak mempelajari struktur aljabar seperti grup, ring, dan field.', 'Pelajari tentang aljabar linear dan aljabar abstrak', 'umum', 'definisi'),
(45, 6, 'Gunung', 'Bentuklahan alam yang menonjol dari permukaan tanah sekitarnya dengan ketinggian yang signifikan, terbentuk dari proses geologi', 'Gunung terbentuk dari aktivitas tektonik lempeng, vulkanisme, dan erosi selama jutaan tahun. Indonesia memiliki lebih dari 100 gunung berapi aktif karena terletak di Cincin Api Pasifik (Ring of Fire). Gunung tertinggi di dunia adalah Gunung Everest dengan ketinggian 8.849 meter di atas permukaan laut. Gunung berperan penting sebagai sumber air, habitat flora dan fauna, serta objek wisata.', 'Pelajari tentang geologi dan vulkanologi', 'umum', 'definisi'),
(46, 6, 'Laut', 'Massa air asin yang luas menutupi sebagian besar permukaan Bumi, menjadi habitat bagi berbagai ekosistem laut', 'Laut atau lautan menutupi sekitar 71% permukaan bumi dengan kedalaman rata-rata 3.688 meter. Laut terbagi menjadi lima samudra: Pasifik, Atlantik, Hindia, Arktik, dan Antartika. Ekosistem laut sangat beragam mulai dari terumbu karang, hutan bakau, hingga palung laut terdalam. Laut berperan penting dalam mengatur iklim global, menghasilkan oksigen, dan menyediakan sumber pangan.', 'Pelajari tentang oseanografi dan biologi laut', 'umum', 'definisi'),
(47, 16, 'Hutan', 'Kawasan yang ditumbuhi pepohonan dengan kerapitan tertentu, berfungsi sebagai paru-paru dunia dan habitat keanekaragaman hayati', 'Hutan adalah ekosistem yang didominasi oleh pepohonan dan vegetasi kayu lainnya. Hutan hujan tropis Indonesia merupakan salah satu yang terluas di dunia dan menjadi habitat bagi keanekaragaman hayati yang sangat tinggi. Hutan berfungsi sebagai penyerap karbon dioksida, penghasil oksigen, pengatur siklus air, dan pencegah erosi. Deforestasi menjadi ancaman serius bagi keberlangsungan hutan dan iklim global.', 'Pelajari tentang kehutanan dan konservasi hutan', 'umum', 'definisi'),
(49, 12, 'Pendidikan', 'Proses sistematis untuk mengembangkan pengetahuan, keterampilan, karakter, dan kecerdasan individu melalui pengajaran dan pelatihan', 'Pendidikan adalah proses pembelajaran yang bertujuan mengembangkan potensi individu secara optimal. Sistem pendidikan Indonesia terdiri dari pendidikan formal (sekolah), nonformal (kursus), dan informal (keluarga). Kurikulum pendidikan dirancang untuk mengembangkan kompetensi kognitif, afektif, dan psikomotorik. Teknologi pendidikan modern seperti e-learning semakin memperluas akses pendidikan.', 'Pelajari tentang metode pengajaran dan teknologi pendidikan', 'umum', 'definisi'),
(52, 13, 'Uang', 'Alat tukar yang diterima secara umum sebagai pembayaran barang dan jasa, berfungsi sebagai satuan hitung dan penyimpan nilai', 'Uang adalah alat tukar yang diakui secara umum dalam suatu masyarakat. Fungsi utama uang meliputi medium pertukaran, satuan hitung (unit of account), dan penyimpan nilai (store of value). Uang telah berevolusi dari bentuk barter ke mata uang logam, kertas, hingga uang digital dan cryptocurrency. Bank sentral bertanggung jawab mengatur pasokan uang dan menjaga stabilitas nilai mata uang.', 'Pelajari tentang keuangan dan literasi keuangan', 'umum', 'definisi'),
(53, 11, 'Obat', 'Zat atau bahan yang digunakan untuk mencegah, meringankan, atau menyembuhkan penyakit serta memulihkan kesehatan tubuh', 'Obat adalah substansi yang digunakan untuk diagnosis, pencegahan, pengobatan, atau penyembuhan penyakit. Obat bekerja dengan cara berinteraksi dengan reseptor atau enzim dalam tubuh. Obat tradisional Indonesia seperti jamu telah digunakan sejak zaman dahulu, sedangkan obat modern dikembangkan melalui riset farmakologi ketat. Penggunaan obat harus sesuai dosis dan resep dokter untuk menghindari efek samping.', 'Pelajari tentang farmakologi dan pengobatan tradisional', 'umum', 'definisi'),
(54, 10, 'Olahraga', 'Aktivitas fisik yang terstruktur dan kompetitif untuk menjaga kebugaran, meningkatkan keterampilan, dan memberikan hiburan', 'Olahraga adalah aktivitas fisik yang melibatkan gerakan tubuh secara terstruktur dan teratur. Manfaat olahraga bagi kesehatan sangat banyak meliputi penguatan jantung dan paru-paru, peningkatan kepadatan tulang, pengurangan risiko penyakit kronis, dan peningkatan kesehatan mental. Indonesia memiliki berbagai cabang olahraga populer seperti sepak bola, bulu tangkis, dan tinju.', 'Pelajari tentang ilmu olahraga dan nutrisi atlet', 'umum', 'definisi'),
(55, 19, 'Musik', 'Seni mengatur bunyi dalam waktu melalui ritme, melodi, dan harmoni untuk mengekspresikan emosi dan gagasan secara estetis', 'Musik adalah seni yang mengorganisasikan bunyi dalam waktu dengan elemen-elemen seperti ritme, melodi, harmoni, dan timbre. Musik Indonesia sangat beragam mulai dari gamelan Jawa, angklung Sunda, dangdut, hingga musik pop modern. Musik memiliki efek psikologis yang dapat mempengaruhi mood, produktivitas, dan kesehatan mental. Industri musik modern telah berkembang pesat dengan teknologi streaming digital.', 'Pelajari tentang teori musik dan sejarah musik Indonesia', 'umum', 'definisi'),
(56, 18, 'Makanan', 'Zat padat atau cair yang dikonsumsi makhluk hidup untuk memenuhi kebutuhan nutrisi, energi, dan pertumbuhan tubuh', 'Makanan adalah bahan yang dikonsumsi organisme untuk memperoleh energi dan nutrisi. Nutrisi esensial dalam makanan meliputi karbohidrat, protein, lemak, vitamin, mineral, dan air. Indonesia memiliki kekayaan kuliner yang sangat beragam dengan cita rasa yang khas di setiap daerah seperti rendang, sate, gado-gado, dan nasi goreng. Gizi seimbang sangat penting untuk kesehatan dan pertumbuhan.', 'Pelajari tentang ilmu gizi dan kuliner Indonesia', 'umum', 'definisi'),
(57, NULL, 'Rumah', 'Bangunan yang digunakan sebagai tempat tinggal keluarga atau individu, memberikan perlindungan dari cuaca dan lingkungan luar', 'Rumah adalah bangunan yang berfungsi sebagai tempat tinggal dan berlindung. Rumah tradisional Indonesia sangat beragam seperti rumah Joglo (Jawa), rumah Gadang (Minangkabau), rumah Tongkonan (Toraja), dan rumah Honai (Papua). Setiap rumah tradisional memiliki ciri arsitektural yang mencerminkan kearifan lokal. Rumah modern umumnya menggunakan material beton dan bata dengan desain yang lebih minimalis.', 'Pelajari tentang arsitektur dan desain interior', 'umum', 'definisi'),
(61, 1, 'Server', 'Komputer atau sistem yang menyediakan layanan, data, atau sumber daya kepada komputer klien melalui jaringan', 'Server adalah komputer atau sistem yang menyediakan layanan kepada komputer lain (klien) melalui jaringan. Jenis server meliputi web server, database server, file server, mail server, dan cloud server. Server biasanya memiliki spesifikasi yang lebih tinggi dari komputer biasa dengan prosesor yang lebih cepat, memori yang lebih besar, dan penyimpanan yang lebih luas.', 'Pelajari tentang administrasi server dan cloud computing', 'umum', 'definisi'),
(62, 1, 'Prosesor', 'Komponen komputer yang berfungsi sebagai otak untuk memproses instruksi dan melakukan kalkulasi', 'Prosesor atau CPU (Central Processing Unit) adalah komponen utama komputer yang menjalankan instruksi program. Prosesor modern memiliki miliaran transistor yang bekerja pada kecepatan miliaran siklus per detik (GHz). Arsitektur prosesor meliputi CISC (Complex Instruction Set Computer) dan RISC (Reduced Instruction Set Computer). Produsen prosesor utama meliputi Intel, AMD, dan ARM.', 'Pelajari tentang arsitektur komputer dan assembly', 'umum', 'definisi'),
(63, 1, 'Memori', 'Komponen komputer yang berfungsi menyimpan data dan instruksi sementara atau permanen untuk akses cepat oleh prosesor', 'Memori komputer dibagi menjadi dua jenis utama: RAM (Random Access Memory) yang bersifat volatile dan penyimpanan (hard disk/SSD) yang bersifat non-volatile. RAM menyimpan data yang sedang diproses oleh CPU, sedangkan penyimpanan menyimpan data secara permanen. Kapasitas dan kecepatan memori sangat mempengaruhi performa komputer. Teknologi memori terus berkembang dengan DDR5 dan NVMe SSD.', 'Pelajari tentang hierarki memori dan arsitektur penyimpanan', 'umum', 'definisi'),
(64, 1, 'Kode', 'Kumpulan instruksi yang ditulis dalam bahasa pemrograman untuk membuat program komputer yang dapat dieksekusi', 'Kode sumber (source code) adalah teks yang ditulis programmer menggunakan bahasa pemrograman tertentu. Kode kemudian dikompilasi atau diinterpretasikan menjadi bahasa mesin yang dapat dimengerti oleh komputer. Prinsip-prinsip penulisan kode yang baik meliputi keterbacaan, modularitas, dan dokumentasi. Version control seperti Git digunakan untuk mengelola perubahan kode dalam pengembangan perangkat lunak.', 'Pelajari tentang software engineering dan best practices', 'umum', 'definisi'),
(59, 1, 'Telepon', 'Perangkat komunikasi elektronik yang digunakan untuk mengirim dan menerima suara melalui jaringan telekomunikasi', 'Telepon telah berevolusi dari telepon kabel tradisional menjadi smartphone yang memiliki kemampuan komputer lengkap. Smartphone modern memiliki layar sentuh, kamera, GPS, dan akses internet yang memungkinkan berbagai aktivitas seperti komunikasi, hiburan, produktivitas, dan pembayaran digital. Sistem operasi mobile utama adalah Android dan iOS.', 'Pelajari tentang teknologi telekomunikasi dan jaringan seluler', 'umum', 'definisi'),
(39, 4, 'Batik', 'Seni menghias kain dengan cara membubuhkan malam pada kain tersebut, kemudian dicelup warna, merupakan warisan budaya Indonesia', 'Batik adalah seni dekorasi kain Indonesia yang diakui UNESCO sebagai Warisan Kemanusiaan untuk Budaya Lisan dan Nonbendawi sejak 2009. Teknik pembuatan batik meliputi batik tulis (dibuat tangan), batik cap (menggunakan cetakan), dan batik printing (cetak mesin). Setiap daerah di Indonesia memiliki motif batik khas yang mencerminkan budaya dan filosofi lokal seperti batik parang, batik mega mendung, dan batik truntum.', 'Pelajari tentang seni batik dan warisan budaya Indonesia', 'umum', 'definisi'),
(40, 4, 'Budaya', 'Seluruh sistem nilai, norma, kepercayaan, dan ekspresi kreatif yang dimiliki bersama oleh suatu kelompok masyarakat', 'Budaya mencakup seluruh aspek kehidupan manusia yang diwariskan dari generasi ke generasi melalui pembelajaran dan sosialisasi. Komponen budaya meliputi bahasa, agama, seni, adat istiadat, sistem pengetahuan, dan teknologi. Indonesia memiliki keanekaragaman budaya yang sangat tinggi dengan ratusan suku bangsa dan tradisi yang unik. Pelestarian budaya menjadi tanggung jawab bersama seluruh masyarakat.', 'Pelajari tentang antropologi dan studi budaya Indonesia', 'umum', 'definisi'),
(34, 7, 'Kalimat', 'Satuan bahasa yang mengandung pikiran utuh dan diakhiri dengan tanda baca, terdiri dari subjek, predikat, dan bisa disertai objek atau keterangan', 'Kalimat adalah satuan bahasa terkecil yang menyatakan pikiran yang lengkap dan diakhiri dengan tanda baca. Kalimat dalam bahasa Indonesia memiliki pola SPOK (Subjek, Predikat, Objek, Keterangan) yang dapat divariasikan. Jenis kalimat meliputi kalimat tunggal, kalimat majemuk, kalimat aktif, kalimat pasif, kalimat langsung, dan kalimat tidak langsung.', 'Pelajari tentang tata bahasa Indonesia yang baik dan benar', 'umum', 'definisi'),
(35, 7, 'Sinonim', 'Kata yang memiliki makna sama atau sangat mirip dengan kata lain, memungkinkan variasi penggunaan dalam kalimat tanpa mengubah arti', 'Sinonim adalah hubungan semantik antara dua kata atau lebih yang memiliki makna yang sama atau sangat mirip. Contoh sinonim dalam bahasa Indonesia: besar-ujung, kecil-mungil, cepat-lekas, dan indah-cantik. Penggunaan sinonim memperkaya kosa kata dan membuat penulisan menjadi lebih bervariasi. Kamus sinonim menjadi referensi penting dalam penulisan dan penerjemahan.', 'Pelajari tentang semantik dan leksikografi', 'umum', 'definisi'),
(36, 7, 'Antonim', 'Kata yang memiliki makna berlawanan atau bertentangan dengan kata lain, digunakan untuk menyatakan perbedaan atau kontras', 'Antonim adalah hubungan semantik antara dua kata yang memiliki makna berlawanan. Contoh antonim dalam bahasa Indonesia: besar-kecil, tinggi-rendah, panjang-pendek, dan cepat-lambat. Antonim terbagi menjadi beberapa jenis: gradable (besar-kecil), komplementer (hidup-mati), relasional (guru-murid), dan bertingkat (panas-dingin-sejuk).', 'Pelajari tentang semantik dan hubungan kata', 'umum', 'definisi'),
(37, 7, 'Imbuhan', 'Afiks atau bagian tambahan yang ditempelkan pada kata dasar untuk membentuk kata baru dengan makna atau fungsi gramatikal tertentu', 'Imbuhan dalam bahasa Indonesia meliputi awalan (prefiks) seperti meN-, ber-, di-, ter-, di-, dan akhiran (sufiks) seperti -kan, -an, -i. Selain itu ada konfiks (awalan + akhiran) seperti peN-...-an, per-...-an. Imbuhan dapat mengubah kelas kata, misalnya dari kata benda menjadi kata kerja (tulis menjadi menulis) atau dari kata kerja menjadi kata benda (tulis menjadi tulisan).', 'Pelajari tentang morfologi bahasa Indonesia', 'umum', 'definisi'),
(43, 3, 'Angka', 'Simbol atau tanda yang digunakan untuk menyatakan bilangan dalam sistem bilangan tertentu', 'Angka adalah simbol yang digunakan untuk merepresentasikan bilangan. Sistem bilangan yang umum digunakan meliputi sistem desimal (basis 10), biner (basis 2), oktal (basis 8), dan heksadesimal (basis 16). Angka Arab (0-9) adalah sistem penulisan angka yang paling banyak digunakan di dunia. Bilangan dapat dibedakan menjadi bilangan asli, bulat, rasional, irasional, dan kompleks.', 'Pelajari tentang teori bilangan dan sistem numerasi', 'umum', 'definisi'),
(33, 7, 'Kata', 'Satuan bahasa terkecil yang dapat berdiri sendiri dan memiliki makna dalam komunikasi lisan maupun tulisan', 'Kata adalah satuan bahasa terkecil yang memiliki makna dan dapat digunakan secara mandiri dalam kalimat. Kata dalam bahasa Indonesia diklasifikasikan berdasarkan kelas katanya menjadi kata benda (nomina), kata kerja (verba), kata sifat (adjektiva), kata keterangan (adverbia), kata ganti (pronomina), dan lain-lain. Setiap kata memiliki bentuk dasar dan bentuk terikat melalui proses afiksasi (pembubuhan).', 'Pelajari tentang morfologi dan kelas kata dalam bahasa Indonesia', 'umum', 'definisi');
