#define _CRT_SECURE_NO_WARNINGS

#include "HttpClient.h"
#include "ApiResolve.h"
#include "CRT.h"
#include "HashString.h"

namespace HttpLib {

    void HttpRequest_Init(HttpRequest* req) {
        if (!req) return;
        crt_memset(req, 0, sizeof(HttpRequest));
        req->timeout = 30000;
        req->followRedirects = TRUE;
        req->verifySsl = TRUE;
        req->bodyAllocated = FALSE;
    }

    void HttpRequest_Free(HttpRequest* req) {
        if (!req) return;
        if (req->headers) {
            crt_free(req->headers);
            req->headers = NULL;
            req->headerCount = 0;
        }
        if (req->body && req->bodyAllocated) {
            crt_free(req->body);
            req->body = NULL;
            req->bodyLength = 0;
            req->bodyAllocated = FALSE;
        }
    }

    void HttpRequest_AddHeader(HttpRequest* req, const wchar_t* name, const wchar_t* value) {
        if (!req || !name || !value) return;

        HttpHeader* newHeaders = (HttpHeader*)crt_realloc(req->headers, sizeof(HttpHeader) * (req->headerCount + 1));
        if (!newHeaders) return;

        req->headers = newHeaders;
        crt_wcsncpy(req->headers[req->headerCount].name, name, 255);
        req->headers[req->headerCount].name[255] = L'\0';
        crt_wcsncpy(req->headers[req->headerCount].value, value, 511);
        req->headers[req->headerCount].value[511] = L'\0';
        req->headerCount++;
    }

    void HttpResponse_Init(HttpResponse* resp) {
        if (!resp) return;
        crt_memset(resp, 0, sizeof(HttpResponse));
    }

    void HttpResponse_Free(HttpResponse* resp) {
        if (!resp) return;
        if (resp->body) {
            crt_free(resp->body);
            resp->body = NULL;
        }
        if (resp->headers) {
            crt_free(resp->headers);
            resp->headers = NULL;
        }
        resp->bodyLength = 0;
        resp->headerCount = 0;
        resp->statusCode = 0;
        resp->success = FALSE;
        resp->errorCode = 0;
    }

    static void HttpMethodToString(
        HttpMethod method,
        wchar_t* buffer,
        int bufferSize)
    {
        if (!buffer || bufferSize <= 0)
            return;

        buffer[0] = 0;

        if (method == HTTP_GET)
        {
            if (bufferSize >= 4)
            {
                buffer[0] = L'G';
                buffer[1] = L'E';
                buffer[2] = L'T';
                buffer[3] = 0;
            }
        }
        else if (method == HTTP_POST)
        {
            if (bufferSize >= 5)
            {
                buffer[0] = L'P';
                buffer[1] = L'O';
                buffer[2] = L'S';
                buffer[3] = L'T';
                buffer[4] = 0;
            }
        }
        else if (method == HTTP_PUT)
        {
            if (bufferSize >= 4)
            {
                buffer[0] = L'P';
                buffer[1] = L'U';
                buffer[2] = L'T';
                buffer[3] = 0;
            }
        }
        else if (method == HTTP_DEL)
        {
            if (bufferSize >= 7)
            {
                buffer[0] = L'D';
                buffer[1] = L'E';
                buffer[2] = L'L';
                buffer[3] = L'E';
                buffer[4] = L'T';
                buffer[5] = L'E';
                buffer[6] = 0;
            }
        }
        else if (method == HTTP_PATCH)
        {
            if (bufferSize >= 6)
            {
                buffer[0] = L'P';
                buffer[1] = L'A';
                buffer[2] = L'T';
                buffer[3] = L'C';
                buffer[4] = L'H';
                buffer[5] = 0;
            }
        }
        else if (method == HTTP_HEAD)
        {
            if (bufferSize >= 5)
            {
                buffer[0] = L'H';
                buffer[1] = L'E';
                buffer[2] = L'A';
                buffer[3] = L'D';
                buffer[4] = 0;
            }
        }
        else if (method == HTTP_OPTIONS)
        {
            if (bufferSize >= 8)
            {
                buffer[0] = L'O';
                buffer[1] = L'P';
                buffer[2] = L'T';
                buffer[3] = L'I';
                buffer[4] = L'O';
                buffer[5] = L'N';
                buffer[6] = L'S';
                buffer[7] = 0;
            }
        }
        else
        {
            if (bufferSize >= 4)
            {
                buffer[0] = L'G';
                buffer[1] = L'E';
                buffer[2] = L'T';
                buffer[3] = 0;
            }
        }
    }

    static void GetStatusText(DWORD statusCode, wchar_t* buffer, int bufferSize)
    {
        const wchar_t wsContinue[] = {L'C', L'o', L'n', L't', L'i', L'n', L'u', L'e', L'\0'};
        const wchar_t wsSwitchingProtocols[] = {L'S', L'w', L'i', L't', L'c', L'h', L'i', L'n', L'g', L' ', L'P', L'r', L'o', L't', L'o', L'c', L'o', L'l', L's', L'\0'};
        const wchar_t wsOK[] = {L'O', L'K', L'\0'};
        const wchar_t wsCreated[] = {L'C', L'r', L'e', L'a', L't', L'e', L'd', L'\0'};
        const wchar_t wsNoContent[] = {L'N', L'o', L' ', L'C', L'o', L'n', L't', L'e', L'n', L't', L'\0'};
        const wchar_t wsMovedPermanently[] = {L'M', L'o', L'v', L'e', L'd', L' ', L'P', L'e', L'r', L'm', L'a', L'n', L'e', L'n', L't', L'l', L'y', L'\0'};
        const wchar_t wsFound[] = {L'F', L'o', L'u', L'n', L'd', L'\0'};
        const wchar_t wsBadRequest[] = {L'B', L'a', L'd', L' ', L'R', L'e', L'q', L'u', L'e', L's', L't', L'\0'};
        const wchar_t wsUnauthorized[] = {L'U', L'n', L'a', L'u', L't', L'h', L'o', L'r', L'i', L'z', L'e', L'd', L'\0'};
        const wchar_t wsForbidden[] = {L'F', L'o', L'r', L'b', L'i', L'd', L'd', L'e', L'n', L'\0'};
        const wchar_t wsNotFound[] = {L'N', L'o', L't', L' ', L'F', L'o', L'u', L'n', L'd', L'\0'};
        const wchar_t wsInternalServerError[] = {L'I', L'n', L't', L'e', L'r', L'n', L'a', L'l', L' ', L'S', L'e', L'r', L'v', L'e', L'r', L' ', L'E', L'r', L'r', L'o', L'r', L'\0'};
        const wchar_t wsBadGateway[] = {L'B', L'a', L'd', L' ', L'G', L'a', L't', L'e', L'w', L'a', L'y', L'\0'};
        const wchar_t wsServiceUnavailable[] = {L'S', L'e', L'r', L'v', L'i', L'c', L'e', L' ', L'U', L'n', L'a', L'v', L'a', L'i', L'l', L'a', L'b', L'l', L'e', L'\0'};
        const wchar_t wsUnknown[] = {L'U', L'n', L'k', L'n', L'o', L'w', L'n', L'\0'};

        const wchar_t* text = wsUnknown;

        if (statusCode == 100)
        {
            text = wsContinue;
        }
        else if (statusCode == 101)
        {
            text = wsSwitchingProtocols;
        }
        else if (statusCode == 200)
        {
            text = wsOK;
        }
        else if (statusCode == 201)
        {
            text = wsCreated;
        }
        else if (statusCode == 204)
        {
            text = wsNoContent;
        }
        else if (statusCode == 301)
        {
            text = wsMovedPermanently;
        }
        else if (statusCode == 302)
        {
            text = wsFound;
        }
        else if (statusCode == 400)
        {
            text = wsBadRequest;
        }
        else if (statusCode == 401)
        {
            text = wsUnauthorized;
        }
        else if (statusCode == 403)
        {
            text = wsForbidden;
        }
        else if (statusCode == 404)
        {
            text = wsNotFound;
        }
        else if (statusCode == 500)
        {
            text = wsInternalServerError;
        }
        else if (statusCode == 502)
        {
            text = wsBadGateway;
        }
        else if (statusCode == 503)
        {
            text = wsServiceUnavailable;
        }
        else
        {
            text = wsUnknown;
        }

        crt_wcsncpy(buffer, text, bufferSize - 1);
        buffer[bufferSize - 1] = L'\0';
    }

    //HttpClient& HttpClient::GetInstance() {
    //    static HttpClient instance;
    //    return instance;
    //}

    HttpClient::HttpClient() {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        constexpr unsigned int hashInitializeCriticalSection = ComplexHashForAnsi("InitializeCriticalSection");
        typedef VOID
            (WINAPI*
            _InitializeCriticalSection)(
                _Out_ LPCRITICAL_SECTION lpCriticalSection
            );
        _InitializeCriticalSection pInitializeCriticalSection = (_InitializeCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashInitializeCriticalSection);
        pInitializeCriticalSection(&m_cs);
        m_hSession = NULL;
        wchar_t wsUserAgent[] = {L'M', L'o', L'z', L'i', L'l', L'l', L'a', L'/', L'5', L'.', L'0', L' ', L'(', L'W', L'i', L'n', L'd', L'o', L'w', L's', L' ', L'N', L'T', L' ', L'1', L'0', L'.', L'0', L';', L' ', L'W', L'i', L'n', L'6', L'4', L';', L' ', L'x', L'6', L'4', L')', L' ', L'A', L'p', L'p', L'l', L'e', L'W', L'e', L'b', L'K', L'i', L't', L'/', L'5', L'3', L'7', L'.', L'3', L'6', L'\0'};
        crt_wcscpy(m_userAgent, wsUserAgent);
        m_timeout = 30000;
        m_followRedirects = TRUE;
        m_verifySsl = TRUE;
        m_useProxy = FALSE;
        m_proxyUrl[0] = L'\0';
    }

    HttpClient::~HttpClient() {
        CloseSession();
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
            (WINAPI*
            _DeleteCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
            );
        constexpr unsigned int hashDeleteCriticalSection = ComplexHashForAnsi("DeleteCriticalSection");
        _DeleteCriticalSection pDeleteCriticalSection = (_DeleteCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashDeleteCriticalSection);
        pDeleteCriticalSection(&m_cs);
    }

    void HttpClient::SetUserAgent(const wchar_t* userAgent) {
        if (!userAgent) return;
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
            (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
            );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        crt_wcsncpy(m_userAgent, userAgent, 255);
        m_userAgent[255] = L'\0';
        typedef VOID
            (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
            );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        pLeaveCriticalSection(&m_cs);
    }

    void HttpClient::SetTimeout(DWORD timeoutMs) {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
        (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        m_timeout = timeoutMs;
        typedef VOID
        (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        pLeaveCriticalSection(&m_cs);
    }

    void HttpClient::SetFollowRedirects(BOOL follow) {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
        (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        m_followRedirects = follow;
        typedef VOID
        (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        pLeaveCriticalSection(&m_cs);
    }

    void HttpClient::SetVerifySsl(BOOL verify) {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
        (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        m_verifySsl = verify;
        typedef VOID
        (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        pLeaveCriticalSection(&m_cs);
    }

    void HttpClient::SetProxy(const wchar_t* proxyUrl) {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
        (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        if (proxyUrl) {
            crt_wcsncpy(m_proxyUrl, proxyUrl, 511);
            m_proxyUrl[511] = L'\0';
            m_useProxy = TRUE;
        }
        else {
            m_proxyUrl[0] = L'\0';
            m_useProxy = FALSE;
        }
        typedef VOID
        (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        pLeaveCriticalSection(&m_cs);
    }

    void HttpClient::ClearProxy() {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
        (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        m_proxyUrl[0] = L'\0';
        m_useProxy = FALSE;
        typedef VOID
        (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        pLeaveCriticalSection(&m_cs);
    }

    BOOL HttpClient::ParseUrl(const wchar_t* url, wchar_t* host, int hostSize, wchar_t* path, int pathSize,
        INTERNET_PORT* port, HttpScheme* scheme) {
        if (!url || !host || !path || !port || !scheme) return FALSE;

        URL_COMPONENTS urlComp;
        crt_memset(&urlComp, 0, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);

        wchar_t hostBuffer[256];
        crt_memset(hostBuffer, 0, 256);
        wchar_t pathBuffer[2048];
        crt_memset(pathBuffer, 0, 2048);

        urlComp.lpszHostName = hostBuffer;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = pathBuffer;
        urlComp.dwUrlPathLength = 2048;
        ApiResolve apiResolve;
        constexpr unsigned int hashWinHttp = ComplexHashForWChar(L"winhttp.dll");
        LPVOID lpWinHttp = apiResolve.GetModuleBaseAddress(hashWinHttp);
        constexpr unsigned int hashWinHttpCrackUrl = ComplexHashForAnsi("WinHttpCrackUrl");
        typedef BOOL
            (WINAPI*
            _WinHttpCrackUrl)
            (
                _In_reads_(dwUrlLength) LPCWSTR pwszUrl,
                _In_ DWORD dwUrlLength,
                _In_ DWORD dwFlags,
                _Inout_ LPURL_COMPONENTS lpUrlComponents
            );
        _WinHttpCrackUrl pWinHttpCrackUrl = (_WinHttpCrackUrl)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpCrackUrl);
        if (!pWinHttpCrackUrl(url, (DWORD)crt_wcslen(url), 0, &urlComp)) {
            return FALSE;
        }

        crt_wcsncpy(host, hostBuffer, hostSize - 1);
        host[hostSize - 1] = L'\0';

        crt_wcsncpy(path, pathBuffer, pathSize - 1);
        path[pathSize - 1] = L'\0';

        *port = urlComp.nPort;

        if (urlComp.nScheme == INTERNET_SCHEME_HTTP) {
            *scheme = HTTP_SCHEME_HTTP;
            if (*port == 0) *port = INTERNET_DEFAULT_HTTP_PORT;
        }
        else if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
            *scheme = HTTP_SCHEME_HTTPS;
            if (*port == 0) *port = INTERNET_DEFAULT_HTTPS_PORT;
        }
        else {
            *scheme = HTTP_SCHEME_HTTPS;
            if (*port == 0) *port = INTERNET_DEFAULT_HTTPS_PORT;
        }

        if (path[0] == L'\0') {
            wchar_t pattern[] = { L'/', L'\0' };
            crt_wcscpy(path, pattern);
        }

        return TRUE;
    }

    HINTERNET HttpClient::OpenSession() {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
        (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        typedef VOID
        (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        if (m_hSession) {
            pLeaveCriticalSection(&m_cs);
            return m_hSession;
        }

        DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        LPCWSTR proxyName = WINHTTP_NO_PROXY_NAME;
        LPCWSTR proxyBypass = WINHTTP_NO_PROXY_BYPASS;

        if (m_useProxy && m_proxyUrl[0] != L'\0') {
            accessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            proxyName = m_proxyUrl;
        }

        constexpr unsigned int hashWinHttp = ComplexHashForWChar(L"winhttp.dll");
        LPVOID lpWinHttp = apiResolve.GetModuleBaseAddress(hashWinHttp);
        typedef HINTERNET
            (WINAPI*
            _WinHttpOpen)
            (
                _In_opt_z_ LPCWSTR pszAgentW,
                _In_ DWORD dwAccessType,
                _In_opt_z_ LPCWSTR pszProxyW,
                _In_opt_z_ LPCWSTR pszProxyBypassW,
                _In_ DWORD dwFlags
            );
        constexpr unsigned int hashWinHttpOpen = ComplexHashForAnsi("WinHttpOpen");
        _WinHttpOpen pWinHttpOpen = (_WinHttpOpen)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpOpen);
        m_hSession = pWinHttpOpen(m_userAgent, accessType, proxyName, proxyBypass, 0);

        if (m_hSession && m_timeout > 0) {
            typedef BOOL
                (WINAPI*
                _WinHttpSetTimeouts)
                (
                    IN HINTERNET    hInternet,           // Session/Request handle.
                    IN int          nResolveTimeout,
                    IN int          nConnectTimeout,
                    IN int          nSendTimeout,
                    IN int          nReceiveTimeout
                );
            constexpr unsigned int hashWinHttpSetTimeouts = ComplexHashForAnsi("WinHttpSetTimeouts");
            _WinHttpSetTimeouts pWinHttpSetTimeouts = (_WinHttpSetTimeouts)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpSetTimeouts);
            pWinHttpSetTimeouts(m_hSession, m_timeout, m_timeout, m_timeout, m_timeout);
        }

        pLeaveCriticalSection(&m_cs);
        return m_hSession;
    }

    void HttpClient::CloseSession() {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef VOID
        (WINAPI*
            _EnterCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashEnterCriticalSection = ComplexHashForAnsi("EnterCriticalSection");
        _EnterCriticalSection pEnterCriticalSection = (_EnterCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashEnterCriticalSection);
        pEnterCriticalSection(&m_cs);
        if (m_hSession) {
            constexpr unsigned int hashWinHttp = ComplexHashForWChar(L"winhttp.dll");
            LPVOID lpWinHttp = apiResolve.GetModuleBaseAddress(hashWinHttp);
            typedef BOOL
                (WINAPI*
                _WinHttpCloseHandle)
                (
                    IN HINTERNET hInternet
                );
            constexpr unsigned int hashWinHttpCloseHandle = ComplexHashForAnsi("WinHttpCloseHandle");
            _WinHttpCloseHandle pWinHttpCloseHandle = (_WinHttpCloseHandle)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpCloseHandle);
            pWinHttpCloseHandle(m_hSession);
            m_hSession = NULL;
        }
        typedef VOID
        (WINAPI*
            _LeaveCriticalSection)(
                _Inout_ LPCRITICAL_SECTION lpCriticalSection
                );
        constexpr unsigned int hashLeaveCriticalSection = ComplexHashForAnsi("LeaveCriticalSection");
        _LeaveCriticalSection pLeaveCriticalSection = (_LeaveCriticalSection)apiResolve.GetApiAddress(lpKernel32, hashLeaveCriticalSection);
        pLeaveCriticalSection(&m_cs);
    }

    void HttpClient::ApplySslIgnoreOption(HINTERNET hRequest) {
        if (!hRequest) return;

        DWORD dwSecFlags = 0;
        DWORD dwBuffLen = sizeof(dwSecFlags);
        ApiResolve apiResolve;
        constexpr unsigned int hashWinHttp = ComplexHashForWChar(L"winhttp.dll");
        LPVOID lpWinHttp = apiResolve.GetModuleBaseAddress(hashWinHttp);
        typedef BOOL
            (WINAPI*
            _WinHttpQueryOption)
            (
                IN HINTERNET hInternet,
                IN DWORD dwOption,
                _Out_writes_bytes_to_opt_(*lpdwBufferLength, *lpdwBufferLength) __out_data_source(NETWORK) LPVOID lpBuffer,
                IN OUT LPDWORD lpdwBufferLength
            );
        constexpr unsigned int hashWinHttpQueryOption = ComplexHashForAnsi("WinHttpQueryOption");
        _WinHttpQueryOption pWinHttpQueryOption = (_WinHttpQueryOption)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpQueryOption);
        if (pWinHttpQueryOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecFlags, &dwBuffLen)) {
            dwSecFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
            typedef BOOL
                (WINAPI*
                _WinHttpSetOption)
                (
                    _In_opt_ HINTERNET hInternet,
                    _In_ DWORD dwOption,
                    _When_((dwOption == WINHTTP_OPTION_USERNAME ||
                        dwOption == WINHTTP_OPTION_PASSWORD ||
                        dwOption == WINHTTP_OPTION_PROXY_USERNAME ||
                        dwOption == WINHTTP_OPTION_PROXY_PASSWORD ||
                        dwOption == WINHTTP_OPTION_USER_AGENT),
                        _At_((LPCWSTR)lpBuffer, _In_reads_(dwBufferLength)))
                    _When_((dwOption == WINHTTP_OPTION_CLIENT_CERT_CONTEXT),
                        _In_reads_bytes_opt_(dwBufferLength))
                    _When_((dwOption != WINHTTP_OPTION_USERNAME &&
                        dwOption != WINHTTP_OPTION_PASSWORD &&
                        dwOption != WINHTTP_OPTION_PROXY_USERNAME &&
                        dwOption != WINHTTP_OPTION_PROXY_PASSWORD &&
                        dwOption != WINHTTP_OPTION_CLIENT_CERT_CONTEXT &&
                        dwOption != WINHTTP_OPTION_USER_AGENT),
                        _In_reads_bytes_(dwBufferLength))
                    LPVOID lpBuffer,
                    _In_ DWORD dwBufferLength
                );
            constexpr unsigned int hashWinHttpSetOption = ComplexHashForAnsi("WinHttpSetOption");
            _WinHttpSetOption pWinHttpSetOption = (_WinHttpSetOption)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpSetOption);
            pWinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecFlags, sizeof(dwSecFlags));
        }
    }

    static char* WideToAnsi(const wchar_t* wideStr, DWORD* outLen) {
        if (!wideStr) {
            if (outLen) *outLen = 0;
            return NULL;
        }

        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef int
        (WINAPI*
            _WideCharToMultiByte)(
                _In_ UINT CodePage,
                _In_ DWORD dwFlags,
                _In_NLS_string_(cchWideChar) LPCWCH lpWideCharStr,
                _In_ int cchWideChar,
                _Out_writes_bytes_to_opt_(cbMultiByte, return) LPSTR lpMultiByteStr,
                _In_ int cbMultiByte,
                _In_opt_ LPCCH lpDefaultChar,
                _Out_opt_ LPBOOL lpUsedDefaultChar
                );
        constexpr unsigned int hashWideCharToMultiByte = ComplexHashForAnsi("WideCharToMultiByte");
        _WideCharToMultiByte pWideCharToMultiByte = (_WideCharToMultiByte)apiResolve.GetApiAddress(lpKernel32, hashWideCharToMultiByte);
        int len = pWideCharToMultiByte(CP_ACP, 0, wideStr, -1, NULL, 0, NULL, NULL);
        if (len <= 0) {
            if (outLen) *outLen = 0;
            return NULL;
        }

        char* result = (char*)crt_malloc(len);
        if (!result) {
            if (outLen) *outLen = 0;
            return NULL;
        }

        pWideCharToMultiByte(CP_ACP, 0, wideStr, -1, result, len, NULL, NULL);
        if (outLen) *outLen = (DWORD)(len - 1);
        return result;
    }

    void HttpClient::SendRequestInternal(HttpResponse* response, HttpMethod method, const wchar_t* url,
        const wchar_t* body, DWORD bodyLen, const HttpHeader* headers, int headerCount) {
        if (!response) return;
        HttpResponse_Init(response);
        response->success = FALSE;

        wchar_t host[256];
        crt_memset(host, 0, 256);
        wchar_t path[2048];
        crt_memset(path, 0, 2048);
        INTERNET_PORT port = 0;
        HttpScheme scheme = HTTP_SCHEME_HTTPS;
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        constexpr unsigned int hashWinHttp = ComplexHashForWChar(L"winhttp.dll");
        LPVOID lpWinHttp = apiResolve.GetModuleBaseAddress(hashWinHttp);
        constexpr unsigned int hashGetLastError = ComplexHashForAnsi("GetLastError");
        typedef DWORD
            (WINAPI*
            _GetLastError)(
                VOID
            );
        _GetLastError pGetLastError = (_GetLastError)apiResolve.GetApiAddress(lpKernel32, hashGetLastError);
        if (!ParseUrl(url, host, 256, path, 2048, &port, &scheme)) {
            response->errorCode = pGetLastError();
            wchar_t wsError[] = {L'F', L'a', L'i', L'l', L'e', L'd', L' ', L't', L'o', L' ', L'p', L'a', L'r', L's', L'e', L' ', L'U', L'R', L'L', L'\0'};
            crt_wcscpy(response->errorMessage, wsError);
            return;
        }

        HINTERNET hSession = OpenSession();
        if (!hSession) {
            response->errorCode = pGetLastError();
            wchar_t wsError[] = {L'F', L'a', L'i', L'l', L'e', L'd', L' ', L't', L'o', L' ', L'o', L'p', L'e', L'n', L' ', L's', L'e', L's', L's', L'i', L'o', L'n', L'\0'};
            crt_wcscpy(response->errorMessage, wsError);
            return;
        }

        DWORD flags = WINHTTP_FLAG_SECURE;
        if (scheme == HTTP_SCHEME_HTTP) {
            flags = 0;
        }

        typedef HINTERNET
            (WINAPI*
            _WinHttpConnect)
            (
                IN HINTERNET hSession,
                IN LPCWSTR pswzServerName,
                IN INTERNET_PORT nServerPort,
                IN DWORD dwReserved
            );
        constexpr unsigned int hashWinHttpConnect = ComplexHashForAnsi("WinHttpConnect");
        _WinHttpConnect pWinHttpConnect = (_WinHttpConnect)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpConnect);
        HINTERNET hConnect = pWinHttpConnect(hSession, host, port, 0);
        if (!hConnect) {
            response->errorCode = pGetLastError();
            wchar_t wsError[] = {L'F', L'a', L'i', L'l', L'e', L'd', L' ', L't', L'o', L' ', L'c', L'o', L'n', L'n', L'e', L'c', L't', L'\0'};
            crt_wcscpy(response->errorMessage, wsError);
            CloseSession();
            return;
        }

        wchar_t bufferMethod[10];
        crt_memset(bufferMethod, 0, 10);
        HttpMethodToString(method, bufferMethod, 10);
        typedef HINTERNET
            (WINAPI*
            _WinHttpOpenRequest)
            (
                IN HINTERNET hConnect,
                IN LPCWSTR pwszVerb,
                IN LPCWSTR pwszObjectName,
                IN LPCWSTR pwszVersion,
                IN LPCWSTR pwszReferrer OPTIONAL,
                IN LPCWSTR FAR* ppwszAcceptTypes OPTIONAL,
                IN DWORD dwFlags
            );
        constexpr unsigned int hashWinHttpOpenRequest = ComplexHashForAnsi("WinHttpOpenRequest");
        _WinHttpOpenRequest pWinHttpOpenRequest = (_WinHttpOpenRequest)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpOpenRequest);
        HINTERNET hRequest = pWinHttpOpenRequest(
            hConnect,
            bufferMethod,
            path,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags
        );
        typedef BOOL
        (WINAPI*
            _WinHttpCloseHandle)
            (
                IN HINTERNET hInternet
                );
        constexpr unsigned int hashWinHttpCloseHandle = ComplexHashForAnsi("WinHttpCloseHandle");
        _WinHttpCloseHandle pWinHttpCloseHandle = (_WinHttpCloseHandle)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpCloseHandle);
        if (!hRequest) {
            response->errorCode = pGetLastError();
            wchar_t wsError[] = {L'F', L'a', L'i', L'l', L'e', L'd', L' ', L't', L'o', L' ', L'o', L'p', L'e', L'n', L' ', L'r', L'e', L'q', L'u', L'e', L's', L't', L'\0'};
            crt_wcscpy(response->errorMessage, wsError);
            pWinHttpCloseHandle(hConnect);
            CloseSession();
            return;
        }

        if (!m_verifySsl) {
            ApplySslIgnoreOption(hRequest);
        }

        wchar_t headersStr[8192];
        crt_memset(headersStr, 0, 8192);
        if (headers && headerCount > 0) {
            int offset = 0;
            for (int i = 0; i < headerCount && offset < 8000; i++) {
                wchar_t pattern[] = {L'%', L's', L':', L' ', L'%', L's', L'\\', L'r', L'\\', L'n', L'\0'};
                int len = crt_swprintf(headersStr + offset, 8192 - offset, pattern, headers[i].name, headers[i].value);
                if (len > 0) offset += len;
            }
        }

        char* ansiBody = NULL;
        DWORD ansiBodyLen = 0;
        if (body && bodyLen > 0) {
            ansiBody = WideToAnsi(body, &ansiBodyLen);
        }

        LPVOID dataToSend = ansiBody ? (LPVOID)ansiBody : WINHTTP_NO_REQUEST_DATA;
        DWORD dataLen = ansiBody ? ansiBodyLen : 0;

        typedef BOOL
            (WINAPI*
            _WinHttpSendRequest)
            (
                IN HINTERNET hRequest,
                _In_reads_opt_(dwHeadersLength) LPCWSTR lpszHeaders,
                IN DWORD dwHeadersLength,
                _In_reads_bytes_opt_(dwOptionalLength) LPVOID lpOptional,
                IN DWORD dwOptionalLength,
                IN DWORD dwTotalLength,
                IN DWORD_PTR dwContext
            );
        constexpr unsigned int hashWinHttpSendRequest = ComplexHashForAnsi("WinHttpSendRequest");
        _WinHttpSendRequest pWinHttpSendRequest = (_WinHttpSendRequest)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpSendRequest);
        if (!pWinHttpSendRequest(hRequest,
            headersStr[0] ? headersStr : WINHTTP_NO_ADDITIONAL_HEADERS,
            headersStr[0] ? (DWORD)crt_wcslen(headersStr) : 0,
            dataToSend,
            dataLen,
            dataLen,
            0)) {
            if (ansiBody) crt_free(ansiBody);
            response->errorCode = pGetLastError();
            wchar_t wsError[] = {L'F', L'a', L'i', L'l', L'e', L'd', L' ', L't', L'o', L' ', L's', L'e', L'n', L'd', L' ', L'r', L'e', L'q', L'u', L'e', L's', L't', L'\0'};
            crt_wcscpy(response->errorMessage, wsError);
            pWinHttpCloseHandle(hRequest);
            pWinHttpCloseHandle(hConnect);
            CloseSession();
            return;
        }

        if (ansiBody) crt_free(ansiBody);
        typedef BOOL
            (WINAPI*
            _WinHttpReceiveResponse)
            (
                IN HINTERNET hRequest,
                IN LPVOID lpReserved
            );
        constexpr unsigned int hashWinHttpReceiveResponse = ComplexHashForAnsi("WinHttpReceiveResponse");
        _WinHttpReceiveResponse  pWinHttpReceiveResponse = (_WinHttpReceiveResponse)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpReceiveResponse);
        if (!pWinHttpReceiveResponse(hRequest, NULL)) {
            response->errorCode = pGetLastError();
            wchar_t wsError[] = {L'F', L'a', L'i', L'l', L'e', L'd', L' ', L't', L'o', L' ', L'r', L'e', L'c', L'e', L'i', L'v', L'e', L' ', L'r', L'e', L's', L'p', L'o', L'n', L's', L'e', L'\0'};
            crt_wcscpy(response->errorMessage, wsError);
            pWinHttpCloseHandle(hRequest);
            pWinHttpCloseHandle(hConnect);
            CloseSession();
            return;
        }

        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        typedef BOOL
            (WINAPI*
            _WinHttpQueryHeaders)
            (
                IN     HINTERNET hRequest,
                IN     DWORD     dwInfoLevel,
                IN     LPCWSTR   pwszName OPTIONAL,
                _Out_writes_bytes_to_opt_(*lpdwBufferLength, *lpdwBufferLength) __out_data_source(NETWORK) LPVOID lpBuffer,
                IN OUT LPDWORD   lpdwBufferLength,
                IN OUT LPDWORD   lpdwIndex OPTIONAL
            );
        constexpr unsigned int hashWinHttpQueryHeaders = ComplexHashForAnsi("WinHttpQueryHeaders");
        _WinHttpQueryHeaders pWinHttpQueryHeaders = (_WinHttpQueryHeaders)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpQueryHeaders);
        pWinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusCodeSize,
            WINHTTP_NO_HEADER_INDEX);

        response->statusCode = statusCode;
        GetStatusText(statusCode, response->statusText, 64);

        const DWORD MAX_HEADERS = 50;
        response->headers = (HttpHeader*)crt_malloc(sizeof(HttpHeader) * MAX_HEADERS);
        if (response->headers) {
            crt_memset(response->headers, 0, sizeof(HttpHeader) * MAX_HEADERS);
            response->headerCount = 0;

            DWORD headerIndex = 0;
            wchar_t headerBuffer[1024];
            crt_memset(headerBuffer, 0, 1024);
            DWORD headerBufferSize = sizeof(headerBuffer);
            typedef BOOL
                (WINAPI*
                _WinHttpQueryHeaders)
                (
                    IN     HINTERNET hRequest,
                    IN     DWORD     dwInfoLevel,
                    IN     LPCWSTR   pwszName OPTIONAL,
                    _Out_writes_bytes_to_opt_(*lpdwBufferLength, *lpdwBufferLength) __out_data_source(NETWORK) LPVOID lpBuffer,
                    IN OUT LPDWORD   lpdwBufferLength,
                    IN OUT LPDWORD   lpdwIndex OPTIONAL
                );
            constexpr unsigned int hashWinHttpQueryHeaders = ComplexHashForAnsi("WinHttpQueryHeaders");
            _WinHttpQueryHeaders pWinHttpQueryHeaders = (_WinHttpQueryHeaders)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpQueryHeaders);
            while (response->headerCount < MAX_HEADERS &&
                pWinHttpQueryHeaders(hRequest,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    NULL,
                    headerBuffer,
                    &headerBufferSize,
                    &headerIndex)) {
                wchar_t* colon = crt_wcschr(headerBuffer, L':');
                if (colon) {
                    size_t nameLen = colon - headerBuffer;
                    if (nameLen < 255) {
                        crt_wcsncpy(response->headers[response->headerCount].name, headerBuffer, nameLen);
                        response->headers[response->headerCount].name[nameLen] = L'\0';

                        wchar_t* valueStart = colon + 1;
                        while (*valueStart == L' ' || *valueStart == L'\t') valueStart++;
                        crt_wcsncpy(response->headers[response->headerCount].value, valueStart, 511);
                        response->headers[response->headerCount].value[511] = L'\0';

                        response->headerCount++;
                    }
                }
                headerBufferSize = sizeof(headerBuffer);
                crt_memset(headerBuffer, 0, sizeof(headerBuffer));
            }
        }

        wchar_t* bodyBuffer = (wchar_t*)crt_malloc(1024 * 1024);
        DWORD bodySize = 0;
        DWORD totalSize = 0;

        if (bodyBuffer) {
            crt_memset(bodyBuffer, 0, 1024 * 1024);
            typedef BOOL
                (WINAPI*
                _WinHttpQueryDataAvailable)
                (
                    IN HINTERNET hRequest,
                    __out_data_source(NETWORK) LPDWORD lpdwNumberOfBytesAvailable
                );
            constexpr unsigned int hashWinHttpQueryDataAvailable = ComplexHashForAnsi("WinHttpQueryDataAvailable");
            _WinHttpQueryDataAvailable pWinHttpQueryDataAvailable = (_WinHttpQueryDataAvailable)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpQueryDataAvailable);
            typedef BOOL
                (WINAPI*
                _WinHttpReadData)
                (
                    IN HINTERNET hRequest,
                    _Out_writes_bytes_to_(dwNumberOfBytesToRead, *lpdwNumberOfBytesRead) __out_data_source(NETWORK) LPVOID lpBuffer,
                    IN DWORD dwNumberOfBytesToRead,
                    OUT LPDWORD lpdwNumberOfBytesRead
                );
            constexpr unsigned int hashWinHttpReadData = ComplexHashForAnsi("WinHttpReadData");
            _WinHttpReadData pWinHttpReadData = (_WinHttpReadData)apiResolve.GetApiAddress(lpWinHttp, hashWinHttpReadData);
            do {
                DWORD dwSize = 0;
                if (!pWinHttpQueryDataAvailable(hRequest, &dwSize)) {
                    break;
                }

                if (dwSize == 0) {
                    break;
                }

                DWORD dwDownloaded = 0;
                char chunk[4096];
                crt_memset(chunk, 0, 4096);
                if (dwSize > 4095) dwSize = 4095;

                if (pWinHttpReadData(hRequest, chunk, dwSize, &dwDownloaded)) {
                    if (totalSize + dwDownloaded < 1024 * 1024 - 1) {
                        for (DWORD i = 0; i < dwDownloaded; i++) {
                            bodyBuffer[totalSize++] = (wchar_t)chunk[i];
                        }
                    }
                }
            } while (TRUE);

            bodyBuffer[totalSize] = L'\0';
            response->body = bodyBuffer;
            response->bodyLength = (DWORD)totalSize * sizeof(wchar_t);
        }

        response->success = TRUE;
        response->errorCode = ERROR_SUCCESS;

        pWinHttpCloseHandle(hRequest);
        pWinHttpCloseHandle(hConnect);
    }

    void HttpClient::SendRequest(HttpResponse* response, const HttpRequest* request) {
        if (!response || !request) return;
        SendRequestInternal(response, request->method, request->url,
            request->body, request->bodyLength,
            request->headers, request->headerCount);
    }

    void HttpClient::Get(HttpResponse* response, const wchar_t* url) {
        SendRequestInternal(response, HTTP_GET, url, NULL, 0, NULL, 0);
    }

    void HttpClient::Get(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount) {
        SendRequestInternal(response, HTTP_GET, url, NULL, 0, headers, headerCount);
    }

    void HttpClient::Post(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen) {
        SendRequestInternal(response, HTTP_POST, url, body, bodyLen, NULL, 0);
    }

    void HttpClient::Post(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen,
        const HttpHeader* headers, int headerCount) {
        SendRequestInternal(response, HTTP_POST, url, body, bodyLen, headers, headerCount);
    }

    void HttpClient::Put(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen) {
        SendRequestInternal(response, HTTP_PUT, url, body, bodyLen, NULL, 0);
    }

    void HttpClient::Put(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen,
        const HttpHeader* headers, int headerCount) {
        SendRequestInternal(response, HTTP_PUT, url, body, bodyLen, headers, headerCount);
    }

    void HttpClient::Delete(HttpResponse* response, const wchar_t* url) {
        SendRequestInternal(response, HTTP_DEL, url, NULL, 0, NULL, 0);
    }

    void HttpClient::Delete(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount) {
        SendRequestInternal(response, HTTP_DEL, url, NULL, 0, headers, headerCount);
    }

    void HttpClient::Patch(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen) {
        SendRequestInternal(response, HTTP_PATCH, url, body, bodyLen, NULL, 0);
    }

    void HttpClient::Patch(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen,
        const HttpHeader* headers, int headerCount) {
        SendRequestInternal(response, HTTP_PATCH, url, body, bodyLen, headers, headerCount);
    }

    void HttpClient::Head(HttpResponse* response, const wchar_t* url) {
        SendRequestInternal(response, HTTP_HEAD, url, NULL, 0, NULL, 0);
    }

    void HttpClient::Head(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount) {
        SendRequestInternal(response, HTTP_HEAD, url, NULL, 0, headers, headerCount);
    }

    void HttpClient::Options(HttpResponse* response, const wchar_t* url) {
        SendRequestInternal(response, HTTP_OPTIONS, url, NULL, 0, NULL, 0);
    }

    void HttpClient::Options(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount) {
        SendRequestInternal(response, HTTP_OPTIONS, url, NULL, 0, headers, headerCount);
    }

    void HttpClient::UrlEncode(const wchar_t* input, wchar_t* output, int outputSize) {
        if (!input || !output || outputSize < (int)crt_wcslen(input) * 3 + 1) return;

        int outIdx = 0;
        for (int i = 0; input[i] != L'\0' && outIdx < outputSize - 4; i++) {
            wchar_t c = input[i];
            if (crt_iswalnum(c) || c == L'-' || c == L'_' || c == L'.' || c == L'~') {
                output[outIdx++] = c;
            }
            else if (c == L' ') {
                output[outIdx++] = L'+';
            }
            else {
                wchar_t pattern[] = {L'%', L'%', L'%', L'0', L'2', L'X', L'\0'};
                crt_swprintf(output + outIdx, outputSize - outIdx, pattern, (unsigned char)c);
                outIdx += 3;
            }
        }
        output[outIdx] = L'\0';
    }

    void HttpClient::UrlDecode(const wchar_t* input, wchar_t* output, int outputSize) {
        if (!input || !output || outputSize < 1) return;

        int outIdx = 0;
        for (int i = 0; input[i] != L'\0' && outIdx < outputSize - 1; i++) {
            if (input[i] == L'%' && input[i + 1] != L'\0' && input[i + 2] != L'\0') {
                wchar_t hex[3] = { input[i + 1], input[i + 2], L'\0' };
                wchar_t* endPtr;
                unsigned long code = crt_wcstoul(hex, &endPtr, 16);
                if (endPtr == hex + 2) {
                    output[outIdx++] = (wchar_t)code;
                    i += 2;
                }
                else {
                    output[outIdx++] = input[i];
                }
            }
            else if (input[i] == L'+') {
                output[outIdx++] = L' ';
            }
            else {
                output[outIdx++] = input[i];
            }
        }
        output[outIdx] = L'\0';
    }

}
