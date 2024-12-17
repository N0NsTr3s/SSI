/*
An operating system (OS) is a fundamental software component that manages computer hardware and
software resources and provides common services for computer programs. It acts as an intermediary
between users and the computer hardware, ensuring that applications can run efficiently and effectively.
*/

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <tchar.h>
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
#include <mutex>
#include <cwchar>
#include <typeinfo>
#include <atomic>

std::mutex players_mutex;
std::atomic<bool> running = true;
std::vector<uintptr_t> entities;
int screen_width = 1540;
int screen_height = 1000;

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
    INT r, g, b, x, y, w, h, t;
    void* output;
    const char* module_name;
    ULONG64 base_address;
    BOOLEAN draw_text;
    WCHAR text[256];
} NULL_MEMORY;
#pragma pack(pop)

HDC hdc;

/**
 * @brief Calls a function from the win32u.dll library with the provided arguments.
 *
 * This template function is used to call a specific function from the win32u.dll library.
 * The function to be called is determined by its name, "NtCreateImplicitCompositionInputSink".
 * The template allows for flexibility in the number and types of arguments that can be passed to the function.
 *
 * @tparam Arg Variadic template parameter pack representing the types of arguments to be passed to the function.
 * @param args The arguments to be passed to the function.
 * @return The result of the function call, or FALSE if the library or function could not be loaded.
 */
template<typename ... Arg>
uint64_t call_hook(const Arg ... args) {
    static HMODULE hModule = LoadLibrary("win32u.dll");
    if (!hModule) {
        std::cerr << "Failed to load win32u.dll.\n";
        return FALSE;
    }

    /**
     * Retrieves the address of the function "NtCreateImplicitCompositionInputSink" from the loaded library.
     *
     * The reinterpret_cast is used here to convert the function pointer obtained from GetProcAddress to the appropriate type.
     * 
     * @note reinterpret_cast is used because it allows any pointer type to be converted into any other pointer type.
     * This is necessary here because GetProcAddress returns a generic FARPROC (essentially a void pointer), and we need to
     * convert it to a specific function pointer type. Other casts like static_cast or dynamic_cast are not suitable for this
     * purpose because they are more restrictive and do not allow such conversions.
     */
    static auto func = reinterpret_cast<uint64_t(_stdcall*)(Arg...)>(GetProcAddress(hModule, "NtCreateImplicitCompositionInputSink"));
    if (!func) {
        std::cerr << "Failed to get the address of NtCreateImplicitCompositionInputSink.\n";
        return FALSE;
    }

    return func(args...);
}

struct HandleDisposer {
    using pointer = HANDLE;
    void operator()(HANDLE handle) const {
        if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }
};

using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;

//Converts string to lowercase
std::string to_lowercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}

/**
 * Retrieves the process ID of a given process name.
 *
 * This function takes a process name as input and returns the process ID (PID) of the first matching process.
 * It uses the Toolhelp32 API to create a snapshot of all running processes and then iterates through them
 * to find the process with the specified name.
 *
 * @param process_name The name of the process to find.
 * @return The process ID of the specified process, or 0 if the process is not found or an error occurs.
 */
static std::uint32_t get_process_id(std::string_view process_name) {
    PROCESSENTRY32 process_entry; // Structure to store process information
    const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL)); // Create a snapshot of all running processes
    if (snapshot_handle.get() == INVALID_HANDLE_VALUE) { // Check if snapshot creation failed
        std::cerr << "Failed to create snapshot handle.\n";
        return NULL;
    }

    process_entry.dwSize = sizeof(PROCESSENTRY32); // Initialize the size of the process entry structure
    if (!Process32First(snapshot_handle.get(), &process_entry)) { // Get the first process in the snapshot
        std::cerr << "Failed to get first process entry.\n";
        return 0;
    }

    std::string target_process_name = to_lowercase(std::string(process_name)); // Convert the target process name to lowercase

    do {
        std::string current_process_name = to_lowercase(process_entry.szExeFile); // Convert the current process name to lowercase
        if (target_process_name == current_process_name) { // Check if the current process name matches the target process name
            return process_entry.th32ProcessID; // Return the process ID if a match is found
        }
    } while (Process32Next(snapshot_handle.get(), &process_entry)); // Iterate through the remaining processes in the snapshot

    std::cerr << "Process not found: " << process_name << "\n";
    return 0;
}

static ptrdiff_t get_module_base_address(const char* module_name) {
    NULL_MEMORY instructions = { 0 };
    instructions.pid = get_process_id("cs2.exe");
    instructions.req_base = TRUE;
    instructions.module_name = module_name;

    call_hook(&instructions);

    return instructions.base_address;
}

template<class T>
T Read(UINT_PTR read_address) {
    T response{};
    NULL_MEMORY instructions = { 0 };
    instructions.pid = PID;
    instructions.read = TRUE;
    instructions.size = sizeof(T);
    instructions.address = read_address;
    instructions.output = &response;

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
	instructions.draw_text = FALSE;
    call_hook(&instructions);

    return true;
}

bool draw_text(int x, int y, const wchar_t* text) {
    if (!text || wcslen(text) == 0) {
        std::wcerr << L"Invalid or empty text provided.\n";
        return false;
    }

    NULL_MEMORY instructions = { 0 };
    instructions.write = FALSE;
    instructions.read = FALSE;
    instructions.req_base = FALSE;
    instructions.draw_box = FALSE;
    instructions.draw_text = TRUE;
    instructions.x = x;
    instructions.y = y;

    // Safely copy the text into the fixed-size buffer
    wcsncpy_s(instructions.text, text, _countof(instructions.text) - 1);
    instructions.text[_countof(instructions.text) - 1] = L'\0'; // Null-terminate

    std::wcout << L"User-mode text: " << instructions.text << L"\n";

    // Call the kernel hook
    call_hook(&instructions);

    return true;
}




bool draw_box(int x, int y, int w, int h, int r, int g, int b, int t) {
    NULL_MEMORY instructions;
    instructions.write = FALSE;
    instructions.read = FALSE;
    instructions.req_base = FALSE;
    instructions.draw_box = TRUE;
	instructions.draw_text = FALSE;
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
    /**
     * Calculates the relative angle of the vector.
     *
     * This function computes the relative angle of the vector in degrees.
     * It uses the arctangent function to determine the angle in the x-y plane
     * and the angle relative to the z-axis.
     *
     * @return A vec3 object containing the relative angles in degrees.
     */
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

/**
 *  Converts world coordinates to screen coordinates.
 *
 * This function takes world coordinates and a transformation matrix, and converts the world coordinates
 * to screen coordinates. It returns true if the conversion is successful, and false otherwise.
 *
 * @param world The world coordinates to be converted.
 * @param screen The resulting screen coordinates.
 * @param m The transformation matrix.
 * @return True if the conversion is successful, false otherwise.
 */
bool w2s(const vec3& world, vec2& screen, const float m[16]) {
    vec4 clipCoord;
    // Transform the world coordinates to clip space coordinates using the transformation matrix.
    clipCoord.x = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
    clipCoord.y = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
    clipCoord.z = world.x * m[8] + world.y * m[9] + world.z * m[10] + m[11];
    clipCoord.w = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];

    // If the w component is less than a small threshold, the point is behind the camera.
    if (clipCoord.w < 0.1f) return false;

    vec3 ndc;
    // Normalize the clip space coordinates to get normalized device coordinates (NDC).
    ndc.x = clipCoord.x / clipCoord.w;
    ndc.y = clipCoord.y / clipCoord.w;

    // Convert NDC to screen coordinates.
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

struct PlayerInfo {
    uintptr_t entity;
    int health;
    vec3 absOrigin;
    vec3 eyePos;
    vec3 prevAbsOrigin;
};

std::vector<PlayerInfo> players;
PlayerInfo playerBuffer1[64];
PlayerInfo playerBuffer2[64];
std::atomic<int> currentBuffer = 0; // Atomic to avoid race conditions


void loop(ptrdiff_t base_address = get_module_base_address("client.dll")) {
    std::cout << "Starting game loop...\n";

    static const uintptr_t entity_list = Read<uintptr_t>(base_address + client_dll::dwEntityList);
    static const int local_team = returnPlayerTeam(entity_list);

    while (get_process_id("cs2.exe") != 0) {
        
        /**
         * Determine the write buffer
         * The current buffer index is stored in an atomic variable `currentBuffer`.
         * The write buffer index is set to 1 if the current buffer index is 0, otherwise it is set to 0.
         * The write buffer is then selected based on the write buffer index.

         */
        int writeBufferIndex = currentBuffer.load() == 0 ? 1 : 0;
        PlayerInfo* writeBuffer = (writeBufferIndex == 0) ? playerBuffer1 : playerBuffer2;

        size_t playerCount = 0;

        for (int i = 0; i < 64 && playerCount < 64; i++) {
            /*
            * Breaking down listEntry calculation:
            * i & 0x7fff: A bitwise AND operation that limits i to the lower 15 bits (i.e., i % 32768), ensuring the result stays within a range of 0 to 32767 (2^15).
            * 8 * (i & 0x7fff): Multiplies the result of (i & 0x7fff) by 8. This represents the size of each entry in the entity list.
            * >> 9: A bitwise right shift by 9 positions, equivalent to dividing by 2^9 = 512. This reduces the index range further, effectively selecting a page of entities.
            * ((8 * (i & 0x7fff) >> 9) + 16): Adds 16 to the result. This is the offset within the structure, pointing to a specific field.
           */
            uintptr_t listEntry = Read<uintptr_t>(entity_list + ((8 * (i & 0x7fff) >> 9) + 16));
            if (!listEntry) continue;
            /*
			 * Breaking down entityController calculation:
             * i & 0x1ff: A bitwise AND operation with 0x1ff (binary 111111111 or decimal 511). In effect, this divides the entities into "pages" or blocks of size 512 and selects an entity within the current page.
             * 120 * (i & 0x1ff): This calculation gives the byte offset to the specific entity within the list.
            */
            uintptr_t entityController = Read<uintptr_t>(listEntry + 120 * (i & 0x1ff));
            if (!entityController) continue;

            uintptr_t entityControllerPawn = Read<uintptr_t>(entityController + CCSPlayerController::m_hPlayerPawn);
            if (!entityControllerPawn) continue;

            /*
			 * Breaking Down CSPlayerPawn Calculation:
             * & 0x1ff: A bitwise AND operation with 0x1ff (binary 111111111 or decimal 511). This ensures the access is restricted to one of 512 possible slots in a "page" or group of entries.
			 * 120 * (entityControllerPawn & 0x1ff): This calculation determines the byte offset for the specific entry corresponding to the entityControllerPawn.
            */
            uintptr_t CSPlayerPawn = Read<uintptr_t>(listEntry + 120 * (entityControllerPawn & 0x1ff));
            if (!CSPlayerPawn) continue;

            int health = Read<int>(CSPlayerPawn + C_BaseEntity::m_iHealth);
            if (health < 1 || health > 100) continue;

            int team = Read<int>(CSPlayerPawn + C_BaseEntity::m_iTeamNum);
            if (team == local_team) continue;

            vec3 absOrigin = Read<vec3>(CSPlayerPawn + C_BasePlayerPawn::m_vOldOrigin);
            vec3 viewOffset = Read<vec3>(CSPlayerPawn + C_BaseModelEntity::m_vecViewOffset);

            writeBuffer[playerCount++] = PlayerInfo{
                CSPlayerPawn,
                health,
                absOrigin,
                absOrigin + viewOffset,
                absOrigin // Store previous position for potential interpolation
            };
        }

        // Lock and switch the current buffer
        {
            std::lock_guard<std::mutex> lock(players_mutex);
            players.assign(writeBuffer, writeBuffer + playerCount);
            currentBuffer.store(writeBufferIndex); // Update the buffer index atomically
        }

       
    }
}

struct RenderInfo {
    vec2 head;
    vec2 feet;
    int health;
};

std::vector<RenderInfo> renderBuffer1;
std::vector<RenderInfo> renderBuffer2;
std::atomic<int> currentRenderBuffer = 0; // Atomic index to determine the active render buffer

void ShowingPl(ptrdiff_t base_address = get_module_base_address("client.dll")) {
    std::cout << "Preparing rendering instructions...\n";
    view_matrix_t vm = Read<view_matrix_t>(base_address + client_dll::dwViewMatrix);

    // Determine the write buffer
    int writeBufferIndex = currentRenderBuffer.load() == 0 ? 1 : 0;
    std::vector<RenderInfo>& writeBuffer = (writeBufferIndex == 0) ? renderBuffer1 : renderBuffer2;

    writeBuffer.clear(); // Clear the buffer for new rendering data

    // Lock the players vector and prepare rendering instructions
    {
        std::lock_guard<std::mutex> lock(players_mutex);
        for (const auto& player : players) {
            vec2 head, feet;

            if (w2s(player.absOrigin, head, vm.matrix) && w2s(player.eyePos, feet, vm.matrix)) {
                float height = feet.y - head.y;
                float width = height / 2.2f;

                // Add rendering information to the write buffer
                writeBuffer.push_back(RenderInfo{
                    head,
                    feet,
                    player.health
                    });
            }
        }
    }

    // Switch the active render buffer atomically
    currentRenderBuffer.store(writeBufferIndex);

    // Render using the other buffer
    int readBufferIndex = (writeBufferIndex == 0) ? 1 : 0;
    const std::vector<RenderInfo>& readBuffer = (readBufferIndex == 0) ? renderBuffer1 : renderBuffer2;

    std::cout << "Rendering players...\n";
    for (const auto& renderInfo : readBuffer) {
        float height = renderInfo.feet.y - renderInfo.head.y;
        float width = height / 2.2f;
        int x = static_cast<int>(renderInfo.head.x);
        int y = static_cast<int>(renderInfo.head.y);

        // Draw bounding box
        draw_box(x, y, static_cast<int>(width), static_cast<int>(height), 255, 0, 0, 2);

        // Draw health
        WCHAR buffer[32];
        swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%d", renderInfo.health);
        draw_text(x + static_cast<int>(width) + 5, y, buffer); // Offset health display
    }
}


// Function to run the loop in a separate thread
void run_loop_in_thread(ptrdiff_t base_address) {
    std::thread loop_thread(loop, base_address);
    loop_thread.detach(); // Detach the thread to run independently
}



void run_showPL_in_thread(ptrdiff_t base_address) {
    std::thread showPL_thread(ShowingPl, base_address);
    showPL_thread.detach(); // Detach the thread to run independently
}

// Function to create the transparent overlay window <--- This is the function that creates the overlay and is has to be deleted, as the whole point of the driver is to not have anything over the game 
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

	Sleep(1900);
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
		//run_showPL_in_thread(base_address);
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