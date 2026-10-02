CC      = cc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic
LDLIBS  = -lncurses

TARGET  = demo
UNITY_TARGET = demo-unity
TEST_TARGET = test/test_controls
TEST_SRC = test/test_main.c test/test_support.c \
	test/test_listbox.c test/test_edit.c test/test_button.c \
	test/test_window.c test/test_menu.c test/test_statusbar.c \
	test/test_label.c test/test_core.c test/test_checkbox.c \
	test/test_radiobutton.c test/test_combobox.c
TUI_SRC = tui.c tui_window.c tui_button.c tui_label.c \
	tui_edit.c tui_listbox.c tui_menu.c tui_statusbar.c \
	tui_checkbox.c tui_radiobutton.c tui_combobox.c
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

clean:
	rm -f $(TARGET) $(UNITY_TARGET) $(TEST_TARGET) $(COVERAGE_TARGET) \
		$(COVERAGE_DIR)/test_controls.profraw \
		$(COVERAGE_DIR)/test_controls.profdata

.PHONY: all clean test coverage unity