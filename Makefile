# ========================================================================== #
# AJUDAN 4.0 - Makefile                                                       #
# ========================================================================== #
SHELL := /bin/bash
# Build system untuk chatbot bahasa Indonesia (C89 + SQLite3)                #
#                                                                            #
# Penggunaan:                                                                 #
#   make              - Build binary release                                  #
#   make debug        - Build dengan simbol debug (-g -O0)                    #
#   make release      - Build optimisasi (-O2)                               #
#   make clean        - Hapus semua file build                               #
#   make init         - Inisialisasi database + seed data                    #
#   make seed         - Muat ulang semua seed data                           #
#   make seed-v5      - Muat data ekspansi v5 (views, triggers, FTS5)        #
#   make chat         - Jalankan mode interaktif                             #
#   make test         - Jalankan smoke test                                  #
#   make stats        - Tampilkan statistik database                        #
#   make sqlcheck     - Validasi SQL tanpa mengeksekusi                     #
#   make lint         - Jalankan static analysis                            #
#   make valgrind     - Jalankan dengan valgrind (memory check)              #
#   make tags         - Buat ctags untuk navigasi kode                      #
#   make help         - Tampilkan bantuan                                    #
# ========================================================================== #

# -------------------------------------------------------------------------- #
# KONFIGURASI                                                                 #
# -------------------------------------------------------------------------- #

# Compiler & flags
CC       := gcc
CFLAGS   := -std=c89 -Wall -Wextra -Wpedantic -Wno-long-long -Wno-unused-parameter
LDFLAGS  := -lsqlite3

# Nama program
PROG     := ajudan
BINARY   := $(PROG)

# Direktori
SRCDIR   := .
BUILDDIR := build
OBJDIR   := $(BUILDDIR)/obj

# Database
DB_NAME  := basisdata.db

# File sumber C
SRCS     := $(wildcard $(SRCDIR)/*.c)

# File objek
OBJS     := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# File SQL
SQL_V3   := $(SRCDIR)/ajudan.sql
SQL_V4   := $(SRCDIR)/ajudan_v4.sql
SQL_V5   := $(SRCDIR)/ajudan_v5.sql
SQL_KAMUS := $(SRCDIR)/kamus_A.sql

# Warna output
RED     := \033[0;31m
GREEN   := \033[0;32m
YELLOW  := \033[0;33m
BLUE    := \033[0;34m
CYAN    := \033[0;36m
NC      := \033[0m

# -------------------------------------------------------------------------- #
# TARGET UTAMA                                                               #
# -------------------------------------------------------------------------- #

.PHONY: all build debug release clean init seed seed-v5 chat test stats \
	sqlcheck lint valgrind tags help info

# Default: build release
all: release

# -------------------------------------------------------------------------- #
# BUILD TARGETS                                                              #
# -------------------------------------------------------------------------- #

## Build dengan optimisasi standar
release: CFLAGS += -O2 -DNDEBUG
release: $(BINARY)
	@echo -e "$(GREEN)✓ Build release selesai: $(BINARY)$(NC)"

## Build dengan debug info
debug: CFLAGS += -g -O0 -DDEBUG -DDEBUG_LOG
debug: LDFLAGS += -g
debug: $(BINARY)
	@echo -e "$(GREEN)✓ Build debug selesai: $(BINARY)$(NC)"

## Alias untuk build standar
build: release

# Link binary
$(BINARY): $(OBJS)
	@echo -e "$(CYAN)Linking $(BINARY)...$(NC)"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo -e "$(GREEN)✓ Binary: $(BINARY) ($(shell du -h $(BINARY) | cut -f1))$(NC)"

# Kompilasi file objek
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo -e "$(BLUE)  CC $<$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

# Buat direktori build
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# -------------------------------------------------------------------------- #
# DATABASE TARGETS                                                           #
# -------------------------------------------------------------------------- #

## Inisialisasi database lengkap (init + semua seed)
init: $(BINARY)
	@echo -e "$(CYAN)Inisialisasi database...$(NC)"
	@rm -f $(DB_NAME)
	./$(BINARY) -init
	@echo -e "$(GREEN)✓ Database terinisialisasi: $(DB_NAME)$(NC)"
	@$(MAKE) --no-print-directory stats

## Muat ulang semua seed data (v3 + v4)
seed: $(SQL_V3) $(SQL_V4)
	@echo -e "$(CYAN)Memuat seed data v3 + v4...$(NC)"
	@sqlite3 $(DB_NAME) < $(SQL_V3) 2>/dev/null || true
	@sqlite3 $(DB_NAME) < $(SQL_V4) 2>/dev/null || true
	@echo -e "$(GREEN)✓ Seed v3 + v4 dimuat$(NC)"

## Muat data ekspansi v5 (views, triggers, FTS5, pengetahuan tambahan)
seed-v5: $(SQL_V5)
	@echo -e "$(CYAN)Memuat ekspansi v5...$(NC)"
	@sqlite3 $(DB_NAME) < $(SQL_V5) 2>/dev/null
	@echo -e "$(GREEN)✓ Ekspansi v5 dimuat (views, triggers, FTS5)$(NC)"
	@echo -e "$(YELLOW)Catatan: Untuk mengisi ulang FTS5, jalankan seed-fts$(NC)"

## Muat kamus KBBI huruf A
seed-kamus: $(SQL_KAMUS)
	@echo -e "$(CYAN)Memuat kamus KBBI (A)...$(NC)"
	@sqlite3 $(DB_NAME) < $(SQL_KAMUS) 2>/dev/null
	@echo -e "$(GREEN)✓ Kamus KBBI dimuat$(NC)"

## Rebuild FTS5 index
seed-fts:
	@echo -e "$(CYAN)Rebuilding FTS5 index...$(NC)"
	@sqlite3 $(DB_NAME) "INSERT INTO fts_kata(fts_kata) VALUES('rebuild');"
	@sqlite3 $(DB_NAME) "INSERT INTO fts_pengetahuan(fts_pengetahuan) VALUES('rebuild');"
	@echo -e "$(GREEN)✓ FTS5 index rebuilt$(NC)"

## Tampilkan statistik database
stats:
	@if [ ! -f $(DB_NAME) ]; then \
	        echo -e "$(RED)Database $(DB_NAME) tidak ditemukan. Jalankan 'make init'$(NC)"; \
	        exit 1; \
	fi
	@echo -e "$(CYAN)=== Statistik Database AJUDAN ===$(NC)"
	@echo ""
	@echo -e "$(BLUE)Ukuran file:$(NC) $(shell du -h $(DB_NAME) | cut -f1)"
	@echo ""
	@echo -e "$(BLUE)Tabel utama:$(NC)"
	@sqlite3 -header -column $(DB_NAME) \
	        "SELECT 'kata' AS tabel, COUNT(*) AS total FROM kata UNION ALL" \
	        "SELECT 'arti_kata', COUNT(*) FROM arti_kata UNION ALL" \
	        "SELECT 'morfologi_kata', COUNT(*) FROM morfologi_kata UNION ALL" \
	        "SELECT 'semantik_kata', COUNT(*) FROM semantik_kata UNION ALL" \
	        "SELECT 'pengetahuan_umum', COUNT(*) FROM pengetahuan_umum UNION ALL" \
	        "SELECT 'pengetahuan_tambahan', COUNT(*) FROM pengetahuan_tambahan UNION ALL" \
	        "SELECT 'sinonim_map', COUNT(*) FROM sinonim_map UNION ALL" \
	        "SELECT 'antonim_map', COUNT(*) FROM antonim_map UNION ALL" \
	        "SELECT 'normalisasi_ejaan', COUNT(*) FROM normalisasi_ejaan UNION ALL" \
	        "SELECT 'normalisasi_frasa', COUNT(*) FROM normalisasi_frasa UNION ALL" \
	        "SELECT 'log_percakapan', COUNT(*) FROM log_percakapan;" 2>/dev/null
	@echo ""
	@echo -e "$(BLUE)Tabel v4:$(NC)"
	@sqlite3 -header -column $(DB_NAME) \
	        "SELECT 'kata_tanya', COUNT(*) AS total FROM kata_tanya UNION ALL" \
	        "SELECT 'pemisah_klausa', COUNT(*) FROM pemisah_klausa UNION ALL" \
	        "SELECT 'kata_lumpat', COUNT(*) FROM kata_lumpat UNION ALL" \
	        "SELECT 'penanda_kalimat', COUNT(*) FROM penanda_kalimat UNION ALL" \
	        "SELECT 'pola_respon', COUNT(*) FROM pola_respon UNION ALL" \
	        "SELECT 'kata_operasi_mat', COUNT(*) FROM kata_operasi_mat UNION ALL" \
	        "SELECT 'referensi_waktu', COUNT(*) FROM referensi_waktu UNION ALL" \
	        "SELECT 'sapaan_waktu', COUNT(*) FROM sapaan_waktu;" 2>/dev/null
	@echo ""
	@echo -e "$(BLUE)Views tersedia:$(NC)"
	@sqlite3 $(DB_NAME) \
	        "SELECT name FROM sqlite_master WHERE type='view' ORDER BY name;" 2>/dev/null
	@echo ""

## Validasi SQL tanpa eksekusi
sqlcheck:
	@echo -e "$(CYAN)Validasi SQL...$(NC)"
	@if command -v sqlite3 >/dev/null 2>&1; then \
	        for f in $(SQL_V3) $(SQL_V4) $(SQL_V5); do \
	                if [ -f "$$f" ]; then \
	                        echo -e "  $(BLUE)$$f$(NC)"; \
	                        sqlite3 :memory: < "$$f" 2>&1 | head -5; \
	                fi; \
	        done; \
	        echo -e "$(GREEN)✓ Validasi SQL selesai$(NC)"; \
	else \
	        echo -e "$(RED)sqlite3 tidak ditemukan$(NC)"; \
	fi

## Dump database ke SQL
dump:
	@echo -e "$(CYAN)Dumping database...$(NC)"
	sqlite3 $(DB_NAME) .dump > ajudan_dump.sql
	@echo -e "$(GREEN)✓ Dump disimpan ke: ajudan_dump.sql ($(shell du -h ajudan_dump.sql | cut -f1))$(NC)"

# -------------------------------------------------------------------------- #
# INTERAKSI TARGETS                                                         #
# -------------------------------------------------------------------------- #

## Jalankan mode chat interaktif
chat: $(BINARY)
	@echo -e "$(CYAN)=== AJUDAN 4.0 Mode Chat ===$(NC)"
	@./$(BINARY) -chat

## Jalankan dengan log aktif
chat-log: $(BINARY)
	@echo -e "$(CYAN)=== AJUDAN 4.0 Mode Chat (LOG) ===$(NC)"
	@BOT_LOG=1 ./$(BINARY) -log -chat

# -------------------------------------------------------------------------- #
# TESTING TARGETS                                                            #
# -------------------------------------------------------------------------- #

## Smoke test: compile, init, query dasar
test: $(BINARY)
	@echo -e "$(CYAN)=== Smoke Test ===$(NC)"
	@rm -f $(DB_NAME)
	@echo -e "$(BLUE)[1/4] Inisialisasi database...$(NC)"
	./$(BINARY) -init
	@echo -e "$(BLUE)[2/4] Verifikasi database...$(NC)"
	@if [ -f $(DB_NAME) ]; then \
	        FILE_SIZE=$$(du -h $(DB_NAME) | cut -f1); \
	        echo -e "  $(GREEN)✓ Database: $(DB_NAME) ($$FILE_SIZE)$(NC)"; \
	else \
	        echo -e "  $(RED)✗ Database tidak dibuat$(NC)"; \
	        exit 1; \
	fi
	@echo -e "$(BLUE)[3/4] Muat v5 expansion...$(NC)"
	@if command -v sqlite3 >/dev/null 2>&1; then \
	        sqlite3 $(DB_NAME) < $(SQL_V5) 2>/dev/null && \
	                echo -e "  $(GREEN)✓ Ekspansi v5 dimuat$(NC)" || \
	        echo -e "  $(YELLOW)⚠ Ekspansi v5 gagal (sqlite3 CLI tidak tersedia)$(NC)"; \
	else \
	        echo -e "  $(YELLOW)⚠ sqlite3 CLI tidak tersedia, skip v5$(NC)"; \
	fi
	@echo -e "$(BLUE)[4/4] Verifikasi binary...$(NC)"
	@FILE_SIZE=$$(du -h $(BINARY) | cut -f1); \
	echo -e "  $(GREEN)✓ Binary: $(BINARY) ($$FILE_SIZE)$(NC)"
	@echo -e "$(GREEN)✓ Semua smoke test lulus$(NC)"

## Query test interaktif dengan sqlite3
query:
	@echo -e "$(CYAN)=== SQLite3 Interactive ===$(NC)"
	@sqlite3 -header -column $(DB_NAME)

# -------------------------------------------------------------------------- #
# ANALYSIS TARGETS                                                           #
# -------------------------------------------------------------------------- #

## Static analysis dengan cppcheck
lint:
	@echo -e "$(CYAN)=== Static Analysis (cppcheck) ===$(NC)"
	@if command -v cppcheck >/dev/null 2>&1; then \
	        cppcheck --std=c89 --enable=all --suppress=missingInclude \
	                --suppress=unusedFunction \
	                --error-exitcode=1 \
	                $(SRCS) 2>&1; \
	        echo -e "$(GREEN)✓ Lint selesai$(NC)"; \
	else \
	        echo -e "$(YELLOW)cppcheck tidak ditemukan. Install: sudo apt install cppcheck$(NC)"; \
	fi

## Memory check dengan valgrind
valgrind: debug $(BINARY)
	@echo -e "$(CYAN)=== Memory Check (valgrind) ===$(NC)"
	@if command -v valgrind >/dev/null 2>&1; then \
	        rm -f $(DB_NAME); \
	        ./$(BINARY) -init 2>/dev/null; \
	        echo "quit" | valgrind --leak-check=full --show-leak-kinds=all \
	                --error-exitcode=1 --track-origins=yes \
	                ./$(BINARY) -log -chat 2>&1 | tail -20; \
	        echo -e "$(GREEN)✓ Valgrind selesai$(NC)"; \
	else \
	        echo -e "$(YELLOW)valgrind tidak ditemukan. Install: sudo apt install valgrind$(NC)"; \
	fi

## Buat ctags
tags:
	@echo -e "$(CYAN)Generating ctags...$(NC)"
	@if command -v ctags >/dev/null 2>&1; then \
	        ctags -R --c-kinds=+p --fields=+iaS --extra=+q \
	                --languages=C --langmap=c:.c.h \
	                -f tags $(SRCDIR); \
	        echo -e "$(GREEN)✓ Tags generated: tags$(NC)"; \
	else \
	        echo -e "$(YELLOW)ctags tidak ditemukan$(NC)"; \
	fi

## Hitung baris kode
loc:
	@echo -e "$(CYAN)=== Lines of Code ===$(NC)"
	@echo -e "$(BLUE)C source files:$(NC)"
	@wc -l $(SRCS) $(SRCDIR)/ajudan.h 2>/dev/null | tail -1
	@echo ""
	@echo -e "$(BLUE)SQL files:$(NC)"
	@wc -l $(SQL_V3) $(SQL_V4) $(SQL_V5) 2>/dev/null | tail -1
	@echo ""
	@echo -e "$(BLUE)Per file C:$(NC)"
	@wc -l $(SRCS) $(SRCDIR)/ajudan.h 2>/dev/null | sort -rn

# -------------------------------------------------------------------------- #
# CLEAN TARGETS                                                              #
# -------------------------------------------------------------------------- #

## Hapus semua file build
clean:
	@echo -e "$(CYAN)Membersihkan file build...$(NC)"
	@rm -rf $(BUILDDIR)
	@rm -f $(BINARY)
	@rm -f tags
	@rm -f ajudan_dump.sql
	@echo -e "$(GREEN)✓ Clean selesai$(NC)"

## Hapus semua termasuk database
distclean: clean
	@echo -e "$(RED)Menghapus database...$(NC)"
	@rm -f $(DB_NAME)
	@echo -e "$(GREEN)✓ Distclean selesai$(NC)"

# -------------------------------------------------------------------------- #
# INFO & HELP                                                                #
# -------------------------------------------------------------------------- #

## Tampilkan info build
info:
	@echo -e "$(CYAN)=== AJUDAN 4.0 Build Info ===$(NC)"
	@echo ""
	@echo -e "$(BLUE)Compiler:$(NC)    $(CC) $(shell $(CC) --version | head -1)"
	@echo -e "$(BLUE)SQLite3:$(NC)    $(shell sqlite3 --version 2>/dev/null | head -1)"
	@echo -e "$(BLUE)CFLAGS:$(NC)    $(CFLAGS)"
	@echo -e "$(BLUE)LDFLAGS:$(NC)   $(LDFLAGS)"
	@echo ""
	@echo -e "$(BLUE)Source files:$(NC)"
	@for f in $(SRCS); do echo "  $$f"; done
	@echo ""
	@echo -e "$(BLUE)SQL files:$(NC)"
	@for f in $(SQL_V3) $(SQL_V4) $(SQL_V5); do \
	        if [ -f "$$f" ]; then echo "  $$f"; fi; \
	done
	@echo ""

## Tampilkan bantuan
help:
	@echo -e "$(CYAN)╔══════════════════════════════════════════════════════╗$(NC)"
	@echo -e "$(CYAN)║          AJUDAN 4.0 - Build System Help               ║$(NC)"
	@echo -e "$(CYAN)╚══════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo -e "$(GREEN)BUILD:$(NC)"
	@echo "  make              Build binary (release, -O2)"
	@echo "  make debug        Build dengan debug symbols (-g -O0)"
	@echo "  make release      Build optimisasi (-O2) [default]"
	@echo ""
	@echo -e "$(GREEN)DATABASE:$(NC)"
	@echo "  make init         Inisialisasi database + semua seed"
	@echo "  make seed         Muat ulang seed v3 + v4"
	@echo "  make seed-v5      Muat ekspansi v5 (views, triggers, FTS5)"
	@echo "  make seed-kamus   Muat kamus KBBI huruf A"
	@echo "  make seed-fts     Rebuild FTS5 search index"
	@echo "  make stats        Tampilkan statistik database"
	@echo "  make sqlcheck     Validasi SQL tanpa eksekusi"
	@echo "  make dump         Dump database ke SQL file"
	@echo "  make query        Buka sqlite3 interaktif"
	@echo ""
	@echo -e "$(GREEN)INTERAKSI:$(NC)"
	@echo "  make chat         Mode chat interaktif"
	@echo "  make chat-log     Mode chat dengan log aktif"
	@echo ""
	@echo -e "$(GREEN)TESTING:$(NC)"
	@echo "  make test         Jalankan smoke test"
	@echo "  make lint         Static analysis (cppcheck)"
	@echo "  make valgrind     Memory check (valgrind)"
	@echo ""
	@echo -e "$(GREEN)UTILITAS:$(NC)"
	@echo "  make tags         Generate ctags"
	@echo "  make loc          Hitung baris kode"
	@echo "  make info         Tampilkan info build"
	@echo "  make clean        Hapus file build"
	@echo "  make distclean    Hapus semua (termasuk database)"
	@echo "  make help         Tampilkan bantuan ini"
	@echo ""

# -------------------------------------------------------------------------- #
# DEPENDENSI FILE                                                            #
# -------------------------------------------------------------------------- #

# Rebuild jika header berubah
$(OBJDIR)/ajudan.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/data.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/aturan.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/kalimat.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/klausa.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/percakapan.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/rangkaian.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/cari.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/potong.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/waktu.o: $(SRCDIR)/ajudan.h
$(OBJDIR)/matematika.o: $(SRCDIR)/ajudan.h
