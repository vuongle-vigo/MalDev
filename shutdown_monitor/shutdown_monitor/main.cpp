#include <windows.h>
#include <stdio.h>

typedef struct {
    void (*on_event)(BOOL is_shutdown, void* user); // TRUE = shutdown, FALSE = sleep
    void* user;
} ShutdownWatch;

static LRESULT WINAPI WatchProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_NCCREATE) {                 // message dau tien, gan context vao cua so
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lp;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    ShutdownWatch* w = (ShutdownWatch*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_QUERYENDSESSION:
        printf("[watch] WM_QUERYENDSESSION -> dong y shutdown\n");
        return TRUE;                          // dong y shutdown

    case WM_ENDSESSION:
        if (wp && w && w->on_event) w->on_event(TRUE, w->user);
        return 0;

    case WM_POWERBROADCAST:
        if (wp == PBT_APMSUSPEND && w && w->on_event) w->on_event(FALSE, w->user);
        return TRUE;

    case WM_DESTROY:
        if (w) HeapFree(GetProcessHeap(), 0, w);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static DWORD WINAPI WatchThread(LPVOID param)
{
    ShutdownWatch* w = (ShutdownWatch*)param;

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(WNDCLASSW));
    wc.lpfnWndProc = WatchProc;
    wc.lpszClassName = L"ShutdownWatch";
    wc.hInstance = GetModuleHandleW(NULL);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPED,
        0, 0, 0, 0, NULL, NULL, wc.hInstance, w);
    if (!hwnd) { HeapFree(GetProcessHeap(), 0, w); return 1; }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
        DispatchMessageW(&msg);
    return 0;
}

BOOL StartShutdownWatch(void (*on_event)(BOOL, void*), void* user)
{
    ShutdownWatch* w = (ShutdownWatch*)HeapAlloc(GetProcessHeap(), 0, sizeof * w);
    if (!w) return FALSE;
    w->on_event = on_event;
    w->user = user;

    HANDLE t = CreateThread(NULL, 0, WatchThread, w, 0, NULL);
    if (!t) { HeapFree(GetProcessHeap(), 0, w); return FALSE; }
    CloseHandle(t);
    return TRUE;
}

// ---- Vi du su dung: thay bang du lieu that cua app ban ----

typedef struct MyAppState {
    int dummy;
} MyAppState;

static void SaveState(MyAppState* s)
{
    (void)s;
    printf("[callback] MAY DANG SHUTDOWN - luu du lieu ngay!\n");
}

static void PauseWork(MyAppState* s)
{
    (void)s;
    printf("[callback] MAY SAP SLEEP - tam dung cong viec\n");
}

static void MyCallback(BOOL is_shutdown, void* user)
{
    MyAppState* state = (MyAppState*)user;
    if (is_shutdown) SaveState(state);
    else             PauseWork(state);
}

int main(void)
{
    MyAppState state = { 0 };

    if (!StartShutdownWatch(MyCallback, &state)) {
        printf("StartShutdownWatch that bai\n");
        return 1;
    }

    printf("Dang theo doi shutdown/sleep. Nhan Enter de thoat...\n");
    getchar();   // BUOC PHAI CO: giu process song, khong la watcher thread chet ngay
    return 0;
}
