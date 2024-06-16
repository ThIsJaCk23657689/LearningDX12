#include <Windows.h>

const char g_szClassName[] = "Learngin DirectX 12";

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	HDC hDc;
	switch ( uMsg )
	{
		case WM_CLOSE:
			DestroyWindow( hWnd );
			break;
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		case WM_PAINT:
			PAINTSTRUCT kPs;
			hDc = BeginPaint( hWnd, &kPs );
			TextOut( hDc, 150, 50, "Learning DirectX 12", 19 );
			EndPaint( hWnd, &kPs );
			break;
		default:
			return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}
	return 0;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	WNDCLASSEX wc;
	wc.cbSize			= sizeof( WNDCLASSEX );
	wc.style			= 0;
	wc.lpfnWndProc		= WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= g_szClassName;
	wc.hIconSm			= LoadIcon(NULL, IDI_APPLICATION);

	if ( !RegisterClassEx( &wc ) )
	{
		MessageBox( nullptr, "Window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK );
		return 0;
	}

	HWND hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"Hello World!",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if ( !hWnd )
	{
		MessageBox( nullptr, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK );
		return 0;
	}

	ShowWindow( hWnd, nCmdShow );
	UpdateWindow( hWnd );

	MSG kMsg;
	while ( GetMessage( &kMsg, nullptr, 0, 0 ) > 0 )
	{
		TranslateMessage( &kMsg );
		DispatchMessage( &kMsg );
	}

	return kMsg.wParam;
}