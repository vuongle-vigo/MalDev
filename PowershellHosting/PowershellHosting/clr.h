#pragma once

#include <Windows.h>
#include <metahost.h>
#include "mscorlib.h"

#pragma comment(lib, "mscoree.lib")

namespace helper {
	BOOL FindAssemblyInGACW(LPCWSTR assemblyName, LPWSTR resultPath, DWORD resultPathSize);
}

class CLR {
public:
	CLR();
	~CLR();
	BOOL InitCLR();
	BOOL GetAssembly(LPCWSTR lpcwsAsmName, _Assembly** ppAssembly);
	BOOL LoadAssembly(LPCWSTR lpcwsAsmName, _Assembly** ppAssembly);
	BOOL GetType(_Assembly* pAssembly, LPCWSTR pwszTypeFullName, _Type** ppType);
	//BOOL GetMethod(_Type* pType, BindingFlags flags, LPCWSTR pwszMethodName, _MethodInfo** ppMethodInfo);
	BOOL GetMethod(_Type* pType, BindingFlags flags, LPCWSTR pwszMethodName, int paramCount, _MethodInfo** ppMethodInfo);
	BOOL GetField(_Type* pType, BindingFlags bindingFlags, LPCWSTR pwszFieldName, _FieldInfo** ppFieldInfo);
	BOOL GetFieldValue(_Type* pType, BindingFlags bindingFlags, VARIANT vtObject, LPCWSTR pwszFieldName, VARIANT* pvtFieldValue);
	BOOL GetProperty(_Type* pType, BindingFlags bindingFlags, LPCWSTR pwszPropertyName, _PropertyInfo** ppPropertyInfo);
	BOOL GetPropertyValue(_Type* pType, BindingFlags bindingFlags, VARIANT vtObject, LPCWSTR pwszPropertyName, VARIANT* pvtPropertyValue);
	BOOL InvokeMethod(_MethodInfo* pMethodInfo, VARIANT vtObject, SAFEARRAY* pParameters, VARIANT* pvtResult);
	BOOL GetJustInTimeMethodAddress(LPCWSTR pwszAssemblyName, LPCWSTR pwszClassName, LPCWSTR pwszMethodName, DWORD dwNbArgs, PULONG_PTR pMethodAddress);
	BOOL PrepareMethod(VARIANT* pvtMethodHandle);
	BOOL GetFunctionPointer(VARIANT* pvtMethodHandle, PULONG_PTR pFunctionPointer);
	void FreeCLR();
private:
	_AppDomain* m_pAppDomain;
	ICLRMetaHost* m_pMetaHost;
	ICLRRuntimeInfo* m_pRuntimeInfo;
	ICorRuntimeHost* m_pRuntimeHost;
	IUnknown* m_pAppDomainThunk;
};