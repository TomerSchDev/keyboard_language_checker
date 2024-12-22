from pynput import keyboard
from tkinter import *
from tkinter import messagebox
import ctypes
import threading
from ctypes import windll, create_unicode_buffer, Structure, c_ushort, c_int, c_uint, POINTER, byref
from langdetect import detect
from langdetect.lang_detect_exception import LangDetectException

# Windows API constants
WM_ACTIVATE = 0x0006
WM_INPUTLANGCHANGEREQUEST = 0x0050
HWND_BROADCAST = 0xFFFF
VK_SHIFT = 0x10

class POINT(Structure):
    _fields_ = [("x", c_int), ("y", c_int)]

class KeyboardLayout:
    def __init__(self):
        self.user32 = ctypes.WinDLL('user32', use_last_error=True)
        
        # Define function prototypes
        self.user32.GetKeyboardLayout.restype = c_uint
        self.user32.MapVirtualKeyExW.argtypes = [c_uint, c_uint, c_uint]
        self.user32.ToUnicodeEx.argtypes = [c_uint, c_uint, POINTER(c_ushort), 
                                           POINTER(c_ushort), c_int, c_uint, c_uint]
        
        # Virtual key to character mapping constants
        self.MAPVK_VK_TO_VSC = 0
        self.MAPVK_VSC_TO_VK = 1
        self.MAPVK_VK_TO_CHAR = 2
        self.MAPVK_VSC_TO_VK_EX = 3
        
    def get_char_for_key(self, vk, layout_id):
        """Get character for a virtual key code in the specified keyboard layout"""
        # Get scan code
        scan_code = self.user32.MapVirtualKeyExW(vk, self.MAPVK_VK_TO_VSC, layout_id)
        if scan_code == 0:
            return None
            
        # Prepare key state array (256 bytes)
        key_states = (c_ushort * 256)()
        
        # Prepare output buffer
        output_buffer = (c_ushort * 8)()
        
        # Call ToUnicodeEx
        result = self.user32.ToUnicodeEx(
            vk,                    # virtual key code
            scan_code,            # scan code
            key_states,           # key state array
            output_buffer,        # output buffer
            len(output_buffer),   # buffer size
            0,                    # flags
            layout_id             # keyboard layout
        )
        
        if result > 0:
            # Convert the output buffer to a string
            return ''.join(map(chr, output_buffer[:result]))
        return None

class LanguageChecker:
    def __init__(self):
        self.current_text = ""
        self.min_text_length = 5
        self.root = None
        self.layout_mappings = {}
        self.layout_names = {}
        self.keyboard = KeyboardLayout()
        self._init_keyboard_layouts()
        
    def _init_keyboard_layouts(self):
        """Initialize keyboard layouts and their names"""
        # Define the virtual key codes for letters
        vk_codes = {
            'A': 0x41, 'B': 0x42, 'C': 0x43, 'D': 0x44, 'E': 0x45, 'F': 0x46, 'G': 0x47,
            'H': 0x48, 'I': 0x49, 'J': 0x4A, 'K': 0x4B, 'L': 0x4C, 'M': 0x4D, 'N': 0x4E,
            'O': 0x4F, 'P': 0x50, 'Q': 0x51, 'R': 0x52, 'S': 0x53, 'T': 0x54, 'U': 0x55,
            'V': 0x56, 'W': 0x57, 'X': 0x58, 'Y': 0x59, 'Z': 0x5A,
            '0': 0x30, '1': 0x31, '2': 0x32, '3': 0x33, '4': 0x34,
            '5': 0x35, '6': 0x36, '7': 0x37, '8': 0x38, '9': 0x39,
            ';': 0xBA, '=': 0xBB, ',': 0xBC, '-': 0xBD, '.': 0xBE,
            '/': 0xBF, '`': 0xC0, '[': 0xDB, '\\': 0xDC, ']': 0xDD,
            "'": 0xDE, ' ': 0x20
        }
        
        # Get available layouts
        self.layout_names = {
            0x409: "English (US)",
            0x40D: "Hebrew"
        }
        
        # Create mappings between layouts
        for from_id in self.layout_names:
            # Create mapping for this layout
            layout_map = {}
            for char, vk in vk_codes.items():
                result = self.keyboard.get_char_for_key(vk, from_id)
                if result:
                    layout_map[char.lower()] = result.lower()
                    
            self.layout_mappings[from_id] = layout_map
            
            # Create reverse mapping
            for other_id in self.layout_names:
                if other_id != from_id:
                    reverse_map = {}
                    for vk in vk_codes.values():
                        char1 = self.keyboard.get_char_for_key(vk, from_id)
                        char2 = self.keyboard.get_char_for_key(vk, other_id)
                        if char1 and char2:
                            reverse_map[char1.lower()] = char2.lower()
                    self.layout_mappings[(from_id, other_id)] = reverse_map
        
        print("\nInitialized keyboard layouts:")
        for layout_id, name in self.layout_names.items():
            print(f"Layout {name} (0x{layout_id:04x}):")
            for vk in vk_codes.values():
                char = self.keyboard.get_char_for_key(vk, layout_id)
                if char:
                    print(f"  VK 0x{vk:02x} -> '{char}'")
                    
    def _get_current_layout(self):
        """Get current keyboard layout ID"""
        return windll.user32.GetKeyboardLayout(0) & 0xFFFF
        
    def _convert_text(self, text, from_layout, to_layout):
        """Convert text from one layout to another"""
        try:
            # Get the appropriate mapping
            if (from_layout, to_layout) in self.layout_mappings:
                mapping = self.layout_mappings[(from_layout, to_layout)]
                result = ""
                for c in text.lower():
                    result += mapping.get(c, c)
                print(f"Converting '{text}' from 0x{from_layout:04x} to 0x{to_layout:04x}: '{result}'")
                return result
            return text
        except Exception as e:
            print(f"Error converting text: {e}")
            return text
        
    def check_text(self):
        try:
            if len(self.current_text.strip()) < self.min_text_length:
                return
                
            current_text = self.current_text.strip()
            current_layout = self._get_current_layout()
            print(f"\nCurrent layout: {self.layout_names.get(current_layout, f'0x{current_layout:04x}')} (0x{current_layout:04x})")
            print(f"Current text: {current_text}")
            
            # Convert text to all other layouts and check if it makes sense
            conversions = {}
            LAYOUT_TO_LANG = {
                0x409: 'en',  # English
                0x40D: 'he'   # Hebrew
            }
            for layout_id in self.layout_names:
                if isinstance(layout_id, int) and layout_id != current_layout:
                    converted = self._convert_text(current_text, current_layout, layout_id)
                    
                    # Try to detect language
                    try:
                        detected_lang = detect(converted)
                        expected_lang = LAYOUT_TO_LANG.get(layout_id)
                        
                        print(f"Converted to {self.layout_names[layout_id]}: {converted}")
                        print(f"Detected language: {detected_lang}, Expected: {expected_lang}")
                        
                        # Only add to conversions if language matches the layout
                        if expected_lang and detected_lang == expected_lang:
                            layout_name = self.layout_names[layout_id]
                            conversions[layout_name] = converted
                            print(f"Text makes sense in {layout_name}")
                        else:
                            print(f"Text does not make sense in {self.layout_names[layout_id]}")
                            
                    except LangDetectException as e:
                        print(f"Could not detect language: {e}")
                        continue
            
            if conversions:
                print(f"Found {len(conversions)} valid conversions, showing popup...")
                self.show_popup(conversions)
            else:
                print("No valid conversions found")
                
        except Exception as e:
            print(f"Error checking text: {e}")
            
    def on_key_press(self, key):
        try:
            if hasattr(key, 'char'):
                self.current_text += key.char
            elif key == keyboard.Key.space:
                self.current_text += " "
            elif key == keyboard.Key.backspace and self.current_text:
                self.current_text = self.current_text[:-1]
            elif key == keyboard.Key.enter:
                self.check_text()
                self.current_text = ""
                
            if len(self.current_text) >= self.min_text_length:
                self.check_text()
                
        except Exception as e:
            print(f"Error in key press handler: {e}")
            
    def show_popup(self, conversions):
        def create_popup():
            try:
                if self.root is not None:
                    try:
                        self.root.destroy()
                    except:
                        pass
                    self.root = None
                
                print("\nCreating new popup...")
                self.root = Tk()
                self.root.title("Keyboard Layout Checker")
                
                # Make sure window stays on top
                self.root.attributes('-topmost', True)
                self.root.lift()
                
                frame = Frame(self.root, padx=10, pady=10)
                frame.pack(fill=BOTH, expand=True)
                
                Label(frame, text="Current text:", font=('Arial', 10, 'bold')).pack()
                Label(frame, text=self.current_text, wraplength=350).pack(pady=5)
                
                Label(frame, text="Possible conversions:", font=('Arial', 10, 'bold')).pack(pady=10)
                for layout_name, text in conversions.items():
                    Label(frame, text=f"{layout_name}:", font=('Arial', 10)).pack()
                    Label(frame, text=text, wraplength=350).pack(pady=5)
                
                def switch_layout():
                    try:
                        print("Switching keyboard layout...")
                        ctypes.windll.user32.PostMessageW(0xFFFF, 0x50, 0, 0)
                    except Exception as e:
                        print(f"Error switching layout: {e}")
                    finally:
                        self.root.destroy()
                        self.root = None
                        self.current_text = ""
                
                def cancel():
                    print("Canceling popup...")
                    self.root.destroy()
                    self.root = None
                
                Button(frame, text="Switch Layout", command=switch_layout, width=15).pack(pady=5)
                Button(frame, text="Cancel", command=cancel, width=15).pack(pady=5)
                
                # Center the window
                self.root.update_idletasks()
                width = self.root.winfo_width()
                height = self.root.winfo_height()
                x = (self.root.winfo_screenwidth() // 2) - (width // 2)
                y = (self.root.winfo_screenheight() // 2) - (height // 2)
                self.root.geometry(f'{width}x{height}+{x}+{y}')
                
                print("Starting mainloop...")
                self.root.mainloop()
                print("Mainloop ended")
                
            except Exception as e:
                print(f"Error creating popup: {e}")
                if self.root is not None:
                    try:
                        self.root.destroy()
                    except:
                        pass
                    self.root = None
        
        # Start popup in a new thread
        if self.root is None:
            print("\nStarting popup thread...")
            threading.Thread(target=create_popup, daemon=True).start()
        else:
            print("Popup already exists, not creating new one")
            
    def start(self):
        print("Keyboard Language Checker started. Press Ctrl+C to exit.")
        print("Supported layouts:")
        for layout_id, name in self.layout_names.items():
            print(f"- {name} (0x{layout_id:04x})")
        print("\nMonitoring keyboard input...")
        with keyboard.Listener(on_press=self.on_key_press) as listener:
            listener.join()

if __name__ == "__main__":
    checker = LanguageChecker()
    checker.start()
