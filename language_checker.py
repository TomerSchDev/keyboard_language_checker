from pynput import keyboard
from tkinter import *
from tkinter import messagebox
import ctypes
import threading

class LanguageChecker:
    def __init__(self):
        self.current_text = ""
        self.min_text_length = 20
        self.root = None
        
    def on_key_press(self, key):
        try:
            if hasattr(key, 'char'):
                self.current_text += key.char
            elif key == keyboard.Key.space:
                self.current_text += " "
            elif key == keyboard.Key.backspace:
                self.current_text = self.current_text[:-1]
            elif key == keyboard.Key.enter:
                self.check_text()
                self.current_text = ""
                
            if len(self.current_text) >= self.min_text_length:
                self.check_text()
                
        except Exception as e:
            print(f"Error in key press handler: {e}")
            
    def check_text(self):
        if len(self.current_text.strip()) < self.min_text_length:
            return
        self.show_popup()
            
    def show_popup(self):
        def create_popup():
            self.root = Tk()
            self.root.withdraw()  # Hide the main window
            
            response = messagebox.askyesno(
                "Wrong Keyboard Language?",
                "The text you typed might be in the wrong keyboard language.\nWould you like to switch keyboard layout?"
            )
            
            if response:
                # Switch to next available layout
                ctypes.windll.user32.PostMessageW(0xFFFF, 0x50, 0, 0)
                
            self.root.destroy()
            self.root = None
            
        if self.root is None:
            threading.Thread(target=create_popup).start()
            
    def start(self):
        with keyboard.Listener(on_press=self.on_key_press) as listener:
            listener.join()

if __name__ == "__main__":
    print("Starting Keyboard Language Checker...")
    print("Press Ctrl+C to exit")
    checker = LanguageChecker()
    checker.start()
