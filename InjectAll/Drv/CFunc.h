// Helper functions
#pragma once

#include "DrvTypes.h"	// Custom types


#define GCPFN_BUFF_SIZE (sizeof(UNICODE_STRING) + (MAX_PATH + 1) * sizeof(WCHAR))

class CFunc
{
public:
	static BOOLEAN IsSuffixedUnicodeString(PCUNICODE_STRING FullName, PCUNICODE_STRING ShortName, BOOLEAN CaseInsensitive = TRUE);
	static BOOLEAN IsMappedByLdrLoadDll(PCUNICODE_STRING ShortName);
	static BOOLEAN IsSpecificProcessW(HANDLE ProcessId, const WCHAR* ImageName, BOOLEAN bIsDebugged);
};

