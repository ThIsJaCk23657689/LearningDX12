#pragma once
#include <Windows.h>

class DXSample;

class Win32App
{
public:
	static int Run( DXSample* pSample, HINSTANCE hInstance, int nCmdShow );
	static HWND GetHwnd() { return m_hWnd; }
protected:
	static LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

private:
	static HWND m_hWnd;
};