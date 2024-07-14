#pragma once
#include <Windows.h>
#include "StepTimer.hpp"

class DXSample;
class Win32App
{
public:
	static int Run( DXSample* pSample, HINSTANCE hInstance, int nCmdShow );
	static HWND GetHwnd() { return m_hWnd; }
	static StepTimer& GetTimer() { return m_kTimer; }
protected:
	static LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

private:
	static void Tick( DXSample* pSample );

	static HWND m_hWnd;
	static StepTimer m_kTimer;

};