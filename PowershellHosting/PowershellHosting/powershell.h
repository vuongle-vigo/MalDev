#pragma once

#include "clr.h"
#include "common.h"
#include "patch.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <propvarutil.h>

// Helper functions
BOOL System_Object_GetType(CLR& clr, VARIANT vtObject, VARIANT* pvtType);
BOOL System_Type_GetProperty(CLR& clr, VARIANT vtType, LPCWSTR pwszPropertyName, VARIANT* pvtPropertyInfo);
BOOL System_Reflection_PropertyInfo_GetValue(CLR& clr, VARIANT vtPropertyInfo, VARIANT vtObject, SAFEARRAY* pIndex, VARIANT* pvtValue);
BOOL System_Reflection_PropertyInfo_GetValue(CLR& clr, VARIANT vtPropertyInfo, VARIANT vtObject, VARIANT* pvtValue);

// REPL/DLL output functions
void CollectOutput(CLR& clr, VARIANT vtResult, std::wstring* pOut);
void PrintPowerShellOutput(CLR& clr, VARIANT vtResult, std::wstring* out);

// Stream access
BOOL PowerShellGetStream(CLR& clr, VARIANT vtPowerShellInstance, LPCWSTR pwszStreamName, VARIANT* pvtStream);

// Error printing functions (REPL)
void PrintErrorRecord(CLR& clr, VARIANT vtErrorRecord);
void PrintPowerShellErrorStream(CLR& clr, VARIANT vtErrorStream);
void PrintPowerShellInvocationStateInfoReason(CLR& clr, VARIANT vtReason);
void PrintPowerShellInvokeErrors(CLR& clr, VARIANT vtPowerShellInstance);

// Error collection functions (DLL)
void CollectErrorRecord(CLR& clr, VARIANT vtErrorRecord, std::wstring* pOut);
void CollectPowerShellErrorStream(CLR& clr, VARIANT vtErrorStream, std::wstring* pOut);
void CollectPowerShellInvocationStateInfoReason(CLR& clr, VARIANT vtReason, std::wstring* pOut);
void CollectPowerShellInvokeErrors(CLR& clr, VARIANT vtPowerShellInstance, std::wstring* pOut);

// Utility functions
void ClearStreamErrors(CLR& clr, VARIANT vtPowerShellInstance);
BOOL PowerShellHadErrors(CLR& clr, VARIANT vtPowerShellInstance, PBOOL pbHadErrors);
BOOL DisablePowerShellEtwProvider(CLR& clr);

// Patching functions
void Patch(CLR& clr);

// Main entry point
BOOL Invoke(CLR& clr, std::wstring command, std::wstring *out);
