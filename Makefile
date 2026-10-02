CC      = cc
CFLAGS  = -std=c89 -Wall -Wextra -pedantic
LDLIBS  = -lncurses

TARGET  = demo
TEST_TARGET = test/test_controls
COVERAGE_DIR = coverage
COVERAGE_TARGET = $(COVERAGE_DIR)/test_controls
COVERAGE_FLAGS = -fprofile-instr-generate -fcoverage-mapping
LLVM_COV = xcrun llvm-cov
LLVM_PROFDATA = xcrun llvm-profdata

all: $(TARGET)

$(TARGET): main.c tui.c tui.h console.h console_ncurses.c demo.c
	$(CC) $(CFLAGS) -DTUI_BACKEND_NCURSES main.c -o $(TARGET) $(LDLIBS)

$(TEST_TARGET): test/test_controls.c tui.c tui.h console.h
	$(CC) $(CFLAGS) -I. test/test_controls.c tui.c -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(COVERAGE_TARGET): test/test_controls.c tui.c tui.h console.h
	mkdir -p $(COVERAGE_DIR)
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) -I. test/test_controls.c tui.c -o $(COVERAGE_TARGET)

coverage: $(COVERAGE_TARGET)
	rm -f $(COVERAGE_DIR)/test_controls.profraw $(COVERAGE_DIR)/test_controls.profdata
	LLVM_PROFILE_FILE=$(COVERAGE_DIR)/test_controls.profraw ./$(COVERAGE_TARGET)
	$(LLVM_PROFDATA) merge -sparse $(COVERAGE_DIR)/test_controls.profraw -o $(COVERAGE_DIR)/test_controls.profdata
	$(LLVM_COV) report ./$(COVERAGE_TARGET) -instr-profile=$(COVERAGE_DIR)/test_controls.profdata --ignore-filename-regex='(^|/)(test/test_controls\.c|tui\.h|console\.h)$$'

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(COVERAGE_TARGET) \
		$(COVERAGE_DIR)/test_controls.profraw \
		$(COVERAGE_DIR)/test_controls.profdata

.PHONY: all clean test coverage