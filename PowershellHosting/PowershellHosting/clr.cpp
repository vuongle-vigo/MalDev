#include "clr.h"
#include "common.h"
#include <iostream>


BOOL SearchDirectory(LPCWSTR searchPath, LPCWSTR dllName, LPWSTR resultPath, DWORD resultPathSize)
{
	WIN32_FIND_DATAW fd;
	WCHAR path[MAX_PATH];
	wcscpy_s(path, searchPath);
	wcscat_s(path, L"\\*");
	HANDLE hFind = FindFirstFileW(path, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;
	do
	{
		if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
			continue;
		WCHAR fullPath[MAX_PATH];
		wcscpy_s(fullPath, searchPath);
		wcscat_s(fullPath, L"\\");
		wcscat_s(fullPath, fd.cFileName);
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Đệ quy vào folder con
			if (SearchDirectory(fullPath, dllName, resultPath, resultPathSize))
			{
				FindClose(hFind);
				return TRUE;
			}
		}
		else
		{
			// Kiểm tra file .dll
			WCHAR expectedDll[MAX_PATH];
			wcscpy_s(expectedDll, dllName);
			wcscat_s(expectedDll, L".dll");
			if (wcscmp(fd.cFileName, expectedDll) == 0)
			{
				HANDLE hFile = CreateFileW(
					fullPath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL
				);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hFile);
					wcscpy_s(resultPath, resultPathSize, fullPath);
					FindClose(hFind);
					return TRUE;
				}
			}
		}
	} while (FindNextFileW(hFind, &fd));
	FindClose(hFind);
	return FALSE;
}

BOOL helper::FindAssemblyInGACW(LPCWSTR assemblyName, LPWSTR resultPath, DWORD resultPathSize)
{
	const WCHAR* gacPaths[] = {
		L"C:\\Windows\\Microsoft.NET\\assembly\\GAC_MSIL",
		L"C:\\Windows\\Microsoft.NET\\assembly\\GAC_32",
		L"C:\\Windows\\Microsoft.NET\\assembly\\GAC_64",
		L"C:\\Windows\\Microsoft.NET\\assembly\\GAC"
	};
	int gacCount = sizeof(gacPaths) / sizeof(gacPaths[0]);
	for (int i = 0; i < gacCount; i++)
	{
		WCHAR searchPath[MAX_PATH];
		wcscpy_s(searchPath, gacPaths[i]);
		wcscat_s(searchPath, L"\\");
		wcscat_s(searchPath, assemblyName);
		DWORD attr = GetFileAttributesW(searchPath);

		if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
			continue;
		
		if (SearchDirectory(searchPath, assemblyName, resultPath, resultPathSize))
			return TRUE;
	}

	return FALSE;
}

CLR::CLR() {};

CLR::~CLR() {
	FreeCLR();
};

BOOL CLR::InitCLR() {
	HRESULT hr;

	hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*) & m_pMetaHost);
	if (FAILED(hr)) {
		PRINT_ERROR("CLRCreateInstance");
		return false;
	}

	hr = m_pMetaHost->GetRuntime(L"v4.0.30319", IID_ICLRRuntimeInfo, (LPVOID*) & m_pRuntimeInfo);
	if (FAILED(hr)) {
		PRINT_ERROR("m_pMetaHost->GetRuntime");
		m_pMetaHost->Release();
		return false;
	}

	//hr = m_pRuntimeInfo->IsLoadable();
	hr = m_pRuntimeInfo->GetInterface(CLSID_CorRuntimeHost, IID_ICorRuntimeHost, (LPVOID*) & m_pRuntimeHost);
	if (FAILED(hr)) {
		PRINT_ERROR("m_pRuntimeInfo->GetInterface");
		m_pMetaHost->Release();
		m_pRuntimeInfo->Release();
		return false;
	}

	hr = m_pRuntimeHost->Start();
	if (FAILED(hr)) {
		PRINT_ERROR("m_pRuntimeHost->Start");
		m_pMetaHost->Release();
		m_pRuntimeInfo->Release();
		m_pRuntimeHost->Release();
		return false;
	}

	hr = m_pRuntimeHost->CreateDomain(L"PowershellHosting", NULL, &m_pAppDomainThunk);
	if (FAILED(hr)) {
		PRINT_ERROR("m_pRuntimeHost->CreateDomain");
		m_pMetaHost->Release();
		m_pRuntimeInfo->Release();
		m_pRuntimeHost->Release();
		return false;
	}

	hr = m_pAppDomainThunk->QueryInterface(IID_PPV_ARGS(&m_pAppDomain));
	if (FAILED(hr)) {
		PRINT_ERROR("m_pRuntimeHost->QueryInterface");
		m_pMetaHost->Release();
		m_pRuntimeInfo->Release();
		m_pRuntimeHost->Release();
		m_pAppDomainThunk->Release();
		return false;
	}

	return true;
}

BOOL CLR::GetAssembly(LPCWSTR lpcwsAsmName, _Assembly** ppAssembly) {
	if (ppAssembly == NULL)
		return FALSE;

	*ppAssembly = NULL;

	SAFEARRAY* pAssemblies = NULL;
	HRESULT hr = m_pAppDomain->GetAssemblies(&pAssemblies);
	if (FAILED(hr)) {
		PRINT_ERROR("m_pAppDomain->GetAssemblies");
		return false;
	}

	long lBound, uBound;
	SafeArrayGetLBound(pAssemblies, 1, &lBound);
	SafeArrayGetUBound(pAssemblies, 1, &uBound);

	_Assembly** ppLoadedAssembly = NULL;
	hr = SafeArrayAccessData(pAssemblies, (void**)&ppLoadedAssembly);
	if (FAILED(hr)) {
		PRINT_ERROR("SafeArrayAccessData");
		SafeArrayDestroy(pAssemblies);
		return false;
	}

	size_t searchLen = wcslen(lpcwsAsmName);
	BOOL found = FALSE;

	for (long i = lBound; i <= uBound; i++) { 
		BSTR lpwsFullname = NULL;
		hr = ppLoadedAssembly[i]->get_FullName(&lpwsFullname);
		if (FAILED(hr)) {
			PRINT_ERROR("ppLoadedAssembly[i]->get_FullName");
			found = FALSE;
			break;
		}

		// Lấy name từ fullname
		LPCWSTR comma = wcschr(lpwsFullname, L',');
		size_t fullNameLen = (comma != NULL) ? (comma - lpwsFullname) : wcslen(lpwsFullname);

		while (fullNameLen > 0 && lpwsFullname[fullNameLen - 1] == L' ')
			fullNameLen--;

		// So sánh
		if (fullNameLen == searchLen &&
			wcsncmp(lpwsFullname, lpcwsAsmName, fullNameLen) == 0)
		{
			*ppAssembly = ppLoadedAssembly[i];
			(*ppAssembly)->AddRef();
			found = TRUE;
			SysFreeString(lpwsFullname);
			break;
		}

		SysFreeString(lpwsFullname);
	}

	SafeArrayUnaccessData(pAssemblies);
	SafeArrayDestroy(pAssemblies);

	return found;
}

BOOL CLR::LoadAssembly(LPCWSTR lpcwsAsmName, _Assembly** ppAssembly) {
	//need get assembly first
	if (GetAssembly(lpcwsAsmName, ppAssembly)) {
		return true;
	}

	WCHAR wsAssemblyPath[MAX_PATH] = { 0 };
	if (!helper::FindAssemblyInGACW(lpcwsAsmName, wsAssemblyPath, MAX_PATH)) {
		return false;
	}

	HANDLE hFile = CreateFileW(wsAssemblyPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile) {
		PRINT_ERROR("CreateFileW");
		return false;
	}

	DWORD dwFilesize = GetFileSize(hFile, NULL);
	SAFEARRAYBOUND sab = { 0 };
	sab.cElements = dwFilesize;
	SAFEARRAY* saFile = SafeArrayCreate(VT_UI1, 1, &sab);
	if (!ReadFile(hFile, saFile->pvData, dwFilesize, NULL, NULL)) {
		PRINT_ERROR("ReadFile");
		return false;
	}

	m_pAppDomain->Load_3(saFile, ppAssembly);
	CloseHandle(hFile);
	SafeArrayDestroy(saFile);

	return true;
}

BOOL CLR::GetType(_Assembly* pAssembly, LPCWSTR pwszTypeFullName, _Type** ppType) {
	BSTR bstrTypeFullName = SysAllocString(pwszTypeFullName);
	HRESULT hr = pAssembly->GetType_2(bstrTypeFullName, ppType);
	if (FAILED(hr)) {
		PRINT_ERROR("pAssembly->GetType_2(bstrTypeFullName, ppType)");
		return false;
	}

	SysFreeString(bstrTypeFullName);
	return true;
}

//BOOL CLR::GetMethod(_Type* pType, BindingFlags flags, LPCWSTR pwszMethodName, _MethodInfo** ppMethodInfo) {
//	*ppMethodInfo = NULL;
//
//	SAFEARRAY* pMethods = NULL;
//	HRESULT hr = pType->GetMethods(flags, &pMethods);
//
//	if (FAILED(hr) || pMethods == NULL) {
//		PRINT_ERROR("pType->GetMethods");
//		return FALSE;
//	}
//
//	long lBound, uBound;
//	SafeArrayGetLBound(pMethods, 1, &lBound);
//	SafeArrayGetUBound(pMethods, 1, &uBound);
//
//	_MethodInfo** ppMethodArray = NULL;
//	hr = SafeArrayAccessData(pMethods, (void**)&ppMethodArray);
//	if (FAILED(hr)) {
//		SafeArrayDestroy(pMethods);
//		return FALSE;
//	}
//
//	size_t nameLen = wcslen(pwszMethodName);
//	BOOL found = FALSE;
//
//	for (long i = lBound; i <= uBound; i++) {
//		BSTR methodName = NULL;
//		hr = ppMethodArray[i]->get_name(&methodName);
//
//		if (SUCCEEDED(hr) && methodName != NULL) {
//			size_t methodNameLen = SysStringLen(methodName);
//
//			if (methodNameLen == nameLen &&
//				wcsncmp(methodName, pwszMethodName, nameLen) == 0)
//			{
//				*ppMethodInfo = ppMethodArray[i];
//				(*ppMethodInfo)->AddRef();
//				found = TRUE;
//				SysFreeString(methodName);
//				break;
//			}
//
//			SysFreeString(methodName);
//		}
//	}
//
//	SafeArrayUnaccessData(pMethods);
//	SafeArrayDestroy(pMethods);
//
//	if (!found) {
//		return FALSE;
//	}
//
//	return TRUE;
//}

BOOL CLR::GetMethod(_Type* pType, BindingFlags flags, LPCWSTR pwszMethodName, int paramCount, _MethodInfo** ppMethodInfo) {
	*ppMethodInfo = NULL;

	SAFEARRAY* pMethods = NULL;
	HRESULT hr = pType->GetMethods(flags, &pMethods);
	if (FAILED(hr) || !pMethods) {
		PRINT_ERROR("pType->GetMethods");
		return FALSE;
	}

	long lBound, uBound;
	SafeArrayGetLBound(pMethods, 1, &lBound);
	SafeArrayGetUBound(pMethods, 1, &uBound);

	_MethodInfo** ppMethodArray = NULL;
	hr = SafeArrayAccessData(pMethods, (void**)&ppMethodArray);
	if (FAILED(hr)) {
		SafeArrayDestroy(pMethods);
		return FALSE;
	}

	_MethodInfo* pFound = NULL;
	int methodCount = uBound - lBound + 1;

	for (int i = 0; i < methodCount; i++) {
		BSTR methodName = NULL;
		hr = ppMethodArray[i]->get_name(&methodName);

		if (SUCCEEDED(hr) && methodName != NULL) {
			if (_wcsicmp(methodName, pwszMethodName) == 0) {
				SAFEARRAY* pParams = NULL;
				hr = ppMethodArray[i]->GetParameters(&pParams);

				if (SUCCEEDED(hr) && pParams) {
					long pLB, pUB;
					SafeArrayGetLBound(pParams, 1, &pLB);
					SafeArrayGetUBound(pParams, 1, &pUB);

					if (pUB - pLB + 1 == paramCount) {
						pFound = ppMethodArray[i];
					}

					SafeArrayDestroy(pParams);
				}

				if (pFound) {
					SysFreeString(methodName);
					break;
				}
			}

			SysFreeString(methodName);
		}
	}

	SafeArrayUnaccessData(pMethods);
	SafeArrayDestroy(pMethods);

	if (!pFound) {
		wprintf(L"[ERROR] Could not find method '%s' with %d args\n", pwszMethodName, paramCount);
		return FALSE;
	}

	*ppMethodInfo = pFound;
	return TRUE;
}

BOOL CLR::GetProperty(_Type* pType, BindingFlags bindingFlags, LPCWSTR pwszPropertyName, _PropertyInfo** ppPropertyInfo) {
	HRESULT hr;
	BOOL bResult = FALSE;
	BSTR bstrPropertyName = SysAllocString(pwszPropertyName);
	_PropertyInfo* pPropertyInfo = NULL;

	hr = pType->GetProperty(bstrPropertyName, bindingFlags, &pPropertyInfo);
	if (FAILED(hr)) 
		PRINT_ERROR(L"Type->GetProperty");
	
	if (pPropertyInfo == NULL) {
		goto exit;
	}

	*ppPropertyInfo = pPropertyInfo;
	bResult = TRUE;

exit:
	if (bstrPropertyName) SysFreeString(bstrPropertyName);

	return bResult;
}

BOOL CLR::GetPropertyValue(_Type* pType, BindingFlags bindingFlags, VARIANT vtObject, LPCWSTR pwszPropertyName, VARIANT* pvtPropertyValue) {
	BOOL bResult = FALSE;
	HRESULT hr;
	VARIANT vtPropertyValue = { 0 };
	_PropertyInfo* pPropertyInfo = NULL;

	if (!GetProperty(pType, bindingFlags, pwszPropertyName, &pPropertyInfo))
		goto exit;

	hr = pPropertyInfo->GetValue(vtObject, NULL, &vtPropertyValue);
	if (FAILED(hr))
		PRINT_ERROR("PropertyInfo->GetValue");

	memcpy_s(pvtPropertyValue, sizeof(*pvtPropertyValue), &vtPropertyValue, sizeof(vtPropertyValue));
	bResult = TRUE;

exit:
	if (pPropertyInfo) pPropertyInfo->Release();

	return bResult;
}

BOOL CLR::InvokeMethod(_MethodInfo* pMethodInfo, VARIANT vtObject, SAFEARRAY* pParameters, VARIANT* pvtResult) {
	HRESULT hr = pMethodInfo->Invoke_3(vtObject, pParameters, pvtResult);
	if (FAILED(hr)) {
		PRINT_ERROR("pMethodInfo->Invoke_3");
		return false;
	}

	return true;
}

BOOL CLR::GetJustInTimeMethodAddress(LPCWSTR pwszAssemblyName, LPCWSTR pwszClassName, LPCWSTR pwszMethodName, DWORD dwNbArgs, PULONG_PTR pMethodAddress) {
	BOOL bResult = FALSE;
	VARIANT vtMethodHandlePtr = { 0 };
	VARIANT vtMethodHandleVal = { 0 };
	_Type* pType = NULL;
	_Type* pMethodInfoType = NULL;
	_MethodInfo* pTargetMethodInfo = NULL;
	BindingFlags flags = BindingFlags(BindingFlags_Instance |
		BindingFlags_Static |
		BindingFlags_Public |
		BindingFlags_NonPublic |
		BindingFlags_DeclaredOnly);
	_Assembly* pAsmReflect;
	_Assembly* pAsm;
	if (!LoadAssembly(pwszAssemblyName, &pAsm)) {
		goto exit;
	}

	if (!GetType(pAsm, pwszClassName, &pType))
		goto exit;

	if (!GetMethod(pType, flags, pwszMethodName, dwNbArgs, &pTargetMethodInfo))
		goto exit;

	//
	// The method for obtaining the MethodHandle from the MethodInfo object is
	// taken from this article.
	// 
	// Credit:
	//   - https://www.outflank.nl/blog/2024/02/01/unmanaged-dotnet-patching/
	//

	if (!GetAssembly(L"System.Reflection", &pAsmReflect)) {
		goto exit;
	}

	if (!GetType(pAsmReflect, L"System.Reflection.MethodInfo", &pMethodInfoType))
		goto exit;

	vtMethodHandlePtr.vt = VT_UNKNOWN;
	vtMethodHandlePtr.punkVal = pTargetMethodInfo;

	if (!GetPropertyValue(pMethodInfoType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), vtMethodHandlePtr, L"MethodHandle", &vtMethodHandleVal))
		goto exit;

	//
	// Next, we can invoke 'RuntimeHelpers.PrepareMethod' to make sure the target
	// method is JIT-compiled, and finally get its effective address.
	//
	// Credit:
	//   - https://github.com/calebstewart/bypass-clm
	//   - https://www.mdsec.co.uk/2020/08/massaging-your-clr-preventing-environment-exit-in-in-process-net-assemblies/
	//

	if (!PrepareMethod(&vtMethodHandleVal))
		goto exit;

	if (!GetFunctionPointer(&vtMethodHandleVal, pMethodAddress))
		goto exit;

	bResult = TRUE;

exit:
	if (pTargetMethodInfo) pTargetMethodInfo->Release();
	if (pType) pType->Release();
	if (pMethodInfoType) pMethodInfoType->Release();
	if (pAsm) pAsm->Release();
	if (pAsmReflect) pAsmReflect->Release();

	VariantClear(&vtMethodHandleVal);
	VariantClear(&vtMethodHandlePtr);

	return bResult;
}

BOOL CLR::GetFunctionPointer(VARIANT* pvtMethodHandle, PULONG_PTR pFunctionPointer) {
	BOOL bResult = FALSE;
	VARIANT vtFunctionPointer = { 0 };
	_Type* pRuntimeMethodHandleType = NULL;
	_MethodInfo* pGetFunctionPointerInfo = NULL;
	_Assembly* pAsm = NULL;
	if (!LoadAssembly(L"System.Runtime", &pAsm)) {
		goto exit;
	}

	if (!GetType(pAsm, L"System.RuntimeMethodHandle", &pRuntimeMethodHandleType))
		goto exit;

	if (!GetMethod(pRuntimeMethodHandleType, BindingFlags(BindingFlags_Public | BindingFlags_Instance), L"GetFunctionPointer", 0, &pGetFunctionPointerInfo))
		goto exit;

	if (!InvokeMethod(pGetFunctionPointerInfo, *pvtMethodHandle, NULL, &vtFunctionPointer))
		goto exit;

	*pFunctionPointer = vtFunctionPointer.ullVal;
	bResult = TRUE;

exit:
	if (pGetFunctionPointerInfo) pGetFunctionPointerInfo->Release();
	if (pRuntimeMethodHandleType) pRuntimeMethodHandleType->Release();
	if (pAsm) pAsm->Release();

	return bResult;
}

BOOL CLR::PrepareMethod(VARIANT* pvtMethodHandle)
{
	BOOL bResult = FALSE;
	LONG lArgumentIndex;
	SAFEARRAY* pPrepareMethodArguments = NULL;
	VARIANT vtEmpty = { 0 };
	VARIANT vtResult = { 0 };
	_Type* pRuntimeHelpersType = NULL;
	_MethodInfo* pPrepareMethod = NULL;
	_Assembly* pAsm = NULL;

	if (!LoadAssembly(L"System.Runtime", &pAsm)) {
		goto exit;
	}

	if (!GetType(pAsm, L"System.Runtime.CompilerServices.RuntimeHelpers", &pRuntimeHelpersType))
		goto exit;

	if (!GetMethod(pRuntimeHelpersType, BindingFlags(BindingFlags_Public | BindingFlags_Static), L"PrepareMethod", 1, &pPrepareMethod))
		goto exit;

	pPrepareMethodArguments = SafeArrayCreateVector(VT_VARIANT, 0, 1);

	lArgumentIndex = 0;
	SafeArrayPutElement(pPrepareMethodArguments, &lArgumentIndex, pvtMethodHandle);

	if (!InvokeMethod(pPrepareMethod, vtEmpty, pPrepareMethodArguments, &vtResult))
		goto exit;

	bResult = TRUE;

exit:
	if (pPrepareMethodArguments) SafeArrayDestroy(pPrepareMethodArguments);

	if (pPrepareMethod) pPrepareMethod->Release();
	if (pRuntimeHelpersType) pRuntimeHelpersType->Release();
	if (pAsm) pAsm->Release();

	VariantClear(&vtResult);

	return bResult;
}

void CLR::FreeCLR() {
	if (m_pMetaHost) {
		m_pMetaHost->Release();
	}

	if (m_pRuntimeInfo) {
		m_pRuntimeInfo->Release();
	}

	if (m_pRuntimeHost) {
		m_pRuntimeHost->Release();
	}

	if (m_pAppDomainThunk) {
		m_pAppDomainThunk->Release();
	}

	if (m_pAppDomain) {
		m_pAppDomain->Release();
	}
}

