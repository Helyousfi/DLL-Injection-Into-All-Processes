//Class that deals with mapping of section (or DLL)

//  
//    Test solution that demonstrates DLL injection into all running processes
//    Copyright (c) 2021 www.dennisbabkin.com
//
//        https://dennisbabkin.com/blog/?i=AAA10800
//
//    Credit: Rbmm
//
//        https://github.com/rbmm/INJECT
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//    
//        https://www.apache.org/licenses/LICENSE-2.0
//    
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//  
//
#include "CSection.h"


NTSTATUS CSection::Initialize(SECTION_TYPE type)
{
	//Initialize this object
	//INFO: Cannot use constructor/destructor!
	//'type' = type of section (or DLL) that this section will represent
	sectionType = type;

	//Initialize our singleton
	RtlRunOnceInitialize(&SectionSingletonState);

	return STATUS_SUCCESS;
}


NTSTATUS CSection::GetSection(DLL_STATS** ppOutSectionInfo)
{
	//Get DLL section object
	//INFO: Will create it only once, if it wasn't created earlier
	//'ppOutSectionInfo' = if not NULL, receives the section object info, or NULL if error
	//RETURN:
	//		= Status of operation
	NTSTATUS status = STATUS_SUCCESS;

	//Make sure that CSection::Initialize was called!
	ASSERT(sectionType == SEC_TP_NATIVE || sectionType == SEC_TP_WOW);

	//Use the singleton approach
	PVOID Context = NULL;
	status = RtlRunOnceBeginInitialize(&SectionSingletonState, 0, &Context);
	if (status == STATUS_PENDING)
	{
		//We get here only during the first initialization
		Context = NULL;

		//Alloc memory
		DLL_STATS* pDStats = (DLL_STATS*)ExAllocatePoolWithTag(ALLOC_TYPE_OnLoadImage, sizeof(DLL_STATS), TAG('kDSm'));
		if (pDStats)
		{
			//Need to "trick" the system into creating a KnownDll section for us with the SD of the kernel32.dll section

			//Temporarily attach the current thread to the address space of the system process
			KAPC_STATE as;
			KeStackAttachProcess(PsInitialSystemProcess, &as);

			//Create our KnownDll section
			status = CreateKnownDllSection(*pDStats);

			//Revert back
			KeUnstackDetachProcess(&as);


			//Check the result
			if (NT_SUCCESS(status))
			{
				//We'll keep the section info in the context
				Context = pDStats;
			}
			else
			{
				//Error
				DbgPrintLine("ERROR: (0x%x) CreateKnownDllSection, sectionType=%c", status, sectionType);

				//Free memory
				ExFreePool(pDStats);
				pDStats = NULL;
			}

		}
		else
		{
			//Error
			status = STATUS_MEMORY_NOT_ALLOCATED;
			DbgPrintLine("ERROR: (0x%x) ExAllocatePoolWithTag(kDSm), sectionType=%c", status, sectionType);
		}



		//Finalize our singleton
		NTSTATUS status2 = RtlRunOnceComplete(&SectionSingletonState, 0, Context);
		if (!NT_SUCCESS(status2))
		{
			//Error
			DbgPrintLine("ERROR: (0x%x) RtlRunOnceComplete, sectionType=%c", status2, sectionType);
			ASSERT(NULL);

			if (NT_SUCCESS(status))
				status = status2;

			if (pDStats)
			{
				//Free memory
				ExFreePool(pDStats);
				pDStats = NULL;
			}

			Context = NULL;
		}
	}
	else if (status != STATUS_SUCCESS)
	{
		//Error
		DbgPrintLine("ERROR: (0x%x) RtlRunOnceBeginInitialize, sectionType=%c", status, sectionType);
		ASSERT(NULL);
	}


	//Did we get the pointer?
	if (!Context &&
		status == STATUS_SUCCESS)
	{
		//We previously failed to create section
		status = STATUS_NONEXISTENT_SECTOR;
	}

	if (ppOutSectionInfo)
		*ppOutSectionInfo = (DLL_STATS*)Context;

	return status;
}

