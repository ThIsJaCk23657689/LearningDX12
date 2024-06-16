#include <Windows.h>
#include "HelloWindow.hpp"
#include "Win32App.hpp"

_Use_decl_annotations_
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow )
{
	HelloWindow app( 1280, 720, L"Learning DirectX 12" );
	return Win32App::Run( &app, hInstance, nCmdShow );
}