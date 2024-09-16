#pragma warning ( disable : 4996 4267 6387 6011 )

#include "Memory.h"
#include <ntifs.h>
#include <ntdef.h>
#include <ntddk.h>




PVOID get_system_module_base(const char* module_name)
{
    ULONG bytes = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, bytes, &bytes);
    if (!bytes)
        return NULL;

    PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, 'LULN');

    status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(modules, 'LULN');
        return NULL;
    }

    PRTL_PROCESS_MODULE_INFORMATION module = modules->Modules;
    PVOID module_base = NULL;

    for (ULONG i = 0; i < modules->NumberOfModules; i++) {
        if (strcmp((char*)module[i].FullPathName, module_name) == 0) {
            module_base = module[i].ImageBase;
            break;
        }
    }

    ExFreePoolWithTag(modules, 'LULN');
    return module_base;
}

PVOID get_system_module_export(const char* module_name, LPCSTR routine_name)
{
    PVOID lpModule = get_system_module_base(module_name);
    if (!lpModule)
        return NULL;

    return RtlFindExportedRoutineByName(lpModule, routine_name);
}

bool write_memory(void* address, void* buffer, size_t size)
{
    RtlCopyMemory(address, buffer, size);
    return true;
}

bool write_to_read_only_memory(void* address, void* buffer, size_t size)
{
    PMDL Mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);
    if (!Mdl)
        return false;

    MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
    PVOID Mapping = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
    MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);

    write_memory(Mapping, buffer, size);

    MmUnmapLockedPages(Mapping, Mdl);
    MmUnlockPages(Mdl);
    IoFreeMdl(Mdl);

    return true;
}

ULONG64 get_module_base_x64(PEPROCESS proc, UNICODE_STRING module_name)
{
    PPEB pPeb = PsGetProcessPeb(proc);
    if (!pPeb)
        return NULL;

    KAPC_STATE state;
    KeStackAttachProcess(proc, &state);

    PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;
    if (!pLdr) {
        KeUnstackDetachProcess(&state);
        return NULL;
    }

    for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink;
        list != &pLdr->ModuleListLoadOrder;
        list = (PLIST_ENTRY)list->Flink)
    {
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
        if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) == 0) {
            ULONG64 baseAddr = (ULONG64)pEntry->DllBase;
            KeUnstackDetachProcess(&state);
            return baseAddr;
        }
    }

    KeUnstackDetachProcess(&state);
    return NULL;
}

bool read_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, size_t size)
{
    if (!address || !buffer || !size)
        return false;

    SIZE_T bytes = 0;
    PEPROCESS process;
    NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
    if (!NT_SUCCESS(status))
        return false;

    status = MmCopyVirtualMemory(process, (void*)address, PsGetCurrentProcess(), buffer, size, KernelMode, &bytes);
    return NT_SUCCESS(status);
}

bool write_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, size_t size)
{
    if (!address || !buffer || !size)
        return false;

    PEPROCESS process;
    NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
    if (!NT_SUCCESS(status))
        return false;

    KAPC_STATE state;
    KeStackAttachProcess(process, &state);

    MEMORY_BASIC_INFORMATION info;
    status = ZwQueryVirtualMemory(ZwCurrentProcess(), (PVOID)address, MemoryBasicInformation, &info, sizeof(info), NULL);
    if (!NT_SUCCESS(status) || !(info.State & MEM_COMMIT) || (info.Protect & PAGE_NOACCESS)) {
        KeUnstackDetachProcess(&state);
        return false;
    }

    if ((info.Protect & PAGE_READWRITE) || (info.Protect & PAGE_WRITECOPY)) {
        RtlCopyMemory((void*)address, buffer, size);
    }

    KeUnstackDetachProcess(&state);
    return true;
}
