# nasm for Windows has a "w" at the end
ASM = @nasmw

# only Windows specific output files and rules
# the first two targets generate the prefix.h header
# note this requires that OSOBJS be the first target
#
OSOBJS = $(OBJ)/windows/winmain.o $(OBJ)/windows/fileio.o $(OBJ)/windows/config.o \
	 $(OBJ)/windows/ticker.o $(OBJ)/windows/fronthlp.o $(OBJ)/windows/video.o \
	 $(OBJ)/windows/input.o $(OBJ)/windows/sound.o $(OBJ)/windows/blit.o \
	 $(OBJ)/windows/snprintf.o $(OBJ)/windows/rc.o $(OBJ)/windows/misc.o \
	 $(OBJ)/windows/window.o $(OBJ)/windows/wind3d.o $(OBJ)/windows/wind3dfx.o \
	 $(OBJ)/windows/winddraw.o \
	 $(OBJ)/windows/asmblit.o $(OBJ)/windows/asmtile.o

ifdef MESS
CFLAGS += -DWINUI -DEMULATORDLL=\"$(EMULATORDLL)\"
OSOBJS += \
	$(OBJ)/mess/windows/dirio.o		\
	$(OBJ)/mess/windows/dirutils.o	\
	$(OBJ)/mess/windows/messwin.o	\
	$(OBJ)/mess/windows/configms.o	\
	$(OBJ)/mess/windows/menu.o		\
	$(OBJ)/mess/windows/dialog.o	\
	$(OBJ)/mess/windows/tapedlg.o	\
	$(OBJ)/mess/windows/parallel.o	\
	$(OBJ)/mess/windows/strconv.o
endif 

RESFILE=$(OBJ)/mess/windows/mess.res

# uncomment this line to enable guard pages on all memory allocations
ifdef DEBUG
ifdef MSVC
#OSOBJS += $(OBJ)/windows/winalloc.o
endif
endif

# video blitting functions
$(OBJ)/windows/asmblit.o: src/windows/asmblit.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# tilemap blitting functions
$(OBJ)/windows/asmtile.o: src/windows/asmtile.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# add our prefix files to the mix
ifndef MSVC
CFLAGS += -mwindows -include src/$(MAMEOS)/winprefix.h
else
CFLAGS += /FI"windows/winprefix.h"
endif

# add the windows libaries
ifndef MSVC
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid -lwinmm
else
LIBS += dinput.lib
endif

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./

# if building with a UI, set the C flags and include the ui.mak
ifneq ($(WINUI),)
CFLAGS+= -DWINUI=1
include src/ui/ui.mak
endif
