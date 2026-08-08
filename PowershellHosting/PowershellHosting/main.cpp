#include "clr.h"
#include "common.h"

#ifdef _WINDLL

#else
#include <iostream>
#include <string>
#include <propvarutil.h>

// Helper functions
BOOL System_Object_GetType(CLR& clr, VARIANT vtObject, VARIANT* pvtType) {
    _Assembly* pAsm = NULL;
    _Type* pObjectType = NULL;
    _MethodInfo* pGetType = NULL;
    VARIANT vtResult;
    VariantInit(&vtResult);
    BOOL bResult = FALSE;

    if (!clr.LoadAssembly(L"System.Runtime", &pAsm)) {
        wprintf(L"[!] Failed to load assembly\n");
        return 0;
    }

    if (!clr.GetType(pAsm, L"System.Object", &pObjectType))
        goto exit;

    if (!clr.GetMethod(pObjectType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetType", 0, &pGetType))
        goto exit;

    bResult = clr.InvokeMethod(pGetType, vtObject, NULL, &vtResult);

    if (bResult) {
        // VariantCopy addref COM object đúng cách, tránh leak refcount khi caller cũng Clear
        HRESULT hr = VariantCopy(pvtType, &vtResult);
        if (FAILED(hr)) bResult = FALSE;
    }

exit:
    VariantClear(&vtResult);
    if (pGetType) pGetType->Release();
    if (pObjectType) pObjectType->Release();
    if (pAsm) pAsm->Release();
    return bResult;
}

BOOL System_Type_GetProperty(CLR& clr, VARIANT vtType, LPCWSTR pwszPropertyName, VARIANT* pvtPropertyInfo) {
    _Assembly* pAsm = NULL;
    _Type* pTypeType = NULL;
    _MethodInfo* pGetProperty = NULL;
    VARIANT vtPropName;
    VariantInit(&vtPropName);
    SAFEARRAY* pArgs = NULL;
    VARIANT vtResult;
    VariantInit(&vtResult);
    long idx = 0;
    BOOL bResult = FALSE;

    if (!clr.LoadAssembly(L"System.Runtime", &pAsm)) {
        wprintf(L"[!] Failed to load assembly\n");
        return 0;
    }

    if (!clr.GetType(pAsm, L"System.Type", &pTypeType))
        goto exit;

    if (!clr.GetMethod(pTypeType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetProperty", 1, &pGetProperty))
        goto exit;

    vtPropName.vt = VT_BSTR;
    vtPropName.bstrVal = SysAllocString(pwszPropertyName);
    if (!vtPropName.bstrVal) goto exit;

    pArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
    if (!pArgs) goto exit;
    if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtPropName))) goto exit;

    bResult = clr.InvokeMethod(pGetProperty, vtType, pArgs, &vtResult);

    if (bResult) {
        HRESULT hr = VariantCopy(pvtPropertyInfo, &vtResult);
        if (FAILED(hr)) bResult = FALSE;
    }

exit:
    VariantClear(&vtResult);
    if (pArgs) SafeArrayDestroy(pArgs);
    VariantClear(&vtPropName);
    if (pGetProperty) pGetProperty->Release();
    if (pTypeType) pTypeType->Release();
    if (pAsm) pAsm->Release();
    return bResult;
}

BOOL System_Reflection_PropertyInfo_GetValue(CLR& clr, VARIANT vtPropertyInfo, VARIANT vtObject, SAFEARRAY* pIndex, VARIANT* pvtValue) {
    _Assembly* pAsm = NULL;
    _Type* pPropertyInfoType = NULL;
    _MethodInfo* pGetValue = NULL;
    SAFEARRAY* pArgs = NULL;
    LONG lNbArguments = pIndex != NULL ? 2 : 1;
    int argCount = lNbArguments;
    long idx = 0;
    VARIANT vtIndexArray;
    VariantInit(&vtIndexArray);
    VARIANT vtResult;
    VariantInit(&vtResult);
    BOOL bResult = FALSE;

    if (!clr.LoadAssembly(L"System.Reflection", &pAsm)) {
        wprintf(L"[!] Failed to load assembly\n");
        return 0;
    }

    if (!clr.GetType(pAsm, L"System.Reflection.PropertyInfo", &pPropertyInfoType))
        goto exit;

    if (!clr.GetMethod(pPropertyInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetValue", lNbArguments, &pGetValue))
        goto exit;

    pArgs = SafeArrayCreateVector(VT_VARIANT, 0, argCount);
    if (!pArgs) goto exit;
    if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtObject))) goto exit;

    if (pIndex != NULL) {
        vtIndexArray.vt = VT_ARRAY | VT_VARIANT;
        vtIndexArray.parray = pIndex;
        idx = 1;
        if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtIndexArray))) goto exit;
        // vtIndexArray chỉ là wrapper quanh pIndex do caller sở hữu; không VariantClear ở đây
    }

    bResult = clr.InvokeMethod(pGetValue, vtPropertyInfo, pArgs, &vtResult);

    if (bResult) {
        HRESULT hr = VariantCopy(pvtValue, &vtResult);
        if (FAILED(hr)) bResult = FALSE;
    }

exit:
    VariantClear(&vtResult);
    if (pArgs) SafeArrayDestroy(pArgs);
    if (pGetValue) pGetValue->Release();
    if (pPropertyInfoType) pPropertyInfoType->Release();
    if (pAsm) pAsm->Release();
    return bResult;
}

BOOL System_Reflection_PropertyInfo_GetValue(CLR& clr, VARIANT vtPropertyInfo, VARIANT vtObject, VARIANT* pvtValue) {
    return System_Reflection_PropertyInfo_GetValue(clr, vtPropertyInfo, vtObject, NULL, pvtValue);
}

// Main print function
void PrintPowerShellOutput(CLR& clr, VARIANT vtResult) {
    VARIANT vtResultType = { 0 };
    VARIANT vtCountProperty = { 0 };
    VARIANT vtCount = { 0 };
    VARIANT vtItemProperty = { 0 };
    VARIANT vtValue = { 0 };
    VARIANT vtValueAsString = { 0 };
    SAFEARRAY* pIndex = NULL;
    _Type* pPSObjectType = NULL;
    _MethodInfo* pToString = NULL;
    _Assembly* pAsm = NULL;

    // Get result type
    if (!System_Object_GetType(clr, vtResult, &vtResultType))
        goto exit;

    // Get Count property
    if (!System_Type_GetProperty(clr, vtResultType, L"Count", &vtCountProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtCountProperty, vtResult, &vtCount))
        goto exit;

    // Get Item property
    if (!System_Type_GetProperty(clr, vtResultType, L"Item", &vtItemProperty))
        goto exit;

    // Get PSObject type for ToString
    if (clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        BSTR bstrTypeName = SysAllocString(L"System.Management.Automation.PSObject");
        if (bstrTypeName) {
            pAsm->GetType_2(bstrTypeName, &pPSObjectType);
            SysFreeString(bstrTypeName);
        }
        pAsm->Release();
        pAsm = NULL;
    }

    if (pPSObjectType) {
        if (!clr.GetMethod(pPSObjectType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"ToString", 0, &pToString))
            pToString = NULL;  // an toàn: skip ToString bên dưới
    }

    if (vtCount.vt == VT_I4 && vtCount.intVal > 0) {
        for (int i = 0; i < vtCount.intVal; i++) {
            VARIANT vtIndex;
            vtIndex.vt = VT_I4;
            vtIndex.intVal = i;

            pIndex = SafeArrayCreateVector(VT_VARIANT, 0, 1);
            long idx = 0;
            if (pIndex) {
                if (FAILED(SafeArrayPutElement(pIndex, &idx, &vtIndex))) {
                    SafeArrayDestroy(pIndex);
                    pIndex = NULL;
                }
            }

            if (pIndex && System_Reflection_PropertyInfo_GetValue(clr, vtItemProperty, vtResult, pIndex, &vtValue)) {
                VariantInit(&vtValueAsString);
                if (pToString && clr.InvokeMethod(pToString, vtValue, NULL, &vtValueAsString)) {
                    if (vtValueAsString.vt == VT_BSTR && vtValueAsString.bstrVal) {
                        wprintf(L"%ws", vtValueAsString.bstrVal);
                    }
                }
                VariantClear(&vtValueAsString);
                VariantClear(&vtValue);
            }

            SafeArrayDestroy(pIndex);
            pIndex = NULL;
        }
    }

exit:
    if (pIndex) SafeArrayDestroy(pIndex);
    if (pToString) pToString->Release();
    if (pPSObjectType) pPSObjectType->Release();
    if (pAsm) pAsm->Release();

    VariantClear(&vtValueAsString);
    VariantClear(&vtValue);
    VariantClear(&vtItemProperty);
    VariantClear(&vtCountProperty);
    VariantClear(&vtCount);
    VariantClear(&vtResultType);
}

BOOL PowerShellGetStream(CLR &clr, VARIANT vtPowerShellInstance, LPCWSTR pwszStreamName, VARIANT* pvtStream)
{
    BOOL bResult = FALSE;
    VARIANT vtStreams = { 0 };
    VARIANT vtStream = { 0 };
    _Type* pPowerShellType = NULL;
    _Type* pPSDataStreamsType = NULL;
    _Assembly* pAsm = NULL;

    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        goto exit;
    }

    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pPowerShellType))
        goto exit;

    if (!clr.GetPropertyValue(pPowerShellType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPowerShellInstance, L"Streams", &vtStreams))
        goto exit;

    if (!clr.GetType(pAsm, L"System.Management.Automation.PSDataStreams", &pPSDataStreamsType))
        goto exit;

    if (!clr.GetPropertyValue(pPSDataStreamsType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtStreams, pwszStreamName, &vtStream))
        goto exit;

    // VariantCopy addref COM object đúng cách
    if (FAILED(VariantCopy(pvtStream, &vtStream))) goto exit;
    bResult = TRUE;

exit:
    VariantClear(&vtStream);
    VariantClear(&vtStreams);
    if (pPSDataStreamsType) pPSDataStreamsType->Release();
    if (pPowerShellType) pPowerShellType->Release();
    if (pAsm) pAsm->Release();

    return bResult;
}

void PrintErrorRecord(CLR &clr, VARIANT vtErrorRecord)
{
    WORD wOldColor = 0;
    size_t sScriptStackTraceLen;
    VARIANT vtErrorRecordType = { 0 };
    VARIANT vtTargetObjectProperty = { 0 };
    VARIANT vtTargetObject = { 0 };
    VARIANT vtScriptStackTraceProperty = { 0 };
    VARIANT vtScriptStackTrace = { 0 };
    VARIANT vtCategoryInfoProperty = { 0 };
    VARIANT vtCategoryInfo = { 0 };
    VARIANT vtCategoryInfoMessage = { 0 };
    VARIANT vtFullyQualifiedErrorIdProperty = { 0 };
    VARIANT vtFullyQualifiedErrorId = { 0 };
    VARIANT vtExceptionProperty = { 0 };
    VARIANT vtException = { 0 };
    VARIANT vtExceptionType = { 0 };
    VARIANT vtExceptionMessageProperty = { 0 };
    VARIANT vtExceptionMessage = { 0 };
    _Type* pErrorCategoryInfoType = NULL;
    _MethodInfo* pErrorCategoryInfoGetMessageMethodInfo = NULL;
    _Assembly* pAsm = NULL;

    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        goto exit;
    }

    if (!System_Object_GetType(clr, vtErrorRecord, &vtErrorRecordType))
        goto exit;

    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"TargetObject", &vtTargetObjectProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtTargetObjectProperty, vtErrorRecord, &vtTargetObject))
        goto exit;

    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"ScriptStackTrace", &vtScriptStackTraceProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtScriptStackTraceProperty, vtErrorRecord, &vtScriptStackTrace))
        goto exit;

    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"CategoryInfo", &vtCategoryInfoProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtCategoryInfoProperty, vtErrorRecord, &vtCategoryInfo))
        goto exit;

    if (!clr.GetType(pAsm, L"System.Management.Automation.ErrorCategoryInfo", &pErrorCategoryInfoType))
        goto exit;

    if (!clr.GetMethod(pErrorCategoryInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetMessage", 0, &pErrorCategoryInfoGetMessageMethodInfo))
        goto exit;

    if (!clr.InvokeMethod(pErrorCategoryInfoGetMessageMethodInfo, vtCategoryInfo, NULL, &vtCategoryInfoMessage))
        goto exit;

    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"FullyQualifiedErrorId", &vtFullyQualifiedErrorIdProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtFullyQualifiedErrorIdProperty, vtErrorRecord, &vtFullyQualifiedErrorId))
        goto exit;

    if (!System_Type_GetProperty(clr, vtErrorRecordType, L"Exception", &vtExceptionProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtExceptionProperty, vtErrorRecord, &vtException))
        goto exit;

    if (!System_Object_GetType(clr, vtException, &vtExceptionType))
        goto exit;

    if (!System_Type_GetProperty(clr, vtExceptionType, L"Message", &vtExceptionMessageProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtExceptionMessageProperty, vtException, &vtExceptionMessage))
        goto exit;

    //SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY, &wOldColor);

    if (vtTargetObject.vt == VT_BSTR && vtExceptionMessage.vt == VT_BSTR)
    {
        wprintf(L"%ws : %ws\n", vtTargetObject.bstrVal, vtExceptionMessage.bstrVal);
    }
    else if (vtTargetObject.vt != VT_BSTR && vtExceptionMessage.vt == VT_BSTR)
    {
        wprintf(L". : %ws\n", vtExceptionMessage.bstrVal);
    }

    if (vtScriptStackTrace.vt == VT_BSTR)
    {
        wprintf(L"%ws\n", vtScriptStackTrace.bstrVal);
    }

    if (vtTargetObject.vt == VT_BSTR && vtTargetObject.bstrVal)
    {
        sScriptStackTraceLen = wcslen(vtTargetObject.bstrVal);
        wprintf(L"+ %ws\n", vtTargetObject.bstrVal);
        wprintf(L"+ ");
        for (size_t i = 0; i < sScriptStackTraceLen; i++) { wprintf(L"%ws", L"~"); }
        wprintf(L"\n");
    }

    if (vtCategoryInfoMessage.vt == VT_BSTR)
    {
        wprintf(L"    + CategoryInfo           : %ws\n", vtCategoryInfoMessage.bstrVal);
    }

    if (vtFullyQualifiedErrorId.vt == VT_BSTR)
    {
        wprintf(L"    + FullyQualifiedErrorId  : %ws\n", vtFullyQualifiedErrorId.bstrVal);
    }

    if (wOldColor != 0)
    {
        //SetConsoleTextColor(wOldColor, NULL);
    }

    wprintf(L"\n");

exit:
    if (pErrorCategoryInfoGetMessageMethodInfo) pErrorCategoryInfoGetMessageMethodInfo->Release();
    if (pErrorCategoryInfoType) pErrorCategoryInfoType->Release();
    if (pAsm) pAsm->Release();

    VariantClear(&vtExceptionMessage);
    VariantClear(&vtExceptionMessageProperty);
    VariantClear(&vtExceptionType);
    VariantClear(&vtException);
    VariantClear(&vtExceptionProperty);
    VariantClear(&vtFullyQualifiedErrorId);
    VariantClear(&vtFullyQualifiedErrorIdProperty);
    VariantClear(&vtCategoryInfoMessage);
    VariantClear(&vtCategoryInfo);
    VariantClear(&vtCategoryInfoProperty);
    VariantClear(&vtScriptStackTrace);
    VariantClear(&vtScriptStackTraceProperty);
    VariantClear(&vtTargetObject);
    VariantClear(&vtTargetObjectProperty);
    VariantClear(&vtErrorRecordType);
}

void PrintPowerShellErrorStream(CLR &clr, VARIANT vtErrorStream)
{
    LONG lArgumentIndex;
    VARIANT vtPSDataCollectionType = { 0 };
    VARIANT vtPSDataCollectionCountProperty = { 0 };
    VARIANT vtErrorStreamCount = { 0 };
    VARIANT vtPSDataCollectionItemProperty = { 0 };
    VARIANT vtIndex = { 0 };
    VARIANT vtErrorRecord = { 0 };
    SAFEARRAY* pIndex = NULL;
    _Assembly* pAsm = NULL;

    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        goto exit;
    }

    if (!System_Object_GetType(clr, vtErrorStream, &vtPSDataCollectionType))
        goto exit;

    if (!System_Type_GetProperty(clr, vtPSDataCollectionType, L"Count", &vtPSDataCollectionCountProperty))
        goto exit;

    if (!System_Reflection_PropertyInfo_GetValue(clr, vtPSDataCollectionCountProperty, vtErrorStream, &vtErrorStreamCount))
        goto exit;

    if (!System_Type_GetProperty(clr, vtPSDataCollectionType, L"Item", &vtPSDataCollectionItemProperty))
        goto exit;

    if (vtErrorStreamCount.vt == VT_I4 && vtErrorStreamCount.lVal > 0)
    {
        for (LONG i = 0; i < vtErrorStreamCount.lVal; i++)
        {
            InitVariantFromInt32(i, &vtIndex);
            pIndex = SafeArrayCreateVector(VT_VARIANT, 0, 1);
            lArgumentIndex = 0;
            if (pIndex) {
                if (FAILED(SafeArrayPutElement(pIndex, &lArgumentIndex, &vtIndex))) {
                    SafeArrayDestroy(pIndex);
                    pIndex = NULL;
                }
            }

            if (pIndex && System_Reflection_PropertyInfo_GetValue(clr, vtPSDataCollectionItemProperty, vtErrorStream, pIndex, &vtErrorRecord))
            {
                PrintErrorRecord(clr, vtErrorRecord);
                VariantClear(&vtErrorRecord);
            }

            SafeArrayDestroy(pIndex);
            pIndex = NULL;
            VariantClear(&vtIndex);
        }
    }

exit:
    if (pIndex) SafeArrayDestroy(pIndex);
    if (pAsm) pAsm->Release();

    VariantClear(&vtErrorStreamCount);
    VariantClear(&vtIndex);
    VariantClear(&vtPSDataCollectionItemProperty);
    VariantClear(&vtPSDataCollectionCountProperty);
    VariantClear(&vtPSDataCollectionType);
}

void PrintPowerShellInvocationStateInfoReason(CLR &clr, VARIANT vtReason)
{
    WORD wOldColor = 0;
    VARIANT vtExceptionAsString = { 0 };
    _Type* pExceptionType = NULL;
    _MethodInfo* pToStringMethod = NULL;
    _Assembly* pAsm = NULL;

    if (!clr.LoadAssembly(L"System.Runtime", &pAsm)) {
        goto exit;
    }

    if (!clr.GetType(pAsm, L"System.Exception", &pExceptionType))
        goto exit;

    if (!clr.GetMethod(pExceptionType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"ToString", 0, &pToStringMethod))
        goto exit;

    if (!clr.InvokeMethod(pToStringMethod, vtReason, NULL, &vtExceptionAsString))
        goto exit;

    if (vtExceptionAsString.vt == VT_BSTR && vtExceptionAsString.bstrVal && wcslen(vtExceptionAsString.bstrVal) > 0)
    {
        //PRINT_ERROR("An exception was thrown while executing the input script.\n\n");

        //SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY, &wOldColor);

        wprintf(L"%ws\n\n", vtExceptionAsString.bstrVal);

        if (wOldColor != 0)
        {
            //SetConsoleTextColor(wOldColor, NULL);
        }
    }

exit:
    if (pToStringMethod) pToStringMethod->Release();
    if (pExceptionType) pExceptionType->Release();
    if (pAsm) pAsm->Release();

    VariantClear(&vtExceptionAsString);
}

void PrintPowerShellInvokeErrors(CLR &clr, VARIANT vtPowerShellInstance)
{
    VARIANT vtErrorStream = { 0 };
    VARIANT vtInvocationStateInfo = { 0 };
    VARIANT vtReason = { 0 };
    _Type* pPowerShellType = NULL;
    _Type* pPSInvocationStateInfoType = NULL;
    _Assembly* pAsm = NULL;

    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        goto exit;
    }

    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pPowerShellType))
        goto exit;

    if (!PowerShellGetStream(clr, vtPowerShellInstance, L"Error", &vtErrorStream))
        goto exit;

    if (vtErrorStream.vt != VT_EMPTY)
    {
        PrintPowerShellErrorStream(clr, vtErrorStream);
    }

    if (!clr.GetPropertyValue(pPowerShellType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPowerShellInstance, L"InvocationStateInfo", &vtInvocationStateInfo))
        goto exit;

    if (!clr.GetType(pAsm, L"System.Management.Automation.PSInvocationStateInfo", &pPSInvocationStateInfoType))
        goto exit;

    if (!clr.GetPropertyValue(pPSInvocationStateInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtInvocationStateInfo, L"Reason", &vtReason))
        goto exit;

    if (vtReason.vt != VT_EMPTY)
    {
        PrintPowerShellInvocationStateInfoReason(clr, vtReason);
    }

exit:
    if (pPSInvocationStateInfoType) pPSInvocationStateInfoType->Release();
    if (pPowerShellType) pPowerShellType->Release();
    if (pAsm) pAsm->Release();

    VariantClear(&vtReason);
    VariantClear(&vtInvocationStateInfo);
    VariantClear(&vtErrorStream);
}

void ClearStreamErrors(CLR& clr, VARIANT vtPowerShellInstance) {
    //pass
    (void)clr; (void)vtPowerShellInstance;
}

BOOL PowerShellHadErrors(CLR &clr, VARIANT vtPowerShellInstance, PBOOL pbHadErrors)
{
    BOOL bResult = FALSE;
    VARIANT vtHadErrors = { 0 };
    _Type* pPowerShellType = NULL;
    _Assembly* pAsm = NULL;

    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        goto exit;
    }

    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pPowerShellType))
        goto exit;

    if (!clr.GetPropertyValue(pPowerShellType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPowerShellInstance, L"HadErrors", &vtHadErrors))
        goto exit;

    *pbHadErrors = vtHadErrors.boolVal;
    bResult = TRUE;

exit:
    VariantClear(&vtHadErrors);
    if (pPowerShellType) pPowerShellType->Release();
    if (pAsm) pAsm->Release();

    return bResult;
}

int main() {
    int nRet = 0;
    CLR clr;
    _Assembly* pAsm = NULL;
    _Type* pTypePS = NULL;
    _MethodInfo* pMethodCreate = NULL;
    _MethodInfo* pMethodAddScript = NULL;
    _MethodInfo* pMethodInvoke = NULL;
    VARIANT vtPSInstance;
    VariantInit(&vtPSInstance);
    SAFEARRAY* pEmptyArgs = NULL;
    VARIANT vtScript;
    VariantInit(&vtScript);
    SAFEARRAY* pArgs = NULL;
    VARIANT vtResult;
    VariantInit(&vtResult);
    long idx = 0;
    BOOL bHadErrors = FALSE;

    if (!clr.InitCLR()) {
        wprintf(L"[!] Failed to init CLR\n");
        return 0;
    }

    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        wprintf(L"[!] Failed to load assembly\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pTypePS)) {
        wprintf(L"[!] Failed to get PowerShell type\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetMethod(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Static), L"Create", 0, &pMethodCreate)) {
        wprintf(L"[!] Failed to get Create method\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetMethod(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"AddScript", 1, &pMethodAddScript)) {
        wprintf(L"[!] Failed to get AddScript method\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetMethod(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"Invoke", 0, &pMethodInvoke)) {
        wprintf(L"[!] Failed to get Invoke method\n");
        nRet = 0;
        goto cleanup;
    }

    pEmptyArgs = SafeArrayCreateVector(VT_VARIANT, 0, 0);
    if (!pEmptyArgs) goto cleanup;
    if (!clr.InvokeMethod(pMethodCreate, vtPSInstance, pEmptyArgs, &vtPSInstance)) {
        wprintf(L"[!] Failed to create PowerShell instance\n");
        nRet = 0;
        goto cleanup;
    }
    SafeArrayDestroy(pEmptyArgs);
    pEmptyArgs = NULL;

    wprintf(L"[+] PowerShell ready (type 'exit' to quit)\n\n");

    while (true) {
        wprintf(L"PS> ");

        std::wstring input;
        std::getline(std::wcin, input);

        if (input == L"exit" || input == L"quit")
            break;

        if (input.empty())
            continue;

        std::wstring inputOut = input + L"| Out-String";
        BSTR bstrScript = SysAllocString(inputOut.c_str());
        if (!bstrScript) goto cleanup;
        vtScript.vt = VT_BSTR;
        vtScript.bstrVal = bstrScript;

        pArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
        if (!pArgs) goto cleanup;
        if (FAILED(SafeArrayPutElement(pArgs, &idx, &vtScript))) goto cleanup;

        VariantClear(&vtResult);
        VariantInit(&vtResult);
        clr.InvokeMethod(pMethodAddScript, vtPSInstance, pArgs, &vtResult);

        SafeArrayDestroy(pArgs);
        pArgs = NULL;
        VariantClear(&vtScript);
        VariantClear(&vtResult);

        VariantInit(&vtResult);

        if (clr.InvokeMethod(pMethodInvoke, vtPSInstance, NULL, &vtResult)) {
            PrintPowerShellOutput(clr, vtResult);
        }

        if (!PowerShellHadErrors(clr, vtPSInstance, &bHadErrors))
            goto cleanup;

        if (bHadErrors)
        {
            PrintPowerShellInvokeErrors(clr, vtPSInstance);

            // Reset PowerShell instance: release cái cũ, tạo cái mới.
            // Cách này xóa mọi errors/streams/commands — không cần gọi Clear() từng phần.
            VariantClear(&vtPSInstance);

            SAFEARRAY* pResetArgs = SafeArrayCreateVector(VT_VARIANT, 0, 0);
            if (!pResetArgs) goto cleanup;
            if (!clr.InvokeMethod(pMethodCreate, vtPSInstance, pResetArgs, &vtPSInstance)) {
                SafeArrayDestroy(pResetArgs);
                wprintf(L"[!] Failed to recreate PowerShell instance\n");
                goto cleanup;
            }
            SafeArrayDestroy(pResetArgs);
        }

        VariantClear(&vtResult);
        wprintf(L"\n");
    }

    nRet = 1;

cleanup:
    VariantClear(&vtResult);
    if (pArgs) SafeArrayDestroy(pArgs);
    VariantClear(&vtScript);
    if (pEmptyArgs) SafeArrayDestroy(pEmptyArgs);
    VariantClear(&vtPSInstance);
    if (pMethodInvoke) pMethodInvoke->Release();
    if (pMethodAddScript) pMethodAddScript->Release();
    if (pMethodCreate) pMethodCreate->Release();
    if (pTypePS) pTypePS->Release();
    if (pAsm) pAsm->Release();

    return nRet;
}

#endif
