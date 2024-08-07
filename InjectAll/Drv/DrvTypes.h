// Driver custom types and definitions
#pragma once

#include <ntifs.h>
#include <minwindef.h>
#include <ntimage.h>
#include <ntstrsafe.h>

#include "SharedDefs.h"

#define DBG_PREFIX DBG_PREFIX_ALL "Drv: " // Prefix to be added in all DbgPrint calls in this project
#define DbgPrintLine(s, ...) DbgPrint(DBG_PREFIX s "\n", __VA_ARGS__)


enum IMAGE_LOAD_FLAGS
{
	flImageNotifySet, //[set] when PsSetLoadImageNotifyRoutine was enabled

};