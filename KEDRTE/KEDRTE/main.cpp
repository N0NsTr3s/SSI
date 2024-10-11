#include "hook.h"




extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING reg_path)
{
	_NULL_MEMORY instructions;
	instructions.pid = NULL;
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


	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(reg_path);

	DbgPrint("Driver entry initiated.\n");


	if (!(nullhook::call_kernel_function(&nullhook::hook_handler)))
	{
		DbgPrint("Failed to hook kernel function.\n");
		return STATUS_UNSUCCESSFUL;
	}
	else {
		DbgPrint("Kernel function hooked successfully.\n");
		
	}
	NTSTATUS result = nullhook::hook_handler(&instructions);

		// Check the result of the hook handler call
		if (result == STATUS_SUCCESS) {
			DbgPrint("hook_handler executed successfully.\n");
		}
		else {
			DbgPrint("hook_handler test failed with status: 0x%X\n", result);
		}
		for(int i=0; i<1000; i++)
			nullhook::hook_handler(&instructions);

	DbgPrint("Driver entry completed.\n");
	return STATUS_SUCCESS;
}
	
	
