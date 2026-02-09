CXX = g++
MCS = mcs

BUILD_DIR = build
NATIVE_SRC = native/hello.cpp
MANAGED_SRC = managed/Program.cs

DYLIB = $(BUILD_DIR)/libhello.dylib
EXE = $(BUILD_DIR)/Program.exe

all: $(DYLIB) $(EXE)

$(DYLIB): $(NATIVE_SRC) | $(BUILD_DIR)
	$(CXX) -shared -o $@ $<

$(EXE): $(MANAGED_SRC) | $(BUILD_DIR)
	$(MCS) -out:$@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	DYLD_LIBRARY_PATH=$(BUILD_DIR) mono $(EXE)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all     Build native .dylib and C# .exe (default)"
	@echo "  run     Build and run the program with Mono"
	@echo "  clean   Remove build artifacts"
	@echo "  help    Show this help message"

.PHONY: all run clean help
