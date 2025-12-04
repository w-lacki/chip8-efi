CC := clang
LD := lld

CFLAGS := -ffreestanding -MMD -mno-red-zone -std=c11 \
	-I/usr/include/efi -I/usr/include/efi/x86_64 \
	-target x86_64-unknown-windows
LDFLAGS := -flavor link -subsystem:efi_application -entry:efi_main

SRCS := main.c main2.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bootx64.efi: main.o
	$(LD) $(LDFLAGS) $< -out:$@

-include $(SRCS:.c=.d)

.PHONY: clean all default

all: bootx64.efi
