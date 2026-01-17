#include <efi/efi.h>
#include <efi/efilib.h>
#include "emulator.c"

EFI_FILE_HANDLE get_volume(EFI_HANDLE image) {
    EFI_LOADED_IMAGE *loaded_image;
    uefi_call_wrapper(BS->HandleProtocol, 3, image, &gEfiLoadedImageProtocolGuid, (void **) &loaded_image);
    return LibOpenRoot(loaded_image->DeviceHandle);
}

UINT64 get_file_size(EFI_FILE_HANDLE FileHandle) {
    EFI_FILE_INFO *file_info = LibFileInfo(FileHandle);
    UINT64 size = file_info->FileSize;
    FreePool(file_info);
    return size;
}

bool load_rom_from_file(EFI_HANDLE image, CHAR16 *file_name) {
    EFI_FILE_HANDLE volume_handle = get_volume(image);
    EFI_FILE_HANDLE file_handle;
    if (EFI_ERROR(uefi_call_wrapper(volume_handle->Open, 5, volume_handle, &file_handle, file_name, EFI_FILE_MODE_READ,
        EFI_FILE_READ_ONLY))) {
        return false;
    }

    UINT64 size = get_file_size(file_handle);
    UINT8 *buffer = AllocatePool(size);
    uefi_call_wrapper(file_handle->Read, 3, file_handle, &size, buffer);

    load(buffer, size);

    FreePool(buffer);
    FreePool(file_handle);
    FreePool(volume_handle);
    return true;
}

CHAR16 *read_input() {
    CHAR16 *input = AllocatePool(256 * sizeof(CHAR16));
    Print(L"ROM path: ");
    Input(L"", input, 255);
    return input;
}

void get_rom_from_user_input(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
    uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);

    CHAR16 *input = read_input();
    if (!load_rom_from_file(image, input)) {
        FreePool(input);
        get_rom_from_user_input(image, system_table);
        return;
    }

    FreePool(input);
}

void render(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {
    if (!drawFlag) return;
    drawFlag = false;

    UINTN screen_width = gop->Mode->Info->HorizontalResolution;
    UINTN screen_height = gop->Mode->Info->VerticalResolution;

    static UINT32 *framebuffer;
    if (!framebuffer) {
        framebuffer = AllocatePool(screen_width * screen_height * sizeof(UINT32));
    }

    UINTN scale_x = screen_width / 64;
    UINTN scale_y = screen_height / 32;

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            for (UINTN dy = 0; dy < scale_y; dy++) {
                for (UINTN dx = 0; dx < scale_x; dx++) {
                    UINTN fb_x = x * scale_x + dx;
                    UINTN fb_y = y * scale_y + dy;
                    framebuffer[fb_y * screen_width + fb_x] = gfx[y * 64 + x] ? 0x00FFFFFF : 0x00000000;
                }
            }
        }
    }

    uefi_call_wrapper(gop->Blt,
                      10,
                      gop,
                      framebuffer,
                      EfiBltBufferToVideo,
                      0, 0,
                      0, 0,
                      screen_width, screen_height,
                      0
    );
}

UINT8 map_key(EFI_INPUT_KEY key) {
    switch (key.UnicodeChar) {
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0xC;
        case 'q':
        case 'Q': return 0x4;
        case 'w':
        case 'W': return 0x5;
        case 'e':
        case 'E': return 0x6;
        case 'r':
        case 'R': return 0xD;
        case 'a':
        case 'A': return 0x7;
        case 's':
        case 'S': return 0x8;
        case 'd':
        case 'D': return 0x9;
        case 'f':
        case 'F': return 0xE;
        case 'z':
        case 'Z': return 0xA;
        case 'x':
        case 'X': return 0x0;
        case 'c':
        case 'C': return 0xB;
        case 'v':
        case 'V': return 0xF;
        default: return 0xFF;
    }
}

void read_keypad(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
    EFI_INPUT_KEY input_key;
    while (uefi_call_wrapper(system_table->ConIn->ReadKeyStroke,
                             2,
                             system_table->ConIn,
                             &input_key) == EFI_SUCCESS) {
        if (input_key.ScanCode == SCAN_ESC) {
            get_rom_from_user_input(image, system_table);
            return;
        }
        UINT8 mapped_key = map_key(input_key);
        if (mapped_key != 0xFF) {
            key[mapped_key] = 1;
        }
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image, system_table);

    uefi_call_wrapper(system_table->ConOut->EnableCursor, 2, system_table->ConOut, FALSE);

    Print(L"CHIP-8 UEFI Emulator\n");
    get_rom_from_user_input(image, system_table);

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    uefi_call_wrapper(system_table->BootServices->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL,
                      (void**)&gop);
    for (;;) {
        uefi_call_wrapper(system_table->BootServices->Stall, 1, 2000);
        read_keypad(image, system_table);
        run_cycle();
        render(gop);
    }
}
