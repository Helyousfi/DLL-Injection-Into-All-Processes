//Custom types for our DLL
#pragma once

#include <Windows.h>                //Win32 APIs
#include <winnt.h>                  //Internal function
#include <winternl.h>               //Also internal stuff

#include <stddef.h>

#include "..\Drv\SharedDefs.h"      //Shared definitions




#define DBG_FILE_PATH  L"C:\\InjectAll\\Log_InjectAll.txt"          //Log file to write from the injected DLL


#define DBG_PREFIX DBG_PREFIX_ALL INJECTED_DLL_FILE_NAME ": "       //Prefix to be added in all DbgPrint call in this project

#define DbgPrintLine(s, ...) LogToFileFmt(DBG_PREFIX s "\r\n", __VA_ARGS__)




typedef short CSHORT;

typedef struct _TIME_FIELDS {
    CSHORT Year;        // range [1601...]
    CSHORT Month;       // range [1..12]
    CSHORT Day;         // range [1..31]
    CSHORT Hour;        // range [0..23]
    CSHORT Minute;      // range [0..59]
    CSHORT Second;      // range [0..59]
    CSHORT Milliseconds;// range [0..999]
    CSHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS* PTIME_FIELDS;


//Undocumented functions:
///////////////////////////////////////////////////////
extern "C" {
    __declspec(dllimport) NTSTATUS NTAPI RtlDosPathNameToNtPathName_U_WithStatus(
        IN PCWSTR  	DosName,
        OUT PUNICODE_STRING  NtName,
        OUT PWSTR* PartName,
        OUT PVOID  	RelativeName
    );

    __declspec(dllimport) NTSTATUS NTAPI NtWriteFile(
        _In_ HANDLE FileHandle,
        _In_opt_ HANDLE Event,
        _In_opt_ PIO_APC_ROUTINE ApcRoutine,
        _In_opt_ PVOID ApcContext,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _In_reads_bytes_(Length) PVOID Buffer,
        _In_ ULONG Length,
        _In_opt_ PLARGE_INTEGER ByteOffset,
        _In_opt_ PULONG Key
    );

    __declspec(dllimport) int __CRTDECL vsprintf_s(
        _Out_writes_(_BufferCount) _Always_(_Post_z_) char* const _Buffer,
        _In_                                          size_t      const _BufferCount,
        _In_z_ _Printf_format_string_                 char const* const _Format,
        va_list           _ArgList
    );

    __declspec(dllimport) void NTAPI RtlSystemTimeToLocalTime(
        PLARGE_INTEGER SystemTime,
        PLARGE_INTEGER LocalTime
    );

    __declspec(dllimport) VOID NTAPI RtlTimeToTimeFields(
        _In_ PLARGE_INTEGER Time,
        _Out_ PTIME_FIELDS TimeFields
    );

    __declspec(dllimport) NTSTATUS NTAPI ZwQueueApcThread(HANDLE hThread,
        void* ApcRoutine, PVOID ApcContext, PVOID Argument1, PVOID Argument2);
};
///////////////////////////////////////////////////////




#pragma pack(push)
#pragma pack(1)                 //Align by 1 byte
#pragma warning(push)
#pragma warning(disable : 4200)
struct SEARCH_TAG_W {
    const GUID tag;
    WCHAR s[];
};
#pragma warning(pop)
#pragma pack(pop)





