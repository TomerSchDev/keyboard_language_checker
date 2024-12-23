#include "keyboard_checker.h"
#include "logger.h"
#include <iostream>
#include <windows.h>

int main() {
    FUNCTION_START;
    
    try {
        // Check if we're running in console or background
        bool hasConsole = GetConsoleWindow() != nullptr;
        
        if (!hasConsole) {
            // Create logs directory if it doesn't exist
            CreateDirectoryW(L"logs", nullptr);
            LOG(LOG_INF, L"Starting in background mode");
        } else {
            LOG(LOG_INF, L"Starting in console mode");
        }
        
        KeyboardChecker checker;
        checker.Start();
        
        LOG(LOG_INF, L"Application exiting normally");
        return 0;
    }
    catch (const std::exception& e) {
        LOG(LOG_ERR, std::wstring(e.what(), e.what() + strlen(e.what())));
        return 1;
    }
}
