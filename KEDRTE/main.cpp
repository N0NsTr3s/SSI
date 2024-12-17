// Include the necessary header file for hooking functionality
#include "hook.h"


/*
A driver is a specialized software component that allows the operating system and other software 
applications to interact with hardware devices. Drivers act as a bridge between the hardware and the
operating system, translating high-level commands from the operating system into low-level
commands that the hardware can understand.
*/


// Driver entry point
// This function is the entry point for the driver. It is called when the driver is loaded.
// Parameters:
// - driver_object: A pointer to the driver object created by the I/O manager.
// - reg_path: A pointer to a Unicode string that specifies the path to the driver's registry key.
// Returns:
// - STATUS_SUCCESS if the driver initializes successfully, otherwise an appropriate error code.

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING reg_path) {
	DbgPrint("Start Driver.\n");
    
    
    // Initialize the instructions structure
    //NULL_MEMORY instructions;
    /*instructions.pid = NULL;
    -----------Test Box
    instructions.read = FALSE;
    instructions.write = FALSE;
    instructions.req_base = FALSE;
    instructions.draw_box = TRUE;
    instructions.x = 100;
    instructions.y = 100;
    instructions.w = 100;
    instructions.h = 100;
    instructions.r = 255;
    instructions.g = 0;
    instructions.b = 0;
    instructions.t = 5;
    
	------------Test Something -------------- 
    //instructions.pid = 15888;
    instructions.read = FALSE;
    instructions.write = FALSE;
    instructions.req_base = FALSE;
    instructions.draw_box = FALSE;
    //instructions.module_name = "client.dll"; 
	instructions.draw_text = TRUE;
	instructions.x = 100;
	instructions.y = 100;
	instructions.text = L"Hello World!"; //--> this works just with wchar_t* or LPWSTR
  */
	// Print the driver entry message
    
    UNREFERENCED_PARAMETER(driver_object);
    UNREFERENCED_PARAMETER(reg_path);

    DbgPrint("Driver entry initiated.\n");

    
	//* ----------------------Test Functions---------------------- *
	
    // Hook kernel function
    // This function attempts to hook a kernel function using the nullhook library.
    // If the hooking fails, it prints an error message and returns STATUS_UNSUCCESSFUL.
    // If the hooking succeeds, it prints a success message.
    if (!nullhook::call_kernel_function(&nullhook::hook_handler)) {
        DbgPrint("Failed to hook kernel function.\n");
        return STATUS_UNSUCCESSFUL;
    }
    else {
        DbgPrint("Kernel function hooked successfully.\n");
    }
    /*
    // Call the hook handler for testing
    for (int i = 0; i <= 100; i++) {
        NTSTATUS result = nullhook::hook_handler(&instructions);
  
    // Check the result of the hook handler call
    if (result == STATUS_SUCCESS) {
        DbgPrint("hook_handler executed successfully.\n");
    }
    else {
        DbgPrint("hook_handler test failed with status: 0x%X\n", result);
    }
    }
    
    */
	

    DbgPrint("Driver entry completed.\n");
    return STATUS_SUCCESS;
}