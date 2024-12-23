#include "keyboard_checker.h"
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <locale>
#include <codecvt>
#include "logger.h"

// Link with Common Controls library
#pragma comment(lib, "comctl32.lib")

// Initialize static members
KeyboardChecker* KeyboardChecker::s_instance = nullptr;
const UINT KeyboardChecker::WM_UPDATE_TEXT;
const UINT KeyboardChecker::WM_CHECK_LAYOUT;

/**
 * @brief Constructor - initializes member variables and keyboard layouts
 */
KeyboardChecker::KeyboardChecker() 
{
    FUNCTION_START;
    s_instance = this;           // Set static instance for callbacks
    
    // Initialize member variables
    m_mainWindow = nullptr;
    m_popup = nullptr;
    m_keyboardHook = nullptr;
    m_minTextLength = 5;
    m_isRunning = false;
    
    InitializeLayouts();         // Get available keyboard layouts
    LOG(LOG_INF, L"Keyboard checker initialized");
}

/**
 * @brief Destructor - ensures cleanup
 */
KeyboardChecker::~KeyboardChecker() {
    FUNCTION_START;
    Stop();                      // Stop monitoring and cleanup
    s_instance = nullptr;        // Clear static instance
    LOG(LOG_INF, L"Keyboard checker destroyed");
}

/**
 * @brief Get all available keyboard layouts from the system
 */
void KeyboardChecker::InitializeLayouts() {
    FUNCTION_START;
    
    // Clear any existing layouts
    m_availableLayouts.clear();
    m_layoutNames.clear();
    
    // Get number of keyboard layouts
    int layoutCount = GetKeyboardLayoutList(0, nullptr);
    if (layoutCount <= 0) {
        LOG(LOG_ERR, L"No keyboard layouts found");
        return;
    }
    
    // Get actual layout handles
    std::vector<HKL> layouts(layoutCount);
    GetKeyboardLayoutList(layoutCount, reinterpret_cast<HKL*>(layouts.data()));
    
    // Filter out duplicates and store unique layouts
    std::set<std::wstring> uniqueLayoutNames;
    for (HKL layout : layouts) {
        wchar_t name[KL_NAMELENGTH];
        if (GetKeyboardLayoutNameW(name)) {
            std::wstring layoutName = name;
            if (uniqueLayoutNames.insert(layoutName).second) {
                m_availableLayouts.push_back(layout);
                m_layoutNames[layout] = layoutName;
                LOG(LOG_INF, L"Registered layout: " + layoutName);
            }
        }
    }
    
    LOG(LOG_INF, L"Initialized " + std::to_wstring(m_availableLayouts.size()) + L" unique keyboard layouts");
}

/**
 * @brief Create and initialize program windows
 * 
 * Creates two windows:
 * 1. Hidden main window for message processing
 * 2. Popup window for displaying suggestions
 */
bool KeyboardChecker::InitializeWindow() {
    FUNCTION_START;
    
    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;                // Window procedure
    wc.hInstance = GetModuleHandle(nullptr);  // Program instance
    wc.lpszClassName = L"KeyboardCheckerClass";     // Class name for window creation
    
    // Register the window class
    if (!RegisterClassExW(&wc)) {
        LOG(LOG_ERR, L"Failed to register window class");
        return false;
    }
    
    // Create hidden main window
    m_mainWindow = CreateWindowExW(
        0,                          // No extended styles
        L"KeyboardCheckerClass",          // Window class
        L"Keyboard Checker",        // Window title
        WS_OVERLAPPEDWINDOW,        // Standard window style
        CW_USEDEFAULT, CW_USEDEFAULT, // Default position
        400, 300,                   // Size
        nullptr,                    // No parent window
        nullptr,                    // No menu
        GetModuleHandle(nullptr),   // Program instance
        nullptr                     // No extra data
    );
    
    if (!m_mainWindow) {
        LOG(LOG_ERR, L"Failed to create main window");
        return false;
    }
    
    LOG(LOG_INF, L"Created main window");
    
    // Create popup window for suggestions
    m_popup = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,  // Always on top, no taskbar button
        L"KeyboardCheckerClass",                  // Same window class
        L"Layout Suggestions",              // Window title
        WS_POPUP | WS_BORDER,              // Popup style with border
        CW_USEDEFAULT, CW_USEDEFAULT,      // Default position
        300, 200,                          // Initial size
        nullptr,                           // No parent
        nullptr,                           // No menu
        GetModuleHandle(nullptr),          // Program instance
        nullptr                            // No extra data
    );
    
    if (!m_popup) {
        LOG(LOG_ERR, L"Failed to create popup window");
        DestroyWindow(m_mainWindow);
        m_mainWindow = nullptr;
        return false;
    }
    
    LOG(LOG_INF, L"Created popup window");
    return true;
}

/**
 * @brief Start keyboard monitoring
 * 
 * Sets up keyboard hook and enters message loop
 */
void KeyboardChecker::Start() {
    FUNCTION_START;
    
    if (m_isRunning) {
        LOG(LOG_WRN, L"Already running");
        return;
    }
    
    if (!InitializeWindow()) {
        LOG(LOG_ERR, L"Failed to initialize windows");
        return;
    }
    
    // Install keyboard hook
    m_keyboardHook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandle(nullptr),
        0
    );
    
    if (!m_keyboardHook) {
        LOG(LOG_ERR, L"Failed to install keyboard hook");
        DestroyWindow(m_popup);
        DestroyWindow(m_mainWindow);
        m_popup = m_mainWindow = nullptr;
        return;
    }
    
    m_isRunning = true;
    LOG(LOG_INF, L"Started successfully");
    
    // Message loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

/**
 * @brief Stop keyboard monitoring and cleanup
 */
void KeyboardChecker::Stop() {
    FUNCTION_START;
    
    if (!m_isRunning) {
        LOG(LOG_INF, L"Already stopped");
        return;
    }
    
    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
        LOG(LOG_INF, L"Unhooked keyboard hook");
    }
    
    if (m_popup) {
        DestroyWindow(m_popup);
        m_popup = nullptr;
        LOG(LOG_INF, L"Destroyed popup window");
    }
    
    if (m_mainWindow) {
        DestroyWindow(m_mainWindow);
        m_mainWindow = nullptr;
        LOG(LOG_INF, L"Destroyed main window");
    }
    
    m_isRunning = false;
}

/**
 * @brief Window procedure for handling window messages
 */
LRESULT CALLBACK KeyboardChecker::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    FUNCTION_START;
    
    if (s_instance) {
        switch (msg) {
            case WM_DESTROY:
                LOG(LOG_INF, L"Window destroyed");
                PostQuitMessage(0);  // Exit message loop
                return 0;
                
            case WM_PAINT:
                if (hwnd == s_instance->m_popup) {
                    LOG(LOG_INF, L"Painting popup window");
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hwnd, &ps);
                    
                    // Set up DC for text drawing
                    SetBkMode(hdc, TRANSPARENT);
                    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
                    
                    // Get client area
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    
                    // Fill background
                    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
                    FillRect(hdc, &rect, hBrush);
                    DeleteObject(hBrush);
                    
                    // Draw window text
                    int len = GetWindowTextLengthW(hwnd);
                    if (len > 0) {
                        std::vector<wchar_t> text(len + 1);
                        GetWindowTextW(hwnd, text.data(), len + 1);
                        DrawTextW(hdc, text.data(), -1, &rect, DT_WORDBREAK);
                    }
                    
                    EndPaint(hwnd, &ps);
                    return 0;
                }
                break;
                
            case WM_UPDATE_TEXT:
                LOG(LOG_INF, L"Updating text");
                // Handle text update
                return 0;
                
            case WM_CHECK_LAYOUT:
                LOG(LOG_INF, L"Checking layout");
                // Check layout and show popup if needed
                return 0;
        }
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/**
 * @brief Keyboard hook procedure for processing keystrokes
 */
LRESULT CALLBACK KeyboardChecker::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && s_instance) {
        KBDLLHOOKSTRUCT* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            bool textChanged = false;
            
            // Handle key press
            if (kbd->vkCode == VK_BACK) {  // Backspace
                if (!s_instance->m_currentText.empty()) {
                    s_instance->m_currentText.pop_back();
                    textChanged = true;
                    LOG(LOG_INF, L"Backspace pressed, current text: " + s_instance->m_currentText);
                }
            }
            else if (!(kbd->flags & LLKHF_INJECTED)) {  // Only handle real keystrokes
                wchar_t ch = s_instance->GetCharForKey(kbd->vkCode, GetKeyboardLayout(0));
                if (ch && !iswcntrl(ch)) {  // Ignore control characters
                    s_instance->m_currentText += ch;
                    textChanged = true;
                    LOG(LOG_INF, L"Key pressed: " + std::wstring(1, ch) + L", current text: " + s_instance->m_currentText);
                }
            }
            
            // Update popup if text changed
            if (textChanged) {
                HKL currentLayout = GetKeyboardLayout(0);
                wchar_t layoutName[KL_NAMELENGTH];
                GetKeyboardLayoutNameW(layoutName);
                LOG(LOG_INF, L"Current layout: " + std::wstring(layoutName));
                
                std::unordered_map<HKL, std::wstring> conversions;
                for (HKL layout : s_instance->m_availableLayouts) {
                    if (layout != currentLayout) {
                        std::wstring converted = s_instance->ConvertText(
                            s_instance->m_currentText, currentLayout, layout);
                        if (!converted.empty()) {
                            conversions[layout] = converted;
                            LOG(LOG_INF, L"Converted to layout " + 
                                s_instance->m_layoutNames[layout] + L": " + converted);
                        }
                    }
                }
                
                s_instance->UpdatePopup(s_instance->m_currentText, conversions);
            }
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

/**
 * @brief Convert a virtual key to character in specific layout
 */
wchar_t KeyboardChecker::GetCharForKey(UINT vk, HKL layout) {
    FUNCTION_START;
    
    BYTE keyState[256] = {0};
    wchar_t buff[10] = {0};
    
    // Get scan code for virtual key
    UINT scanCode = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, layout);
    if (!scanCode) return 0;
    
    // Convert to unicode character
    int result = ToUnicode(
        vk,
        scanCode,
        keyState,
        buff,
        10,
        0
    );
    
    if (result == 1) {
        LOG(LOG_INF, L"Mapped virtual key " + std::to_wstring(vk) + L" to: " + std::wstring(1, buff[0]));
        return buff[0];
    }
    
    return 0;
}

/**
 * @brief Convert text between keyboard layouts
 */
std::wstring KeyboardChecker::ConvertText(const std::wstring& text, HKL fromLayout, HKL toLayout) {
    FUNCTION_START;
    
    std::wstring result;
    result.reserve(text.length());
    
    for (wchar_t ch : text) {
        // Get virtual key code for character in source layout
        SHORT vk = VkKeyScanExW(ch, fromLayout);
        if (vk != -1) {
            // Get character for this virtual key in target layout
            wchar_t converted = GetCharForKey(LOBYTE(vk), toLayout);
            if (converted) {
                result += converted;
            }
        }
    }
    
    LOG(LOG_INF, L"Converted text from " + m_layoutNames[fromLayout] + 
             L" to " + m_layoutNames[toLayout] + L": " + result);
    return result;
}

/**
 * @brief Check if text is valid in given layout
 */
bool KeyboardChecker::IsValidInLayout(const std::wstring& text, HKL layout) {
    FUNCTION_START;
    
    for (wchar_t ch : text) {
        if (VkKeyScanExW(ch, layout) == -1) {
            LOG(LOG_WRN, L"Character '" + std::wstring(1, ch) + L"' is not valid in layout");
            return false;
        }
    }
    
    LOG(LOG_INF, L"Text is valid in layout " + m_layoutNames[layout]);
    return true;
}

/**
 * @brief Update popup window with current text and suggestions
 */
void KeyboardChecker::UpdatePopup(const std::wstring& currentText, 
                                const std::unordered_map<HKL, std::wstring>& conversions) {
    FUNCTION_START;
    
    if (currentText.length() < m_minTextLength || conversions.empty()) {
        ShowWindow(m_popup, SW_HIDE);
        LOG(LOG_INF, L"Hiding popup (text too short or no conversions)");
        return;
    }

    // Get cursor position and screen dimensions
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    // Get monitor info for proper window placement
    MONITORINFO mi = { sizeof(MONITORINFO) };
    HMONITOR hMonitor = MonitorFromPoint(cursorPos, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hMonitor, &mi);
    RECT workArea = mi.rcWork;

    // Create text to display
    std::wstring displayText = L"Current text:\n" + currentText + L"\n\nSuggested conversions:\n";
    for (const auto& [layout, text] : conversions) {
        wchar_t layoutName[KL_NAMELENGTH];
        GetKeyboardLayoutNameW(layoutName);
        displayText += L"• " + text + L"\n";
    }

    // Calculate window size
    RECT rect = {0, 0, 400, 0}; // Fixed width of 400 pixels
    HDC hdc = GetDC(m_popup);
    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    DrawTextW(hdc, displayText.c_str(), -1, &rect, DT_CALCRECT | DT_WORDBREAK);
    ReleaseDC(m_popup, hdc);

    // Add padding to window size
    int width = rect.right + 40;   // 20px padding on each side
    int height = rect.bottom + 40;  // 20px padding on top and bottom

    // Calculate window position
    int x = cursorPos.x;
    int y = cursorPos.y + 20;  // Small offset from cursor

    // Keep window on screen
    if (x + width > workArea.right) {
        x = workArea.right - width;
    }
    if (y + height > workArea.bottom) {
        y = cursorPos.y - height - 10;  // Show above cursor
    }
    if (x < workArea.left) x = workArea.left;
    if (y < workArea.top) y = workArea.top;

    // Set window style for better appearance
    LONG_PTR style = GetWindowLongPtr(m_popup, GWL_STYLE);
    style |= WS_POPUP | WS_BORDER;
    style &= ~(WS_CAPTION | WS_THICKFRAME);
    SetWindowLongPtr(m_popup, GWL_STYLE, style);

    // Set extended window style
    LONG_PTR exStyle = GetWindowLongPtr(m_popup, GWL_EXSTYLE);
    exStyle |= WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    SetWindowLongPtr(m_popup, GWL_EXSTYLE, exStyle);

    // Set window transparency (94% opaque)
    SetLayeredWindowAttributes(m_popup, 0, 240, LWA_ALPHA);

    // Update window content and position
    SetWindowTextW(m_popup, displayText.c_str());
    SetWindowPos(m_popup, HWND_TOPMOST, x, y, width, height, 
                SWP_SHOWWINDOW | SWP_NOACTIVATE);

    // Force window to repaint
    InvalidateRect(m_popup, nullptr, TRUE);
    UpdateWindow(m_popup);
    LOG(LOG_INF, L"Updated and showed popup");
}
