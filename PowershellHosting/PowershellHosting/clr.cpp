#include "clr.h"
#include "common.h"

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

