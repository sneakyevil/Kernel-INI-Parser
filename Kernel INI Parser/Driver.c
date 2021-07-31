#include "Driver.h"
#define PRINT_PREFIX "[Kernel INI Parser]"

NTSTATUS Thread()
{
	DbgPrint("%s Thread", PRINT_PREFIX);

	SINIParser* pINI = INIParser("C:\\Driver\\test.ini");
	if (pINI)
	{
		DbgPrint("%s (bool) - %i", PRINT_PREFIX, INIParser_GetBool(pINI, "bool", FALSE));
		DbgPrint("%s (int) - %i", PRINT_PREFIX, INIParser_GetInt(pINI, "int", -1));
		DbgPrint("%s (int_hex) - %i", PRINT_PREFIX, INIParser_GetInt(pINI, "int_hex", -1));
		DbgPrint("%s (hex) - %x", PRINT_PREFIX, INIParser_GetUnsignedInt(pINI, "hex", 0U));

		INIParser_Free(pINI);
	}

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	UNREFERENCED_PARAMETER(pDriverObject);
	UNREFERENCED_PARAMETER(pRegistryPath);

	DbgPrint("%s DriverEntry", PRINT_PREFIX);

	pDriverObject->DriverUnload = DriverUnload;

	// Create Thread
	HANDLE hThread;
	CLIENT_ID cThread;
	if (NT_SUCCESS(PsCreateSystemThread(&hThread, 0x1F0000u, NULL, NULL, &cThread, (PKSTART_ROUTINE)Thread, NULL))) ZwClose(hThread);

	return STATUS_SUCCESS;
}

NTSTATUS DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNREFERENCED_PARAMETER(pDriverObject);

	DbgPrint("%s DriverUnload", PRINT_PREFIX);

	return STATUS_SUCCESS;
}