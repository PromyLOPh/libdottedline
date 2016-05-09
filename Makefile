PREFIX=install
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

#### Setup ####
XMCLIB = /home/lbraun/projekte/treufunk/software/xmclib
SRC = $(wildcard src/*.c)
UC = XMC4500
UC_TYPE = F100
UC_MEM = 1024
TOOLCHAIN = arm-none-eabi
CPU = cortex-m4
FPU = fpv4-sp-d16
FABI = hard
GDB_ARGS = -ex "target extended-remote :3333" -ex "monitor reset halt"

CMSIS = $(XMCLIB)/CMSIS/
CMSIS_SRC = $(CMSIS)/Infineon/$(UC)_series/Source/
LINKER_FILE = $(CMSIS_SRC)/GCC/$(UC)x$(UC_MEM).ld

CC   = $(TOOLCHAIN)-gcc
AR   = $(TOOLCHAIN)-ar
GDB  = $(TOOLCHAIN)-gdb

CFLAGS = -mthumb -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI)
CFLAGS+= -O0 -ggdb -ffunction-sections -fdata-sections
CFLAGS+= -MD -std=c11 -Wall -Werror -fstrict-aliasing -fms-extensions
CFLAGS+= -DXMC_ASSERT_ENABLE -DXMC_USER_ASSERT_FUNCTION
CFLAGS+= -fmessage-length=0 -I$(CMSIS)/Include
CFLAGS+= -I$(CMSIS)/Infineon/Include
CFLAGS+= -I$(CMSIS)/Infineon/$(UC)_series/Include
CFLAGS+= -I$(XMCLIB)/XMCLib/inc/
#CFLAGS+= -nostdlib -D__SKIP_LIBC_INIT_ARRAY
# define uc type for xmclib
CFLAGS+= -D$(UC)_$(UC_TYPE)x$(UC_MEM)
LFLAGS = -L$(CMSIS)/Lib/GCC -Wl,--gc-sections -nostartfiles -L$(XMCLIB)
CPFLAGS = -Obinary
ODFLAGS = -S

OBJS  = $(SRC:.c=.o)
LIBS = -lxmc
INSTALLHEADERS=src/8b10b.h

#### Rules ####
all: src/libdottedline.a bin/benchmark.axf

-include $(SRC:.c=.d)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

src/libdottedline.a: src/8b10b.o
	$(AR) rcs $@ $<

src/8b10btables.h: eightbtenb/__init__.py
	python3 $< > $@

bin/benchmark.axf: src/benchmark.o src/uart.o src/util.o src/syscalls.o src/libdottedline.a | bin/
	$(CC) -T $(LINKER_FILE) $(LFLAGS) -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI) -o $@ $^ $(LIBS)

bin/:
	mkdir -p bin

.PHONY: clean install gdb

install: $(INSTALLHEADERS) src/libdottedline.a
	install -d $(LIBDIR)
	install -m 644 src/libdottedline.a $(LIBDIR)
	install -d $(INCDIR)
	install -m 644 $(INSTALLHEADERS) $(INCDIR)

clean:
	rm -f $(OBJS) src/libdottedline.a src/8b10btables.h bin/benchmark.axf

gdb: bin/benchmark.axf
	$(GDB) bin/benchmark.axf $(GDB_ARGS)

