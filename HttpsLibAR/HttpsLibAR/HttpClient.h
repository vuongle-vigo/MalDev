#pragma once

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

namespace HttpLib {

    enum HttpMethod {
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_DEL,
        HTTP_PATCH,
        HTTP_HEAD,
        HTTP_OPTIONS
    };

    enum HttpScheme {
        HTTP_SCHEME_HTTP,
        HTTP_SCHEME_HTTPS
    };

    struct HttpHeader {
        wchar_t name[256];
        wchar_t value[512];
    };

    struct HttpRequest {
        HttpMethod method;
        wchar_t url[2048];
        HttpHeader* headers;
        int headerCount;
        wchar_t* body;
        DWORD bodyLength;
        BOOL bodyAllocated;
        DWORD timeout;
        BOOL followRedirects;
        BOOL verifySsl;
    };

    struct HttpResponse {
        DWORD statusCode;
        wchar_t statusText[64];
        wchar_t* body;
        DWORD bodyLength;
        HttpHeader* headers;
        int headerCount;
        BOOL success;
        DWORD errorCode;
        wchar_t errorMessage[512];
    };

    void HttpRequest_Init(HttpRequest* req);
    void HttpRequest_Free(HttpRequest* req);
    void HttpRequest_AddHeader(HttpRequest* req, const wchar_t* name, const wchar_t* value);
    void HttpResponse_Init(HttpResponse* resp);
    void HttpResponse_Free(HttpResponse* resp);

#pragma comment(lib, "winhttp.lib")

    class HttpClient {
    public:
        static HttpClient& GetInstance();

        HttpClient();
        ~HttpClient();

        HttpClient(const HttpClient&) = delete;
        HttpClient& operator=(const HttpClient&) = delete;

        void SetUserAgent(const wchar_t* userAgent);
        void SetTimeout(DWORD timeoutMs);
        void SetFollowRedirects(BOOL follow);
        void SetVerifySsl(BOOL verify);
        void SetProxy(const wchar_t* proxyUrl);
        void ClearProxy();

        void Get(HttpResponse* response, const wchar_t* url);
        void Get(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount);

        void Post(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen);
        void Post(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen, const HttpHeader* headers, int headerCount);

        void Put(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen);
        void Put(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen, const HttpHeader* headers, int headerCount);

        void Delete(HttpResponse* response, const wchar_t* url);
        void Delete(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount);

        void Patch(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen);
        void Patch(HttpResponse* response, const wchar_t* url, const wchar_t* body, DWORD bodyLen, const HttpHeader* headers, int headerCount);

        void Head(HttpResponse* response, const wchar_t* url);
        void Head(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount);

        void Options(HttpResponse* response, const wchar_t* url);
        void Options(HttpResponse* response, const wchar_t* url, const HttpHeader* headers, int headerCount);

        void SendRequest(HttpResponse* response, const HttpRequest* request);

        static void UrlEncode(const wchar_t* input, wchar_t* output, int outputSize);
        static void UrlDecode(const wchar_t* input, wchar_t* output, int outputSize);

    private:
        void SendRequestInternal(HttpResponse* response, HttpMethod method, const wchar_t* url,
            const wchar_t* body, DWORD bodyLen, const HttpHeader* headers, int headerCount);

        BOOL ParseUrl(const wchar_t* url, wchar_t* host, int hostSize, wchar_t* path, int pathSize,
            INTERNET_PORT* port, HttpScheme* scheme);

        HINTERNET OpenSession();
        void CloseSession();
        void ApplySslIgnoreOption(HINTERNET hRequest);

        HINTERNET m_hSession;
        wchar_t m_userAgent[256];
        DWORD m_timeout;
        BOOL m_followRedirects;
        BOOL m_verifySsl;
        wchar_t m_proxyUrl[512];
        BOOL m_useProxy;
        CRITICAL_SECTION m_cs;
    };

}
