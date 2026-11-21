CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror
BUILD ?= build-host
.PHONY: all test
all: $(BUILD)/scrcap
$(BUILD)/scrcap: src/main.c src/capture.c src/capture.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -Isrc src/main.c src/capture.c -o $@
test: all
	python3 tests/test_capture.py $(BUILD)/scrcap
	$(CC) $(CFLAGS) -Isrc tests/faults.c -o $(BUILD)/test-faults
	$(BUILD)/test-faults
	$(CC) $(CFLAGS) tests/hotkey.c -o $(BUILD)/test-hotkey
	$(BUILD)/test-hotkey
	python3 tests/check_install.py

.PHONY: elks install clean
elks:
	sh build.sh
install:
	test -f build/scrcap
	mkdir -p "$(DESTDIR)/bin" "$(DESTDIR)/lib/scrcap"
	cp build/scrcap "$(DESTDIR)/bin/scrcap"
	chmod 755 "$(DESTDIR)/bin/scrcap"
	cp COPYING "$(DESTDIR)/lib/scrcap/copying"
	cp USAGE.TXT "$(DESTDIR)/lib/scrcap/usage.txt"
clean:
	rm -rf build build-host
