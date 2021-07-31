#pragma once
#include <ntifs.h>
#include <ntstrsafe.h>
#include <stdlib.h>
#include <wdf.h>
#include "INIParser.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

NTSTATUS DriverUnload(PDRIVER_OBJECT pDriverObject);