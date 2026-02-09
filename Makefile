MCS = mcs

BUILD_DIR = build
NATIVE_BUILD = build/native

# Hello demo
NATIVE_SRC = native/hello.cpp
MANAGED_SRC = managed/Program.cs
DYLIB = $(BUILD_DIR)/libhello.dylib
EXE = $(BUILD_DIR)/Program.exe

# Viewer config
VIEWER_DYLIB = $(BUILD_DIR)/librenderer.dylib
VIEWER_EXE = $(BUILD_DIR)/Viewer.exe
VIEWER_CS = managed/Viewer.cs managed/ecs/World.cs managed/ecs/Components.cs managed/ecs/Systems.cs managed/ecs/NativeBridge.cs
SHADER_DIR = $(BUILD_DIR)/shaders
VERT_SPV = $(SHADER_DIR)/vert.spv
FRAG_SPV = $(SHADER_DIR)/frag.spv
VK_ICD = /opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json

# --- Hello demo (existing) ---

all: $(DYLIB) $(EXE)

$(DYLIB): $(NATIVE_SRC) | $(BUILD_DIR)
	g++ -shared -o $@ $<

$(EXE): $(MANAGED_SRC) | $(BUILD_DIR)
	$(MCS) -out:$@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# --- Viewer ---

shaders: $(VERT_SPV) $(FRAG_SPV)

$(SHADER_DIR):
	mkdir -p $(SHADER_DIR)

$(VERT_SPV): native/shaders/shader.vert | $(SHADER_DIR)
	glslc $< -o $@

$(FRAG_SPV): native/shaders/shader.frag | $(SHADER_DIR)
	glslc $< -o $@

$(VIEWER_DYLIB): native/renderer.cpp native/bridge.cpp native/renderer.h native/CMakeLists.txt
	cmake -S native -B $(NATIVE_BUILD) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(NATIVE_BUILD)
	@ln -sf $(NATIVE_BUILD)/compile_commands.json compile_commands.json

$(VIEWER_EXE): $(VIEWER_CS) | $(BUILD_DIR)
	$(MCS) -out:$@ $(VIEWER_CS)

viewer: shaders $(VIEWER_DYLIB) $(VIEWER_EXE)

run: viewer
	DYLD_LIBRARY_PATH=$(BUILD_DIR):/opt/homebrew/lib VK_ICD_FILENAMES=$(VK_ICD) \
		mono $(VIEWER_EXE)

# --- App bundle ---

app: viewer
	@./build-app.sh

# --- Shared ---

clean:
	rm -rf $(BUILD_DIR) compile_commands.json

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all          Build native .dylib and C# .exe (default)"
	@echo "  run          Build and run the viewer with a sample model"
	@echo "  viewer       Build Vulkan glTF viewer (shaders + native lib + C# exe)"
	@echo "  app          Build macOS .app bundle (requires Mono installed)"
	@echo "  shaders      Compile GLSL shaders to SPIR-V"
	@echo "  clean        Remove build artifacts"
	@echo "  help         Show this help message"

.PHONY: all run clean help shaders viewer app
