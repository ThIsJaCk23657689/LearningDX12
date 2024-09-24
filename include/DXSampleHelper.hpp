#pragma once
#include <stdexcept>
#include "stdafx.hpp"

inline std::string HrToString( HRESULT hr )
{
	char s_str[ 64 ] = {};
	sprintf_s( s_str, "HRESULT of 0x%08X", static_cast< UINT >( hr ) );
	return std::string( s_str );
}

class HrException : public std::runtime_error
{
public:
	HrException( HRESULT hr ) : std::runtime_error( HrToString( hr ) ), m_hr( hr ) {}
	HRESULT Error() const { return m_hr; }

private:
	const HRESULT m_hr;

};

inline void ThrowIfFailed( HRESULT hr )
{
	if ( FAILED( hr ) )
	{
		throw HrException( hr );
	}
}

inline void GetAssetsPath( _Out_writes_( pathSize ) WCHAR* path, UINT pathSize )
{
	if ( !path )
	{
		throw std::exception();
	}

	DWORD size = GetModuleFileName( nullptr, path, pathSize );
	if ( size == 0 || size == pathSize )
	{
		// Method failed ot path was truncated.
		throw std::exception();
	}

	WCHAR* lastSlash = wcsrchr( path, L'\\' );
	if ( lastSlash )
	{
		*( lastSlash + 1 ) = L'\0';
	}
}

// Temp
class HeapPropertiesFactory
{
public:
	static CD3DX12_HEAP_PROPERTIES* GetUploadHeapProperties()
	{
		return &m_upload;
	}

	static CD3DX12_HEAP_PROPERTIES* GetDefaultHeapProperties()
	{
		return &m_default;
	}

private:
	static CD3DX12_HEAP_PROPERTIES m_upload;
	static CD3DX12_HEAP_PROPERTIES m_default;

};

inline std::string WstrToStr( const std::wstring& wstr )
{
	int size_needed = WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), ( int ) wstr.size(), NULL, 0, NULL, NULL );
	std::string str( size_needed, 0 );
	WideCharToMultiByte( CP_UTF8, 0, wstr.c_str(), ( int ) wstr.size(), &str[ 0 ], size_needed, NULL, NULL );
	return str;
}

inline std::wstring StrToWstr( const std::string& str ) {
	int size_needed = MultiByteToWideChar( CP_UTF8, 0, str.c_str(), ( int ) str.size(), NULL, 0 );
	std::wstring wstr( size_needed, 0 );
	MultiByteToWideChar( CP_UTF8, 0, str.c_str(), ( int ) str.size(), &wstr[ 0 ], size_needed );
	return wstr;
}
