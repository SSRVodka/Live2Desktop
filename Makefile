ifeq ($(OS),Windows_NT)
	RM = rd /s /q
	MKDIR = mkdir
	CP = copy
	BUILD_DIR_WIN = $(subst /,\,$(BUILD_DIR))
	CONFIG_DIR = $(BUILD_DIR_WIN)\bin\config
	CONFIG_SRC = config\*
	CMAKE_FLAGS = -DBUILD_SHARED_LIBS=OFF
else
	RM = rm -rf
	MKDIR = mkdir -p
	CP = cp
	CONFIG_DIR = $(BUILD_DIR)/bin/config
	CONFIG_SRC = config/*
endif

APP_NAME=Live2Desktop
CMAKE_FLAGS += -DGGML_BLAS=ON
# debug
# CMAKE_FLAGS += -DDEBUG=ON
# CMAKE_BUILD_FLAGS += --config Debug
# release
CMAKE_BUILD_FLAGS += --config Release
BUILD_DIR := build
PACK_DIR := pack

all: help

help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  build         - compile project"
	@echo "  clean         - clean output files"
	@echo "  help          - print this message"
	@echo "  update-config - update modified config/*"

build:
	@cmake -B $(BUILD_DIR) $(CMAKE_FLAGS)
	@cmake --build $(BUILD_DIR) ${CMAKE_BUILD_FLAGS}

clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR_WIN)" $(RM) "$(BUILD_DIR_WIN)"
else
	@$(RM) $(BUILD_DIR)
endif

update-config:
ifeq ($(OS),Windows_NT)
	@if not exist "$(CONFIG_DIR)" $(MKDIR) "$(CONFIG_DIR)"
	@$(CP) $(CONFIG_SRC) "$(CONFIG_DIR)"
else
	@$(MKDIR) $(CONFIG_DIR)
	@$(CP) $(CONFIG_SRC) $(CONFIG_DIR)
endif

pack: build
ifeq ($(OS),Windows_NT)
	@echo "[ERROR] pack not supported on Windows"
else
	$(RM) $(PACK_DIR)
	# copying binaries & configurations
	$(MKDIR) $(PACK_DIR)/usr/bin $(PACK_DIR)/usr/lib $(PACK_DIR)/usr/share
	$(MKDIR) $(PACK_DIR)/usr/share/applications $(PACK_DIR)/usr/share/icons/hicolor/128x128/apps
	$(CP) misc/${APP_NAME}.desktop $(PACK_DIR)/usr/share/applications/
	$(CP) misc/${APP_NAME}.ico $(PACK_DIR)/usr/share/icons/hicolor/128x128/apps/
	$(CP) misc/${APP_NAME}.png $(PACK_DIR)
	$(CP) $(BUILD_DIR)/bin/${APP_NAME} $(PACK_DIR)/usr/bin/
	$(CP) -r $(BUILD_DIR)/bin/config $(PACK_DIR)/usr/bin/
	$(CP) -r $(BUILD_DIR)/bin/models $(PACK_DIR)/usr/bin/
	$(CP) -r $(BUILD_DIR)/bin/Resources $(PACK_DIR)/usr/bin/
	$(CP) -r $(BUILD_DIR)/lib/* $(PACK_DIR)/usr/lib/
	# patching rpath of dynamic libraries
	find $(PACK_DIR)/usr/lib/ -name "*.so*" | xargs -i patchelf --set-rpath '$$ORIGIN' {}
	# deploy with linuxdeployqt
	# linuxdeployqt $(PACK_DIR)/usr/share/applications/Live2Desktop.desktop -extra-plugins=iconengines,platformthemes/libqgtk3.so -appimage
endif

.PHONY: all build clean help update-config