#include "Win32App.hpp"
#include "DXSample.hpp"

HWND Win32App::m_hWnd = nullptr;

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

	// create window and store and handle to it.
	m_hWnd = CreateWindow(
		windowClass.lpszClassName,
		pSample->GetTitle(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
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

	// Initialize the sample. OnInit is defined in each child-implementation of DXSample.
	pSample->OnInit();

	ShowWindow( m_hWnd, nCmdShow );

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
	}

	pSample->OnDestroy();

	// return this part of the WM_QUIT message to Windows.
	return static_cast< char >( msg.wParam );
}

LRESULT CALLBACK Win32App::WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
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

		case WM_PAINT:
		{
			if ( pSample )
			{
				pSample->OnUpdate();
				pSample->OnRender();
			}
			return 0;
		}

		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;
	}

	// Handle any message the switch statement didn't.
	return DefWindowProc( hWnd, message, wParam, lParam );
}