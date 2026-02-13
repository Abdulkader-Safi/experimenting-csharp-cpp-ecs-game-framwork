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
MANAGED_CS = managed/Viewer.cs managed/World.cs managed/Components.cs \
             managed/NativeBridge.cs managed/FreeCameraState.cs \
             managed/PhysicsBridge.cs managed/PhysicsWorld.cs
GAMELOGIC_CS_FILES = game_logic/Game.cs game_logic/Systems.cs game_logic/GameConstants.cs
VIEWER_CS = $(MANAGED_CS) $(GAMELOGIC_CS_FILES)
SHADER_DIR = $(BUILD_DIR)/shaders
VERT_SPV = $(SHADER_DIR)/vert.spv
FRAG_SPV = $(SHADER_DIR)/frag.spv
UI_VERT_SPV = $(SHADER_DIR)/ui_vert.spv
UI_FRAG_SPV = $(SHADER_DIR)/ui_frag.spv
VK_ICD = /opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json

# Physics (joltc)
PHYSICS_BUILD = build/physics
PHYSICS_DYLIB = $(BUILD_DIR)/libjoltc.dylib

# --- Hello demo (existing) ---

all: $(DYLIB) $(EXE)

$(DYLIB): $(NATIVE_SRC) | $(BUILD_DIR)
	g++ -shared -o $@ $<

$(EXE): $(MANAGED_SRC) | $(BUILD_DIR)
	$(MCS) -out:$@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# --- Viewer ---

shaders: $(VERT_SPV) $(FRAG_SPV) $(UI_VERT_SPV) $(UI_FRAG_SPV)

$(SHADER_DIR):
	mkdir -p $(SHADER_DIR)

$(VERT_SPV): native/shaders/shader.vert | $(SHADER_DIR)
	glslc $< -o $@

$(FRAG_SPV): native/shaders/shader.frag | $(SHADER_DIR)
	glslc $< -o $@

$(UI_VERT_SPV): native/shaders/ui.vert | $(SHADER_DIR)
	glslc $< -o $@

$(UI_FRAG_SPV): native/shaders/ui.frag | $(SHADER_DIR)
	glslc $< -o $@

$(VIEWER_DYLIB): native/renderer.cpp native/bridge.cpp native/renderer.h native/CMakeLists.txt
	cmake -S native -B $(NATIVE_BUILD) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(NATIVE_BUILD)
	@ln -sf $(NATIVE_BUILD)/compile_commands.json compile_commands.json

$(PHYSICS_DYLIB): native/joltc/CMakeLists.txt
	cmake -S native/joltc -B $(PHYSICS_BUILD) \
		-DJPH_BUILD_SHARED=ON \
		-DJPH_BUILD_TESTS=OFF \
		-DJPH_SAMPLES=OFF \
		-DCMAKE_OSX_ARCHITECTURES=arm64 \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(PHYSICS_BUILD) --config Release --target joltc
	cp $(PHYSICS_BUILD)/lib/libjoltc.dylib $(BUILD_DIR)/libjoltc.dylib

$(VIEWER_EXE): $(VIEWER_CS) | $(BUILD_DIR)
	$(MCS) -out:$@ $(VIEWER_CS)

viewer: shaders $(VIEWER_DYLIB) $(PHYSICS_DYLIB) $(VIEWER_EXE)

run: viewer
	DYLD_LIBRARY_PATH=$(BUILD_DIR):/opt/homebrew/lib VK_ICD_FILENAMES=$(VK_ICD) \
		mono $(VIEWER_EXE)

# --- App bundle ---

app: viewer
	@./build-app.sh

# --- Dev mode (hot reload) ---

ENGINE_CS = managed/World.cs managed/Components.cs \
            managed/NativeBridge.cs managed/FreeCameraState.cs \
            managed/PhysicsBridge.cs managed/PhysicsWorld.cs
ENGINE_DLL = $(BUILD_DIR)/Engine.dll

GAMELOGIC_CS = game_logic/Game.cs game_logic/Systems.cs game_logic/GameConstants.cs
GAMELOGIC_DLL = $(BUILD_DIR)/GameLogic.dll

VIEWERDEV_CS = managed/Viewer.cs managed/HotReload.cs
VIEWERDEV_EXE = $(BUILD_DIR)/ViewerDev.exe

$(ENGINE_DLL): $(ENGINE_CS) | $(BUILD_DIR)
	$(MCS) -target:library -out:$@ $(ENGINE_CS)

$(GAMELOGIC_DLL): $(GAMELOGIC_CS) $(ENGINE_DLL) | $(BUILD_DIR)
	$(MCS) -target:library -r:$(ENGINE_DLL) -out:$@ $(GAMELOGIC_CS)

$(VIEWERDEV_EXE): $(VIEWERDEV_CS) $(ENGINE_DLL) $(GAMELOGIC_DLL) | $(BUILD_DIR)
	$(MCS) -define:HOT_RELOAD -r:$(ENGINE_DLL) -r:$(GAMELOGIC_DLL) -out:$@ $(VIEWERDEV_CS)

dev: shaders $(VIEWER_DYLIB) $(PHYSICS_DYLIB) $(ENGINE_DLL) $(GAMELOGIC_DLL) $(VIEWERDEV_EXE)
	DYLD_LIBRARY_PATH=$(BUILD_DIR):/opt/homebrew/lib VK_ICD_FILENAMES=$(VK_ICD) \
		MONO_PATH=$(BUILD_DIR) mono $(VIEWERDEV_EXE)

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
	@echo "  dev          Build and run with hot reload (edit C# game logic live)"
	@echo "  app          Build macOS .app bundle (requires Mono installed)"
	@echo "  shaders      Compile GLSL shaders to SPIR-V"
	@echo "  clean        Remove build artifacts"
	@echo "  help         Show this help message"

.PHONY: all run clean help shaders viewer app dev
