CC      = cc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic
LDLIBS  = -lncurses

TARGET  = demo
UNITY_TARGET = demo-unity
WIN32_TARGET = demo-win32.exe
WIN32_CC ?= gcc
TEST_TARGET = test/test_controls
TEST_SRC = test/test_main.c test/test_support.c \
	test/test_listbox.c test/test_edit.c test/test_button.c \
	test/test_window.c test/test_menu.c test/test_statusbar.c \
	test/test_label.c test/test_core.c test/test_checkbox.c \
	test/test_radiobutton.c test/test_combobox.c \
	test/test_scrollbar.c test/test_textarea.c \
	test/test_textmodel.c test/test_editor.c \
	test/test_mini_keys.c
TUI_SRC = tui.c tui_window.c tui_button.c tui_label.c \
	tui_edit.c tui_listbox.c tui_menu.c tui_statusbar.c \
	tui_checkbox.c tui_radiobutton.c tui_combobox.c tui_scrollbar.c \
	tui_textarea.c tui_textmodel.c tui_editor.c
DEMO_SRC = demo.c console_ncurses.c
COVERAGE_DIR = coverage
COVERAGE_TARGET = $(COVERAGE_DIR)/test_controls
COVERAGE_FLAGS = -fprofile-instr-generate -fcoverage-mapping
LLVM_COV = xcrun llvm-cov
LLVM_PROFDATA = xcrun llvm-profdata

all: $(TARGET)

$(TARGET): $(DEMO_SRC) $(TUI_SRC) tui.h tui_internal.h console.h
	$(CC) $(CFLAGS) -DTUI_BACKEND_NCURSES $(DEMO_SRC) $(TUI_SRC) -o $(TARGET) $(LDLIBS)

$(UNITY_TARGET): tui_unity.c $(DEMO_SRC) $(TUI_SRC) tui.h tui_internal.h console.h
	$(CC) $(CFLAGS) -DTUI_BACKEND_NCURSES tui_unity.c -o $(UNITY_TARGET) $(LDLIBS)

unity: $(UNITY_TARGET)

$(WIN32_TARGET): demo.c console_win32.c \
	$(TUI_SRC) tui.h tui_internal.h console.h
	$(WIN32_CC) $(CFLAGS) -DTUI_BACKEND_WIN32 \
		demo.c console_win32.c $(TUI_SRC) -o $(WIN32_TARGET)

win32: $(WIN32_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(TUI_SRC) tui.h tui_internal.h console.h \
	test/test_support.h
	$(CC) $(CFLAGS) -I. $(TEST_SRC) $(TUI_SRC) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(COVERAGE_TARGET): $(TEST_SRC) $(TUI_SRC) tui.h tui_internal.h console.h \
	test/test_support.h
	mkdir -p $(COVERAGE_DIR)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -I. $(TEST_SRC) $(TUI_SRC) -o $(COVERAGE_TARGET)

coverage: $(COVERAGE_TARGET)
	rm -f $(COVERAGE_DIR)/test_controls.profraw $(COVERAGE_DIR)/test_controls.profdata
	LLVM_PROFILE_FILE=$(COVERAGE_DIR)/test_controls.profraw ./$(COVERAGE_TARGET)
	$(LLVM_PROFDATA) merge -sparse $(COVERAGE_DIR)/test_controls.profraw -o $(COVERAGE_DIR)/test_controls.profdata
	$(LLVM_COV) report ./$(COVERAGE_TARGET) -instr-profile=$(COVERAGE_DIR)/test_controls.profdata --ignore-filename-regex='(^|/)test/.*|(^|/)(tui\.h|console\.h)$$'

# Demo para MiniCPU (prototipo 30): compilar, ensamblar, probar y subir.
#
#   make mini               compila y ensambla a _build/tui_mini.bin
#   make mini-sim           la ejecuta en el simulador (Esc la cierra)
#   make mini-run           la sube a la placa y la arranca
#   make mini-run PORT=...  igual, con el puerto a mano
#
# Usa los lanzadores de ../tools (hay que tenerlos en el PATH).
MINI_PROTOTYPE = 30
MINI_BIN = _build/tui_mini.bin
MINI_SRC = tui_unity_mini.c tui_unity.c console_mini.c demo.c $(TUI_SRC)
MINI_SIM_MAX = 50000000
MINI_PORT = $(if $(PORT),--port $(PORT),)

$(MINI_BIN): $(MINI_SRC) tui.h tui_internal.h console.h
	mkdir -p _build
	mini-lcc tui_unity_mini.c -o _build/tui_mini.s
	mini-asm _build/tui_mini.s -o $(MINI_BIN)

mini: $(MINI_BIN)

mini-sim: $(MINI_BIN)
	printf '\033' > _build/keys.bin
	cpusim $(MINI_BIN) --serial-input _build/keys.bin --console-output _build/screen.txt --run-limit $(MINI_SIM_MAX)
	cat _build/screen.txt

mini-run: $(MINI_BIN)
	run-board --prototype $(MINI_PROTOTYPE) --program $(MINI_BIN) $(MINI_PORT)

clean:
	rm -f $(TARGET) $(UNITY_TARGET) $(WIN32_TARGET) $(TEST_TARGET) $(COVERAGE_TARGET) \
		$(COVERAGE_DIR)/test_controls.profraw \
		$(COVERAGE_DIR)/test_controls.profdata \
		_build/tui_mini.s $(MINI_BIN) _build/keys.bin _build/screen.txt

.PHONY: all clean test coverage unity win32 mini mini-sim mini-run
