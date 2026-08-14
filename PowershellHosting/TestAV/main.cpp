int main() {
    int nRet = 0;
    CLR clr;
    _Assembly* pAsm = NULL;
    _Assembly* pAsmSystemReflect = NULL;
    _Type* pTypePS = NULL;
    _Type* pTypeRunspace = NULL;
    _Type* pTypeRunspaceFactory = NULL;
    _MethodInfo* pMethodCreate = NULL;
    _MethodInfo* pMethodClose = NULL;
    _MethodInfo* pMethodAddScript = NULL;
    _MethodInfo* pMethodInvoke = NULL;
    _MethodInfo* pMethodCreateRunspace = NULL;
    _MethodInfo* pMethodOpen = NULL;
    VARIANT vtPSInstance;
    VariantInit(&vtPSInstance);
    SAFEARRAY* pEmptyArgs = NULL;
    VARIANT vtScript;
    VariantInit(&vtScript);
    VARIANT vtRunspace;
    VariantInit(&vtRunspace);
    SAFEARRAY* pArgs = NULL;
    VARIANT vtResult;
    VariantInit(&vtResult);
    VARIANT vtRunspaceProperty;
    VariantInit(&vtRunspaceProperty);
    long idx = 0;
    BOOL bHadErrors = FALSE;
    //execute_debug_context();
    if (!clr.InitCLR()) {
        wprintf(L"[!] Failed to init CLR\n");
        return 0;
    }

    if (!clr.LoadAssembly(L"System.Management.Automation", &pAsm)) {
        wprintf(L"[!] Failed to load assembly\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.LoadAssembly(L"System.Reflection", &pAsmSystemReflect)) {
        wprintf(L"[!] Failed to load assembly\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetType(pAsm, L"System.Management.Automation.PowerShell", &pTypePS)) {
        wprintf(L"[!] Failed to get PowerShell type\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetType(pAsm, L"System.Management.Automation.Runspaces.RunspaceFactory", &pTypeRunspaceFactory)) {
        wprintf(L"[!] Failed to get PowerShell type RunspaceFactory\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetType(pAsm, L"System.Management.Automation.Runspaces.Runspace", &pTypeRunspace)) {
        wprintf(L"[!] Failed to get PowerShell type Runspace\n");
        nRet = 0;
        goto cleanup;
    }

    if (!clr.GetMethod(pTypeRunspaceFactory, BindingFlags(BindingFlags_Public | BindingFlags_Static), L"CreateRunspace", 0, &pMethodCreateRunspace)) {
        wprintf(L"[!] Failed to get CreateRunspace method\n");
        goto cleanup;
    }

    if (!clr.GetMethod(pTypeRunspace, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"Open", 0, &pMethodOpen)) {
        wprintf(L"[!] Failed to get Open method\n");
        goto cleanup;
    }

    if (!clr.GetMethod(pTypeRunspace, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"Open", 0, &pMethodClose)) {
        wprintf(L"[!] Failed to get Open method\n");
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

    //if (!PatchEtwRet(clr)) {
    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
    //}

    //if (!PatchEtw()) {
    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
    //}

    //Patch(clr);

    //if (!PatchEtw()) {
    //    PRINT_ERROR("Failed to PatchEtwRet.\n");
    //}

    // Create runspace instance
    //if (!clr.InvokeMethod(pMethodCreateRunspace, vtRunspace, NULL, &vtRunspace)) {
    //    wprintf(L"[!] Failed to Create Runspace\n");
    //    nRet = 0;
    //    goto cleanup;
    //}

    //if (!clr.InvokeMethod(pMethodOpen, vtRunspace, NULL, NULL)) {
    //    wprintf(L"[!] Failed to Open Runspace\n");
    //    nRet = 0;
    //    goto cleanup;
    //}

    while (true) {
        if (!clr.InvokeMethod(pMethodCreate, vtPSInstance, NULL, &vtPSInstance)) {
            wprintf(L"[!] Failed to create PowerShell instance\n");
            nRet = 0;
            goto cleanup;
        }

        //MessageBoxA(NULL, NULL, NULL, 0);
        //if (!clr.GetPropertyValue(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPSInstance, L"Runspace", &vtRunspaceProperty)) {
        //    wprintf(L"[!] Failed to get property Runspace\n");
        //    nRet = 0;
        //    goto cleanup;
        //}

        //if (!clr.SetPropertyValue(pTypePS, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtPSInstance, L"Runspace", vtRunspace)) {
        //    wprintf(L"[!] Failed to set property Runspace\n");
        //    nRet = 0;
        //    goto cleanup;
        //}

        wprintf(L"PS> ");

        std::wstring input;
        std::getline(std::wcin, input);

        if (input == L"exit" || input == L"quit")
            break;

        if (input.empty())
            continue;

        BSTR bstrScript = SysAllocString(input.c_str());
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

        Patch(clr);

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
        }

        VariantClear(&vtResult);
    }

    nRet = 1;

cleanup:

    // SAFEARRAY arguments
    if (pArgs)
    {
        SafeArrayDestroy(pArgs);
        pArgs = NULL;
    }

    if (pEmptyArgs)
    {
        SafeArrayDestroy(pEmptyArgs);
        pEmptyArgs = NULL;
    }

    // VARIANTs
    VariantClear(&vtResult);
    VariantClear(&vtScript);
    VariantClear(&vtPSInstance);
    VariantClear(&vtRunspaceProperty);

    // Close Runspace before releasing it
    if (pMethodClose && vtRunspace.vt != VT_EMPTY)
    {
        clr.InvokeMethod(pMethodClose, vtRunspace, NULL, NULL);
    }

    VariantClear(&vtRunspace);

    // Methods
    if (pMethodClose)
        pMethodClose->Release();

    if (pMethodOpen)
        pMethodOpen->Release();

    if (pMethodCreateRunspace)
        pMethodCreateRunspace->Release();

    if (pMethodInvoke)
        pMethodInvoke->Release();

    if (pMethodAddScript)
        pMethodAddScript->Release();

    if (pMethodCreate)
        pMethodCreate->Release();

    // Types
    if (pTypeRunspace)
        pTypeRunspace->Release();

    if (pTypeRunspaceFactory)
        pTypeRunspaceFactory->Release();

    if (pTypePS)
        pTypePS->Release();

    // Assemblies
    if (pAsmSystemReflect)
        pAsmSystemReflect->Release();

    if (pAsm)
        pAsm->Release();

    return nRet;
}