gcc main.c                             \
      -c                                 \
      -fno-stack-protector               \
      -fpic                              \
      -fshort-wchar                      \
      -mno-red-zone                      \
      -I /path/to/gnu-efi/headers        \
      -I /path/to/gnu-efi/headers/x86_64 \
      -DEFI_FUNCTION_WRAPPER             \
      -o main.o

ld main.o \
   /usr/lib/crt0-efi-x86_64.o \
   -nostdlib \
   -znocombreloc \
   -T /usr/lib/elf_x86_64_efi.lds \
   -shared \
   -Bsymbolic \
   -L /usr/lib \
   -lgnuefi -lefi \
   -o main.so


objcopy -j .text                \
          -j .sdata               \
          -j .data                \
          -j .rodata              \
          -j .dynamic             \
          -j .dynsym              \
          -j .rel                 \
          -j .rela                \
          -j .reloc               \
          --target=efi-app-x86_64 \
          main.so                 \
          bootx64.efi
cp bootx64.efi root/efi/boot
qemu-system-x86_64 \
  -drive if=pflash,format=raw,file=/home/anon/dev/uczelnia/os/chip8-efi/OVMF.fd \
  -drive format=raw,file=fat:rw:root \
  -net none \
  -nographic