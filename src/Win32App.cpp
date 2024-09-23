#include "stdafx.hpp"
#include "Win32App.hpp"
#include "DXSample.hpp"

#define MIN_WIDTH 400
#define MIN_HEIGHT 300

HWND Win32App::m_hWnd = nullptr;
std::pair< uint32_t, uint32_t > Win32App::m_windowSize = { MIN_WIDTH, MIN_HEIGHT };
std::pair< int32_t, int32_t > Win32App::m_windowPos = { CW_USEDEFAULT, CW_USEDEFAULT };

int Win32App::Run( DXSample* pSample, HINSTANCE hInstance, int nCmdShow )
{
	// parse the command line parameters.
	int argc;
	LPWSTR* argv = CommandLineToArgvW( GetCommandLineW(), &argc );
	pSample->ParseCommandLineArgs( argv, argc );
	LocalFree( argv );

	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof( WNDCLASSEX );
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	windowClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = L"DXSampleClass";
	windowClass.hIconSm = LoadIcon( NULL, IDI_APPLICATION );
	if ( !RegisterClassEx( &windowClass ) )
	{
		MessageBox( nullptr, L"Window registration failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK );
		return 0;
	}

	RECT windowRect = { 0, 0, static_cast< LONG >( pSample->GetWidth() ), static_cast< LONG >( pSample->GetHeight() ) };
	AdjustWindowRect( &windowRect, WS_OVERLAPPEDWINDOW, FALSE );

	int screenWidth = GetSystemMetrics( SM_CXSCREEN );
	int screenHeight = GetSystemMetrics( SM_CYSCREEN );

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	int posX = max( 0, ( screenWidth - windowWidth ) / 2 );
	int posY = max( 0, ( screenHeight - windowHeight ) / 2 );

	// create window and store and handle to it.
	m_hWnd = CreateWindow(
		windowClass.lpszClassName,
		pSample->GetTitle(),
		WS_OVERLAPPEDWINDOW,
		posX,
		posY,
		windowWidth,
		windowHeight,
		nullptr,	// we have no parent window.
		nullptr,	// we aren't using menus.
		hInstance,
		pSample
	); 
	if ( !m_hWnd )
	{
		MessageBox( nullptr, L"Window creation failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK );
		return 0;
	}

	// show windows and get client area dimensions.
	ShowWindow( m_hWnd, nCmdShow );
	GetClientRect( m_hWnd, &windowRect );

	// Initialize the sample. OnInit is defined in each child-implementation of DXSample.
	auto renderWidth = static_cast< uint32_t >( windowRect.right - windowRect.left );
	auto renderHeight = static_cast< uint32_t >( windowRect.bottom - windowRect.top );
	pSample->OnInit( renderWidth, renderHeight );
	
	// store the window size and position.
	m_windowSize = { renderWidth, renderHeight };
	m_windowPos = { posX, posY };

	// main sample loop
	MSG msg = {};
	while ( msg.message != WM_QUIT )
	{
		// process any messages in the queue.
		if ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			pSample->OnTick();
		}
	}

	pSample->OnDestroy();

	// return this part of the WM_QUIT message to Windows.
	return static_cast< char >( msg.wParam );
}

// Forward declare the message from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

LRESULT CALLBACK Win32App::WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static bool g_bInSizeMove = false;
	static bool g_bInSuspend = false;
	static bool g_bMinimized = false;
	static bool g_bFullScreen = false;

	if ( ImGui_ImplWin32_WndProcHandler( hWnd, message, wParam, lParam ) )
	{
		return true;
	}

	DXSample* pSample = reinterpret_cast< DXSample* >( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );

	switch ( message )
	{
		case WM_CREATE:
		{
			// Save the DXSample* passed in to CreateWindow
			LPCREATESTRUCT pCreateStruct = reinterpret_cast< LPCREATESTRUCT >( lParam );
			SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast< LONG_PTR >( pCreateStruct->lpCreateParams ) );
			return 0;
		}

		case WM_PAINT:
		{
			if ( g_bInSizeMove && pSample )
			{
				pSample->OnTick();
			}
			else 
			{
				PAINTSTRUCT ps;
				std::ignore = BeginPaint( hWnd, &ps );
				EndPaint( hWnd, &ps );
			}
			return 0;
		}

		case WM_SIZE:
		{
			if ( wParam == SIZE_MINIMIZED )
			{
				if ( !g_bMinimized )
				{
					g_bMinimized = true;
					if ( !g_bInSuspend && pSample )
					{
						pSample->OnSuspending();
					}
					g_bInSuspend = true;
				}
			}
			else if ( g_bMinimized )
			{
				g_bMinimized = false;
				if ( g_bInSuspend && pSample )
				{
					pSample->OnResuming();
				}
				g_bInSuspend = false;
			}
			else if ( !g_bInSizeMove && pSample )
			{
				auto width = static_cast< uint32_t >( LOWORD( lParam ) );
				auto height = static_cast< uint32_t >( HIWORD( lParam ) );
				pSample->OnSizeChanged( width, height );
			}
			return 0;
		}

		case WM_ENTERSIZEMOVE:
		{
			g_bInSizeMove = true;
			return 0;
		}

		case WM_EXITSIZEMOVE:
		{
			g_bInSizeMove = false;
			if ( pSample )
			{
				RECT clienRect;
				GetClientRect( hWnd, &clienRect );

				RECT windowRect;
				GetWindowRect( hWnd, &windowRect );

				auto width = static_cast< uint32_t >( clienRect.right - clienRect.left );
				auto height = static_cast< uint32_t >( clienRect.bottom - clienRect.top );
				auto posX = static_cast< int32_t >( windowRect.left );
				auto posY = static_cast< int32_t >( windowRect.top );
				m_windowSize = { width, height };
				m_windowPos = { posX, posY };

				pSample->OnSizeChanged( width, height );
			}
			return 0;
		}

		case WM_GETMINMAXINFO:
		{
			// set minimum window size
			auto pMinMaxInfo = reinterpret_cast< MINMAXINFO* >( lParam );
			pMinMaxInfo->ptMinTrackSize.x = MIN_WIDTH;
			pMinMaxInfo->ptMinTrackSize.y = MIN_HEIGHT;
			return 0;
		}

		case WM_ACTIVATEAPP:
		{
			if ( pSample )
			{
				if ( wParam )
				{
					pSample->OnActivated();
				}
				else
				{
					pSample->OnDeactivated();
				}
			}
			return 0;
		}

		case WM_POWERBROADCAST:
		{
			switch ( wParam )
			{
				case PBT_APMQUERYSUSPEND:
				{
					// system is asking permission to suspend.
					if ( !g_bInSuspend && pSample )
					{
						pSample->OnSuspending();
					}
					g_bInSuspend = true;
					return TRUE;
				}

				case PBT_APMRESUMESUSPEND:
				{
					// system has resumed from sleep.
					if ( !g_bMinimized )
					{
						if ( g_bInSuspend && pSample )
						{
							pSample->OnResuming();
						}
						g_bInSuspend = false;
					}
					return TRUE;
				}
			}
			return 0;
		}

		case WM_KEYDOWN:
		{
			if ( pSample )
			{
				pSample->OnKeyDown( static_cast< uint8_t >( wParam ) );
			}
			return 0;
		}

		case WM_KEYUP:
		{
			if ( pSample )
			{
				pSample->OnKeyUp( static_cast< uint8_t >( wParam ) );
			}
			return 0;
		}

		case WM_SYSKEYDOWN:
		{
			if ( wParam == VK_RETURN && ( lParam & 0x60000000 ) == 0x20000000 )
			{
				// ALT + ENTER fullscreen toggle
				if ( g_bFullScreen )
				{
					SetWindowLongPtr( hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW );
					SetWindowLongPtr( hWnd, GWL_EXSTYLE, 0 );

					auto width	= static_cast< int >( m_windowSize.first );
					auto height	= static_cast< int >( m_windowSize.second );
					auto posX	= static_cast< int >( m_windowPos.first );
					auto posY	= static_cast< int >( m_windowPos.second );
					SetWindowPos( hWnd, HWND_TOP, posX, posY, width, height, SWP_NOZORDER | SWP_FRAMECHANGED );
					ShowWindow( hWnd, SW_SHOWNORMAL );
				}
				else
				{
					RECT clienRect;
					GetClientRect( hWnd, &clienRect );

					RECT windowRect;
					GetWindowRect( hWnd, &windowRect );

					auto width = static_cast< uint32_t >( clienRect.right - clienRect.left );
					auto height = static_cast< uint32_t >( clienRect.bottom - clienRect.top );
					auto posX = static_cast< int32_t >( windowRect.left );
					auto posY = static_cast< int32_t >( windowRect.top );
					m_windowSize = { width, height };
					m_windowPos = { posX, posY };

					SetWindowLongPtr( hWnd, GWL_STYLE, WS_POPUP );
					SetWindowLongPtr( hWnd, GWL_EXSTYLE, WS_EX_TOPMOST );

					SetWindowPos( hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
					ShowWindow( hWnd, SW_SHOWMAXIMIZED );
				}
				g_bFullScreen = !g_bFullScreen;
			}
			return 0;
		}

		case WM_MENUCHAR:
		{
			// A menu is active and the user presses a key that does not correspond
			// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
			return MAKELRESULT( 0, MNC_CLOSE );
		}

		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;
	}

	// Handle any message the switch statement didn't.
	return DefWindowProc( hWnd, message, wParam, lParam );
}
