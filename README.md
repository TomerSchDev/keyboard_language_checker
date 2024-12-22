# Keyboard Language Checker

This application runs in the background and monitors your typing to detect potential keyboard language issues. If it detects that you might be typing with the wrong keyboard layout, it will show a popup asking if you want to switch layouts.

## Features

- Monitors keyboard input in real-time
- Detects potential wrong language usage
- Shows popup notification when wrong language is detected
- Offers to switch keyboard layout automatically
- Runs in the background

## Requirements

- Python 3.6+
- Windows OS (due to Windows-specific keyboard layout detection)

## Installation

1. Install the required packages:
```
pip install -r requirements.txt
```

## Usage

1. Run the script:
```
python language_checker.py
```

2. The program will run in the background and monitor your typing
3. When it detects potential wrong language usage, it will show a popup
4. Click "Yes" to switch keyboard layout or "No" to keep current layout

## How it works

The program monitors your typing and checks the text when:
- You've typed more than 20 characters
- You press Enter

It uses language detection to determine if the text makes sense in any known language. If it doesn't, it assumes you might be typing with the wrong keyboard layout and shows a popup.

## To Exit

Press Ctrl+C in the terminal window to stop the program.
