#pragma once
#include <ntifs.h>
#include <ntstrsafe.h>
#include <stdlib.h>
#include <wdf.h>
int _fltused = 0; // unresolved external symbol...
#define INIParser_DBGInfo 1
#define INIParser_KeyLength 64

typedef struct _INIParser
{
	UNICODE_STRING uName;
	HANDLE hFile;
	char* pData;
	ULONG uSize;
} SINIParser;

SINIParser* INIParser_New()
{
	SINIParser* pNew = (SINIParser*)(ExAllocatePool(PagedPool, sizeof(SINIParser)));
	if (pNew)
	{
		pNew->hFile = NULL;
		pNew->pData = NULL;
	}
	
	return pNew;
}

void INIParser_Free(SINIParser* pINI)
{
	if (!pINI) return;

	// Close File
	if (pINI->hFile) ZwClose(pINI->hFile);

	// Free Data
	if (pINI->pData) ExFreePool(pINI->pData);

	ExFreePool(pINI);
}

SINIParser* INIParser(const char* pPath)
{
	NTSTATUS nReturn = 0L;

	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
	{
		DbgPrint("INIParser - IRQL is not PASSIVE!");
		return NULL;
	}

	SINIParser* pINI = INIParser_New();
	if (!pINI) return NULL;

	// Object Name Insert
	char cPath[_MAX_PATH];
	sprintf_s(cPath, sizeof(cPath), "\\??\\%s", pPath);

	// Convert ASCII to Unicode
	ANSI_STRING aName;
	RtlInitAnsiString(&aName, cPath);
	RtlAnsiStringToUnicodeString(&pINI->uName, &aName, TRUE);

#ifdef INIParser_DBGInfo
	DbgPrint("INIParser - %ws", pINI->uName.Buffer);
#endif

	OBJECT_ATTRIBUTES oAttributes;
	InitializeObjectAttributes(&oAttributes, &pINI->uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	IO_STATUS_BLOCK IOStatusBlock;
	nReturn = ZwOpenFile(&pINI->hFile, GENERIC_READ | SYNCHRONIZE, &oAttributes, &IOStatusBlock, 0, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (nReturn != STATUS_SUCCESS)
	{
#ifdef INIParser_DBGInfo
		DbgPrint("%ws - ZwOpenFile Error: 0x%X", pINI->uName.Buffer, nReturn);
#endif
		INIParser_Free(pINI);
		return NULL;
	}

	FILE_STANDARD_INFORMATION fInfo = { 0 };
	nReturn = ZwQueryInformationFile(pINI->hFile, &IOStatusBlock, &fInfo, sizeof(IO_STATUS_BLOCK), FileStandardInformation);
	if (nReturn != STATUS_SUCCESS)
	{
#ifdef INIParser_DBGInfo
		DbgPrint("%ws - ZwQueryInformationFile Error: 0x%X", pINI->uName.Buffer, nReturn);
#endif
		INIParser_Free(pINI);
		return NULL;
	}

	// Allocate Data
	pINI->uSize = (ULONG)(fInfo.EndOfFile.QuadPart);
	pINI->pData = (char*)(ExAllocatePool(PagedPool, (SIZE_T)(pINI->uSize + 1UL)));
	if (!pINI->pData)
	{
#ifdef INIParser_DBGInfo
		DbgPrint("%ws - Data ExAllocatePool Failed!", pINI->uName.Buffer);
#endif
		INIParser_Free(pINI);
		return NULL;
	}

	LARGE_INTEGER lOffset = { 0 };
	nReturn = ZwReadFile(pINI->hFile, NULL, NULL, NULL, &IOStatusBlock, pINI->pData, pINI->uSize, &lOffset, NULL);
	if (nReturn != STATUS_SUCCESS)
	{
#ifdef INIParser_DBGInfo
		DbgPrint("%ws - ZwReadFile Error: 0x%X", pINI->uName.Buffer, nReturn);
#endif
		INIParser_Free(pINI);
		return NULL;
	}
	ZwClose(pINI->hFile);
	pINI->hFile = NULL;

	pINI->pData[pINI->uSize] = '\0';
	
	// Replace \n with \0
	char* pFind = pINI->pData;
	while (1)
	{
		pFind = strchr(pFind, '\n');
		if (!pFind) break;

		pINI->pData[(unsigned long)(pFind - pINI->pData)] = '\0';
		pFind++;
	}
	
	return pINI;
}

unsigned long INIParser_FindKey(SINIParser* pINI, const char* pKey)
{
	char cKey[INIParser_KeyLength];
	sprintf_s(cKey, INIParser_KeyLength, "%s=", pKey);
	size_t sKeyLength = strlen(cKey);

	char* pFind = pINI->pData;
	while (1)
	{
		pFind = strchr(pFind, '\0');
		if (!pFind) break;

		pFind++;
		unsigned long uPos = (unsigned long)(pFind - pINI->pData);

		// Compare all chars
		BOOLEAN bAllMatch = TRUE;
		for (size_t i = 0; sKeyLength > i; ++i)
		{
			unsigned long uPosCheck = uPos + (unsigned long)(i);
			if (pINI->pData[uPosCheck] == cKey[i]) continue;

			bAllMatch = FALSE;
			break;
		}
		if (bAllMatch)
		{
			uPos += (unsigned long)(sKeyLength);
			return uPos;
		}

		if (uPos >= pINI->uSize) break;
	}

	return 0UL;
}

BOOLEAN INIParser_GetBool(SINIParser* pINI, const char* pKey, BOOLEAN bDefaultValue)
{
	unsigned long uPos = INIParser_FindKey(pINI, pKey);
	if (!uPos) return bDefaultValue;

	char cValue = pINI->pData[uPos];
	switch (cValue)
	{
		case '1': case 't': case 'T': return TRUE;
		case '0': case 'f': case 'F': return FALSE;
		default: return bDefaultValue;
	}
}

unsigned int INIParser_HexStringConvert(char* pHex)
{
	unsigned int uReturn = 0U;

	while (1)
	{
		char cByte = *pHex;
		if (cByte == '\0') break;

		if (cByte >= '0' && cByte <= '9') cByte = cByte - '0';
		else if (cByte >= 'a' && cByte <= 'f') cByte = cByte - 'a' + 10;
		else if (cByte >= 'A' && cByte <= 'F') cByte = cByte - 'A' + 10;
		else
		{
			pHex++;
			continue;
		}

		uReturn = (uReturn << 4) | (cByte & 0xF);
		pHex++;
	}

	return uReturn;
}

int INIParser_GetInt(SINIParser* pINI, const char* pKey, int iDefaultValue)
{
	unsigned long uPos = INIParser_FindKey(pINI, pKey);
	if (!uPos) return iDefaultValue;

	char* pValue = &pINI->pData[uPos];
	if (pValue[1] == 'x') return (int)(INIParser_HexStringConvert(&pValue[2]));

	return atoi(pValue);
}

unsigned int INIParser_GetUnsignedInt(SINIParser* pINI, const char* pKey, unsigned int uDefaultValue)
{
	unsigned long uPos = INIParser_FindKey(pINI, pKey);
	if (!uPos) return uDefaultValue;

	char* pValue = &pINI->pData[uPos];
	if (pValue[1] == 'x') return (int)(INIParser_HexStringConvert(&pValue[2]));

	return (unsigned int)(atoi(pValue));
}

// Should work fine with basic floating numbers.
float INIParser_FloatConvert(char* pFloat)
{
	float fReturn = 0.f;
	float fDecimal = 1.f;

	while (1)
	{
		char cByte = *pFloat;
		if (cByte == '\0') break;

		if (cByte == '.' || cByte == ',') fDecimal *= 0.1f;
		else if (cByte >= '0' && cByte <= '9')
		{
			int iNumber = cByte - '0';
			if (fDecimal == 1.f) fReturn *= 10.f;
			fReturn += (float)(iNumber)*fDecimal;
			if (fDecimal != 1.f) fDecimal *= 0.1f;
		}

		pFloat++;
		continue;
	}

	return fReturn;
}

float INIParser_GetFloat(SINIParser* pINI, const char* pKey, float fDefaultValue)
{
	unsigned long uPos = INIParser_FindKey(pINI, pKey);
	if (!uPos) return fDefaultValue;

	return INIParser_FloatConvert(&pINI->pData[uPos]);
}
