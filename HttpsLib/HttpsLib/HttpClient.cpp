#define _CRT_SECURE_NO_WARNINGS

#include "HttpClient.h"
#include <stdlib.h>
#include <wchar.h>

namespace HttpLib {

void HttpRequest_Init(HttpRequest* req) {
    if (!req) return;
    memset(req, 0, sizeof(HttpRequest));
    req->timeout = 30000;
    req->followRedirects = TRUE;
    req->verifySsl = TRUE;
    req->bodyAllocated = FALSE;
}

void HttpRequest_Free(HttpRequest* req) {
    if (!req) return;
    if (req->headers) {
        free(req->headers);
        req->headers = NULL;
        req->headerCount = 0;
    }
    if (req->body && req->bodyAllocated) {
        free(req->body);
        req->body = NULL;
        req->bodyLength = 0;
        req->bodyAllocated = FALSE;
    }
}

void HttpRequest_AddHeader(HttpRequest* req, const wchar_t* name, const wchar_t* value) {
    if (!req || !name || !value) return;

    HttpHeader* newHeaders = (HttpHeader*)realloc(req->headers, sizeof(HttpHeader) * (req->headerCount + 1));
    if (!newHeaders) return;

    req->headers = newHeaders;
    wcsncpy(req->headers[req->headerCount].name, name, 255);
    req->headers[req->headerCount].name[255] = L'\0';
    wcsncpy(req->headers[req->headerCount].value, value, 511);
    req->headers[req->headerCount].value[511] = L'\0';
    req->headerCount++;
}

void HttpResponse_Init(HttpResponse* resp) {
    if (!resp) return;
    memset(resp, 0, sizeof(HttpResponse));
}

void HttpResponse_Free(HttpResponse* resp) {
    if (!resp) return;
    if (resp->body) {
        free(resp->body);
        resp->body = NULL;
    }
    if (resp->headers) {
        free(resp->headers);
        resp->headers = NULL;
    }
    resp->bodyLength = 0;
    resp->headerCount = 0;
    resp->statusCode = 0;
    resp->success = FALSE;
    resp->errorCode = 0;
}

static const wchar_t* HttpMethodToString(HttpMethod method) {
    switch (method) {
        case HTTP_GET: return L"GET";
        case HTTP_POST: return L"POST";
        case HTTP_PUT: return L"PUT";
        case HTTP_DEL: return L"DELETE";
        case HTTP_PATCH: return L"PATCH";
        case HTTP_HEAD: return L"HEAD";
        case HTTP_OPTIONS: return L"OPTIONS";
        default: return L"GET";
    }
}

static void GetStatusText(DWORD statusCode, wchar_t* buffer, int bufferSize) {
    const wchar_t* text = L"Unknown";
    switch (statusCode) {
        case 100: text = L"Continue"; break;
        case 101: text = L"Switching Protocols"; break;
        case 200: text = L"OK"; break;
        case 201: text = L"Created"; break;
        case 204: text = L"No Content"; break;
        case 301: text = L"Moved Permanently"; break;
        case 302: text = L"Found"; break;
        case 400: text = L"Bad Request"; break;
        case 401: text = L"Unauthorized"; break;
        case 403: text = L"Forbidden"; break;
        case 404: text = L"Not Found"; break;
        case 500: text = L"Internal Server Error"; break;
        case 502: text = L"Bad Gateway"; break;
        case 503: text = L"Service Unavailable"; break;
        default: text = L"Unknown";
    }
    wcsncpy(buffer, text, bufferSize - 1);
    buffer[bufferSize - 1] = L'\0';
}

HttpClient& HttpClient::GetInstance() {
    static HttpClient instance;
    return instance;
}

HttpClient::HttpClient() {
    InitializeCriticalSection(&m_cs);
    m_hSession = NULL;
    wcscpy(m_userAgent, L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    m_timeout = 30000;
    m_followRedirects = TRUE;
    m_verifySsl = TRUE;
    m_useProxy = FALSE;
    m_proxyUrl[0] = L'\0';
}

HttpClient::~HttpClient() {
    CloseSession();
    DeleteCriticalSection(&m_cs);
}

void HttpClient::SetUserAgent(const wchar_t* userAgent) {
    if (!userAgent) return;
    EnterCriticalSection(&m_cs);
    wcsncpy(m_userAgent, userAgent, 255);
    m_userAgent[255] = L'\0';
    LeaveCriticalSection(&m_cs);
}

void HttpClient::SetTimeout(DWORD timeoutMs) {
    EnterCriticalSection(&m_cs);
    m_timeout = timeoutMs;
    LeaveCriticalSection(&m_cs);
}

void HttpClient::SetFollowRedirects(BOOL follow) {
    EnterCriticalSection(&m_cs);
    m_followRedirects = follow;
    LeaveCriticalSection(&m_cs);
}

void HttpClient::SetVerifySsl(BOOL verify) {
    EnterCriticalSection(&m_cs);
    m_verifySsl = verify;
    LeaveCriticalSection(&m_cs);
}

void HttpClient::SetProxy(const wchar_t* proxyUrl) {
    EnterCriticalSection(&m_cs);
    if (proxyUrl) {
        wcsncpy(m_proxyUrl, proxyUrl, 511);
        m_proxyUrl[511] = L'\0';
        m_useProxy = TRUE;
    } else {
        m_proxyUrl[0] = L'\0';
        m_useProxy = FALSE;
    }
    LeaveCriticalSection(&m_cs);
}

void HttpClient::ClearProxy() {
    EnterCriticalSection(&m_cs);
    m_proxyUrl[0] = L'\0';
    m_useProxy = FALSE;
    LeaveCriticalSection(&m_cs);
}

BOOL HttpClient::ParseUrl(const wchar_t* url, wchar_t* host, int hostSize, wchar_t* path, int pathSize,
                         INTERNET_PORT* port, HttpScheme* scheme) {
    if (!url || !host || !path || !port || !scheme) return FALSE;

    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostBuffer[256] = {};
    wchar_t pathBuffer[2048] = {};

    urlComp.lpszHostName = hostBuffer;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = pathBuffer;
    urlComp.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(url, (DWORD)wcslen(url), 0, &urlComp)) {
        return FALSE;
    }

    wcsncpy(host, hostBuffer, hostSize - 1);
    host[hostSize - 1] = L'\0';

    wcsncpy(path, pathBuffer, pathSize - 1);
    path[pathSize - 1] = L'\0';

    *port = urlComp.nPort;

    if (urlComp.nScheme == INTERNET_SCHEME_HTTP) {
        *scheme = HTTP_SCHEME_HTTP;
        if (*port == 0) *port = INTERNET_DEFAULT_HTTP_PORT;
    } else if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        *scheme = HTTP_SCHEME_HTTPS;
        if (*port == 0) *port = INTERNET_DEFAULT_HTTPS_PORT;
    } else {
        *scheme = HTTP_SCHEME_HTTPS;
        if (*port == 0) *port = INTERNET_DEFAULT_HTTPS_PORT;
    }

    if (path[0] == L'\0') {
        wcscpy(path, L"/");
    }

    return TRUE;
}

HINTERNET HttpClient::OpenSession() {
    EnterCriticalSection(&m_cs);

    if (m_hSession) {
        LeaveCriticalSection(&m_cs);
        return m_hSession;
    }

    DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    LPCWSTR proxyName = WINHTTP_NO_PROXY_NAME;
    LPCWSTR proxyBypass = WINHTTP_NO_PROXY_BYPASS;

    if (m_useProxy && m_proxyUrl[0] != L'\0') {
        accessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        proxyName = m_proxyUrl;
    }

    m_hSession = WinHttpOpen(m_userAgent, accessType, proxyName, proxyBypass, 0);

    if (m_hSession && m_timeout > 0) {
        WinHttpSetTimeouts(m_hSession, m_timeout, m_timeout, m_timeout, m_timeout);
    }

    LeaveCriticalSection(&m_cs);
    return m_hSession;
}

void HttpClient::CloseSession() {
    EnterCriticalSection(&m_cs);
    if (m_hSession) {
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }
    LeaveCriticalSection(&m_cs);
}

void HttpClient::ApplySslIgnoreOption(HINTERNET hRequest) {
    if (!hRequest) return;

    DWORD dwSecFlags = 0;
    DWORD dwBuffLen = sizeof(dwSecFlags);

    if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecFlags, &dwBuffLen)) {
        dwSecFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                      SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                      SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                      SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecFlags, sizeof(dwSecFlags));
    }
}

static char* WideToAnsi(const wchar_t* wideStr, DWORD* outLen) {
    if (!wideStr) {
        if (outLen) *outLen = 0;
        return NULL;
    }
    
    int len = WideCharToMultiByte(CP_ACP, 0, wideStr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) {
        if (outLen) *outLen = 0;
        return NULL;
    }
    
    char* result = (char*)malloc(len);
    if (!result) {
        if (outLen) *outLen = 0;
        return NULL;
    }
    
    WideCharToMultiByte(CP_ACP, 0, wideStr, -1, result, len, NULL, NULL);
    if (outLen) *outLen = (DWORD)(len - 1);
    return result;
}

void HttpClient::SendRequestInternal(HttpResponse* response, HttpMethod method, const wchar_t* url,
                                     const wchar_t* body, DWORD bodyLen, const HttpHeader* headers, int headerCount) {
    if (!response) return;
    HttpResponse_Init(response);
    response->success = FALSE;

    wchar_t host[256] = {};
    wchar_t path[2048] = {};
    INTERNET_PORT port = 0;
    HttpScheme scheme = HTTP_SCHEME_HTTPS;

    if (!ParseUrl(url, host, 256, path, 2048, &port, &scheme)) {
        response->errorCode = GetLastError();
        wcscpy(response->errorMessage, L"Failed to parse URL");
        return;
    }

    HINTERNET hSession = OpenSession();
    if (!hSession) {
        response->errorCode = GetLastError();
        wcscpy(response->errorMessage, L"Failed to open session");
        return;
    }

    DWORD flags = WINHTTP_FLAG_SECURE;
    if (scheme == HTTP_SCHEME_HTTP) {
        flags = 0;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
    if (!hConnect) {
        response->errorCode = GetLastError();
        wcscpy(response->errorMessage, L"Failed to connect");
        CloseSession();
        return;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        HttpMethodToString(method),
        path,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!hRequest) {
        response->errorCode = GetLastError();
        wcscpy(response->errorMessage, L"Failed to open request");
        WinHttpCloseHandle(hConnect);
        CloseSession();
        return;
    }

    if (!m_verifySsl) {
        ApplySslIgnoreOption(hRequest);
    }

    wchar_t headersStr[8192] = {};
    if (headers && headerCount > 0) {
        int offset = 0;
        for (int i = 0; i < headerCount && offset < 8000; i++) {
            int len = swprintf(headersStr + offset, 8192 - offset, L"%s: %s\r\n", headers[i].name, headers[i].value);
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

    if (!WinHttpSendRequest(hRequest,
        headersStr[0] ? headersStr : WINHTTP_NO_ADDITIONAL_HEADERS,
        headersStr[0] ? (DWORD)wcslen(headersStr) : 0,
        dataToSend,
        dataLen,
        dataLen,
        0)) {
        if (ansiBody) free(ansiBody);
        response->errorCode = GetLastError();
        wcscpy(response->errorMessage, L"Failed to send request");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        CloseSession();
        return;
    }

    if (ansiBody) free(ansiBody);

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        response->errorCode = GetLastError();
        wcscpy(response->errorMessage, L"Failed to receive response");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        CloseSession();
        return;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX);

    response->statusCode = statusCode;
    GetStatusText(statusCode, response->statusText, 64);

    const DWORD MAX_HEADERS = 50;
    response->headers = (HttpHeader*)malloc(sizeof(HttpHeader) * MAX_HEADERS);
    if (response->headers) {
        memset(response->headers, 0, sizeof(HttpHeader) * MAX_HEADERS);
        response->headerCount = 0;

        DWORD headerIndex = 0;
        wchar_t headerBuffer[1024] = {};
        DWORD headerBufferSize = sizeof(headerBuffer);

        while (response->headerCount < MAX_HEADERS &&
               WinHttpQueryHeaders(hRequest,
                   WINHTTP_HEADER_NAME_BY_INDEX,
                   NULL,
                   headerBuffer,
                   &headerBufferSize,
                   &headerIndex)) {
            wchar_t* colon = wcschr(headerBuffer, L':');
            if (colon) {
                size_t nameLen = colon - headerBuffer;
                if (nameLen < 255) {
                    wcsncpy(response->headers[response->headerCount].name, headerBuffer, nameLen);
                    response->headers[response->headerCount].name[nameLen] = L'\0';

                    wchar_t* valueStart = colon + 1;
                    while (*valueStart == L' ' || *valueStart == L'\t') valueStart++;
                    wcsncpy(response->headers[response->headerCount].value, valueStart, 511);
                    response->headers[response->headerCount].value[511] = L'\0';

                    response->headerCount++;
                }
            }
            headerBufferSize = sizeof(headerBuffer);
            memset(headerBuffer, 0, sizeof(headerBuffer));
        }
    }

    wchar_t* bodyBuffer = (wchar_t*)malloc(1024 * 1024);
    DWORD bodySize = 0;
    DWORD totalSize = 0;

    if (bodyBuffer) {
        memset(bodyBuffer, 0, 1024 * 1024);

        do {
            DWORD dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                break;
            }

            if (dwSize == 0) {
                break;
            }

            DWORD dwDownloaded = 0;
            char chunk[4096] = {};

            if (dwSize > 4095) dwSize = 4095;

            if (WinHttpReadData(hRequest, chunk, dwSize, &dwDownloaded)) {
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

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
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
    if (!input || !output || outputSize < (int)wcslen(input) * 3 + 1) return;

    int outIdx = 0;
    for (int i = 0; input[i] != L'\0' && outIdx < outputSize - 4; i++) {
        wchar_t c = input[i];
        if (iswalnum(c) || c == L'-' || c == L'_' || c == L'.' || c == L'~') {
            output[outIdx++] = c;
        } else if (c == L' ') {
            output[outIdx++] = L'+';
        } else {
            swprintf(output + outIdx, outputSize - outIdx, L"%%%02X", (unsigned char)c);
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
            wchar_t hex[3] = {input[i + 1], input[i + 2], L'\0'};
            wchar_t* endPtr;
            unsigned long code = wcstoul(hex, &endPtr, 16);
            if (endPtr == hex + 2) {
                output[outIdx++] = (wchar_t)code;
                i += 2;
            } else {
                output[outIdx++] = input[i];
            }
        } else if (input[i] == L'+') {
            output[outIdx++] = L' ';
        } else {
            output[outIdx++] = input[i];
        }
    }
    output[outIdx] = L'\0';
}

}
