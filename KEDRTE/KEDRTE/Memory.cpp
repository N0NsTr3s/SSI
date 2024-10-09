#pragma warning (disable : 4996 4267 6387 6011)

#include "Memory.h"
#include <ntifs.h>
#include <ntdef.h>
#include <ntddk.h>

// Function to get system module base by module name
PVOID get_system_module_base(const char* module_name)
{
    ULONG bytes = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &bytes);
    if (status != STATUS_INFO_LENGTH_MISMATCH) {  // Expected failure to get size
        return NULL;
    }

    PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, 'LULN');
    if (!modules) {
        return NULL;
    }

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

    ExFreePoolWithTag(modules, 'LULN');  // Always free memory
    return module_base;
}

// Function to get system routine address by name
PVOID get_system_routine_address(PCWSTR routine_name)
{
    UNICODE_STRING Name;
    RtlInitUnicodeString(&Name, routine_name);
    return MmGetSystemRoutineAddress(&Name);
}

PVOID get_system_module_export(const char* module_name, LPCSTR routine_name)
{
	PVOID lpModule = get_system_module_base(module_name);
	if (!lpModule) {
		return NULL;
	}

	return RtlFindExportedRoutineByName(lpModule, routine_name);
}
// Function to get system module export
PVOID get_system_module_export(LPCWSTR module_name, LPCSTR routine_name)
{
    // Get the list of loaded modules from the PsLoadedModuleList
    PLIST_ENTRY moduleList = reinterpret_cast<PLIST_ENTRY>(get_system_routine_address(L"PsLoadedModuleList"));
    DbgPrint("PsLoadedModuleList address: %p\n", moduleList);

    if (!moduleList) {
        DbgPrint("Failed to get PsLoadedModuleList.\n");
        return NULL;
    }

    // Iterate through each loaded module
    for (PLIST_ENTRY link = moduleList; link != moduleList->Blink; link = link->Flink)
    {
        // Access the loaded module entry
        LDR_DATA_TABLE_ENTRY* entry = CONTAINING_RECORD(link, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

        if (!entry) {
            DbgPrint("Invalid module entry.\n");
            continue;
        }

        // Check if the BaseDllName is valid
        if (entry->BaseDllName.Buffer) {
            DbgPrint("Inspecting module: %wZ, %wZ, (Length: %u, MaximumLength: %u)\n",
                &entry->BaseDllName, entry->FullDllName, entry->BaseDllName.Length, entry->BaseDllName.MaximumLength);
        }
        else {
            DbgPrint("Module with NULL BaseDllName encountered.\n");
            continue;
        }

        // Compare against the base name (just the filename, not the full path)
        UNICODE_STRING name;
        RtlInitUnicodeString(&name, module_name);

        if (RtlEqualUnicodeString(&entry->BaseDllName, &name, TRUE))
        {
            if (entry->DllBase)
            {
				DbgPrint("Found module: %wZ at address: %p\n", &entry->BaseDllName, entry->DllBase);
                return RtlFindExportedRoutineByName(entry->DllBase, routine_name);
            }
            else
            {
				DbgPrint("Found module: %wZ, but DllBase is NULL.\n", &entry->BaseDllName);
                return NULL;
            }
        }
    }

}




// Function to write memory (generic)
bool write_memory(void* address, void* buffer, size_t size)
{
	DbgPrint("Writing to address: %p, buffer: %p, size: %u\n", address, buffer, size);
    RtlCopyMemory(address, buffer, size);
    return true;
}

// Function to write to read-only memory
bool write_to_read_only_memory(void* address, void* buffer, size_t size)
{
    DbgPrint("Writing to read-only memory: %p, buffer: %p, size: %u\n", address, buffer, size);
    PMDL Mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        DbgPrint("Failed to allocate MDL.\n");
        return false;
    }
    DbgPrint("Successfully allocated MDL.\n");

    MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
    DbgPrint("Successfully probed and locked pages.\n");

    PVOID Mapping = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
    if (!Mapping)
    {
        DbgPrint("Failed to map locked pages.\n");
        MmUnlockPages(Mdl);
        IoFreeMdl(Mdl);
        return false;
    }
    DbgPrint("Successfully mapped locked pages.\n");

    MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);
    write_memory(Mapping, buffer, size);
    DbgPrint("Successfully wrote to memory.\n");

    MmUnmapLockedPages(Mapping, Mdl);
    MmUnlockPages(Mdl);
    IoFreeMdl(Mdl);
    DbgPrint("Successfully wrote to read-only memory.\n");
    return true;
}

// Function to get module base for x64
ULONG64 get_module_base_x64(PEPROCESS proc, UNICODE_STRING module_name)
{
	DbgPrint("Getting module base for: %wZ\n", &module_name);
    PPEB pPeb = PsGetProcessPeb(proc);
    if (!pPeb)
		DbgPrint("Failed to get PEB.\n");
        return NULL;

    KAPC_STATE state;
    KeStackAttachProcess(proc, &state);
	DbgPrint("Successfully attached to process.\n");
    PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;
    if (!pLdr) {
		DbgPrint("Failed to get LDR.\n");
        KeUnstackDetachProcess(&state);
        return NULL;
    }
    DbgPrint("Iterating through module list.\n");
    for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink;
        list != &pLdr->ModuleListLoadOrder;
        list = (PLIST_ENTRY)list->Flink)
    {
		DbgPrint("Inspecting module.\n");
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
        if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) == 0) {
            ULONG64 baseAddr = (ULONG64)pEntry->DllBase;
            KeUnstackDetachProcess(&state);
			DbgPrint("Found module base: %p\n", baseAddr);
            return baseAddr;
        }
    }

    KeUnstackDetachProcess(&state);
    return NULL;
}

// Function to read kernel memory
bool read_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, size_t size)
{
    DbgPrint("Reading kernel memory: PID=%lu, Address=%p, Buffer=%p, Size=%u\n", pid, address, buffer, size);
    if (!address || !buffer || !size) {
        DbgPrint("Invalid parameters.\n");
        return false;
    }
    DbgPrint("Parameters are valid.\n");
    SIZE_T bytes = 0;
    PEPROCESS process;
    NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
    if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to lookup process.\n");
        return false;
    }
    status = MmCopyVirtualMemory(process, (void*)address, PsGetCurrentProcess(), buffer, size, KernelMode, &bytes);
	DbgPrint("Copied %u bytes.\n", bytes);
    return NT_SUCCESS(status);
}

// Function to write to kernel memory
bool write_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, size_t size)
{
	DbgPrint("Writing to kernel memory: PID=%lu, Address=%p, Buffer=%p, Size=%u\n", pid, address, buffer, size);
    if (!address || !buffer || !size)
    {
		DbgPrint("Invalid parameters2.\n");
        return false;
    }
    PEPROCESS process;
    NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	DbgPrint("Lookup process status: %x\n", status);
    if (!NT_SUCCESS(status))
    {
		DbgPrint("Failed to lookup process2.\n");
        return false;
    }
    KAPC_STATE state;
    KeStackAttachProcess(process, &state);

    MEMORY_BASIC_INFORMATION info;
    status = ZwQueryVirtualMemory(ZwCurrentProcess(), (PVOID)address, MemoryBasicInformation, &info, sizeof(info), NULL);
	DbgPrint("Query virtual memory status: %x\n", status);
    if (!NT_SUCCESS(status) || !(info.State & MEM_COMMIT) || (info.Protect & PAGE_NOACCESS)) {
        KeUnstackDetachProcess(&state);
		DbgPrint("Failed to query virtual memory.\n");
        return false;
    }

    if ((info.Protect & PAGE_READWRITE) || (info.Protect & PAGE_WRITECOPY) || (info.Protect & PAGE_EXECUTE_READWRITE)) {
		DbgPrint("Memory is writable.\n");
        RtlCopyMemory((void*)address, buffer, size);

    }

    KeUnstackDetachProcess(&state);
	DbgPrint("Successfully wrote to kernel memory.\n");
    return true;
}
