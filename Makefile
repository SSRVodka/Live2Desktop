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

CMAKE_FLAGS += -DGGML_BLAS=ON
CMAKE_BUILD_FLAGS += --config Release
BUILD_DIR := build

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
	@cmake --build $(BUILD_DIR)

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

.PHONY: all build clean help update-config