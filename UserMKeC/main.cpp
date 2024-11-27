#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <tchar.h>-
#include "Offsets.h"
#include <thread>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <vector>
#include <numbers>
#include <iomanip>
#include <WinUser.h>
#include <dwmapi.h>

std::atomic<bool> running = true;
std::vector<uintptr_t> entities;
int screen_width = 1540;//GetSystemMetrics(SM_CXSCREEN);
int screen_height = 1000;//GetSystemMetrics(SM_CYSCREEN);


int PID;
#pragma pack(push, 1)
typedef struct _NULL_MEMORY {
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
#pragma pack(pop)


HDC hdc;

template<typename ... Arg>
uint64_t call_hook(const Arg ... args) {
    LoadLibrary("user32.dll");
    void* hooked_func = nullptr;
    HMODULE hModule = LoadLibrary("win32u.dll");
    if (hModule) {
        hooked_func = GetProcAddress(hModule, "NtCreateImplicitCompositionInputSink");
        if (!hooked_func) {
            std::cerr << "Failed to get the address of NtUserGetAltTabInfo / NtCreateImplicitCompositionInputSink.\n";
            return FALSE;
        }
    }
    else {
        std::cerr << "Failed to load win32u.dll.\n";
        return FALSE;
    }

    std::cout << "Hooked function address: " << hooked_func << "\n";

    // Cast the function pointer to the correct type
    //auto func = reinterpret_cast<uint64_t(__fastcall*)(Arg...)>(hooked_func);
    auto func = static_cast<uint64_t(_stdcall*)(Arg...)>(hooked_func);              //<- This is the original line
    //auto func = reinterpret_cast<uint64_t(__fastcall*)(Arg...)>(hooked_func);
	std::cout << "Function pointer: " << func << "\n";

	//std::cout << " Attempting to return function pointer: " << func(args...) << "\n";
	return func(args...);                                                           //<- This does not return anything so we just have to figure it why? may be the function signature or a bug
}






struct HandleDisposer {
    using pointer = HANDLE;
    void operator()(HANDLE handle) const {
        if (handle != NULL || handle != INVALID_HANDLE_VALUE) {
            std::cout << "Closing handle: " << handle << std::endl;
            CloseHandle(handle);
        }
    }
};

using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;


// Helper function to convert a string to lowercase
std::string to_lowercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}

static std::uint32_t get_process_id(std::string_view process_name) {
    std::cout << "Attempting to get process ID for: " << process_name << "\n";
    PROCESSENTRY32 process_entry;
    const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL));
    if (snapshot_handle.get() == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot handle.\n";
        return NULL;
    }

    process_entry.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snapshot_handle.get(), &process_entry)) {
        std::cerr << "Failed to get first process entry.\n";
        return 0;
    }

    std::string target_process_name = to_lowercase(std::string(process_name));

    do {
        std::string current_process_name = to_lowercase(process_entry.szExeFile);
        if (target_process_name == current_process_name) {
            std::cout << "Found process: " << process_entry.szExeFile << ", Process ID: " << process_entry.th32ProcessID << std::endl;
            return process_entry.th32ProcessID;
        }
    } while (Process32Next(snapshot_handle.get(), &process_entry));

    std::cerr << "Process not found: " << process_name << "\n";
    return 0;
}

static ptrdiff_t get_module_base_address(const char* module_name) {
    std::cout << "Attempting to get module base address...\n";

    NULL_MEMORY instructions = { 0 };
	instructions.pid = get_process_id("cs2.exe");
    instructions.req_base = TRUE;
    instructions.read = FALSE;
    instructions.write = FALSE;
    instructions.draw_box = FALSE;
    instructions.module_name = module_name;

    std::cout << "NULL_MEMORY details: PID: " << instructions.pid << '\n'
        << "Module: " << instructions.module_name << '\n';

    call_hook(&instructions);
    std::cout << "After call_hook, base_address: " << instructions.base_address << "\n";  // Log base_address

    ptrdiff_t base = NULL;
	base = instructions.base_address;
    if (!base) {
        std::cerr << "Failed to get the base address from call_hook.\n";
    }
    else {
        std::cout << "Module base address: " << base << "\n";
    }

    return instructions.base_address;
}

template<class T>
T Read(UINT_PTR read_address) {
    T response{};
    NULL_MEMORY instructions;
    std::cout << "Address being passed: " << read_address << std::endl;
    instructions.pid = PID;
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

bool write_memory(UINT_PTR write_address, UINT_PTR source_address, SIZE_T write_size) {
    NULL_MEMORY instructions;
    instructions.pid = PID;
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

bool draw_box(int x, int y, int w, int h, int r, int g, int b, int t) {
    NULL_MEMORY instructions;
    instructions.write = FALSE;
    instructions.read = FALSE;
    instructions.req_base = FALSE;
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
bool write(UINT_PTR write_address, const S& value) {
    return write_memory(write_address, (UINT_PTR)&value, sizeof(S));
}


struct view_matrix_t {
    float matrix[16];
};

struct vec4
{
    float w, x, y, z;
};

struct vec3
{
    float x, y, z;
    vec3 operator+(vec3 other)
    {
        return { this->x + other.x, this->y + other.y, this->z + other.z };
    }
    vec3 operator-(vec3 other)
    {
        return { this->x - other.x, this->y - other.y, this->z - other.z };
    }

    vec3 RelativeAngle()
    {
        return {
            std::atan2(-z, std::hypot(x, y)) * (180.0f / std::numbers::pi_v<float>),
            std::atan2(y, x) * (180.0f / std::numbers::pi_v<float>),
            0.0f
        };
    }

};

struct vec2
{
    float x, y;
};

bool w2s(const vec3& world, vec2& screen, const float m[16]) {
    vec4 clipCoord;
    clipCoord.x = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
    clipCoord.y = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
    clipCoord.z = world.x * m[8] + world.y * m[9] + world.z * m[10] + m[11];
    clipCoord.w = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];

    if (clipCoord.w < 0.1f) return false;

    vec3 ndc;
    ndc.x = clipCoord.x / clipCoord.w;
    ndc.y = clipCoord.y / clipCoord.w;

    screen.x = ((screen_width / 2) * (1 + ndc.x));
    screen.y = ((screen_height / 2) * (1 - ndc.y)); // Note the inversion of y-axis

    return true;
}

int returnPlayerTeam(uintptr_t entity_list) {
    for (int i = 0; i < 64; i++) {
        uintptr_t listEntry = Read<uintptr_t>(entity_list + ((8 * (i & 0x7fff) >> 9) + 16));
        std::cout << '\n' << '\n';
        if (!listEntry) {
            std::cout << "List entry is null.\n\n";
            continue;
        }

        uintptr_t entityController = Read<uintptr_t>(listEntry + 120 * (i & 0x1ff));
        if (!entityController) {
            std::cout << "Entity controller is null.\n\n";
            continue;
        }

        uintptr_t entityControllerPawn = Read<uintptr_t>(entityController + CCSPlayerController::m_hPlayerPawn);
        if (!entityControllerPawn) {
            std::cout << "Entity controller pawn is null.\n\n";
            continue;
        }

        uintptr_t CSPlayerPawn = Read<uintptr_t>(listEntry + 120 * (entityControllerPawn & 0x1ff));


        int team = Read<int>(CSPlayerPawn + C_BaseEntity::m_iTeamNum);
        std::cout << "Player Team: " << team << std::endl;
        if (team != 0) 
            return team;


    }
};


void loop(ptrdiff_t base_address=get_module_base_address("client.dll")) {
    int team = {};
    // All of this should be multithreaded
	std::cout << "Entering Game loop...\n";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    std::cout << "Base Address: 0x" << std::hex << base_address << "\n";
    std::cout << "Offset: 0x" << std::hex << client_dll::dwEntityList << "\n";
    std::cout << "Target Address: 0x" << std::hex << (base_address + client_dll::dwEntityList) << "\n";
    uintptr_t entity_list = Read<std::uintptr_t>(base_address + client_dll::dwEntityList);
	std::cout << "Entity list: " << entity_list << std::endl;

    std::cout << "Base Address: 0x" << std::hex << base_address << "\n";
    std::cout << "Offset: 0x" << std::hex << client_dll::dwLocalPlayerPawn << "\n";
    std::cout << "Target Address: 0x" << std::hex << (base_address + client_dll::dwLocalPlayerPawn) << "\n";
    uint32_t localPlayerPawn = static_cast<uint32_t>(Read<uintptr_t>(base_address + client_dll::dwLocalPlayerPawn));
	std::cout << "Local player pawn: " << localPlayerPawn << std::endl;

	uint32_t localPlayerController = Read<uint32_t>(base_address + client_dll::dwLocalPlayerController);
    std::cout << "localPlayerController: " << localPlayerController << std::endl;


            
	team = returnPlayerTeam(entity_list);
        
    
	std::cout << "My Team: " << team << std::endl;
	//Sleep(5000);// <- That`s for testing purposes
    while (get_process_id("cs2.exe") != 0) {
		std::cout << "Looping inside the game...\n";
        std::vector<uintptr_t> buffer = {};
        for (int i = 0; i < 64; i++) {
           
            uintptr_t listEntry = Read<uintptr_t>(entity_list + ((8 * (i & 0x7fff) >> 9) + 16));
            std::cout << '\n' << '\n';
            if (!listEntry) {
				std::cout << "List entry is null.\n\n";
                continue;
            }

            uintptr_t entityController = Read<uintptr_t>(listEntry + 120 * (i & 0x1ff));
            if (!entityController) {
				std::cout << "Entity controller is null.\n\n";
                continue;
            }

            uintptr_t entityControllerPawn = Read<uintptr_t>(entityController + CCSPlayerController::m_hPlayerPawn);
            if (!entityControllerPawn) {
				std::cout << "Entity controller pawn is null.\n\n";
                continue;
            }

			uintptr_t CSPlayerPawn = Read<uintptr_t>(listEntry + 120 * (entityControllerPawn & 0x1ff));

			if (!CSPlayerPawn) continue;

			int health = Read<int>(CSPlayerPawn + C_BaseEntity::m_iHealth);
			std::cout << "Health: " << health << std::endl;

			if (health<1 || health > 100) continue;

			int PlayerTeam = Read<int>(CSPlayerPawn + C_BaseEntity::m_iTeamNum);
			std::cout << "Player Team: " << PlayerTeam << std::endl;
			if (PlayerTeam == team) continue;

            uintptr_t entity = Read<uintptr_t>(listEntry + 120 * (entityControllerPawn & 0x1ff));
            
            if (entity) {
				std::cout <<"Nr. of entities: " << entity << std::endl;
                buffer.emplace_back(entity);
            }

            
            entities = buffer;
        
           
            
        };

    };
}
void ShowingPl(ptrdiff_t base_address = get_module_base_address("client.dll")) {
    std::cout << "Trying to draw the players...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    view_matrix_t vm = Read<view_matrix_t>(base_address + client_dll::dwViewMatrix);
    std::cout << entities.size() << std::endl;
    for (uintptr_t entity : entities) {
        std::cout << "Entity: " << entity << std::endl;
        vec3 absOrigin = Read<vec3>(entity + C_BasePlayerPawn::m_vOldOrigin);
        vec3 eyePos = absOrigin + Read<vec3>(entity + C_BaseModelEntity::m_vecViewOffset);

        vec2 head, feet;

        if (w2s(absOrigin, head, vm.matrix)) {
            std::cout << "Head: " << head.x << ", " << head.y << std::endl;

            if (w2s(eyePos, feet, vm.matrix)) {
                float height = feet.y - head.y;
                float width = height / 2.2f; // Adjust width as needed
                feet.x = head.x + width;
                feet.y = head.y + height;
                draw_box(static_cast<int>(head.x), static_cast<int>(head.y), static_cast<int>(width), static_cast<int>(height), 255, 0, 0, 2);
            }
        }
    }
}



// Function to run the loop in a separate thread
void run_loop_in_thread(ptrdiff_t base_address) {
    std::thread loop_thread(loop, base_address);
    loop_thread.detach(); // Detach the thread to run independently
}






// Global variables
HWND g_hWnd = NULL;

// Window procedure for the transparent overlay
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        // Refresh the overlay periodically
        InvalidateRect(hwnd, NULL, TRUE); // Mark the entire window for redrawing
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        // Fill the window with a transparent color (or draw anything else as needed)
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0)); // Black for now
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Function to create the transparent overlay window
void TransparentOverlayThread(HINSTANCE hInstance) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TransparentOverlay";
    RegisterClass(&wc);

    g_hWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT, // Styles for transparent, topmost window
        wc.lpszClassName,                                  // Class name
        "Overlay",                                        // Window title (optional)
        WS_POPUP,                                          // No border, popup style
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), // Fullscreen dimensions
        NULL, NULL, hInstance, NULL);

    if (!g_hWnd) {
        MessageBox(NULL, "Failed to create overlay window.", "Error", MB_ICONERROR);
        return;
    }

    // Make the window fully transparent
    if (!SetLayeredWindowAttributes(g_hWnd, 0, 0, LWA_ALPHA)) {
        MessageBox(NULL, "Failed to set layered window attributes.", "Error", MB_ICONERROR);
        return;
    }

    // Show the window and ensure it stays on top
    ShowWindow(g_hWnd, WS_EX_TOPMOST);
    UpdateWindow(g_hWnd);
    SetWindowPos(g_hWnd,HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // Set a timer for periodic refresh (e.g., ~60 FPS with 16ms interval)
    SetTimer(g_hWnd, 1, 3, NULL);

    // Message loop for the overlay
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}



int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::thread overlayThread(TransparentOverlayThread, hInstance);
    PID = get_process_id("cs2.exe");
    // Take a look if you can search for client.dll memory region
	try {
		std::cout << "Getting process ID...\n";
		uint32_t process_id = get_process_id("cs2.exe");
		if (!process_id) {
			std::cerr << "Failed to get process ID.\n";

            //Sleep(10000);
			return 1;
		}
        else {
			std::cout << "Process ID: " << process_id << "\n";
        }

		std::cout << "Getting module base address...\n";
		ptrdiff_t base_address = get_module_base_address("client.dll");
		std::cout << base_address << std::endl;
        std::cout << "Entering loop...\n";

      
        
		run_loop_in_thread(base_address);

		/*for (int i = 10; i < 10; i++)
        {
            ShowingPl(base_address);
        }*/
        //std::thread window(CreateTransparentWindow);
        //window.detach();
        overlayThread.detach();
		while (true)
		{       
            //Sleep(5000);// <- That`s for testing purposes
			ShowingPl(base_address);
		}

	}
		
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    catch (...) {
        std::cerr << "Unknown exception occurred.\n";
    }
	Sleep(10000);
    return 0;



}