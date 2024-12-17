#pragma once
#pragma comment(lib, "ntoskrnl.lib")
#pragma warning (disable : 4083)
#pragma comment(lib, "ntfis.h")
#include <ntdef.h>
#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <ntstrsafe.h>
#include <wdm.h>

// Contains basic information about the system.
typedef struct _SYSTEM_BASIC_INFORMATION {
    BYTE Reserved1[24];
    PVOID Reserved2[4];
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION;

// Enum representing different system information classes.
typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation, // Fixed typo
    SystemPathInformation,
    SystemProcessInformation,    // Fixed typo
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,      // Fixed typo
    SystemCallTimeInformation,
    SystemModuleInformation = 0x0B
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

// Contains information about a system process.
typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    BYTE Reserved1[48];
    PVOID Reserved2[3];
    HANDLE UniqueProcessId;
    PVOID Reserved3;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION;



// Contains information about the loader data in the PEB.
// Represents the Process Environment Block (PEB) loader data.
// The PEB_LDR_DATA structure is used by the system to store information about the loaded modules for a process.
// It contains various fields that provide information about the modules, such as their load order, memory order, and initialization order.
typedef struct _PEB_LDR_DATA {
    ULONG Length;                      // The length of the structure.
    UCHAR Initialized;                 // Indicates whether the structure has been initialized.
    PVOID SsHandle;                    // A handle to the session space.
    LIST_ENTRY ModuleListLoadOrder;    // A list of modules in load order.
    LIST_ENTRY ModuleListMemoryOrder;  // A list of modules in memory order.
    LIST_ENTRY ModuleListInitOrder;    // A list of modules in initialization order.
} PEB_LDR_DATA, *PPEB_LDR_DATA;

// Represents an entry in the loader data table.
// The LDR_DATA_TABLE_ENTRY structure is used by the system to store information about a loaded module.
// It contains various fields that provide information about the module, such as its base address, entry point, size, and name.
typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderModuleList;    // List entry for the module in load order.
    LIST_ENTRY InMemoryOrderModuleList;  // List entry for the module in memory order.
    LIST_ENTRY InInitializationOrderModuleList; // List entry for the module in initialization order.
    PVOID DllBase;                      // Base address of the module.
    PVOID EntryPoint;                   // Entry point of the module.
    ULONG SizeOfImage;                  // Size of the module image.
    UNICODE_STRING FullDllName;         // Full name of the module.
    UNICODE_STRING BaseDllName;         // Base name of the module.
    ULONG Flags;                        // Flags associated with the module, such as load status or attributes.
    USHORT LoadCount;                   // Load count of the module.
    // TLS (Thread Local Storage) is a mechanism that allows data to be stored separately for each thread.
    // Each thread has its own copy of the data, which is not shared with other threads.
    // This is useful for storing data that is specific to a particular thread, such as thread-specific variables or buffers.
    USHORT TlsIndex;                    // TLS index of the module.
    LIST_ENTRY HashLinks;               // List entry for the module in hash order.
    PVOID SectiomPointer;               // Pointer to the section object.
    // Checksum of the module.
    // The checksum is a value used to verify the integrity of the module.
    // It is calculated based on the contents of the module and can be used to detect changes or corruption.
    ULONG CheckSum;
    ULONG TimeDateStamp;                // Time and date stamp of the module.
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;


// Contains information about a process module.
// The RTL_PROCESS_MODULE_INFORMATION structure is used by the system to store information about a loaded module.
// It contains various fields that provide information about the module, such as its section, base address, size, and name.
typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    // A handle is an abstract reference to a resource, such as a file, a process, or a thread.
    // It is used by the operating system to manage and access these resources.
    // Handles are typically represented as opaque values (e.g., pointers or integers) that are unique within the context of a process.
    // They provide a way to interact with system resources without exposing the underlying implementation details.
    HANDLE Section;                // Handle to the section object.
    // Base address of the mapped module.
    // This is the address where the module is mapped into the virtual address space of the process.
    PVOID MappedBase;              // Base address of the mapped module.
    // Base address of the module image.
    // This is the address where the module's image is loaded in memory.
    PVOID ImageBase;               // Base address of the module image.
    ULONG ImageSize;               // Size of the module image.
    ULONG Flags;                   // Flags associated with the module.
    USHORT LoadOrderIndex;         // Load order index of the module.
    USHORT InitOrderIndex;         // Initialization order index of the module.
    USHORT LoadCount;              // Load count of the module.
    // Offset to the module's file name.
    // An offset is a value that represents the distance (in bytes) from the beginning of a data structure or memory block to a specific element or point within that structure or block.
    // It is used to locate and access elements within a data structure or memory block by adding the offset to the base address of the structure or block.
    USHORT OffsetToFileName;       // Offset to the module's file name.
    UCHAR FullPathName[256];       // Full path name of the module.
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

// Contains information about all process modules.
typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

// Contains user process parameters.
typedef struct _RTL_USER_PROCESS_PARAMETERS {
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// Function pointer for post-process initialization routine.
typedef void(__stdcall* PPS_POST_PROCESS_INIT_ROUTINE)(void);




// Contains information about the loader data in the PEB.
// Represents the Process Environment Block (PEB).
// The PEB is a data structure in the Windows operating system that is used by the system to store information about a process.
// It contains various fields that provide information about the process, such as its loader data, process parameters, and session ID.
typedef struct _PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    BYTE Reserved4[3];
    PVOID AtlThunkSListPtr;
    PVOID Reserved5;
    ULONG Reserved6;
    PVOID Reserved7;
    ULONG Reserved8;
    ULONG AtlThunkSListPtr32;
    PVOID Reserved9[45];
    BYTE Reserved10[96];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved11[128];
    PVOID Reserved12[1];
    ULONG SessionId;
} PEB, * PPEB;

// Function to change the protection on a region of virtual memory.
// Parameters:
// - ProcessHandle: Handle to the process whose memory protection is to be changed.
// - BaseAddress: Pointer to the base address of the region of pages whose access protection attributes are to be changed.
// - ProtectSize: Pointer to a variable that specifies the size of the region whose protection attributes are to be changed, in bytes.
// - NewProtect: Memory protection to be applied to the region of pages.
// - OldProtect: Pointer to a variable that receives the previous access protection of the first page in the specified region of pages.
extern "C" __declspec(dllimport)
NTSTATUS NTAPI ZwProtectVirtualMemory(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    PULONG ProtectSize,
    ULONG NewProtect,
    PULONG OldProtect
);

extern "C" NTKERNELAPI
PVOID
NTAPI
RtlFindExportedRoutineByName(
    _In_ PVOID ImageBase,
    _In_ PCCH RoutineName   
);

// Function to query system information.
// Parameters:
// - InfoClass: The type of system information to be queried.
// - Buffer: A pointer to a buffer that receives the requested information.
// - Length: The size of the buffer, in bytes.
// - ReturnLength: A pointer to a variable that receives the number of bytes written to the buffer.
// Returns: An NTSTATUS code indicating the success or failure of the operation.
extern "C" NTSTATUS ZwQuerySystemInformation(
    ULONG InfoClass, 
    PVOID Buffer, 
    ULONG Length, 
    PULONG ReturnLength
);

extern "C" NTKERNELAPI
PPEB PsGetProcessPeb(
    IN PEPROCESS Process
);

// Function to copy virtual memory from one process to another.
// Parameters:
// - SourceProcess: Pointer to the source process object.
// - SourceAddress: Pointer to the source address in the source process's virtual address space.
// - TargetProcess: Pointer to the target process object.
// - TargetAddress: Pointer to the target address in the target process's virtual address space.
// - BufferSize: Size of the memory to be copied, in bytes.
// - PreviousMode: The processor mode in which the copy operation is to be performed.
// - ReturnSize: Pointer to a variable that receives the number of bytes copied.
// Returns: An NTSTATUS code indicating the success or failure of the operation.
extern "C" NTSTATUS NTAPI MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize
);

// Contains information about a system module entry.
// The SYSTEM_MODULE_ENTRY structure is used by the system to store information about a loaded module.
// It contains various fields that provide information about the module, such as its section, base address, size, and name.
typedef struct _SYSTEM_MODULE_ENTRY
{
    HANDLE Section;                // Handle to the section object.
    PVOID MappedBase;              // Base address of the mapped module.
    PVOID ImageBase;               // Base address of the module image.
    ULONG ImageSize;               // Size of the module image.
    ULONG Flags;                   // Flags associated with the module.
    USHORT LoadOrderIndex;         // Load order index of the module.
    USHORT InitOrderIndex;         // Initialization order index of the module.
    USHORT LoadCount;              // Load count of the module.
    USHORT OffsetToFileName;       // Offset to the module's file name.
    UCHAR FullPathName[256];       // Full path name of the module.
} SYSTEM_MODULE_ENTRY, * PSYSTEM_MODULE_ENTRY;

// Contains information about all system modules.
typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG Count;
    SYSTEM_MODULE_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

// Contains parameters for drawing text.
// The DRAWTEXTPARAMS structure is used by the system to store parameters for drawing text.
// It contains various fields that provide information about the text drawing parameters, such as the size, tab length, margins, and length drawn.
typedef struct tagDRAWTEXTPARAMS
{
    UINT cbSize;          // The size of the structure, in bytes.
    int  iTabLength;      // The length of the tab stops, in characters.
    int  iLeftMargin;     // The left margin, in pixels.
    int  iRightMargin;    // The right margin, in pixels.
    UINT uiLengthDrawn;   // The length of the text that was drawn, in characters.
} DRAWTEXTPARAMS, * LPDRAWTEXTPARAMS;

// Contains information for painting.
// The PAINTSTRUCT structure is used by the system to store information needed for painting operations.
// It contains various fields that provide information about the painting context, such as the device context handle, 
// whether the background should be erased, the area to be painted, and other painting-related flags.
typedef struct tagPAINTSTRUCT {
    HDC  hdc;            // Handle to the device context.
    BOOL fErase;         // Indicates whether the background should be erased.
    RECT rcPaint;        // Rectangle that needs to be painted.
    BOOL fRestore;       // Reserved; must be FALSE.
    BOOL fIncUpdate;     // Reserved; must be FALSE.
    BYTE rgbReserved[32];// Reserved for future use.
} PAINTSTRUCT, * PPAINTSTRUCT, * NPPAINTSTRUCT, * LPPAINTSTRUCT;
