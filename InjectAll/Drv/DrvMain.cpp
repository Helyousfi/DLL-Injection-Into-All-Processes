// Main Driver entry cpp file

#include "CFunc.h"


extern "C" NTSTATUS NTAPI DriverEntry(
	PDRIVER_OBJECT  DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	// Main Driver entry routine
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	return 0;
}