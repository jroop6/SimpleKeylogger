/**************************************************************************************************
 * A template for a Window Class. To Define a new type of window, the following 2 functions must  *
 * be overwritten in an extending class:                                                          *
 *   PCWSTR ClassName() const   <-- The name that this window class will be registered under.     *
 *                                  This is not the displayed title of the window, but it's       *
 *                                  under-the-hood name.                                          *
 *   LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)   <-- How this window class   *
 *                                                                        handles messages.       *
 **************************************************************************************************/

#pragma once
#include <Windows.h>

template <class DERIVED_TYPE> class BaseWindow 
{
public:
	// Constructor
	BaseWindow() : m_hwnd(NULL) {}

	/**********************************************************************************************
	 * The Template's Message handler. Upon instantiating a derived class, the new window's       *
	 * CREATESTRUCT structure is modified to include a pointer to the derived class' definition.  *
	 * When further messages intended for that window are received, this function can then        *
	 * delegate the messages to the appropriate message handler for that window.                  *
	 **********************************************************************************************/
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DERIVED_TYPE * currentState = NULL;

		// Get the state info for the window; set the window pointer if this is the initial call.
		if (uMsg == WM_CREATE)
		{
			// retrieve the CREATESTRUCT structure pointer from lParam:
			CREATESTRUCT * pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			// retrieve the pointer to the custom-defined structure from pCreate:
			currentState = (DERIVED_TYPE*)(pCreate->lpCreateParams);
			// store the WindowState pointer in the window instance:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)currentState);
			currentState->m_hwnd = hwnd;
		}
		else
		{
			currentState = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}
		if (currentState) // A currentState exists. Call its HandleMessage() method.
		{
			return currentState->HandleMessage(uMsg, wParam, lParam);
		}
		else // currentState was a null pointer. This is probably an error, actually.
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	/**********************************************************************************************
	 * Registers the derived Window Class and instantiates a window.                              *
	 *   Inputs:                                                                                  *
	 *      lpWindowName: pointer to a long-character string; The name that will be displayed at  *
	 *                    in the title bar the top of the window.                                 *
	 *      dwStyle: The styles applied to the window, such as WS_BORDER or WS_OVERLAPPED. See    *
	 *               the Windows documentation on Window Styles.                                  *
	 *      nwidth: The width of the window, in pixels                                            *
	 *      nHeight: The height of the window, in pixels                                          *
	 *      x: The distance between the left edge of the parent window and this window.           *
	 *      y: The distance between the top edge of the parent window and this window.            *
	 *      dwExStyle: (optional) Any Extended Windows Styles applied to this window, such as     *
	 *                 WS_EX_ACCEPTFILES or WS_EX_PALETTEWINDOW. See the Windows documentation    *
	 *                 on Extended Windows Styles.                                                *
	 *      hWndParent: (optional) A handle to the parent of this window. If 0 or not set, then   *
	 *                  This window is treated as a top-level window.                             *
	 *      HWND: (optional) A handle to a Menu object or an identifier if this is a child        *
	 *            window. See Windows documentation on CreateWindow or CreateWindowEx()           *
	 *   return value: TRUE, if a window instance was successfully created, FALSE otherwise.      *
	 **********************************************************************************************/
	BOOL Create(
		PCWSTR lpWindowName,			// displayed Window text/title
		DWORD dwStyle,					// Window Style
		int nWidth = CW_USEDEFAULT,		// Size and position
		int nHeight = CW_USEDEFAULT,	// Size and position
		int x = CW_USEDEFAULT,			// Size and position
		int y = CW_USEDEFAULT,			// Size and position
		DWORD dwExStyle = 0,			// Optional Window styles
		HWND hWndParent = NULL,			// Parent window
		HMENU hMenu = NULL				// Menu
	)
	{
		// Register the window class
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = DERIVED_TYPE::WindowProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = ClassName();
		RegisterClass(&wc);

		// Instantiate a window
		m_hwnd = CreateWindowExW(dwExStyle, ClassName(), lpWindowName, dwStyle, x, y, nWidth, 
			                     nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this);

		return (m_hwnd ? TRUE : FALSE); 
	}

	// Retrieves a handle to the window
	HWND Window() const { return m_hwnd; }

protected:
	// Sets the name of the Window Class
	virtual PCWSTR ClassName() const = 0; 
	
	// A function for handling messages on the window's message queue
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	// A handle to the window instance
	HWND m_hwnd;
};