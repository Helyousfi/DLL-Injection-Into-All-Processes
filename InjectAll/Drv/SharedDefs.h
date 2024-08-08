// Definitiions that are shared across projects in this solution
#pragma once

#define DBG_PREFIX_ALL "InjectAll_"		// Prefix to be added in all DBGPrint


#define DBG_PREFIX_ALL "InjectAll_"              //prefix to be added in all DbgPrint calls

#define INJECTED_DLL_FILE_NAME64 "FAKE64.DLL"    //File name of the injected 64-bit DLL (name only!)
#define INJECTED_DLL_FILE_NAME32 "FAKE32.DLL"    //File name of the injected 32-bit DLL (name only!)

#ifdef _WIN64
#define INJECTED_DLL_FILE_NAME INJECTED_DLL_FILE_NAME64
#else
#define INJECTED_DLL_FILE_NAME INJECTED_DLL_FILE_NAME32
#endif


// {9C74596E-7279-4FD9-9B8D-2CA5C7F9FDBE}
#define GUID_SearchTag_DllName_Bin 0x9C74596E, 0x7279, 0x4FD9, 0x9B, 0x8D, 0x2C, 0xA5, 0xC7, 0xF9, 0xFD, 0xBE


