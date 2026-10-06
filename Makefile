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
	test/test_scrollbar.c test/test_editor_buffer.c \
	test/test_textmodel.c test/test_editor.c \
	test/test_mini_keys.c
TUI_SRC = tui.c tui_window.c tui_button.c tui_label.c tui_panel.c \
	tui_edit.c tui_listbox.c tui_menu.c tui_statusbar.c \
	tui_checkbox.c tui_radiobutton.c tui_combobox.c tui_scrollbar.c \
	tui_textmodel.c tui_editor.c
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

# Medicion de eventos pendientes al empezar cada repintado (TODO de z.tui). La
# demo compilada con TUI_PROFILE_EVENTS escribe el resultado en las dos primeras
# filas de la pantalla: repintados, eventos, repintados con la cola no vacia,
# eventos en cola, maximo, fotogramas repintando y el histograma de la cola.
#
#   make mini-profile       arrastra una ventana (test/profile_drag.txt)
#   make mini-profile MINI_PROFILE_SCRIPT=test/profile_editor_repeat.txt
#                           flecha sostenida en el Editor
#   make mini-profile MINI_PROFILE_SCRIPT=test/profile_editor_type.txt
#                           teclear deprisa en el Editor
#   test/profile_editor_nav.txt, _letter.txt y _click.txt miden el coste de UNA
#   tecla o clic en el Editor, restando test/profile_editor_none.txt (vease el TODO).
#   test/profile_tab.txt mide un TAB en la pantalla Controls, restando
#   test/profile_tab_none.txt.
#
# El reloj del simulador son instrucciones: 75000 por fotograma (4,5 MIPS a 60 Hz).
MINI_PROFILE_BIN = _build/tui_profile.bin
MINI_PROFILE_SCRIPT ?= test/profile_drag.txt

$(MINI_PROFILE_BIN): tui_unity_profile.c $(MINI_SRC) tui.h tui_internal.h console.h
	mkdir -p _build
	mini-lcc tui_unity_profile.c -o _build/tui_profile.s
	mini-asm _build/tui_profile.s -o $(MINI_PROFILE_BIN)

mini-profile: $(MINI_PROFILE_BIN)
	: > _build/empty.bin
	cpusim $(MINI_PROFILE_BIN) --serial-input _build/empty.bin --input-script $(MINI_PROFILE_SCRIPT) --console-output _build/profile.txt --frame-instructions 75000 --run-limit $(MINI_SIM_MAX)
	head -n 2 _build/profile.txt

# Los tramos de celdas de la consola de la MiniCPU (console.h): comprobacion y medida.
#
#   make mini-runcheck      tui_console_fill y tui_console_text contra tui_console_cell
#                           en el simulador; debe escribir "DIFF 0000"
#   make mini-bench MINI_BENCH_MODE=4
#                           instrucciones de escribir celdas (modos en test/mini_bench.c)
MINI_BENCH_MODE ?= 8

_build/mini_runcheck.bin: test/mini_runcheck.c console_mini.c console.h
	mkdir -p _build
	mini-lcc test/mini_runcheck.c -o _build/mini_runcheck.s
	mini-asm _build/mini_runcheck.s -o _build/mini_runcheck.bin

mini-runcheck: _build/mini_runcheck.bin
	: > _build/empty.bin
	printf '# none\n' > _build/noscript.txt
	cpusim _build/mini_runcheck.bin --serial-input _build/empty.bin --input-script _build/noscript.txt --console-output _build/runcheck.txt --frame-instructions 75000 --run-limit $(MINI_SIM_MAX)
	head -n 1 _build/runcheck.txt

mini-bench: test/mini_bench.c console_mini.c console.h
	mkdir -p _build
	printf '#define BENCH_MODE $(MINI_BENCH_MODE)\n#include "../test/mini_bench.c"\n' > _build/mini_bench_main.c
	mini-lcc _build/mini_bench_main.c -o _build/mini_bench.s
	mini-asm _build/mini_bench.s -o _build/mini_bench.bin
	: > _build/empty.bin
	printf '# none\n' > _build/noscript.txt
	cpusim _build/mini_bench.bin --serial-input _build/empty.bin --input-script _build/noscript.txt --console-output _build/bench.txt --frame-instructions 75000 --run-limit $(MINI_SIM_MAX) | grep HALT

# Medida en la placa de cuatro formas de escribir tramos (test/mini_perf.c): la
# comparacion con mini_shadow, dos tablas como los PC, una tabla leyendo la RAM de
# texto, y sin tabla. Usa los contadores de CPU PERFORMANCE, asi que solo vale en la
# placa; el resultado se lee de la pantalla.
#
#   make mini-perf          compila, sube y arranca; luego monitor.py screen
mini-perf:
	mkdir -p _build
	mini-lcc test/mini_perf.c -o _build/mini_perf.s
	mini-asm _build/mini_perf.s -o _build/mini_perf.bin
	run-board --prototype $(MINI_PROTOTYPE) --program _build/mini_perf.bin $(MINI_PORT)
	@echo "Espera unos segundos y mira: monitor.py screen"

clean:
	rm -f $(TARGET) $(UNITY_TARGET) $(WIN32_TARGET) $(TEST_TARGET) $(COVERAGE_TARGET) \
		$(COVERAGE_DIR)/test_controls.profraw \
		$(COVERAGE_DIR)/test_controls.profdata \
		_build/tui_mini.s $(MINI_BIN) _build/keys.bin _build/screen.txt \
		_build/tui_profile.s $(MINI_PROFILE_BIN) _build/empty.bin _build/profile.txt \
		_build/mini_runcheck.s _build/mini_runcheck.bin _build/runcheck.txt \
		_build/mini_bench_main.c _build/mini_bench.s _build/mini_bench.bin \
		_build/bench.txt _build/noscript.txt

.PHONY: all clean test coverage unity win32 mini mini-sim mini-run mini-profile mini-runcheck mini-bench mini-perf
