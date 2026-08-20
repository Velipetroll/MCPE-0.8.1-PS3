# ==========================================
# MAKEFILE PARA EL PORT DE MINECRAFT PE 0.8.1 - PS3
# ==========================================

# --- OBJETIVO PRINCIPAL ---
# Esto asegura que si solo escribes "make", compile todo el proyecto.
.PHONY: default all clean
default: all

TARGET = main_ps3

PS3DEV ?= /usr/local/ps3dev
PSL1GHT ?= $(PS3DEV)
PORTLIBS ?= $(PS3DEV)/portlibs/ppu

# CARPETA DE COMPILACION
BUILD_DIR = build

# --- HERRAMIENTAS DE SHADERS (PSL1GHT) ---
CGCOMP := $(PS3DEV)/bin/cgcomp
BIN2S  := $(PS3DEV)/bin/bin2s

# 1. RUTAS DE INCLUSION
INC_FLAGS = -I. \
            -I./minecraftpe/impl \
            -I./minecraftpe/headers \
            -I./stb \
            -I./glm \
            -I./jsoncpp/jsoncpp/include \
            -I./RakNet/Source \
            -I./utf8proc \
            -I$(PORTLIBS)/include \
            -I$(PS3DEV)/portlibs/ppu/include

# 2. BUSQUEDA AUTOMATICA DEL JUEGO
GAME_CPP := $(shell find minecraftpe/impl -type f -name "*.cpp" | grep -v "/android/" | grep -v "minecraftpe/impl/main.cpp" | grep -v "AppPlatform_sdl.cpp")
GAME_C   := $(shell find minecraftpe/impl -type f -name "*.c" | grep -v "/android/")

# Forzamos la inclusión de pcm_data.c por si es la primera vez y aun no se ha extraido
ifeq (,$(findstring minecraftpe/impl/pcm_data.c,$(GAME_C)))
    GAME_C += minecraftpe/impl/pcm_data.c
endif

# 3. BUSQUEDA AUTOMATICA DE LIBRERIAS EXTERNAS
EXT_CPP  := $(shell find jsoncpp/jsoncpp/src/lib_json RakNet/Source -type f -name "*.cpp" | grep -v "Getche.cpp" | grep -v "Gets.cpp" | grep -v "_FindFirst.cpp")
EXT_C    := utf8proc/utf8proc.c

# 4. BUSQUEDA DE SHADERS
SHADERS_VCG := $(wildcard shaders/*.vcg)
SHADERS_FCG := $(wildcard shaders/*.fcg)

OFILES_SHADERS := $(patsubst shaders/%.vcg,$(BUILD_DIR)/shaders/%_vcg.o,$(SHADERS_VCG)) \
                  $(patsubst shaders/%.fcg,$(BUILD_DIR)/shaders/%_fcg.o,$(SHADERS_FCG))

# 5. LISTA FINAL A COMPILAR
OFILES = $(OFILES_SHADERS) \
         $(GAME_CPP:%.cpp=$(BUILD_DIR)/%.o) \
         $(GAME_C:%.c=$(BUILD_DIR)/%.o) \
         $(EXT_CPP:%.cpp=$(BUILD_DIR)/%.o) \
         $(EXT_C:%.c=$(BUILD_DIR)/%.o)

MACHDEP = -mcpu=cell -mhard-float -fmodulo-sched -ffunction-sections -fdata-sections

# Dejamos -g y -O0 por si necesitamos debuggear, luego lo pasamos a -O2
CXXFLAGS = -g -O0 -Wall $(MACHDEP) -I$(PSL1GHT)/ppu/include $(INC_FLAGS) -std=gnu++11 -D__PS3__ -include ps3_polyfill.hpp
CFLAGS   = -g -O0 -Wall $(MACHDEP) -I$(PSL1GHT)/ppu/include $(INC_FLAGS) -D__PS3__ -include ps3_polyfill.hpp

LDFLAGS = -L$(PSL1GHT)/ppu/lib -L$(PORTLIBS)/lib -L$(PS3DEV)/portlibs/ppu/lib $(MACHDEP)
LIBS = -lrsx -lgcm_sys -lio -lsysutil -laudio -lrt -llv2 -lnet -lm -lc -lsysmodule -lpng -lz -lpthread

CC = ppu-gcc
CXX = ppu-g++
LD = ppu-g++

TITLE = Minecraft PE 0.8.1
APPID = MCPE00801

# Encontrar dinámicamente el archivo APK en la raíz
APK_FILE := $(firstword $(wildcard *.apk))

include $(PSL1GHT)/ppu_rules

# ==========================================
# REGLAS DE PREPARACION (APK Y ASSETS)
# ==========================================
mcpe_apk/assets:
	@echo "================================================="
	@echo "[PREPARACION] Buscando y extrayendo archivo APK..."
	@echo "================================================="
	@if [ -z "$(APK_FILE)" ]; then \
		echo "[ERROR] No se encontro ningun archivo .apk en la carpeta raiz."; \
		echo "Por favor, coloca el archivo de MCPE 0.8.1 (ej. mcpe.apk) en este directorio."; \
		exit 1; \
	fi
	@echo "[INFO] Extrayendo $(APK_FILE) con Python..."
	@python -c "import zipfile; zipfile.ZipFile('$(APK_FILE)', 'r').extractall('mcpe_apk')"
	@touch mcpe_apk/assets

minecraftpe/impl/pcm_data.c: mcpe_apk/assets
	@echo "================================================="
	@echo "[PREPARACION] Generando pcm_data.c desde el APK..."
	@echo "================================================="
	@python tools/get_sound_data.py mcpe_apk/lib/armeabi-v7a/libminecraftpe.so
	@mv pcm_data.c minecraftpe/impl/pcm_data.c

# ==========================================
# REGLAS PRINCIPALES
# ==========================================
all: $(TARGET).pkg
	@echo "================================================="
	@echo "[PASO 1] Armando carpeta oficial de PS3 (JB Folder)..."
	@echo "================================================="
	@mkdir -p $(BUILD_DIR)/$(APPID)/USRDIR
	@if [ -f $(TARGET).self ]; then \
		cp $(TARGET).self $(BUILD_DIR)/$(APPID)/USRDIR/EBOOT.BIN; \
		echo "[EXITO] EBOOT.BIN creado correctamente."; \
	else \
		echo "[ERROR] El archivo .self NO se generó. Hubo un error de compilación previo."; \
		exit 1; \
	fi
	@if [ -f param.sfo ]; then \
		cp param.sfo $(BUILD_DIR)/$(APPID)/PARAM.SFO; \
		echo "[EXITO] PARAM.SFO (Portada) creado correctamente."; \
	fi
	@if [ -d "mcpe_apk/assets" ]; then \
		cp -r mcpe_apk/assets $(BUILD_DIR)/$(APPID)/USRDIR/; \
		echo "[EXITO] Texturas copiadas desde 'mcpe_apk/assets'"; \
	elif [ -d "assets" ]; then \
		cp -r assets $(BUILD_DIR)/$(APPID)/USRDIR/; \
		echo "[EXITO] Texturas copiadas desde 'assets'"; \
	else \
		echo "[ADVERTENCIA] No se encontraron las texturas, el juego crasheara."; \
	fi
	@echo "================================================="
	@echo "[PASO 2] Moviendo ejecutables a $(BUILD_DIR)/..."
	@echo "================================================="
	@mv -f $(TARGET).elf $(BUILD_DIR)/ 2>/dev/null || true
	@mv -f $(TARGET).self $(BUILD_DIR)/ 2>/dev/null || true
	@mv -f $(TARGET).fake.self $(BUILD_DIR)/ 2>/dev/null || true
	@mv -f $(TARGET).pkg $(BUILD_DIR)/ 2>/dev/null || true
	@mv -f $(TARGET).gnpdrm.pkg $(BUILD_DIR)/ 2>/dev/null || true
	@rm -f param.sfo
	@echo "[EXITO TOTAL] ¡Todo listo! Copia la carpeta $(BUILD_DIR)/$(APPID) a tu RPCS3 o consola PS3"

# ==========================================
# REGLAS DE COMPILACION DE SHADERS (PSL1GHT)
# ==========================================
$(BUILD_DIR)/shaders/%_vcg.vpo: shaders/%.vcg
	@echo "[CGCOMP-VP] $<"
	@mkdir -p $(dir $@)
	@$(CGCOMP) -v $< $@

$(BUILD_DIR)/shaders/%_fcg.fpo: shaders/%.fcg
	@echo "[CGCOMP-FP] $<"
	@mkdir -p $(dir $@)
	@$(CGCOMP) -f $< $@

$(BUILD_DIR)/shaders/%_vcg.o: $(BUILD_DIR)/shaders/%_vcg.vpo
	@echo "[BIN2S-VP] $<"
	@mkdir -p $(dir $@)
	@$(BIN2S) -a 128 $< | $(CC) -c -x assembler - -o $@

$(BUILD_DIR)/shaders/%_fcg.o: $(BUILD_DIR)/shaders/%_fcg.fpo
	@echo "[BIN2S-FP] $<"
	@mkdir -p $(dir $@)
	@$(BIN2S) -a 128 $< | $(CC) -c -x assembler - -o $@

# ==========================================
# REGLAS DE COMPILACION DE C/C++
# ==========================================
$(BUILD_DIR)/%.o: %.cpp
	@echo "[CXX] $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@echo "[CC] $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# ==========================================
# REGLA DEL LINKER
# ==========================================
# Añadimos 'mcpe_apk/assets' como dependencia del elf para asegurar que el APK se extrae antes de linkear
$(TARGET).elf: $(OFILES) mcpe_apk/assets
	@echo "================================================="
	@echo "[LINKER] Combinando todo en el ejecutable final..."
	@echo "================================================="
	$(CXX) $(OFILES) $(LDFLAGS) $(LIBS) -o $@

clean:
	@echo "Limpiando proyecto..."
	@rm -f *.elf *.self *.fake.self *.pkg *.gnpdrm.pkg param.sfo
	@rm -rf $(BUILD_DIR)/ pkg/ mcpe_apk/
	@rm -f minecraftpe/impl/pcm_data.c
	@find . -type f -name "*.o" -exec rm -f {} +
	@echo "Limpieza completada!"
