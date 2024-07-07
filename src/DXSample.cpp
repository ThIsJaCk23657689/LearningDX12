#include <stdafx.hpp>
#include "DXSample.hpp"

using Microsoft::WRL::ComPtr;

DXSample::DXSample( uint32_t width, uint32_t height, std::wstring title ) :
    m_width( width ),
    m_height( height ),
    m_title( title )
{
    WCHAR assetsPath[ 512 ];
    GetAssetsPath( assetsPath, _countof( assetsPath ) );
    m_assesPath = assetsPath;

	m_aspectRatio = static_cast< float >( width ) / static_cast< float >( height );
}

DXSample::~DXSample()
{
}

_Use_decl_annotations_
void DXSample::ParseCommandLineArgs( wchar_t* argv[], int argc )
{
    return;
}

std::wstring DXSample::GetAssetFullPath( LPCWSTR assertName )
{
    return m_assesPath + assertName;
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be founded, ppAdapter will be set to nullptr.
_Use_decl_annotations_
void DXSample::GetHardwareAdapter( IDXGIFactory1* pFactory,
                                   IDXGIAdapter1** ppAdapter,
                                   bool requestHightPerformanceAdapter )
{
    *ppAdapter = nullptr;

    ComPtr< IDXGIAdapter1 > spAdapter;
    ComPtr< IDXGIFactory6 > spFactory6;
    if ( SUCCEEDED( pFactory->QueryInterface( IID_PPV_ARGS( &spFactory6 ) ) ) )
    {
        for ( 
            UINT adapterIndex = 0;
            SUCCEEDED( spFactory6->EnumAdapterByGpuPreference( 
                adapterIndex ,
                requestHightPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS( &spAdapter ) ) );
            ++adapterIndex )
        {
            DXGI_ADAPTER_DESC1 desc;
            spAdapter->GetDesc1( &desc );

            if ( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
            {
                continue;
            }

            if ( SUCCEEDED( D3D12CreateDevice( spAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), nullptr ) ) )
            {
                break;
            }
        }
    }

    if ( !spAdapter.Get() )
    {
        for ( UINT adapterIndex = 0; SUCCEEDED( pFactory->EnumAdapters1( adapterIndex, &spAdapter ) ); ++adapterIndex )
        {
            DXGI_ADAPTER_DESC1 desc;
            spAdapter->GetDesc1( &desc );

            if ( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
            {
                continue;
            }

            if ( SUCCEEDED( D3D12CreateDevice( spAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), nullptr ) ) )
            {
                break;
            }
        }
    }

    *ppAdapter = spAdapter.Detach();
}