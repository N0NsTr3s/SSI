#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <vector>

typedef struct _NULL_MEMORY
{
    void* buffer_address;
    UINT_PTR address;
    ULONGLONG size;
    ULONG pid;
    BOOLEAN write;
    BOOLEAN read;
    BOOLEAN req_base;
    BOOLEAN draw_box;
    int r, g, b, x, y, w, h, t;
    void* output;
    const char* module_name;
    ULONG64 base_address;

}NULL_MEMORY;


uintptr_t base_address = 0;
std::uint32_t process_id = 0;
HDC hdc;

template<typename ... Arg>
uint64_t call_hook(const Arg ... args) {
    void* hooked_func = GetProcAddress(LoadLibrary("win32u.dll"), "NtUserQueryInformationThread");
	std::cout << "Hooked function address: " << hooked_func << "\n";
    auto func = static_cast<uint64_t(_stdcall*)(Arg...)>(hooked_func);
	std::cout << "Returning hooked function...\n";
    return func(args ...);
}



struct HandleDisposer
{
    using pointer = HANDLE;
    void operator()(HANDLE handle) const
    {
        if (handle != NULL || handle != INVALID_HANDLE_VALUE) {
			std::cout << "Closing handle: " << handle << std::endl;
            CloseHandle(handle);
        }
    }
};
using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;

static std::uint32_t get_process_id(const std::string_view process_name) {
    PROCESSENTRY32 process_entry;
    const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (snapshot_handle.get() == INVALID_HANDLE_VALUE) {
        return 0;
    }

    process_entry.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snapshot_handle.get(), &process_entry)) {
        return 0;
    }

    while (Process32Next(snapshot_handle.get(), &process_entry)) {
        if (process_name.compare(process_entry.szExeFile) == 0) {
            std::cout << "Found process: " << process_entry.szExeFile << ", Process ID: " << process_entry.th32ProcessID << std::endl;
            return process_entry.th32ProcessID;
        }
    }

    return 0;
}

ULONG64 get_module_base_address(const char* module_name)
{
    std::cout << "Attempting to get module base address...\n";

    NULL_MEMORY instructions;
    instructions.pid = get_process_id(module_name);
    if (!instructions.pid) {
        std::cerr << "Failed to get process ID for " << module_name << ".\n";
        return 0;
    }

    instructions.req_base = TRUE;
    instructions.read = FALSE;
    instructions.write = FALSE;
    instructions.draw_box = FALSE;
    instructions.module_name = module_name;

    std::cout << "NULL_MEMORY details: PID: " << instructions.pid << '\n'
        << ", Module: " << instructions.module_name << '\n';
    if (instructions.req_base == TRUE) {
        std::cout << ", Request base: TRUE'\n";
    }
    else if (instructions.req_base == FALSE) {
        std::cout << ", Request base: FALSE '\n";
    }
    else {
        std::cout << ", Request base: Error" << "\n";
    }

    call_hook(&instructions);
    std::cout << "After call_hook, base_address: " << instructions.base_address << "\n";  // Log base_address


    ULONG64 base = instructions.base_address;
    if (!base) {
        std::cerr << "Failed to get the base address from call_hook.\n";
    }
    else {
        std::cout << "Module base address: " << base << "\n";
    }

    return base;
}

template<class T>
T Read(UINT_PTR read_address)
{
    T response{};
    NULL_MEMORY instructions;
    instructions.pid = process_id;
    instructions.read = TRUE;
    instructions.size = sizeof(T);
    instructions.address = read_address;
    instructions.output = &response;
    instructions.req_base = FALSE;
    instructions.write = FALSE;
    instructions.draw_box = FALSE;
    call_hook(&instructions);

    return response;
}

bool write_memory(UINT_PTR write_address, UINT_PTR source_address, SIZE_T write_size)
{
    NULL_MEMORY instructions;
    instructions.pid = process_id;
    instructions.write = TRUE;
    instructions.address = write_address;
    instructions.buffer_address = (void*)source_address;
    instructions.size = write_size;
    instructions.req_base = FALSE;
    instructions.read = FALSE;
    instructions.draw_box = FALSE;
    call_hook(&instructions);

    return true;
}

bool draw_box(int x, int y, int w, int h, int r, int g, int b, int t)
{
    NULL_MEMORY instructions;
    instructions.write = FALSE;
    instructions.read = FALSE;
    instructions.req_base = FALSE;
    instructions.pid = process_id;
    instructions.draw_box = TRUE;
    instructions.x = x;
    instructions.y = y;
    instructions.w = w;
    instructions.h = h;
    instructions.r = r;
    instructions.g = g;
    instructions.b = b;
    instructions.t = t;
    call_hook(&instructions);


    std::cout << "Drawing box at (" << x << ", " << y << ") with width " << w << " and height " << h << "\n";
    return true;
}

template<typename S>
bool write(UINT_PTR write_address, const S& value)
{
    return write_memory(write_address, reinterpret_cast<UINT_PTR>(&value), sizeof(S));
}

int main()
{
    std::cout << "Starting the program...\n";
	std::cout << get_module_base_address("Taskmgr.exe") << std::endl;

    std::cout << "Entering loop...";
    while (true) {
        draw_box(100, 100, 100, 100, 255, 0, 0, 5);
    }


    return 0;
}