#include "hook.h"




extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING reg_path)
{
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(reg_path);

	DbgPrint("Driver entry initiated.\n");

	nullhook::call_kernel_function(&nullhook::hook_handler);

	DbgPrint("Driver entry completed.\n");
	return STATUS_SUCCESS;
}
	
	
