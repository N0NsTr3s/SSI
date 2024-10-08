#include "hook.h"

extern "C"
{
    DRIVER_INITIALIZE DriverEntry;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING reg_path)
{
    UNREFERENCED_PARAMETER(driver_object);
    UNREFERENCED_PARAMETER(reg_path);

    DbgPrint("Driver entry initiated.\n");

    if (!call_kernel_function(&hook_handler)) {
        DbgPrint("call_kernel_function failed.\n");
        return STATUS_UNSUCCESSFUL;
    }

    DbgPrint("Driver entry successful.\n");
    return STATUS_SUCCESS;
}
