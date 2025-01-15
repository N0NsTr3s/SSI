#pragma disable (4996 28751)
#include "hook.h"
#include <wingdi.h>

using namespace std;




NtGdiSelectBrush_t NtGdiSelectBrush = NULL;
NtPatBlt_t NtGdiPatBlt = NULL;
NtUserGetDC_t NtUserGetDC = NULL;
NtGdiCreateSolidBrush_t NtGdiCreateSolidBrush = NULL;
ReleaseDC_t NtUserReleaseDC = NULL;
DeleteObjectApp_t NtGdiDeleteObjectApp = NULL;
GreSetBkColor_t greSetBkColor = NULL;
NtGdiExtTextOutW_t NtGdiExtTextOutW = NULL;
GreSetTextColor_t greSetTextColor = NULL;
GreSetBkMode_t greSetBkMode = NULL;
NtGdiSelectFont_t NtGdiSelectFont = NULL;


bool nullhook::call_kernel_function(void* kernel_function_address)
{
    DbgPrint("Entering call_kernel_function.\n");
    
    if (!kernel_function_address) {
        DbgPrint("Failed to get kernel function address.\n");
        return false;
    }

    PVOID* function = reinterpret_cast<PVOID*>(get_system_module_export("\\SystemRoot\\System32\\win32kbase.sys", "NtCreateImplicitCompositionInputSink"));


    if (!function) {
        DbgPrint("Failed to get function address from win32kfull.sys or dxgkrnl.sys.\n");
        return false;
    }

    DbgPrint("Hooking function at address: %p\n", function);

    BYTE orig[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    BYTE shell_code[] = { 0x48, 0xB8 }; // mov rax
    BYTE shell_code_end[] = { 0xFF, 0xE0 }; // jmp rax

    RtlSecureZeroMemory(&orig, sizeof(orig));
    memcpy((PVOID)((ULONG_PTR)orig), &shell_code, sizeof(shell_code));
    uintptr_t hook_address = reinterpret_cast<uintptr_t>(kernel_function_address);
    memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code)), &hook_address, sizeof(void*));
    memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code) + sizeof(void*)), &shell_code_end, sizeof(shell_code_end));

    write_to_read_only_memory(function, &orig, sizeof(orig));

    DbgPrint("Successfully wrote hook to memory.\n");

    NtGdiSelectBrush = (NtGdiSelectBrush_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kfull.sys", "NtGdiSelectBrush");
    DbgPrint("[+] SysCall: GdiSelectBrush module_export: 0x%p \n", NtGdiSelectBrush);
    NtGdiCreateSolidBrush = (NtGdiCreateSolidBrush_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kfull.sys", "NtGdiCreateSolidBrush");
    DbgPrint("[+] SysCall: NtGdiCreateSolidBrush module_export: 0x%p \n", NtGdiCreateSolidBrush);
    NtGdiPatBlt = (NtPatBlt_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kfull.sys", "NtGdiPatBlt");
    DbgPrint("[+] SysCall: NtGdiPatBlt module_export: 0x%p \n", NtGdiPatBlt);
    NtUserGetDC = (NtUserGetDC_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kbase.sys", "NtUserGetDC");
    DbgPrint("[+] SysCall: NtUserGetDC module_export: 0x%p \n", NtUserGetDC);
    NtUserReleaseDC = (ReleaseDC_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kbase.sys", "NtUserReleaseDC");
    DbgPrint("[+] SysCall: NtUserReleaseDC module_export: 0x%p \n", NtUserReleaseDC);
    NtGdiDeleteObjectApp = (DeleteObjectApp_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kbase.sys", "NtGdiDeleteObjectApp");
    DbgPrint("[+] SysCall: NtGdiDeleteObjectApp module_export: 0x%p \n", NtGdiDeleteObjectApp);
    NtGdiExtTextOutW = (NtGdiExtTextOutW_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kfull.sys", "NtGdiExtTextOutW");
    DbgPrint("[+] SysCall: NtGdiExtTextOutW module_export: 0x%p \n", NtGdiExtTextOutW);
	NtGdiSelectFont = (NtGdiSelectFont_t)get_system_module_export(L"\\SystemRoot\\System32\\win32kfull.sys", "NtGdiSelectFont");
    DbgPrint("[+] SysCall: NtGdiSelectFont module_export: 0x%p \n", NtGdiSelectFont);



    PVOID greSetBkMode = FindPatternInModule("\\SystemRoot\\System32\\win32kfull.sys", "\x7C\x24\x20\x55\x48\x8B\xEC\x48\x83\xEC\x50\x33\xF6\x48\x8B\xD9", "xxxxxxxxxxxxxxxx");
    DbgPrint("[+] SysCall: greSetBkMode module_export: 0x%p \n", greSetBkMode);

    PVOID greSetTextColor = FindPatternInModule("\\SystemRoot\\System32\\win32kfull.sys", "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x20\x55", "xxxxxxxxxxxxxxxx");

    DbgPrint("[+] SysCall: greSetTextColor module_export: 0x%p \n", greSetTextColor);

    PVOID greSetBkColor = FindPatternInModule("\\SystemRoot\\System32\\win32kfull.sys", "\x7C\x24\x18\x55\x48\x8B\xEC\x48\x83\xEC\x50\x48\x83\x65\xD0\x00", "xxxxxxxxxxxxxxxx");
    DbgPrint("[+] SysCall: greSetBkColor module_export: 0x%p \n", greSetBkColor);


    PVOID module_base = get_system_module_base("\\SystemRoot\\System32\\win32kfull.sys");
    DbgPrint("Module base address of win32kfull.sys: %p\n", module_base);
    //Checks syscall
    if (NtUserGetDC == NULL) {
        DbgPrint("Failed to get NtUserGetDC");
    }
    if (NtUserReleaseDC == NULL) {
        DbgPrint("Failed to get NtUserReleaseDC");
    }
    if (NtGdiCreateSolidBrush == NULL) {
        DbgPrint("Failed to get NtGdiCreateSolidBrush");
    }
    if (NtGdiDeleteObjectApp == NULL) {
        DbgPrint("Failed to get NtGdiDeleteObjectApp");
    }
    if (NtGdiSelectBrush == NULL) {
        DbgPrint("Failed to get GdiSelectBrush");
    }

	if (NtGdiPatBlt == NULL) {
		DbgPrint("Failed to get NtGdiPatBlt");
	}
	if (NtGdiExtTextOutW == NULL) {
		DbgPrint("Failed to get NtGdiExtTextOutW");
	}
	if (NtGdiSelectFont == NULL) {
		DbgPrint("Failed to get NtGdiSelectFont");
	}

    DbgPrint("Successfully hooked GDI functions.\n");
    if (greSetBkMode) {
        DbgPrint("Found greSetBkMode at address: %p\n", greSetBkMode);
    }
    else {
        DbgPrint("Failed to find greSetBkMode by pattern scanning.\n");
    }

    if (greSetTextColor) {
        DbgPrint("Found greSetTextColor at address: %p\n", greSetTextColor);
    }
    else {
        DbgPrint("Failed to find greSetTextColor by pattern scanning.\n");
    }


    if (greSetBkColor) {
        DbgPrint("Found GreSetBkColor at address: %p\n", greSetBkColor);
    }
    else {
        DbgPrint("Failed to find GreSetBkColor by pattern scanning.\n");
    }
    return true;
}

NTSTATUS nullhook::hook_handler(PVOID called_param) {
    //DbgPrint("Entering hook_handler.\n");

    NULL_MEMORY* instructions = (NULL_MEMORY*)(called_param);

    if (instructions->req_base == TRUE) {
        //DbgPrint("Requesting base address for module: %s\n", instructions->module_name);

        ANSI_STRING AS;
        UNICODE_STRING ModuleName;
        __try {
            RtlInitAnsiString(&AS, instructions->module_name);
            RtlAnsiStringToUnicodeString(&ModuleName, &AS, TRUE);

            PEPROCESS Process;
            NTSTATUS status = PsLookupProcessByProcessId((HANDLE)instructions->pid, &Process);
            if (!NT_SUCCESS(status)) {
                DbgPrint("Failed to lookup process by PID: %lu\n", instructions->pid);
                return STATUS_UNSUCCESSFUL;
            }

            ptrdiff_t base_address64 = NULL;
            base_address64 = get_module_base_x64(Process, ModuleName);
            instructions->base_address = base_address64;

            RtlFreeUnicodeString(&ModuleName);

            //DbgPrint("Base address: %llx\n", base_address64);
        }
        __except(EXCEPTION_EXECUTE_HANDLER){
			DbgPrint("Failed to get base address.\n");
			return STATUS_UNSUCCESSFUL;
        }
        }
    else if (instructions->write == TRUE) {
        //DbgPrint("Writing to memory: PID=%lu, Address=%p, Size=%llu\n", instructions->pid, instructions->address, instructions->size);

        if (instructions->address < 0x7FFFFFFFFFFF && instructions->address > 0) {
            PVOID kernelBuff = ExAllocatePool(NonPagedPool, instructions->size);

            if (!kernelBuff) {
                DbgPrint("Failed to allocate kernel buffer.\n");
                return STATUS_UNSUCCESSFUL;
            }

            if (!memcpy(kernelBuff, instructions->buffer_address, instructions->size)) {
                DbgPrint("Failed to copy data to kernel buffer.\n");
                ExFreePool(kernelBuff);
                return STATUS_UNSUCCESSFUL;
            }

            PEPROCESS process;
            PsLookupProcessByProcessId((HANDLE)instructions->pid, &process);
            write_kernel_memory((HANDLE)instructions->pid, instructions->address, kernelBuff, instructions->size);
            ExFreePool(kernelBuff);
        }
    }
    else if (instructions->read == TRUE) {
        //DbgPrint("Reading from memory: PID=%lu, Address=%p, Size=%llu\n", instructions->pid, instructions->address, instructions->size);

        if (instructions->address < 0x7FFFFFFFFFFF && instructions->address > 0) {
            read_kernel_memory((HANDLE)instructions->pid, instructions->address, instructions->output, instructions->size);
			//DbgPrint("Data read: %s\n", instructions->output);
        }
    }
    else if (instructions->draw_box == TRUE) {
        //DbgPrint("Drawing box: R=%d, G=%d, B=%d, X=%d, Y=%d, W=%d, H=%d, T=%d\n", instructions->r, instructions->g, instructions->b, instructions->x, instructions->y, instructions->w, instructions->h, instructions->t);
        HDC hdc = NtUserGetDC(NULL);
        if (!hdc) {
            DbgPrint("Failed to get device context.\n");
            return STATUS_UNSUCCESSFUL;
        }

        HBRUSH brush = NtGdiCreateSolidBrush(RGB(instructions->r, instructions->g, instructions->b), NULL);
        if (!brush) {
            DbgPrint("Failed to create solid brush.\n");
            NtUserReleaseDC(hdc);
            return STATUS_UNSUCCESSFUL;
        }

        RECT rect = { instructions->x, instructions->y, instructions->x + instructions->w, instructions->y + instructions->h };
        FrameRect(hdc, &rect, brush, instructions->t);

        NtUserReleaseDC(hdc);
        NtGdiDeleteObjectApp(brush);

        //DbgPrint("Box drawn successfully.\n");

        NtUserReleaseDC(hdc);
        NtGdiDeleteObjectApp(brush);
		
    }
    else if (instructions->draw_text == TRUE) {
        //DbgPrint("Entering draw_text handler.\n");
        /*
        // Check the validity of the received text
        WCHAR kernelBuffer[256] = { 0 };
        __try {
            RtlCopyMemory(kernelBuffer, instructions->text, sizeof(kernelBuffer) - sizeof(WCHAR));
            kernelBuffer[255] = L'\0'; // Null-terminate
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("Failed to copy text to kernel buffer.\n");
            return STATUS_ACCESS_VIOLATION;
        }
        //DbgPrint("Raw instructions.text in kernel:\n");
        //for (int i = 0; i < sizeof(instructions->text) / sizeof(WCHAR); i++) {
        //    DbgPrint("%04X ", instructions->text[i]);
        //}
        //DbgPrint("\n");

        DbgPrint("Text received in kernel: %ls\n", kernelBuffer);


        // Define the spacing array
        INT spacingArray[256] = { 0 };
        for (size_t i = 0; i < wcslen(kernelBuffer); ++i) {
            spacingArray[i] = 1; // Set the spacing between characters (adjust as needed)
        }
        */


        // Check the validity of the received text
        CHAR kernelBuffer[256] = { 0 };
        __try {
            RtlCopyMemory(kernelBuffer, instructions->text, sizeof(kernelBuffer) - sizeof(CHAR));
            kernelBuffer[255] = '\0'; // Null-terminate
			//DbgPrint("Text copied to kernel buffer: %s\n", kernelBuffer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("Failed to copy text to kernel buffer.\n");
            return STATUS_ACCESS_VIOLATION;
        }

        //DbgPrint("Text received in kernel: %s\n", kernelBuffer);

        HDC hdc = NtUserGetDC(NULL);
        if (!hdc) {
            DbgPrint("Failed to get device context.\n");
            return STATUS_UNSUCCESSFUL;
        }
        RECT clipRect = { instructions->x, instructions->y, instructions->x + 200, instructions->y + 50 };

        

		NtGdiSelectFont(hdc, NULL);
        

        BOOL result = extTextOutA(
            hdc,
            instructions->x,
            instructions->y,
            ETO_CLIPPED,
            &clipRect,
            instructions->text,
            strlen(instructions->text),
            NULL
        );

        NtUserReleaseDC(hdc);

        if (!result) {
            DbgPrint("NtGdiExtTextOutW failed to draw text.\n");
            return STATUS_UNSUCCESSFUL;
        }

        //DbgPrint("Text drawn successfully.\n");
        return STATUS_SUCCESS;
    }
    
    else {
        DbgPrint("Invalid instruction.\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
    }

INT nullhook::FrameRect(HDC hDC, CONST RECT* lprc, HBRUSH hbr, int thickness) {
    //DbgPrint("Entering FrameRect.\n");

    HBRUSH oldbrush;
    RECT r = *lprc;

    if (!(oldbrush = NtGdiSelectBrush(hDC, hbr))) {
        DbgPrint("Failed to select brush.\n");
        return 0;
    }

    NtGdiPatBlt(hDC, r.left, r.top, thickness, r.bottom - r.top, PATCOPY);
    NtGdiPatBlt(hDC, r.right - thickness, r.top, thickness, r.bottom - r.top, PATCOPY);
    NtGdiPatBlt(hDC, r.left, r.top, r.right - r.left, thickness, PATCOPY);
    NtGdiPatBlt(hDC, r.left, r.bottom - thickness, r.right - r.left, thickness, PATCOPY);

    NtGdiSelectBrush(hDC, oldbrush);
    
    DbgPrint("Exiting FrameRect.\n");
    NtUserReleaseDC(hDC);
    NtGdiDeleteObjectApp(hDC);
    return TRUE;
}

/**
 * ExtTextOutW function to draw text using GDI functions.
 *
 * This function draws text at a specified location using the GDI ExtTextOutW function.
 * It handles memory allocation and copying for the text and rectangle parameters.
 *
 * @param hdc Handle to the device context.
 * @param x X-coordinate of the reference point.
 * @param y Y-coordinate of the reference point.
 * @param fuOptions Text output options.
 * @param lprc Pointer to a RECT structure that specifies the clipping rectangle.
 * @param lpString Pointer to the string to be drawn.
 * @param cwc Number of characters in the string.
 * @param lpDx Pointer to an array of spacing values.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL extTextOutW(HDC hdc, INT x, INT y, UINT fuOptions, RECT* lprc, LPWSTR lpString, UINT cwc, INT* lpDx)
{
    BOOL nRet = FALSE; // Return value
    PVOID local_lpString = NULL; // Local copy of the string
    RECT* local_lprc = NULL; // Local copy of the rectangle
    INT* local_lpDx = NULL; // Local copy of the spacing values

    // Allocate and copy the rectangle if provided
    if (lprc != NULL)
    {
        SIZE_T Len = sizeof(RECT);
        local_lprc = (RECT*)AllocateVirtualMemory(Len);
        if (local_lprc != NULL)
        {
            __try
            {
                RtlZeroMemory(local_lprc, Len); // Zero out the memory
                RtlCopyMemory(local_lprc, lprc, Len); // Copy the rectangle
            }
            __except (1)
            {
                DbgPrint("GDI.cpp Line RtlCopyMemory  Triggers An Error.ExtTextOutW Internal Function\r\n");
                goto $EXIT;
            }
        }
        else
        {
            DbgPrint("GDI.cpp Line local_lprc = null  Triggers An Error.ExtTextOutW Internal Function\r\n");
            goto $EXIT;
        }
    }

    // Allocate and copy the string if provided
    if (cwc != 0)
    {
        SIZE_T AllocSize = sizeof(WCHAR) * cwc + 1;
        local_lpString = AllocateVirtualMemory(AllocSize);

        if (local_lpString != NULL)
        {
            __try
            {
                RtlZeroMemory(local_lpString, AllocSize); // Zero out the memory
                RtlCopyMemory(local_lpString, lpString, AllocSize); // Copy the string
            }
            __except (1)
            {
                DbgPrint("[-] GDI.cpp Line RtlCopyMemory  Triggers An Error.ExtTextOutW Internal Function\r\n");
                goto $EXIT;
            }
        }
        else
        {
            DbgPrint("[-] GDI.cpp Line local_lpString = null  Triggers An Error.ExtTextOutW Internal Function\r\n");
            goto $EXIT;
        }
    }

    // Allocate and copy the spacing values if provided
    if (local_lpDx != NULL)
    {
        SIZE_T AllocSize = sizeof(INT);
        local_lpDx = (INT*)AllocateVirtualMemory(AllocSize);
        if (local_lpDx != NULL)
        {
            __try
            {
                RtlZeroMemory(local_lpString, AllocSize); // Zero out the memory
                *local_lpDx = *lpDx; // Copy the spacing values
            }
            __except (1)
            {
                DbgPrint("[-] GDI.cpp Line RtlCopyMemory  Triggers An Error.ExtTextOutW Internal Function\r\n");
                goto $EXIT;
            }
        }
        else
        {
            DbgPrint("[-] GDI.cpp Line local_lpDx = null  Triggers An Error.ExtTextOutW Internal Function\r\n");
        }
    }

    // Call the NtGdiExtTextOutW function if it is available
    if (NtGdiExtTextOutW != NULL) {
        nRet = NtGdiExtTextOutW(hdc, x, y, fuOptions, local_lprc, (LPWSTR)local_lpString, cwc, local_lpDx, 0);
    }
    else {
        DbgPrint("[-] GDI.cpp Line NtGdiExtTextOutW = NULL Triggers An Error.TextOutW Internal Function\r\n");
    }

$EXIT:
    // Free allocated memory
    if (lprc != NULL)
    {
        FreeVirtualMemory(lprc, sizeof(RECT));
        lprc = NULL;
    }

    if (local_lpDx != NULL)
    {
        FreeVirtualMemory(local_lpDx, sizeof(INT));
        local_lpDx = NULL;
    }

    if (local_lpString != NULL)
    {
        FreeVirtualMemory(local_lpString, cwc);
        local_lpString = NULL;
    }

    return nRet;
}

/**
 * Draws text using ANSI string.
 *
 * This function converts an ANSI string to a Unicode string and then calls the
 * extTextOutW function to draw the text.
 *
 * @param hdc Handle to the device context.
 * @param x X-coordinate of the reference point.
 * @param y Y-coordinate of the reference point.
 * @param fuOptions Text output options.
 * @param lprc Pointer to a RECT structure that specifies the clipping rectangle.
 * @param lpString Pointer to the ANSI string to be drawn.
 * @param cch Number of characters in the ANSI string.
 * @param lpDx Pointer to an array of spacing values.
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL extTextOutA(HDC hdc, INT x, INT y, UINT fuOptions, RECT* lprc, LPCSTR lpString, UINT cch, INT* lpDx) {
    // Check if the input string is valid and has a non-zero length
    if (!lpString || cch == 0) {
        DbgPrint("Invalid input string or length.\n");
        return FALSE;
    }

    ANSI_STRING ansiString; // ANSI string structure
    UNICODE_STRING unicodeString; // Unicode string structure

    // Initialize the ANSI string with the input string
    RtlInitAnsiString(&ansiString, lpString);

    // Convert the ANSI string to a Unicode string
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE))) {
        DbgPrint("Failed to convert ANSI to Unicode.\n");
        return FALSE;
    }

    // Call the extTextOutW function to draw the text using the Unicode string
    BOOL result = extTextOutW(hdc, x, y, fuOptions, lprc, unicodeString.Buffer, (unicodeString.Length / sizeof(WCHAR)), lpDx);

    // Free the memory allocated for the Unicode string
    RtlFreeUnicodeString(&unicodeString);

    return result; // Return the result of the text drawing operation
}

INT set_background_mode(HDC hdC, INT mode)
{
    INT previous_background_mode = NULL;
    INT current_background_mode = NULL;
    if (greSetBkMode == NULL) {
        DbgPrint("Failed to get setBkMode\n");
        return 0;
    }
    previous_background_mode = greSetBkMode(hdC, mode);
    current_background_mode = mode;
    DbgPrint("Current background mode: %d\n", current_background_mode);
    return current_background_mode;
};

