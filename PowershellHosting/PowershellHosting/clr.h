#pragma once

#include <Windows.h>
#include <metahost.h>
#include "mscorlib.h"

#pragma comment(lib, "mscoree.lib")

class CLR {
public:
	CLR();
	~CLR();
	BOOL InitCLR();
	void FreeCLR();
private:
	_AppDomain* m_pAppDomain;
	ICLRMetaHost* m_pMetaHost;
	ICLRRuntimeInfo* m_pRuntimeInfo;
	ICorRuntimeHost* m_pRuntimeHost;
	IUnknown* m_pAppDomainThunk;
};