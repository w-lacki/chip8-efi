#include <efi/efi.h>
#include <efi/efilib.h>

#include <stdint.h>
#include "emulator.c"

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = nullptr;

// Scale CHIP-8 64x32 graphics to screen
UINTN scaleX = 1;
UINTN scaleY = 1;

// Draw a single pixel on framebuffer
void draw_pixel(UINTN x, UINTN y, UINT32 color) {
    UINT32 *framebuffer = (UINT32 *)gop->Mode->FrameBufferBase;
    UINTN fbWidth = gop->Mode->Info->HorizontalResolution;
    framebuffer[y * fbWidth + x] = color;
}

// Render CHIP-8 graphics to screen
void render_chip8() {
    UINTN width = gop->Mode->Info->HorizontalResolution;
    UINTN height = gop->Mode->Info->VerticalResolution;
    scaleX = width / 64;
    scaleY = height / 32;

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            UINT32 color = gfx[y * 64 + x] ? 0xFFFFFFFF : 0xFF000000;
            // draw scaled rectangle
            for (UINTN dy = 0; dy < scaleY; dy++) {
                for (UINTN dx = 0; dx < scaleX; dx++) {
                    draw_pixel(x * scaleX + dx, y * scaleY + dy, color);
                }
            }
        }
    }
}

// Map ASCII/UEFI key codes to CHIP-8 keys
uint8_t map_key(EFI_INPUT_KEY key) {
    switch (key.UnicodeChar) {
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0xC;
        case 'q': case 'Q': return 0x4;
        case 'w': case 'W': return 0x5;
        case 'e': case 'E': return 0x6;
        case 'r': case 'R': return 0xD;
        case 'a': case 'A': return 0x7;
        case 's': case 'S': return 0x8;
        case 'd': case 'D': return 0x9;
        case 'f': case 'F': return 0xE;
        case 'z': case 'Z': return 0xA;
        case 'x': case 'X': return 0x0;
        case 'c': case 'C': return 0xB;
        case 'v': case 'V': return 0xF;
        default: return 0xFF; // not mapped
    }
}

unsigned char demo_rom[] = {
    0x00, 0xE0,       // CLS
    0x61, 0x00,       // LD V1, 0
    0x62, 0x00,       // LD V2, 0
    0xA2, 0x0A,       // LD I, 0x20A (sprite address)
    0xD1, 0x25,       // DRW V1, V2, 5
    0x12, 0x00,       // JP 0x200 (loop forever)
    // Sprite data at 0x20A
    0xF0, 0x90, 0xF0, 0x90, 0x90
};

#include <efi/efi.h>
#include <efi/efilib.h>

// Load ROM from file path
EFI_STATUS load_rom_from_file(CHAR16 *filepath, unsigned char **rom_buffer, UINTN *rom_size, EFI_HANDLE ImageHandle) {
    EFI_STATUS status;
    EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *file_system = NULL;
    EFI_FILE_PROTOCOL *root = NULL;
    EFI_FILE_PROTOCOL *file = NULL;
    EFI_FILE_INFO *file_info = NULL;
    UINTN info_size = 0;
    UINTN read_size = 0;

    // Get loaded image protocol
    status = uefi_call_wrapper(
        BS->HandleProtocol,
        3,
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void**)&loaded_image
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to get LoadedImageProtocol: %r\n", status);
        return status;
    }

    // Get file system protocol from the device
    status = uefi_call_wrapper(
        BS->HandleProtocol,
        3,
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void**)&file_system
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to get FileSystemProtocol: %r\n", status);
        return status;
    }

    // Open root directory
    status = uefi_call_wrapper(
        file_system->OpenVolume,
        2,
        file_system,
        &root
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to open root volume: %r\n", status);
        return status;
    }

    // Open the ROM file
    status = uefi_call_wrapper(
        root->Open,
        5,
        root,
        &file,
        filepath,
        EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to open file %s: %r\n", filepath, status);
        uefi_call_wrapper(root->Close, 1, root);
        return status;
    }

    // Get file info to determine size
    info_size = 0;
    status = uefi_call_wrapper(
        file->GetInfo,
        4,
        file,
        &gEfiFileInfoGuid,
        &info_size,
        NULL
    );

    // Allocate buffer for file info
    file_info = AllocatePool(info_size);
    if (!file_info) {
        Print(L"Failed to allocate memory for file info\n");
        uefi_call_wrapper(file->Close, 1, file);
        uefi_call_wrapper(root->Close, 1, root);
        return EFI_OUT_OF_RESOURCES;
    }

    // Get actual file info
    status = uefi_call_wrapper(
        file->GetInfo,
        4,
        file,
        &gEfiFileInfoGuid,
        &info_size,
        file_info
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to get file info: %r\n", status);
        FreePool(file_info);
        uefi_call_wrapper(file->Close, 1, file);
        uefi_call_wrapper(root->Close, 1, root);
        return status;
    }

    *rom_size = (UINTN)file_info->FileSize;
    FreePool(file_info);

    // Check if ROM is too large (CHIP-8 has 4KB memory, ROM starts at 0x200)
    if (*rom_size > (4096 - 0x200)) {
        Print(L"ROM too large: %u bytes (max: %u bytes)\n", *rom_size, 4096 - 0x200);
        uefi_call_wrapper(file->Close, 1, file);
        uefi_call_wrapper(root->Close, 1, root);
        return EFI_BUFFER_TOO_SMALL;
    }

    // Allocate buffer for ROM
    *rom_buffer = AllocatePool(*rom_size);
    if (!*rom_buffer) {
        Print(L"Failed to allocate memory for ROM\n");
        uefi_call_wrapper(file->Close, 1, file);
        uefi_call_wrapper(root->Close, 1, root);
        return EFI_OUT_OF_RESOURCES;
    }

    // Read file into buffer
    read_size = *rom_size;
    status = uefi_call_wrapper(
        file->Read,
        3,
        file,
        &read_size,
        *rom_buffer
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to read file: %r\n", status);
        FreePool(*rom_buffer);
        *rom_buffer = NULL;
        uefi_call_wrapper(file->Close, 1, file);
        uefi_call_wrapper(root->Close, 1, root);
        return status;
    }

    // Close file and root
    uefi_call_wrapper(file->Close, 1, file);
    uefi_call_wrapper(root->Close, 1, root);

    Print(L"Loaded ROM: %s (%u bytes)\n", filepath, read_size);
    return EFI_SUCCESS;
}

CHAR16* read_input(CHAR16 *prompt) {
    CHAR16 *input_buffer = AllocatePool(256 * sizeof(CHAR16));
    if (!input_buffer) {
        return NULL;
    }

    Print(prompt);
    Input(L"", input_buffer, 255);

    return input_buffer;
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    Print(L"CHIP-8 UEFI Emulator\n");

    // Locate GOP
    EFI_STATUS status = uefi_call_wrapper(
        SystemTable->BootServices->LocateProtocol,
        3,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        (void**)&gop
    );

    uefi_call_wrapper(SystemTable->ConOut->EnableCursor, 2, SystemTable->ConOut, FALSE);
    uefi_call_wrapper(SystemTable->ConOut->ClearScreen, 1, SystemTable->ConOut);

    if (EFI_ERROR(status)) {
        Print(L"GOP not found!\n");
        return status;
    }

    UINTN width = gop->Mode->Info->HorizontalResolution;
    UINTN height = gop->Mode->Info->VerticalResolution;
    Print(L"Screen resolution: %u x %u\n", width, height);

    // Clear screen
    UINT32 *framebuffer = (UINT32 *)gop->Mode->FrameBufferBase;
    for (UINTN i = 0; i < gop->Mode->FrameBufferSize / sizeof(UINT32); i++) {
        framebuffer[i] = 0xFF000000; // black
    }

    CHAR16 *filename = read_input(L"Enter ROM filename: ");

    if (!filename) {
        Print(L"Failed to allocate input buffer\n");
        return EFI_OUT_OF_RESOURCES;
    }

    // Load ROM from file
    unsigned char *rom_buffer = NULL;
    UINTN rom_size = 0;

     status = load_rom_from_file(filename, &rom_buffer, &rom_size, ImageHandle);

    FreePool(filename); // Don't forget to free the input buffer

    if (EFI_ERROR(status)) {
        Print(L"Failed to load ROM, using demo instead\n");
        Exit(0, 0, 0);
    } else {
        load(rom_buffer, (unsigned int)rom_size);
        FreePool(rom_buffer);
    }

    Print(L"Loaded demo!\n");
    // Main emulation loop
    while (true) {
        run_cycle();

        EFI_INPUT_KEY input_key;
        status = uefi_call_wrapper(SystemTable->ConIn->ReadKeyStroke, 2, SystemTable->ConIn, &input_key);

        while (uefi_call_wrapper(SystemTable->ConIn->ReadKeyStroke, 2, SystemTable->ConIn, &input_key) == EFI_SUCCESS) {
            if (input_key.ScanCode == SCAN_ESC) {
                Print(L"Exiting...\n");
                return EFI_SUCCESS;
            }

            uint8_t chip8_key = map_key(input_key);
            if (chip8_key != 0xFF) {
                key[chip8_key] = 1;
            }
        }

        // Render screen if draw flag set
        if (drawFlag) {
            drawFlag = false;
            render_chip8();
            Print(L"Rendered! demo!\n");
        }

        // Simple sleep
        uefi_call_wrapper(SystemTable->BootServices->Stall, 1, 120000); // microseconds
    }

    return EFI_SUCCESS;
}
