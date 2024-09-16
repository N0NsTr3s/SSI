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
	void* output;
	const char* module_name;
	ULONG64 base_address;

}NULL_MEMORY;

uintptr_t base_address = 0;
std::uint32_t process_id = 0;


template<typename ... Arg>
uint64_t call_hook(const Arg ... args) {
	void* hooked_func = nullptr;
	HMODULE hModule = LoadLibrary("ntdll.dll");  // Use ntdll.dll for more stability

	if (!hModule) {
		std::cerr << "Failed to load ntdll.dll.\n";
		return 0;
	}
	//must find a not documented function and make sure to find it`s dll
	hooked_func = GetProcAddress(hModule, "NtQuerySystemInformation");  // A known function

	if (!hooked_func) {
		std::cerr << "Failed to get the address of NtQuerySystemInformation.\n";
		return 0;
	}

	auto func = static_cast<uint64_t(_stdcall*)(Arg...)>(hooked_func);
	if (func) {
		return func(args ...);
	}
	else {
		std::cerr << "Failed to cast the function pointer.\n";
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
	NULL_MEMORY instructions;
	instructions.pid = get_process_id(module_name);
	instructions.req_base = TRUE;
	instructions.module_name = module_name;

	auto result = call_hook(&instructions);
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
	call_hook(&instructions);

	return response;
}

bool write_memory(UINT_PTR write_address, UINT_PTR source_address, SIZE_T write_size)
{
	NULL_MEMORY instructions;
	instructions.pid = process_id;
	instructions.write = TRUE;
	instructions.address = write_address;
	instructions.buffer_address = reinterpret_cast<void*>(source_address);
	instructions.size = write_size;
	instructions.req_base = FALSE;
	instructions.read = FALSE;
	call_hook(&instructions);

	return true;
}

template<typename S>
bool write(UINT_PTR write_address, const S& value)
{
	return write_memory(write_address, reinterpret_cast<UINT_PTR>(&value), sizeof(S));
}

int main()
{
	
	/*process_id = get_process_id("Taskmgr.exe");
	if (!process_id) {
		std::cerr << "Failed to get the process ID of Taskmgr.exe.\n";
	}
	else {
		std::cout << "Taskmgr.exe process ID: " << process_id << "\n";
	}
	*/
	base_address = get_module_base_address("Taskmgr.exe");

	if (!base_address)
	{
		std::cout << "Failed to get the base address of the module" << std::endl;

	}
	else {
		std::cout << "Success" << std::endl;
	}
	Sleep(60000);
	return NULL;
}