#pragma once

/**************************************************************************************************
 * A simple utility function for safely releasing a COM interface pointer. Use this on the        *
 * pointer assigned in CoCreateInstance once you're finished with the COM object.                 *
 *   Inputs:                                                                                      *
 *      ptr: A pointer to the COM object to be released.                                          *
 *   Return value: none                                                                           *
 **************************************************************************************************/
template <typename T> void SafeRelease(T** ptr)
{
	if (*ptr)
	{
		(*ptr)->Release();
		(*ptr) = NULL;
	}
}