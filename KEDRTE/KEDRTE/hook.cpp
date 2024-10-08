#include "hook.h"
#include <wingdi.h>

using namespace std;

GdiSelectBrush_t GdiSelectBrush = NULL;
PatBlt_t NtGdiPatBlt = NULL;
NtUserGetDC_t NtUserGetDC = NULL;
NtGdiCreateSolidBrush_t NtGdiCreateSolidBrush = NULL;
ReleaseDC_t NtUserReleaseDC = NULL;
DeleteObjectApp_t NtGdiDeleteObjectApp = NULL;

bool call_kernel_function(void* kernel_function_address)
{
    DbgPrint("Entering call_kernel_function.\n");

    if (!kernel_function_address) {
        DbgPrint("Failed to get kernel function address.\n");
        return false;
    }

    PVOID* function = reinterpret_cast<PVOID*>(get_system_module_export(L"\\SystemRoot\\System32\\drivers\\dxgkrnl.sys", "NtQuerySection"));

    if (!function) {
        DbgPrint("Failed to get function address from dxgkrnl.sys.\n");
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

    if (!write_to_read_only_memory(function, &orig, sizeof(orig))) {
        DbgPrint("Failed to write to read-only memory.\n");
        return false;
    }

    DbgPrint("Successfully wrote hook to memory.\n");

    GdiSelectBrush = (GdiSelectBrush_t)get_system_module_export(L"win32kfull.sys", "NtGdiSelectBrush");
    NtGdiCreateSolidBrush = (NtGdiCreateSolidBrush_t)get_system_module_export(L"win32kfull.sys", "NtGdiCreateSolidBrush");
    NtGdiPatBlt = (PatBlt_t)get_system_module_export(L"win32kfull.sys", "NtGdiPatBlt");
    NtUserGetDC = (NtUserGetDC_t)get_system_module_export(L"win32kbase.sys", "NtUserGetDC");
    NtUserReleaseDC = (ReleaseDC_t)get_system_module_export(L"win32kbase.sys", "NtUserReleaseDC");
    NtGdiDeleteObjectApp = (DeleteObjectApp_t)get_system_module_export(L"win32kbase.sys", "NtGdiDeleteObjectApp");

    if (!GdiSelectBrush || !NtGdiCreateSolidBrush || !NtGdiPatBlt || !NtUserGetDC || !NtUserReleaseDC || !NtGdiDeleteObjectApp) {
        DbgPrint("Failed to get one or more function addresses from win32k modules.\n");
        return false;
    }

    DbgPrint("Successfully hooked GDI functions.\n");

    return true;
}

NTSTATUS hook_handler(PVOID called_param) {
    DbgPrint("Entering hook_handler.\n");

    NULL_MEMORY* instructions = static_cast<NULL_MEMORY*>(called_param);

    if (instructions->req_base == TRUE) {
        DbgPrint("Requesting base address for module: %s\n", instructions->module_name);

        ANSI_STRING AS;
        UNICODE_STRING ModuleName;

        RtlInitAnsiString(&AS, instructions->module_name);
        RtlAnsiStringToUnicodeString(&ModuleName, &AS, TRUE);

        PEPROCESS Process;
        PsLookupProcessByProcessId(HANDLE(instructions->pid), &Process);
        ULONG64 base_address64 = get_module_base_x64(Process, ModuleName);
        instructions->base_address = base_address64;

        RtlFreeUnicodeString(&ModuleName);

        //DbgPrint("Base address: %p\n", base_address64);
    }
    else if (instructions->write == TRUE) {
        //DbgPrint("Writing to memory: PID=%lu, Address=%p, Size=%llu\n", instructions->pid, instructions->address, instructions->size);

        if (instructions->address < 0x7FFFFFFFFFF && instructions->address > 0) {
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

        if (instructions->address < 0x7FFFFFFFFFF && instructions->address > 0) {
            read_kernel_memory((HANDLE)instructions->pid, instructions->address, instructions->output, instructions->size);
        }
    }
    else if (instructions->draw_box == TRUE) {
        DbgPrint("Drawing box: (%d, %d) Width=%d, Height=%d, Color=(%d, %d, %d), Thickness=%d\n",
            instructions->x, instructions->y, instructions->w, instructions->h,
            instructions->r, instructions->g, instructions->b, instructions->t);

        HDC hdc = NtUserGetDC(NULL);
        if (!hdc) {
            DbgPrint("Failed to get device context.\n");
            return STATUS_UNSUCCESSFUL;
        }

        HBRUSH hbr = NtGdiCreateSolidBrush(RGB(instructions->r, instructions->g, instructions->b), NULL);
        if (!hbr) {
            DbgPrint("Failed to create solid brush.\n");
            NtUserReleaseDC(hdc);
            return STATUS_UNSUCCESSFUL;
        }

        RECT rect = { instructions->x, instructions->y, instructions->x + instructions->w, instructions->y + instructions->h };
        nullhook::FrameRect(hdc, &rect, hbr, instructions->t);

        NtUserReleaseDC(hdc);
        NtGdiDeleteObjectApp(hbr);

        DbgPrint("Box drawn successfully.\n");
    }

    return STATUS_SUCCESS;
}

INT nullhook::FrameRect(HDC hDC, CONST RECT* lprc, HBRUSH hbr, int thickness) {
    DbgPrint("Entering FrameRect.\n");

    HBRUSH oldbrush;
    RECT r = *lprc;

    oldbrush = GdiSelectBrush(hDC, hbr);
    if (!oldbrush) {
        DbgPrint("Failed to select brush.\n");
        return 0;
    }

    NtGdiPatBlt(hDC, r.left, r.top, thickness, r.bottom - r.top, PATCOPY);
    NtGdiPatBlt(hDC, r.right - 1, r.top, thickness, r.bottom - r.top, PATCOPY);
    NtGdiPatBlt(hDC, r.left, r.top, r.right - r.left, thickness, PATCOPY);
    NtGdiPatBlt(hDC, r.left, r.bottom - 1, r.right - r.left, thickness, PATCOPY);

    GdiSelectBrush(hDC, oldbrush);

    DbgPrint("Exiting FrameRect.\n");
    return TRUE;
}
