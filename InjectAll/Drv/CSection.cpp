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
#include "CFunc.h"			//Aux functions

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
		DLL_STATS* pDStats = (DLL_STATS*)ExAllocatePool2(ALLOC_TYPE_OnLoadImage, sizeof(DLL_STATS), TAG('kDSm'));
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


NTSTATUS CSection::FreeSection()
{
	//Release resources held for the mapped section
	//INFO: Doesn't do anything if GetSection() wasn't called yet
	//RETURN:
	//		= Status of the operations
	NTSTATUS status;

	PVOID Context = NULL;
	status = RtlRunOnceBeginInitialize(&SectionSingletonState, 0, &Context);
	if (NT_SUCCESS(status))
	{
		//We have initialized our singleton

		//Do we have the context - otherwise there's nothing to delete
		if (Context)
		{
			DLL_STATS* pDStats = (DLL_STATS*)Context;

#ifdef DBG_VERBOSE_DRV
			DbgPrintLine("FreeSection, sectionType=%c", sectionType);
#endif

			//Remove permanent flag from the section object
			ObMakeTemporaryObject(pDStats->Section);

			//And derefence it
			ObDereferenceObjectWithTag(pDStats->Section, TAG('hFkS'));
			pDStats->Section = NULL;


			//Free memory
			ExFreePool(Context);
			Context = NULL;
		}

		//Reset the singleton back
		RtlRunOnceInitialize(&SectionSingletonState);
	}
	else if (status == STATUS_UNSUCCESSFUL)
	{
		//GetSection() wasn't called yet
		status = STATUS_SUCCESS;
	}
	else
	{
		//Error
		DbgPrintLine("ERROR: (0x%x) FreeSection, sectionType=%c", status, sectionType);
		ASSERT(NULL);
	}

	return status;
}


NTSTATUS CSection::CreateKnownDllSection(DLL_STATS& outStats)
{
	//Create a known-DLL system section of our own
	//'outStatus' = receives information on created known section (only if return success)
	//RETURN:
	//		= Status of the operations
	NTSTATUS status;

	//Clear the returned data (assuming only primitive data types)
	memset(&outStats, 0, sizeof(outStats));


	POBJECT_ATTRIBUTES poaKernel32;
	PCUNICODE_STRING pstrFakeDll;
	PCOBJECT_ATTRIBUTES poaPathFakeDll;

#ifdef _WIN64
	if (sectionType == SEC_TP_WOW)
	{
		//32-bit section loaded on a 64-bit OS
		STATIC_OBJECT_ATTRIBUTES(oaKernel32, "\\KnownDlls32\\kernel32.dll");
		STATIC_UNICODE_STRING(strFakeDll, "\\KnownDlls32\\" INJECTED_DLL_FILE_NAME32);
		STATIC_OBJECT_ATTRIBUTES(oaPathFakeDll, INJECTED_DLL_NT_PATH_WOW);

		poaKernel32 = &oaKernel32;
		pstrFakeDll = &strFakeDll;
		poaPathFakeDll = &oaPathFakeDll;
	}
	else
#endif
	{
		//64-bit section loaded on a 64-bit OS, or
		//32-bit section loaded on a 32-bit OS
		STATIC_OBJECT_ATTRIBUTES(oaKernel32, "\\KnownDlls\\kernel32.dll");
		STATIC_UNICODE_STRING(strFakeDll, "\\KnownDlls\\" INJECTED_DLL_FILE_NAME);
		STATIC_OBJECT_ATTRIBUTES(oaPathFakeDll, INJECTED_DLL_NT_PATH_NTV);

		poaKernel32 = &oaKernel32;
		pstrFakeDll = &strFakeDll;
		poaPathFakeDll = &oaPathFakeDll;
	}


	//Need to "steal" a security descriptor from existing KnownDll - we'll use kernel32.dll
	HANDLE hSectionK32;
	status = ZwOpenSection(&hSectionK32, READ_CONTROL, const_cast<POBJECT_ATTRIBUTES>(poaKernel32));
	if (NT_SUCCESS(status))
	{
		status = STATUS_GENERIC_COMMAND_FAILED;

		//INFO: Make our section "permanent", which means that it won't be deleted if all of its handles are closed
		//      and we will need to call ZwMakeTemporaryObject() on it first to allow it
		OBJECT_ATTRIBUTES oaFakeDll = { sizeof(oaFakeDll), 0,
			const_cast<PUNICODE_STRING>(pstrFakeDll),
			OBJ_CASE_INSENSITIVE | OBJ_PERMANENT };


		//Allocate needed memory
		ULONG uicbMemSz = 0;

		for (;;)
		{
			ULONG uicbMemNeededSz = 0;

			status = ZwQuerySecurityObject(hSectionK32,
				PROCESS_TRUST_LABEL_SECURITY_INFORMATION |
				DACL_SECURITY_INFORMATION |
				LABEL_SECURITY_INFORMATION |
				OWNER_SECURITY_INFORMATION,
				oaFakeDll.SecurityDescriptor,		//SD
				uicbMemSz,							//mem size
				&uicbMemNeededSz);

			if (NT_SUCCESS(status))
			{
				//Got it
				break;
			}
			else if (status == STATUS_BUFFER_TOO_SMALL)
			{
				//Need more memory
				ASSERT(uicbMemNeededSz > uicbMemSz);

				if (oaFakeDll.SecurityDescriptor)
				{
					//Free previous memory
					ExFreePool(oaFakeDll.SecurityDescriptor);
				}

				//Allocate mem
				oaFakeDll.SecurityDescriptor = ExAllocatePool2(ALLOC_TYPE_OnLoadImage, uicbMemNeededSz, TAG('k32m'));
				if (oaFakeDll.SecurityDescriptor)
				{
					//Need to retry
					uicbMemSz = uicbMemNeededSz;
				}
				else
				{
					//Error
					status = STATUS_MEMORY_NOT_ALLOCATED;
					DbgPrintLine("ERROR: (0x%X) ExAllocatePoolWithTag(hSectionK32), PID=%u, sz=%u, sectionType=%c"
						,
						status,
						(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
						uicbMemNeededSz,
						sectionType
					);

					break;
				}
			}
			else
			{
				//Error
				DbgPrintLine("ERROR: (0x%X) ZwQuerySecurityObject(hSectionK32), PID=%u, sz=%u, sectionType=%c"
					,
					status,
					(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
					uicbMemSz,
					sectionType
				);

				break;
			}
		}


		//Close section
		VERIFY(NT_SUCCESS(ZwClose(hSectionK32)));

		if (NT_SUCCESS(status))
		{
			//Now we can create our own section for our injected DLL in the KnownDlls kernel object directory

			HANDLE hFile;
			IO_STATUS_BLOCK iosb;

			//Open existing DLL that we will be injecting
			status = ZwOpenFile(&hFile, FILE_GENERIC_READ | FILE_EXECUTE,
				const_cast<POBJECT_ATTRIBUTES>(poaPathFakeDll), &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);

			if (NT_SUCCESS(status))
			{
				//Open PE file as section
				HANDLE hFakeSection;
				status = ZwCreateSection(&hFakeSection, SECTION_MAP_EXECUTE | SECTION_QUERY,
					&oaFakeDll, 0, PAGE_EXECUTE, SEC_IMAGE, hFile);

				if (NT_SUCCESS(status))
				{
					//Map it into our process
					//INFO: We need two things:
					//		1. Get the offset of our shellcode - or UserModeNormalRoutine function
					//		2. Verify that this is our DLL and get its name from SEARCH_TAG_W
					PVOID BaseAddress = NULL;
					SIZE_T ViewSize = 0;
					status = ZwMapViewOfSection(hFakeSection, NtCurrentProcess(), &BaseAddress, 0, 0, 0,
						&ViewSize, ViewUnmap, 0, PAGE_READONLY);
					if (NT_SUCCESS(status))
					{
						//Need to look up our ordinal function, it will be our ShellCode that we will call later
						//INFO: It is located at the ordinal number 1 (it is defined in the .def file for that DLL exports)
						ASSERT(BaseAddress);
						ULONG OrdinalIndex = 1;
						ULONG uRVA = 0;

						__try
						{
							status = STATUS_INVALID_IMAGE_FORMAT;

							PIMAGE_NT_HEADERS pNtHdr = RtlImageNtHeader(BaseAddress);
							if (pNtHdr)
							{
								ULONG size;
								PIMAGE_EXPORT_DIRECTORY pExpDir = (PIMAGE_EXPORT_DIRECTORY)
									RtlImageDirectoryEntryToData(BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);

								if (pExpDir &&
									size >= sizeof(IMAGE_EXPORT_DIRECTORY))
								{
									OrdinalIndex -= pExpDir->Base;
									if (OrdinalIndex < pExpDir->NumberOfFunctions)
									{
										PULONG pAddressOfFunctions = (PULONG)((BYTE*)BaseAddress + pExpDir->AddressOfFunctions);

										if (pAddressOfFunctions)
										{
											//Get our needed SHellCode function's RVA
											uRVA = pAddressOfFunctions[OrdinalIndex];

											if (uRVA > 0)		//Our DLL is small - do this quick check
											{

												//Locate the offset of the search tag
												//INFO: It will give use the injected DLL name (it will be used for Shell Code later)
												static GUID guiSrch = { GUID_SearchTag_DllName_Bin };
												UINT uRVA_StchTag = CFunc::FindStringByTag(BaseAddress,
													pNtHdr->OptionalHeader.SizeOfImage,
													&guiSrch);
												if (uRVA_StchTag != -1)
												{
													//Get information from our section
													SECTION_IMAGE_INFORMATION sii;
													status = ZwQuerySection(hFakeSection, SectionImageInformation,
														&sii, sizeof(sii), 0);
													if (NT_SUCCESS(status))
													{


														//Get our section object pointer & increment its reference count
														status = ObReferenceObjectByHandleWithTag(hFakeSection, 0, NULL,
															KernelMode, TAG('hFkS'),
															&outStats.Section, NULL);

														if (NT_SUCCESS(status))
														{
															//Set return parameters
															outStats.secType = sectionType;
															outStats.SizeOfImage = pNtHdr->OptionalHeader.SizeOfImage;
															outStats.uRVA_ShellCode = uRVA;
															outStats.uRVA_DllName = uRVA_StchTag;

															//SECTION_IMAGE_INFORMATION::TransferAddress = address of entry point
															//       in the module after it was randomly relocated by ASLR
															outStats.PreferredAddress = (BYTE*)sii.TransferAddress -
																pNtHdr->OptionalHeader.AddressOfEntryPoint;

#ifdef  DBG_VERBOSE_DRV
															DbgPrintLine(
																"KnownDll Created! PID=%u, RVA=0x%X, PreferredAddress=0x%p, sectionType=%c"
																,
																(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
																uRVA,
																outStats.PreferredAddress,
																sectionType
															);
#endif

															//Done
															status = STATUS_SUCCESS;
														}
														else
														{
															//Error
															DbgPrintLine("ERROR: (0x%X) ObReferenceObjectByHandle, PID=%u, sectionType=%c"
																,
																status,
																(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
																sectionType
															);
														}
													}
													else
													{
														//Error
														DbgPrintLine("ERROR: (0x%X) ZwQuerySection, PID=%u, sectionType=%c"
															,
															status,
															(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
															sectionType
														);
													}
												}
												else
												{
													//Error
													DbgPrintLine(
														"ERROR: (0x%X) FindStringByTag, PID=%u, base=0x%p, sz=%u, sectionType=%c"
														,
														status,
														(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
														BaseAddress,
														pNtHdr->OptionalHeader.SizeOfImage,
														sectionType
													);
												}
											}
											else
											{
												//Error
												DbgPrintLine("ERROR: (0x%X) Bad RVA=%d, PID=%u, sectionType=%c"
													,
													status,
													uRVA,
													(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
													sectionType
												);
											}
										}
										else
										{
											//Error
											DbgPrintLine("ERROR: (0x%X) Bad AddressOfFunctions, PID=%u, sectionType=%c"
												,
												status,
												(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
												sectionType
											);
										}
									}
									else
									{
										//Error
										DbgPrintLine("ERROR: (0x%X) Bad ordinal, PID=%u cnt=%u, sectionType=%c"
											,
											status,
											(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
											pExpDir->NumberOfFunctions,
											sectionType
										);
									}
								}
								else
								{
									//Error
									DbgPrintLine("ERROR: (0x%X) Export directory, PID=%u size=%u, sectionType=%c"
										,
										status,
										(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
										size,
										sectionType
									);
								}
							}
							else
							{
								//Error
								DbgPrintLine("ERROR: (0x%X) RtlImageNtHeader, PID=%u, sectionType=%c"
									,
									status,
									(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
									sectionType
								);
							}
						}
						__except (EXCEPTION_EXECUTE_HANDLER)
						{
							//Failed to parse PE file
							DbgPrintLine("#EXCEPTION: (0x%X) CreateKnownDllSection(PE-scan), PID=%u, sectionType=%c"
								,
								GetExceptionCode(),
								(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
								sectionType
							);

							status = STATUS_INVALID_IMAGE_FORMAT;
						}


						//Unmap the section
						VERIFY(NT_SUCCESS(ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress)));
					}
					else
					{
						//Error
						DbgPrintLine("ERROR: (0x%X) ZwMapViewOfSection, PID=%u, sectionType=%c"
							,
							status,
							(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
							sectionType
						);
					}




					//Did we fail setting the section up?
					if (!NT_SUCCESS(status))
					{
						//Make it not permanent (so that we can remove)
						VERIFY(NT_SUCCESS(ZwMakeTemporaryObject(hFakeSection)));
					}

					//Close our section
					VERIFY(NT_SUCCESS(ZwClose(hFakeSection)));
				}
				else
				{
					//Error
					DbgPrintLine("ERROR: (0x%X) ZwCreateSection, PID=%u, sectionType=%c"
						,
						status,
						(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
						sectionType
					);
				}


				//Close file
				VERIFY(NT_SUCCESS(ZwClose(hFile)));
			}
			else
			{
				//Error
				DbgPrintLine("ERROR: (0x%X) ZwOpenFile, PID=%u, sectionType=%c, path=\"%wZ\""
					,
					status,
					(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
					sectionType,
					poaPathFakeDll->ObjectName
				);
			}
		}


		//Free our memory
		if (oaFakeDll.SecurityDescriptor)
		{
			ExFreePool(oaFakeDll.SecurityDescriptor);
			oaFakeDll.SecurityDescriptor = NULL;
		}

	}
	else
	{
		//Error
		DbgPrintLine("ERROR: (0x%X) ZwOpenSection(hSectionK32), PID=%u, sectionType=%c"
			,
			status,
			(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
			sectionType
		);
	}


	return status;
}