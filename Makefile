CC := gcc
LD := ld
OBJCOPY := objcopy

LIB_DIR := /usr/lib

SRC := main.c
OBJ := main.o
SO := main.so
EFI := bootx64.efi

CFLAGS := -c \
          -fno-stack-protector \
          -fpic \
          -fshort-wchar \
          -mno-red-zone \
          -DEFI_FUNCTION_WRAPPER

LDFLAGS := $(LIB_DIR)/crt0-efi-x86_64.o \
           -nostdlib \
           -znocombreloc \
           -T $(LIB_DIR)/elf_x86_64_efi.lds \
           -shared \
           -Bsymbolic \
           -L $(LIB_DIR) \
           -lgnuefi -lefi

OBJCOPY_FLAGS := -j .text \
                 -j .sdata \
                 -j .data \
                 -j .rodata \
                 -j .dynamic \
                 -j .dynsym \
                 -j .rel \
                 -j .rela \
                 -j .reloc \
                 --target=efi-app-x86_64

all: $(EFI)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(SO): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

$(EFI): $(SO)
	$(OBJCOPY) $(OBJCOPY_FLAGS) $< $@

clean:
	rm -f $(OBJ) $(SO) $(EFI)
