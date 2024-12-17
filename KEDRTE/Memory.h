#include "definitions.h"

PVOID get_system_module_base(const char* module_name);
PVOID get_system_module_export(const char* module_name, LPCSTR routine_name);
PVOID get_system_module_export(LPCWSTR module_name, LPCSTR routine_name);
bool write_memory(void* address, void* buffer, size_t size);
bool write_to_read_only_memory(void* address, void* buffer, size_t size);
ULONG64 get_module_base_x64(PEPROCESS proc, UNICODE_STRING module_name);
bool read_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, size_t size);
bool write_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, size_t size);
VOID FreeVirtualMemory(PVOID VirtualAddress, SIZE_T Size);
PVOID AllocateVirtualMemory(SIZE_T Size);

// The #pragma pack directive is used to change the current alignment 
// of the structure members to a specified value. 
// #pragma pack(push, 1) saves the current alignment setting and sets 
// the new alignment to 1 byte, which means there will be no padding 
// between the members of the structure. This is useful when you need 
// the structure to have a specific memory layout, such as when 
// interacting with hardware or a binary file format.
#pragma pack(push, 1)
/**
 * Structure to hold memory operation details.
 * 
 * This structure is used to define the parameters for various memory operations
 * such as reading, writing, and drawing on memory. It includes information about
 * the target address, buffer, size, process ID, and additional flags for specific
 * operations.
 */
typedef struct _NULL_MEMORY {
    void* buffer_address;   ///< Address of the buffer to read/write.
    UINT_PTR address;       ///< Target memory address.
    ULONGLONG size;         ///< Size of the memory operation.
    ULONG pid;              ///< Process ID of the target process.
    BOOLEAN write;          ///< Flag to indicate write operation.
    BOOLEAN read;           ///< Flag to indicate read operation.
    BOOLEAN req_base;       ///< Flag to request base address.
    BOOLEAN draw_box;       ///< Flag to indicate drawing a box.
    INT r, g, b, x, y, w, h, t; ///< Parameters for drawing (color, position, size, thickness).
    void* output;           ///< Output buffer for read operations.
    const char* module_name; ///< Name of the module for base address request.
    ULONG64 base_address;   ///< Base address of the module.
    BOOLEAN draw_text;      ///< Flag to indicate drawing text.
    WCHAR text[256];        ///< Fixed-size buffer for text to be drawn.
} NULL_MEMORY;
#pragma pack(pop)


