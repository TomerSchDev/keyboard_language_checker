#pragma once

// Windows API headers
#include <windows.h>
// Standard C++ headers
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <set>
#include <functional>

// Custom headers
#include "logger.h"

/**
 * @class KeyboardChecker
 * @brief Main class for handling keyboard input and layout detection
 * 
 * This class monitors keyboard input in real-time, detects the current keyboard
 * layout, and provides suggestions for text conversion between different layouts
 * (e.g., Hebrew to English or vice versa).
 */
class KeyboardChecker {
public:
    // Constructor and destructor
    KeyboardChecker();
    ~KeyboardChecker();

    /**
     * @brief Start monitoring keyboard input
     * 
     * Initializes the keyboard hook, creates windows, and starts
     * the message loop for handling keyboard events.
     */
    void Start();
    
    /**
     * @brief Stop monitoring and cleanup resources
     * 
     * Removes keyboard hook, destroys windows, and cleans up
     * any allocated resources.
     */
    void Stop();

private:
    /**
     * @brief Initialize available keyboard layouts
     * 
     * Gets all keyboard layouts installed on the system and stores
     * their names and handles.
     */
    void InitializeLayouts();
    
    /**
     * @brief Initialize the main and popup windows
     * 
     * Creates and sets up the hidden main window and the popup window
     * used for displaying layout suggestions.
     * 
     * @return bool True if initialization successful
     */
    bool InitializeWindow();
    
    /**
     * @brief Check if text makes sense in given layout
     * 
     * @param text Text to check
     * @param layout Keyboard layout handle
     * @return bool True if text is valid in layout
     */
    bool IsValidInLayout(const std::wstring& text, HKL layout);
    
    /**
     * @brief Get character for a virtual key in specific layout
     * 
     * @param vk Virtual key code
     * @param layout Keyboard layout handle
     * @return wchar_t Unicode character or 0 if not found
     */
    wchar_t GetCharForKey(UINT vk, HKL layout);
    
    /**
     * @brief Convert text between keyboard layouts
     * 
     * @param text Text to convert
     * @param fromLayout Source layout handle
     * @param toLayout Target layout handle
     * @return std::wstring Converted text
     */
    std::wstring ConvertText(const std::wstring& text, HKL fromLayout, HKL toLayout);
    
    /**
     * @brief Update popup window with current text and suggestions
     * 
     * @param currentText Current input text
     * @param conversions Map of layout handles to converted text
     */
    void UpdatePopup(const std::wstring& currentText, 
                    const std::unordered_map<HKL, std::wstring>& conversions);

    /**
     * @brief Windows procedure for handling window messages
     * 
     * @param hwnd Window handle
     * @param msg Message type
     * @param wParam First message parameter
     * @param lParam Second message parameter
     * @return LRESULT Message result
     */
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    /**
     * @brief Low-level keyboard hook procedure
     * 
     * Handles keyboard events, updates text buffer, and triggers
     * layout detection.
     */
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    // Layout information
    std::vector<HKL> m_availableLayouts;              ///< Available keyboard layouts
    std::unordered_map<HKL, std::wstring> m_layoutNames; ///< Layout names by handle
    
    // Text state
    std::wstring m_currentText;  ///< Current input text buffer
    size_t m_minTextLength;      ///< Minimum text length for suggestions
    bool m_isRunning;            ///< Program running state
    
    // Window handles
    HWND m_mainWindow;    ///< Hidden main window handle
    HWND m_popup;         ///< Popup window for suggestions
    HHOOK m_keyboardHook; ///< Keyboard hook handle
    
    // Static instance for hook callback
    static KeyboardChecker* s_instance; ///< Singleton instance for callbacks
    
    // Custom window messages
    static const UINT WM_UPDATE_TEXT = WM_USER + 1;     ///< Message to update text
    static const UINT WM_CHECK_LAYOUT = WM_USER + 2;    ///< Message to check layout
    
    // Window class name
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"KeyboardCheckerWindow";
};
