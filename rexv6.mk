
ifndef TOP_SRCDIR
    TOP_SRCDIR = .
endif

INCLDIR=$(TOP_SRCDIR)/include
BUILDDIR=$(TOP_SRCDIR)/build

CC = gcc
CXX = g++
LD = ld

OBJCOPY = objcopy
OBJDUMP = objdump
RANLIB = ranlib
QEMU = qemu-system-i386.exe

COMMONCFLAGS += -nostdinc -fno-pic -fno-builtin -fno-stack-protector -Wall -Wextra -m32 -I$(INCLDIR)
COMMONCFLAGS += -fno-pie -static -fno-strict-aliasing -fno-omit-frame-pointer
COMMONCFLAGS +=  -fvar-tracking-assignments -g -ggdb 

# don't optimize for most code in Debug mode
#COMMONCFLAGS += -O2


CFLAGS += $(COMMONCFLAGS)
CFLAGS += -std=gnu11

CXXFLAGS +=  $(COMMONCFLAGS)
CXXFLAGS += -std=gnu++14 -mno-sse -fno-exceptions -fno-rtti -ffreestanding -nostdlib -mno-red-zone 
CXXFLAGS += -fno-builtin -Wall -m32 -nostdinc -fpermissive -fno-stack-protector -fpermissive 

ASFLAGS += -g -m32 -Wa,-divide -nostdinc -I$(INCLDIR)
LDFLAGS += -m elf_i386 --omagic -nostdlib

# Debug Settings
GDBPORT = 26000

# Emulation Settings
CPUS = 4
RAM = 4096
