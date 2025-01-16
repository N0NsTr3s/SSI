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
#include <mutex>
#include <cwchar>
#include <typeinfo>
#include <atomic>
#include <fstream>
std::mutex players_mutex;
std::atomic<bool> running = true;
std::vector<uintptr_t> entities;
int screen_width = 1884;//GetSystemMetrics(SM_CXSCREEN);
int screen_height = 786;// GetSystemMetrics(SM_CYSCREEN);


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
    BOOLEAN draw_text;
    CHAR text[256];
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

    //std::cout << "Hooked function address: " << hooked_func << "\n";

    // Cast the function pointer to the correct type
    //auto func = reinterpret_cast<uint64_t(__fastcall*)(Arg...)>(hooked_func);
    auto func = static_cast<uint64_t(_stdcall*)(Arg...)>(hooked_func);              //<- This is the original line
    //auto func = reinterpret_cast<uint64_t(__fastcall*)(Arg...)>(hooked_func);
    //std::cout << "Function pointer: " << func << "\n";

    //std::cout << " Attempting to return function pointer: " << func(args...) << "\n";
    return func(args...);                                                           //<- This does not return anything so we just have to figure it why? may be the function signature or a bug
}



bool isOptionEnabled(const std::string& option) {
    std::ifstream configFile("config.txt");
    std::string line;
    while (std::getline(configFile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == option && value == "True") {
				std::cout << option << " is enabled.\n";
                return true;
            }
        }
    }
	std::cout << option << " is disabled.\n";
    return false;
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
    //std::cout << "Address being passed: " << read_address << std::endl;
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

    std::ifstream configFile("config.txt");
    std::string line;
   

	if (isOptionEnabled("Players")) {
		call_hook(&instructions);
	}


    //std::cout << "Drawing box at (" << x << ", " << y << ") with width " << w << " and height " << h << "\n";
    return true;
}

bool draw_text(int x, int y, CHAR* text) {


    NULL_MEMORY instructions = { 0 };
    instructions.draw_text = TRUE;
    instructions.write = FALSE;
    instructions.read = FALSE;
    instructions.req_base = FALSE;
    instructions.draw_box = FALSE;

    instructions.x = x;
    instructions.y = y;

    // Safely copy the text into the fixed-size buffer
    strncpy_s(instructions.text, sizeof(instructions.text), text, _TRUNCATE);

    //std::cout << "User-mode text: " << instructions.text << "\n";

    // Call the kernel hook
    call_hook(&instructions);
    memset(instructions.text, 0, sizeof(instructions.text));
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
            //std::cout << "List entry is null.\n\n";
            continue;
        }

        uintptr_t entityController = Read<uintptr_t>(listEntry + 120 * (i & 0x1ff));
        if (!entityController) {
            //std::cout << "Entity controller is null.\n\n";
            continue;
        }

        uintptr_t entityControllerPawn = Read<uintptr_t>(entityController + CCSPlayerController::m_hPlayerPawn);
        if (!entityControllerPawn) {
            //std::cout << "Entity controller pawn is null.\n\n";
            continue;
        }

        uintptr_t CSPlayerPawn = Read<uintptr_t>(listEntry + 120 * (entityControllerPawn & 0x1ff));


        int team = Read<int>(CSPlayerPawn + C_BaseEntity::m_iTeamNum);
        //std::cout << "Player Team: " << team << std::endl;
        if (team != 0)
            return team;


    }
};

std::string read_name(uintptr_t entityController) {
    std::string name;
    char ch;
    uintptr_t address = entityController + CBasePlayerController::m_iszPlayerName;
    while ((ch = Read<char>(address++)) != '\0') {
        name += ch;
    }
    return name;
}

struct PlayerInfo {
    uintptr_t entity;
    int health;
    int armor;
    bool helmet;
    bool vest;
    std::string name;
    short weaponId;
    vec3 absOrigin;
    vec3 eyePos;
    vec3 prevAbsOrigin;
};

std::vector<PlayerInfo> players;
PlayerInfo playerBuffer1[64];
PlayerInfo playerBuffer2[64];
std::atomic<int> currentBuffer = 0; // Atomic to avoid race conditions

static void loop(ptrdiff_t base_address = get_module_base_address("client.dll")) {
    static const uintptr_t entity_list = Read<uintptr_t>(base_address + client_dll::dwEntityList);
    static const int local_team = returnPlayerTeam(entity_list);

    while (running.load()) {
        int writeBufferIndex = currentBuffer.load() == 0 ? 1 : 0;
        PlayerInfo* writeBuffer = (writeBufferIndex == 0) ? playerBuffer1 : playerBuffer2;

        size_t playerCount = 0;

        for (int i = 0; i < 64 && playerCount < 64; i++) {
            uintptr_t listEntry = Read<uintptr_t>(entity_list + ((8 * (i & 0x7fff) >> 9) + 16));
            if (!listEntry) continue;

            uintptr_t entityController = Read<uintptr_t>(listEntry + 120 * (i & 0x1ff));
            if (!entityController) continue;

            uintptr_t entityControllerPawn = Read<uintptr_t>(entityController + CCSPlayerController::m_hPlayerPawn);
            if (!entityControllerPawn) continue;

            uintptr_t CSPlayerPawn = Read<uintptr_t>(listEntry + 120 * (entityControllerPawn & 0x1ff));
            if (!CSPlayerPawn) continue;

            int health = Read<int>(CSPlayerPawn + C_BaseEntity::m_iHealth);
            if (health < 1 || health > 100) continue;

            int team = Read<int>(CSPlayerPawn + C_BaseEntity::m_iTeamNum);
            if (team == local_team) continue;

            intptr_t weapon = Read<intptr_t>(CSPlayerPawn + C_CSPlayerPawnBase::m_pClippingWeapon);
            short weaponId = Read<short>(weapon + C_EconEntity::m_AttributeManager + C_AttributeContainer::m_Item + C_EconItemView::m_iItemDefinitionIndex);

            if (weaponId == -1) continue;

            vec3 absOrigin = Read<vec3>(CSPlayerPawn + C_BasePlayerPawn::m_vOldOrigin);
            vec3 viewOffset = Read<vec3>(CSPlayerPawn + C_BaseModelEntity::m_vecViewOffset);
            int armor = Read<int>(CSPlayerPawn + C_CSPlayerPawn::m_ArmorValue);
            bool helmet = Read<bool>(CSPlayerPawn + C_CSPlayerPawn::m_bPrevHelmet);
            bool vest = (helmet == 0 && armor != 0);

            std::string name = read_name(entityController);

            writeBuffer[playerCount++] = PlayerInfo{
                CSPlayerPawn,
                health,
                armor,
                helmet,
                vest,
                name,
                weaponId,
                absOrigin,
                absOrigin + viewOffset,
                absOrigin
            };
        }

        {
            std::lock_guard<std::mutex> lock(players_mutex);
            players.assign(writeBuffer, writeBuffer + playerCount);
            currentBuffer.store(writeBufferIndex);

        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

struct RenderInfo {
    vec2 head;
    vec2 feet;
    int health;
    int armor;
    bool helmet;
    bool vest;
    std::string name;
    short weaponId;
};

std::vector<RenderInfo> renderBuffer1;
std::vector<RenderInfo> renderBuffer2;
std::atomic<int> currentRenderBuffer = 0; // Atomic index to determine the active render buffer





std::string getWeaponName(int weaponID) {
    switch (weaponID) {
    case 1: return "DEAGLE";
    case 2: return "ELITE";
    case 3: return "FIVESEVEN";
    case 4: return "GLOCK";
    case 7: return "AK47";
    case 8: return "AUG";
    case 9: return "AWP";
    case 10: return "FAMAS";
    case 11: return "G3SG1";
    case 13: return "GALILAR";
    case 14: return "M249";
    case 16: return "M4A1";
    case 17: return "MAC10";
    case 19: return "P90";
    case 24: return "UMP";
    case 25: return "XM1014";
    case 26: return "BIZON";
    case 27: return "MAG7";
    case 28: return "NEGEV";
    case 29: return "SAWEDOFF";
    case 30: return "TEC9";
    case 31: return "TASER";
    case 32: return "HKP2000";
    case 33: return "MP7";
    case 34: return "MP9";
    case 35: return "NOVA";
    case 36: return "P250";
    case 38: return "SCAR20";
    case 39: return "SG556";
    case 40: return "SSG08";
    case 42: return "KNIFE";
    case 43: return "FLASHBANG";
    case 44: return "HEGRENADE";
    case 45: return "SMOKEGRENADE";
    case 46: return "MOLOTOV";
    case 47: return "DECOY";
    case 48: return "INCGRENADE";
    case 49: return "C4";
    case 59: return "KNIFE_T";
    case 60: return "M4A1_SILENCER";
    case 61: return "USP_SILENCER";
    case 63: return "CZ75A";
    case 64: return "REVOLVER";
    case 500: return "BAYONET";
    case 505: return "KNIFE_FLIP";
    case 506: return "KNIFE_GUT";
    case 507: return "KNIFE_KARAMBIT";
    case 508: return "KNIFE_M9_BAYONET";
    case 509: return "KNIFE_TACTICAL";
    case 512: return "KNIFE_FALCHION";
    case 514: return "KNIFE_SURVIVAL_BOWIE";
    case 515: return "KNIFE_BUTTERFLY";
    case 516: return "KNIFE_PUSH";
    case 526: return "KNIFE_KUKRI";
    default: return "UNKNOWN_WEAPON";
    }
}      // Debug prints
        /*
        std::cout << "Rendering player: " << renderInfo.name << "\n";
        std::cout << "Position: (" << x << ", " << y << ")\n";
        std::cout << "Dimensions: width=" << width << ", height=" << height << "\n";
        std::cout << "Health: " << renderInfo.health << "\n";
        std::cout << "Armor: " << renderInfo.armor << "\n";
        std::cout << "Helmet: " << (renderInfo.helmet ? "Yes" : "No") << "\n";
        std::cout << "Vest: " << (renderInfo.vest ? "Yes" : "No") << "\n";
        std::cout << "Weapon: " << getWeaponName(renderInfo.weaponId) << "\n";
        */

void ShowingPl(ptrdiff_t base_address = get_module_base_address("client.dll")) {
    view_matrix_t vm = Read<view_matrix_t>(base_address + client_dll::dwViewMatrix);

    // Determine the write buffer
    int writeBufferIndex = currentRenderBuffer.load() == 0 ? 1 : 0;
    std::vector<RenderInfo>& writeBuffer = (writeBufferIndex == 0) ? renderBuffer1 : renderBuffer2;

    writeBuffer.clear(); // Clear the buffer for new rendering data

    // Lock the players vector and prepare rendering instructions
    {
        std::lock_guard<std::mutex> lock(players_mutex);
        writeBuffer.reserve(players.size()); // Reserve memory to avoid multiple allocations
        for (const auto& player : players) {
            vec2 head, feet;

            if (w2s(player.absOrigin, head, vm.matrix) && w2s(player.eyePos, feet, vm.matrix)) {
                float height = feet.y - head.y;
                float width = height / 2.2f;

                // Add rendering information to the write buffer
                writeBuffer.emplace_back(RenderInfo{
                    head,
                    feet,
                    player.health,
                    player.armor,
                    player.helmet,
                    player.vest,
                    player.name,
                    player.weaponId
                    });
            }
        }
    }

    // Switch the active render buffer atomically
    currentRenderBuffer.store(writeBufferIndex);

    // Render using the other buffer
    int readBufferIndex = (writeBufferIndex == 0) ? 1 : 0;
    const std::vector<RenderInfo>& readBuffer = (readBufferIndex == 0) ? renderBuffer1 : renderBuffer2;

    for (const auto& renderInfo : readBuffer) {
        float height = renderInfo.feet.y - renderInfo.head.y;
        float width = height / 2.0f;
        int x = static_cast<int>(renderInfo.head.x);
        int y = static_cast<int>(renderInfo.head.y);

        // Draw bounding box
        draw_box(x + 40, y + 50, static_cast<int>(width) - 50, static_cast<int>(height) - 50, 50, 50, 50, 1);

        // Draw health
        CHAR HP[32];
        snprintf(HP, sizeof(HP), "HP: %d", renderInfo.health);
		if (isOptionEnabled("HP")){
			draw_text(x + static_cast<int>(width) - 10, y + 52, HP); // Offset health display
		}
       

        // Draw armor/helmet
        CHAR armor[32];
        if (renderInfo.helmet) {
            snprintf(armor, sizeof(armor), "FullArmor: %d", renderInfo.armor);
        }
        else {
            snprintf(armor, sizeof(armor), "Vest: %d", renderInfo.armor);
        }
		if (isOptionEnabled("Armor")) {
			draw_text(x + static_cast<int>(width) - 10, y + 35, armor); // Offset helmet display
		}

        // Draw name
        CHAR name[128];
        snprintf(name, sizeof(name), "%s", renderInfo.name.c_str());
		if (isOptionEnabled("Name")) {
			draw_text(x + static_cast<int>(width) - 13, y + static_cast<int>(height) - 18, name); // Offset name display
		}


        // Draw weapon
        std::string weaponName = getWeaponName(renderInfo.weaponId);
        CHAR weapon[128];
        snprintf(weapon, sizeof(weapon), "Weapon: %s", weaponName.c_str());
		if (isOptionEnabled("Weapon")) {
			draw_text(x + static_cast<int>(width) - 13, y + static_cast<int>(height) - 35, weapon); // Offset weapon display
		}
        
        readBuffer.empty(); // Clear the read buffer
        writeBuffer.clear(); // Clear the write buffer
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
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
    SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // Set a timer for periodic refresh (e.g., ~60 FPS with 16ms interval)
    SetTimer(g_hWnd, 1, 16, NULL);

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

        std::cout << "Getting module base address...\n";
        ptrdiff_t base_address = get_module_base_address(const_cast<char*>("client.dll"));
        std::cout << base_address << std::endl;
        std::cout << "Entering loop...\n";


        overlayThread.detach();
        run_loop_in_thread(base_address);

        while (true) {
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