######################################
# target
######################################
TARGET = DB48X

######################################
# building variables
######################################
ifdef DEBUG
OPT=debug
else
OPT=release
endif

# Warning: macOSX only
MOUNTPOINT=/Volumes/DM42/
EJECT=hdiutil eject $(MOUNTPOINT)


#==============================================================================
#
#  Primary build rules
#
#==============================================================================

# default action: build all
all: $(TARGET).pgm help/$(TARGET).html
install: all help/$(TARGET).md
	(tar cf - $(TARGET).pgm help/$(TARGET).md | \
	(cd $(MOUNTPOINT) && tar xvf -)) && $(EJECT)
sim: sim/simulator.mak recorder/config.h help/$(TARGET).md .ALWAYS
	cd sim; make -f $(<F)
sim/simulator.mak: sim/simulator.pro
	cd sim; qmake $(<F) -o $(@F) CONFIG+=$(OPT)
ttf2font: tools/ttf2fonts/ttf2fonts
tools/ttf2fonts/ttf2fonts: tools/ttf2font/ttf2font.cpp tools/ttf2font/Makefile
	cd tools/ttf2font; $(MAKE)

#BASE_FONT=fonts/C43StandardFont.ttf
BASE_FONT=fonts/FogSans-ddd.ttf
fonts/EditorFont.cc: ttf2font $(BASE_FONT)
	tools/ttf2font/ttf2font -s 48 EditorFont $(BASE_FONT) $@
fonts/StackFont.cc: ttf2font $(BASE_FONT)
	tools/ttf2font/ttf2font -s 32 StackFont $(BASE_FONT) $@
fonts/HelpFont.cc: ttf2font $(BASE_FONT)
	tools/ttf2font/ttf2font -s 18 HelpFont $(BASE_FONT) $@
help/$(TARGET).md: $(wildcard doc/*.md doc/calc-help/*.md doc/commands/*.md)
	cat $^ > $@

debug-%:
	$(MAKE) $* OPT=debug
release-%:
	$(MAKE) $* OPT=release


#######################################
# pathes
#######################################
# Build path
BUILD = build/$(OPT)

# Path to aux build scripts (including trailing /)
# Leave empty for scripts in PATH
TOOLS = tools

######################################
# System sources
######################################
C_INCLUDES += -Idmcp
C_SOURCES += dmcp/sys/pgm_syscalls.c
ASM_SOURCES = dmcp/startup_pgm.s

#######################################
# Custom section
#######################################

# Includes
C_INCLUDES += -Isrc/dm42 -Isrc -Iinc

# C sources
C_SOURCES +=

# Floating point sizes
DECIMAL_SIZES=32 64
DECIMAL_SOURCES=$(DECIMAL_SIZES:%=src/decimal-%.cc)

# C++ sources
CXX_SOURCES +=				\
	src/dm42/target.cc		\
	src/main.cc			\
	src/input.cc			\
	src/stack.cc			\
	src/menu.cc			\
	src/util.cc			\
	src/settings.cc			\
	src/runtime.cc			\
	src/object.cc			\
	src/command.cc			\
	src/integer.cc			\
	src/decimal128.cc		\
	$(DECIMAL_SOURCES)		\
	src/text.cc		        \
	src/symbol.cc			\
	src/algebraic.cc		\
	src/arithmetic.cc		\
	src/functions.cc		\
	src/list.cc			\
	src/font.cc			\
	fonts/HelpFont.cc		\
	fonts/EditorFont.cc		\
	fonts/StackFont.cc



# Generate the sized variants of decimal128
src/decimal-%.cc: src/decimal128.cc src/decimal-%.h
	sed -e s/decimal128.h/decimal-$*.h/g -e s/128/$*/g $< > $@
src/decimal-%.h: src/decimal128.h
	sed -e s/128/$*/g -e s/leb$*/leb128/g $< > $@

# ASM sources
#ASM_SOURCES += src/xxx.s

# Additional defines
#C_DEFS += -DXXX

# Intel library related defines
DEFINES += \
	DECIMAL_CALL_BY_REFERENCE \
	DECIMAL_GLOBAL_ROUNDING \
	DECIMAL_GLOBAL_ROUNDING_ACCESS_FUNCTIONS \
	DECIMAL_GLOBAL_EXCEPTION_FLAGS \
	DECIMAL_GLOBAL_EXCEPTION_FLAGS_ACCESS_FUNCTIONS \
	$(DEFINES_$(OPT))
DEFINES_debug=DEBUG
DEFINES_release=RELEASE

C_DEFS += $(DEFINES:%=-D%)

# Libraries
LIBS += lib/gcc111libbid_hard.a

# Recorder and dependencies
recorder/config.h: recorder/recorder.h recorder/Makefile
	cd recorder && $(MAKE)
$(BUILD)/recorder.o $(BUILD)/recorder_ring.o: recorder/config.h

# ---


#######################################
# binaries
#######################################
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
AS = arm-none-eabi-gcc -x assembler-with-cpp
OBJCOPY = arm-none-eabi-objcopy
AR = arm-none-eabi-ar
SIZE = arm-none-eabi-size
HEX = $(OBJCOPY) -O ihex
BIN = $(OBJCOPY) -O binary -S

#######################################
# CFLAGS
#######################################
# macros for gcc
AS_DEFS =
C_DEFS += -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))"
AS_INCLUDES =


CPUFLAGS += -mthumb -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16

# compile gcc flags
ASFLAGS = $(CPUFLAGS) $(AS_DEFS) $(AS_INCLUDES) $(ASFLAGS_$(OPT)) -Wall -fdata-sections -ffunction-sections
CFLAGS  = $(CPUFLAGS) $(C_DEFS) $(C_INCLUDES) $(CFLAGS_$(OPT)) -Wall -fdata-sections -ffunction-sections
CFLAGS += -Wno-misleading-indentation
DBGFLAGS = -g

CFLAGS_debug += -O0 -DDEBUG
CFLAGS_release += -O4 -Os

CFLAGS  += $(DBGFLAGS)
LDFLAGS += $(DBGFLAGS)

# Generate dependency information
CFLAGS += -MD -MP -MF .dep/$(@F).d

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = src/stm32_program.ld
LIBDIR =
LDFLAGS = $(CPUFLAGS) -T$(LDSCRIPT) $(LIBDIR) $(LIBS) \
	-Wl,-Map=$(BUILD)/$(TARGET).map,--cref \
	-Wl,--gc-sections \
	-Wl,--wrap=_malloc_r




#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# C++ sources
OBJECTS += $(addprefix $(BUILD)/,$(notdir $(CXX_SOURCES:.cc=.o)))
vpath %.cc $(sort $(dir $(CXX_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -Wno-packed-bitfield-compat

$(BUILD)/%.o: %.c Makefile | $(BUILD)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD)/%.o: %.cc Makefile | $(BUILD)
	$(CXX) -c $(CXXFLAGS) -Wa,-a,-ad,-alms=$(BUILD)/$(notdir $(<:.cc=.lst)) $< -o $@

$(BUILD)/%.o: %.s Makefile | $(BUILD)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
$(TARGET).pgm: $(BUILD)/$(TARGET).elf Makefile
	$(OBJCOPY) --remove-section .qspi -O ihex    $<  $(BUILD)/$(TARGET)_flash.hex
	$(OBJCOPY) --remove-section .qspi -O binary  $<  $(BUILD)/$(TARGET)_flash.bin
	$(OBJCOPY) --only-section   .qspi -O ihex    $<  $(BUILD)/$(TARGET)_qspi.hex
	$(OBJCOPY) --only-section   .qspi -O binary  $<  $(BUILD)/$(TARGET)_qspi.bin
	$(TOOLS)/check_qspi_crc $(TARGET) $(BUILD)/$(TARGET)_qspi.bin src/qspi_crc.h || ( $(MAKE) clean && false )
	$(TOOLS)/add_pgm_chsum $(BUILD)/$(TARGET)_flash.bin $@
	$(SIZE) $<
	wc -c $@

$(OBJECTS): $(DECIMAL_SOURCES)

$(BUILD)/%.hex: $(BUILD)/%.elf | $(BUILD)
	$(HEX) $< $@

$(BUILD)/%.bin: $(BUILD)/%.elf | $(BUILD)
	$(BIN) $< $@

$(BUILD):
	mkdir -p $@

#######################################
# clean up
#######################################
clean:
	-rm -fR .dep build sim/*.o


#######################################
# dependencies
#######################################
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

.PHONY: clean all
.ALWAYS:

# *** EOF ***
