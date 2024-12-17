#pragma warning (disable : 4996 4267 6387 6011)

#include "Memory.h"
#include <ntifs.h>
#include <ntdef.h>
#include <ntddk.h>

// Function to get system module base by module name
PVOID get_system_module_base(const char* module_name)
{
    ULONG bytes = 0;
    // Query the size of the system module information
    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &bytes);
    if (status != STATUS_INFO_LENGTH_MISMATCH) {  // Expected failure to get size
        return NULL;
    }

    // Allocate memory for system module information
    PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, 'LULN');
    if (!modules) {
        return NULL;
    }

    // Query system module information
    status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(modules, 'LULN');
        return NULL;
    }

    PRTL_PROCESS_MODULE_INFORMATION module = modules->Modules;
    PVOID module_base = NULL;

    // Iterate over all system modules
    for (ULONG i = 0; i < modules->NumberOfModules; i++) {
        // Compare module names
        if (strcmp((char*)module[i].FullPathName, module_name) == 0) {
            module_base = module[i].ImageBase;
            break;
        }
    }

    // Always free memory
    ExFreePoolWithTag(modules, 'LULN');
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
	DbgPrint("Module base: %p from get_system_module_export\n", lpModule);
	return RtlFindExportedRoutineByName(lpModule, routine_name);
}
// Function to get system module export

PVOID get_system_module_export(LPCWSTR module_name, LPCSTR routine_name)
{
    DbgPrint("Getting system module export: %ws, %s\n", module_name, routine_name);

    // Query the size of the system module information
    ULONG bytes = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &bytes);
    if (status != STATUS_INFO_LENGTH_MISMATCH) {
        DbgPrint("Failed to query system module information size. Status: 0x%X\n", status);
        return NULL;
    }

    // Allocate memory for system module information
    PSYSTEM_MODULE_INFORMATION pMods = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, bytes, 'sysM');
    if (!pMods) {
        DbgPrint("Failed to allocate memory for module information.\n");
        return NULL;
    }

    // Query system module information
    status = ZwQuerySystemInformation(SystemModuleInformation, pMods, bytes, &bytes);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to query system module information. Status: 0x%X\n", status);
        ExFreePoolWithTag(pMods, 'sysM');
        return NULL;
    }

    // Iterate over all system modules
    PSYSTEM_MODULE_ENTRY pMod = pMods->Module;
    UNICODE_STRING unicodeModuleName;
    ANSI_STRING targetModuleName;
    RtlInitUnicodeString(&unicodeModuleName, module_name);
    RtlUnicodeStringToAnsiString(&targetModuleName, &unicodeModuleName, TRUE);
    

    for (ULONG i = 0; i < pMods->Count; i++)
    {
        ANSI_STRING currentModuleName;
        RtlInitAnsiString(&currentModuleName, (PCSZ)pMod[i].FullPathName);

        DbgPrint("Checking module: %Z to see if it is: %X\n", &currentModuleName, &targetModuleName);

        // Compare module names (target module name with the current module)
        if (RtlCompareString(&targetModuleName, &currentModuleName, TRUE) == 0)
        {
            PVOID moduleBase = pMod[i].ImageBase;
            DbgPrint("Found module: %Z at base: %p, size: %lx\n", &currentModuleName, moduleBase, pMod[i].ImageSize);

            // Find the exported routine by name
            PVOID routine_address = RtlFindExportedRoutineByName(moduleBase, routine_name);
            if (routine_address) {
                DbgPrint("Found routine: %s at address: %p\n", routine_name, routine_address);
                ExFreePoolWithTag(pMods, 'sysM');
                return routine_address;
            }
            else {
                DbgPrint("Failed to find routine: %s in module: %Z\n", routine_name, &currentModuleName);
                ExFreePoolWithTag(pMods, 'sysM');
                return NULL;
            }
        }
    }

    DbgPrint("Module %ws not found.\n", module_name);
    ExFreePoolWithTag(pMods, 'sysM');
    return NULL;
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
    // Log the attempt to write to read-only memory
    DbgPrint("Writing to read-only memory: %p, buffer: %p, size: %u\n", address, buffer, size);

    // Allocate a Memory Descriptor List (MDL) for the target address and size
    PMDL Mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        // Log failure to allocate MDL
        DbgPrint("Failed to allocate MDL.\n");
        return false;
    }
    // Log successful MDL allocation
    DbgPrint("Successfully allocated MDL.\n");

    // Lock the pages described by the MDL into memory
    MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
    // Log successful page locking
    DbgPrint("Successfully probed and locked pages.\n");

    // Map the locked pages into the system address space
    PVOID Mapping = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
    if (!Mapping)
    {
        // Log failure to map locked pages
        DbgPrint("Failed to map locked pages.\n");
        MmUnlockPages(Mdl);
        IoFreeMdl(Mdl);
        return false;
    }
    // Log successful page mapping
    DbgPrint("Successfully mapped locked pages.\n");

    // Change the protection of the mapped pages to read-write
    MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);

    // Write the buffer to the mapped pages
    write_memory(Mapping, buffer, size);
    // Log successful memory write
    DbgPrint("Successfully wrote to memory.\n");

    // Unmap the locked pages from the system address space
    MmUnmapLockedPages(Mapping, Mdl);
    // Unlock the pages
    MmUnlockPages(Mdl);
    // Free the MDL
    IoFreeMdl(Mdl);
    // Log successful completion of the write operation
    DbgPrint("Successfully wrote to read-only memory.\n");
    return true;
}

// Function to get module base for x64
ULONG_PTR get_module_base_x64(PEPROCESS proc, UNICODE_STRING module_name)
{
    DbgPrint("Getting module base for: %wZ get_module_base_x64\n", &module_name);

    // Get the Process Environment Block (PEB) of the process
    PPEB pPeb = PsGetProcessPeb(proc);
    if (!pPeb) {
        DbgPrint("Failed to get PEB or Ldr is uninitialized.\n");
        return NULL;
    }

    // Attach to the target process's address space
    KAPC_STATE state;
    KeStackAttachProcess(proc, &state);
    DbgPrint("Successfully attached to process.\n");

    // Get the Loader Data (LDR) from the PEB
    PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;
    if (!pLdr) {
        KeUnstackDetachProcess(&state);
        DbgPrint("Failed to get LDR.\n");
        return NULL;
    }

    DbgPrint("Iterating through module list.\n");

    // Iterate through the list of loaded modules
    for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink;
        list != &pLdr->ModuleListLoadOrder;
        list = (PLIST_ENTRY)list->Flink)
    {
        // Get the current module entry
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

        // Compare the module name with the target module name
        if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) == 0)
        {
            // Get the base address of the module
            ULONG_PTR baseAddr = (ULONG64)pEntry->DllBase;
            KeUnstackDetachProcess(&state);
            DbgPrint("Found module base: %p\n", baseAddr);
            return baseAddr;
        }
    }

    // Detach from the target process's address space
    KeUnstackDetachProcess(&state);
    DbgPrint("Module not found.\n");
    return 0;
}

// Function to read kernel memory
bool read_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, size_t size) {

    DbgPrint("Reading kernel memory: PID=%lu, Address=%p, Buffer=%p, Size=%zu\n", pid, (void*)address, buffer, size);

    // Validate input parameters
    if (!address || !buffer || size == 0) {
        DbgPrint("Invalid parameters.\n");
        return false;
    }

    PEPROCESS process;
    // Lookup the process by its PID
    NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to find process for PID=%lu. Status=0x%X\n", pid, status);
        return false;
    }

    SIZE_T bytes = 0;
    // Copy virtual memory from the target process to the current process
    status = MmCopyVirtualMemory(
        process,                // Source process
        (void*)address,         // Source address
        PsGetCurrentProcess(),  // Target process
        buffer,                 // Target buffer
        size,                   // Number of bytes to copy
        KernelMode,             // Access mode
        &bytes                  // Number of bytes copied
    );

    // Check if the memory copy operation was successful
    if (!NT_SUCCESS(status)) {
        DbgPrint("MmCopyVirtualMemory failed with status: 0x%X\n", status);
        return false;
    }

    // Log the successful memory copy operation
    DbgPrint("Successfully copied %zu bytes from address %p\n", bytes, (void*)address);
    return true;
}


// Function to write to kernel memory
bool write_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size)
{
    // Log the attempt to write to kernel memory
    DbgPrint("Writing to kernel memory: PID=%lu, Address=%p, Buffer=%p, Size=%u\n", pid, address, buffer, size);

    // Validate input parameters
    if (!address || !buffer || !size)
    {
        DbgPrint("Invalid parameters2.\n");
        return false;
    }

    PEPROCESS process;
    NTSTATUS status = STATUS_SUCCESS;

    // Lookup the process by its PID
    PsLookupProcessByProcessId(pid, &process);
    DbgPrint("Lookup process status: %x\n", status);

    KAPC_STATE state;

    // Attach to the target process's address space
    KeStackAttachProcess((PEPROCESS)process, &state);

    MEMORY_BASIC_INFORMATION info;

    // Query the virtual memory information of the target address
    status = ZwQueryVirtualMemory(ZwCurrentProcess(), (PVOID)address, MemoryBasicInformation, &info, sizeof(info), NULL);
    DbgPrint("Query virtual memory status: %x\n", status);

    // Check if the memory query was successful and the memory is committed and accessible
    if (!NT_SUCCESS(status) || !(info.State & MEM_COMMIT) || (info.Protect & PAGE_NOACCESS)) {
        KeUnstackDetachProcess(&state);
        DbgPrint("Failed to query virtual memory.\n");
        return false;
    }

    // Check if the address range is within the region size
    if (((uintptr_t)info.BaseAddress + info.RegionSize) < (address + size)) {
        KeUnstackDetachProcess(&state);
        return false;
    }

    // Check if the memory is committed and not guarded or inaccessible
    if (!(info.State & MEM_COMMIT) || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
        KeUnstackDetachProcess(&state);
        DbgPrint("Memory is not committed.\n");
        return false;
    }

    // Check if the memory is writable
    if ((info.Protect & PAGE_READWRITE) || (info.Protect & PAGE_EXECUTE_WRITECOPY) || (info.Protect & PAGE_EXECUTE_READWRITE) || (info.Protect & PAGE_WRITECOPY)) {
        DbgPrint("Memory is writable.\n");
        // Copy the buffer to the target address
        RtlCopyMemory((void*)address, buffer, size);
    }

    // Detach from the target process's address space
    KeUnstackDetachProcess(&state);
    DbgPrint("Successfully wrote to kernel memory.\n");
    return true;
}

VOID FreeVirtualMemory(PVOID VirtualAddress, SIZE_T Size)
{
    // Check if the virtual address is valid
    if (MmIsAddressValid(VirtualAddress))
    {
        // Attempt to free the virtual memory
        NTSTATUS Status = ZwFreeVirtualMemory(NtCurrentProcess(), &VirtualAddress, &Size, MEM_RELEASE);

        // Check if the memory release was successful
        if (!NT_SUCCESS(Status)) {
       
            DbgPrint("[-] GDI.cpp Warning : Released memory failed.FreeVirtualMemory Internal Function\r\n");
            DbgPrint("[-] GDI.cpp Warning: Failed to free virtual memory. NTSTATUS: 0x%X\n", Status);
            DbgPrint("ZwFreeVirtualMemory Parameters:\n");
            DbgPrint("CurrentProcessHandele:%p", NtCurrentProcess());
            DbgPrint("BaseAddress: %p\n", VirtualAddress);
            DbgPrint("RegionSize: %zu\n", Size);
            DbgPrint("FreeType: %lu\n", MEM_RELEASE);
        }
        return;
    }
    // Log a warning message if the virtual address is not valid
    DbgPrint("[-] GDI.cpp Warning: Released memory does not exist.FreeVirtualMemory Internal Function\r\n");
}

PVOID AllocateVirtualMemory(SIZE_T Size)
{
    PVOID pMem = NULL; // Initialize pointer to NULL
    // Allocate virtual memory with the specified size, commit the memory, and set it as read-write
    NTSTATUS statusAlloc = ZwAllocateVirtualMemory(NtCurrentProcess(), &pMem, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
    // Return the pointer to the allocated memory
    return pMem;
}