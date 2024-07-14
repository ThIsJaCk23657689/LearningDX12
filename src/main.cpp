#include "stdafx.hpp"
#include "HelloWindow.hpp"

_Use_decl_annotations_
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow )
{
	HelloWindow app( 1280, 720, L"Learning DirectX 12" );
	return Win32App::Run( &app, hInstance, nCmdShow );
}