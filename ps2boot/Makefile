EE_BIN = ps2snes2005_boot.elf

CORE_DIR = ..
SRC_DIR  = $(CORE_DIR)/source
COMM_DIR = $(CORE_DIR)/libretro-common

CORE_SRCS = \
	$(SRC_DIR)/c4.c \
	$(SRC_DIR)/c4emu.c \
	$(SRC_DIR)/cheats2.c \
	$(SRC_DIR)/cheats.c \
	$(SRC_DIR)/clip.c \
	$(SRC_DIR)/cpu.c \
	$(SRC_DIR)/cpuexec.c \
	$(SRC_DIR)/cpuops.c \
	$(SRC_DIR)/data.c \
	$(SRC_DIR)/dma.c \
	$(SRC_DIR)/dsp1.c \
	$(SRC_DIR)/fxemu.c \
	$(SRC_DIR)/fxinst.c \
	$(SRC_DIR)/gfx.c \
	$(SRC_DIR)/getset.c \
	$(SRC_DIR)/globals.c \
	$(SRC_DIR)/memmap.c \
	$(SRC_DIR)/obc1.c \
	$(SRC_DIR)/ppu.c \
	$(SRC_DIR)/sa1.c \
	$(SRC_DIR)/sa1cpu.c \
	$(SRC_DIR)/sdd1.c \
	$(SRC_DIR)/sdd1emu.c \
	$(SRC_DIR)/seta010.c \
	$(SRC_DIR)/seta011.c \
	$(SRC_DIR)/seta018.c \
	$(SRC_DIR)/seta.c \
	$(SRC_DIR)/spc7110.c \
	$(SRC_DIR)/spc7110dec.c \
	$(SRC_DIR)/srtc.c \
	$(SRC_DIR)/tile.c \
	$(SRC_DIR)/apu.c \
	$(SRC_DIR)/soundux.c \
	$(SRC_DIR)/spc700.c \
	$(CORE_DIR)/libretro.c

EE_OBJS = main.o ps2_video.o ps2_launcher_video.o ps2_input.o ps2_menu.o ui/select_menu/select_menu.o ui/select_menu/select_menu_actions.o ui/select_menu/select_menu_render.o ui/select_menu/select_menu_pages.o ui/select_menu/font/select_menu_font.o ui/launcher/launcher.o ui/launcher/launcher_actions.o ui/launcher/launcher_render.o ui/launcher/launcher_font.o ui/launcher/launcher_pages.o ui/launcher/launcher_logo.o ui/launcher/launcher_logo_data.o ui/launcher/launcher_browser.o smw_blob.o $(CORE_SRCS:.c=.o)

EE_INCS += -I$(CORE_DIR) -I$(SRC_DIR) -I$(COMM_DIR)/include
EE_CFLAGS += -O2 -G0 -DLOAD_FROM_MEMORY -DLAGFIX
EE_LIBS += -ldebug -lpacket -ldraw -lgraph -ldma -lpad

EXTRA_TARGETS = $(EE_BIN)

PS2SDK = $(PS2DEV)/ps2sdk
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal



ROM_LOADER_OBJS = \
	rom_loader/rom_loader.o \
	rom_loader/rom_zip.o \
	rom_loader/miniz/miniz.o \
	rom_loader/miniz/miniz_tdef.o \
	rom_loader/miniz/miniz_tinfl.o \
	rom_loader/miniz/miniz_zip.o

EE_OBJS += $(ROM_LOADER_OBJS)
EE_CFLAGS += -I. -Irom_loader -Irom_loader/miniz -DMINIZ_NO_ARCHIVE_WRITING_APIS

$(EE_BIN): $(ROM_LOADER_OBJS)
