#include "hook.h"

// Driver entry point
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING reg_path) {
    // Initialize the instructions structure
    _NULL_MEMORY instructions;
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
    
	------------Test Base Address*/
    instructions.pid = 18384;
    instructions.read = FALSE;
    instructions.write = FALSE;
    instructions.req_base = TRUE;
    instructions.draw_box = FALSE;
    instructions.module_name = "client.dll"; 

    UNREFERENCED_PARAMETER(driver_object);
    UNREFERENCED_PARAMETER(reg_path);

    DbgPrint("Driver entry initiated.\n");

    
	//* ----------------------Test Functions---------------------- *
	
    // Hook kernel function
    if (!nullhook::call_kernel_function(&nullhook::hook_handler)) {
        DbgPrint("Failed to hook kernel function.\n");
        return STATUS_UNSUCCESSFUL;
    }
    else {
        DbgPrint("Kernel function hooked successfully.\n");
    }

    // Call the hook handler for testing
    NTSTATUS result = nullhook::hook_handler(&instructions);

    // Check the result of the hook handler call
    if (result == STATUS_SUCCESS) {
        DbgPrint("hook_handler executed successfully.\n");
    }
    else {
        DbgPrint("hook_handler test failed with status: 0x%X\n", result);
    }

    
    
	

    DbgPrint("Driver entry completed.\n");
    return STATUS_SUCCESS;
}