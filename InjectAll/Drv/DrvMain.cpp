// Main Driver entry cpp file

#include "CFunc.h"

void NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
	// Routine that is called when driver is unloaded
	NTSTATUS status = 0;
	DbgPrintLine("DiverUnload(0x%p), status=0x%x", DriverObject, status);
}

extern "C" NTSTATUS NTAPI DriverEntry(
	PDRIVER_OBJECT  DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	// Main Driver entry routine
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	// Debugging output function
	DbgPrintLine("DiverLoad(0x%p, %wZ)", DriverObject, RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	return 0;
}