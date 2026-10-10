# Host-side tests for AdvancePrint (gcc). Run in WSL or any POSIX box:
#   make -C tests/host run
CC      := gcc
CFLAGS  := -O1 -g -Wall -Wextra -I../../include
BIN     := bin

TESTS := $(BIN)/test_advanceprint

.PHONY: run clean
run: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; ./$$t; done

$(BIN)/test_advanceprint: test_advanceprint.c ../../src/advanceprint_packet.c \
                 ../../src/advanceprint_page.c ../../src/advanceprint_text.c \
                 ../../src/advanceprint_font.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

clean:
	rm -rf $(BIN)
