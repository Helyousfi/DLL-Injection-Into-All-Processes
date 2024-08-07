// Main Driver entry cpp file

#include "CFunc.h"

// Global variables

extern "C" {
	PDRIVER_OBJECT g_DriveObject; // Driver Object - read Only (for reference counting)
}

IMAGE_LOAD_FLAGS g_flags; // Global notification flags

void OnLoadImage(
	PUNICODE_STRING FullImageName,
	HANDLE ProcessId,
	PIMAGE_INFO ImageInfo
) 
{
	// Called back notification that an image is loaded (or mapped in memory)
	// ProcessId = process where the image is mapped into (or 0 for a driver)
	UNREFERENCED_PARAMETER(FullImageName);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ImageInfo);

}

NTSTATUS FreeResources()
{
	// Free our resources (must be called before unloading the driver)
	NTSTATUS status = STATUS_SUCCESS;

	// Remove notification callback 
	if (_bittestandset((LONG*)&g_flags, flImageNotifySet))
	{
		status = PsRemoveLoadImageNotifyRoutine(OnLoadImage);
		if (!NT_SUCCESS(status))
		{
			DbgPrintLine("CRITICAL: (0x%X) PsSetLoadImageNotifyRoutine", status);
		}
	}
	return status;
}


void NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
	// Routine that is called when driver is unloaded
	NTSTATUS status = FreeResources();
	DbgPrintLine("DiverUnload(0x%p), status=0x%x", DriverObject, status);
}


extern "C" NTSTATUS NTAPI DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	// Main Driver entry routine
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	g_DriveObject = DriverObject;

	// Debugging output function
	DbgPrintLine("DiverLoad(0x%p, %wZ)", DriverObject, RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	// Set Image loading notification routine
	NTSTATUS status = PsSetLoadImageNotifyRoutine(OnLoadImage);
	if (NT_SUCCESS(status))
	{
		_bittestandset((LONG*) & g_flags, flImageNotifySet);
	}
	else
	{
		// ERROR
		DbgPrintLine("CRITICAL : (0x%X) PsSetLoadImageNotifyRoutine", status);
	}



	return 0;
}