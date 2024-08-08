// Driver custom types and definitions
#pragma once

#include <ntifs.h>
#include <minwindef.h>
#include <ntimage.h>
#include <ntstrsafe.h>

#include "SharedDefs.h"

#define DBG_PREFIX DBG_PREFIX_ALL "Drv: " // Prefix to be added in all DbgPrint calls in this project
#define DbgPrintLine(s, ...) DbgPrint(DBG_PREFIX s "\n", __VA_ARGS__)

#define LIMIT_INJECTION_TO_PROC L"notepad.exe"   //Process to limit injection to (only in Debugger builds)

#ifdef DBG
#define _DEBUG DBG
#define VERIFY(f) ASSERT(f)
#else
#define VERIFY(f) ((void)(f))
#endif

enum IMAGE_LOAD_FLAGS
{
	flImageNotifySet, //[set] when PsSetLoadImageNotifyRoutine was enabled
};


#define echo(x) x
#define label(x) echo(x)##__LINE__
#define RTL_CONSTANT_STRINGW_(s) { sizeof( s ) - sizeof( (s)[0] ), sizeof( s ), const_cast<PWSTR>(s) }

#define STATIC_UNICODE_STRING(name, str) \
static const WCHAR label(__)[] = echo(L)str;\
static const UNICODE_STRING name = RTL_CONSTANT_STRINGW_(label(__))

#define STATIC_OBJECT_ATTRIBUTES(oa, name)\
	STATIC_UNICODE_STRING(label(m), name);\
	static OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, const_cast<PUNICODE_STRING>(&label(m)), OBJ_CASE_INSENSITIVE }

//Flips kernel memory allocation pool tag around (for debuggers)
#define TAG(t) ( ((((ULONG)t) & 0xFF) << (8 * 3)) | ((((ULONG)t) & 0xFF00) << (8 * 1)) | ((((ULONG)t) & 0xFF0000) >> (8 * 1)) | ((((ULONG)t) & 0xFF000000) >> (8 * 3)) )


enum SECTION_TYPE {
	SEC_TP_NATIVE = 'n',     //Native section - meaning: 64-bit on a 64-bit OS, or 32-bit on a 32-bit OS
	SEC_TP_WOW = 'w',        //WOW64 section - meaning: 32-bit on a 64-bit OS
};


extern "C" {
	__declspec(dllimport) BOOLEAN PsIsProcessBeingDebugged(PEPROCESS Process);
	__declspec(dllimport) NTSTATUS ZwQueryInformationProcess
	(
		IN HANDLE ProcessHandle,
		IN  PROCESSINFOCLASS ProcessInformationClass,
		OUT PVOID ProcessInformation,
		IN ULONG ProcessInformationLength,
		OUT PULONG ReturnLength OPTIONAL
	);
}