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
} NULL_MEMORY;

uintptr_t base_address = 0;
std::uint32_t process_id = 0;
HDC hdc;

template<typename ... Arg>
uint64_t call_hook(const Arg ... args) {
    void* hooked_func = nullptr;
    HMODULE hModule = LoadLibrary("ntdll.dll");  // Ensure correct DLL
    if (!hModule) {
        std::cerr << "Failed to load win32u.dll.\n";
        return 0;
    }

    hooked_func = GetProcAddress(hModule, "NtQuerySection");
    if (!hooked_func) {
        std::cerr << "Failed to get the address of NtQuerySection.\n";
        return 0;
    }

    std::cout << "Successfully got address of NtQuerySection.\n";

    auto func = static_cast<uint64_t(_stdcall*)(Arg...)>(hooked_func);
    if (func) {
        auto result = func(args ...);
        std::cout << "Hook executed, result: " << result << "\n";  // Log the result of the function
        return result;
    }
    else {
        std::cout << "Failed to cast the function pointer.\n";
        return 0;
    }
}

struct HandleDisposer
{
    using pointer = HANDLE;
    void operator()(HANDLE handle) const
    {
        if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
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

    std::cout << "NULL_MEMORY details: PID: " << instructions.pid
        << ", Module: " << instructions.module_name
        << ", Request base: " << instructions.req_base << "\n";

    auto result = call_hook(&instructions);
    std::cout << "call_hook result: " << result << "\n";  // Log the result of call_hook
    std::cout << "After call_hook, base_address: " << instructions.base_address << "\n";  // Log base_address

    if (!result) {
        std::cerr << "call_hook failed to retrieve base address.\n";
        return 0;
    }

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
    auto result = call_hook(&instructions);

    if (!result) {
        std::cerr << "call_hook failed to draw box.\n";
        return false;
    }

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

    process_id = get_process_id("Taskmgr.exe");
    if (!process_id) {
        std::cerr << "Failed to get the process ID of Taskmgr.exe.\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }
    else {
        std::cout << "Taskmgr.exe process ID: " << process_id << "\n";
    }

    base_address = get_module_base_address("Taskmgr.exe");
    if (!base_address) {
        std::cerr << "Failed to get the base address of the module.\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }
    else {
        std::cout << "Success in getting the base address.\n";
    }

    auto result = draw_box(50, 50, 50, 50, 255, 0, 0, 5);
    if (!result) {
        std::cerr << "Failed to draw box.\n";
        std::cout << result;

    }
    else {
        std::cout << "Box drawn successfully.\n";
        draw_box(50, 50, 50, 50, 255, 0, 0, 5);
    }
    

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}