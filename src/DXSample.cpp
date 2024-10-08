#include <stdafx.hpp>
#include "DXSample.hpp"

using Microsoft::WRL::ComPtr;

DXSample::DXSample( uint32_t width, uint32_t height, std::wstring title ) :
    m_title( title ),
    m_bIsInitialized( false )
{
    SetWidthAndHeight( width, height );

    WCHAR assetsPath[ 512 ];
    GetAssetsPath( assetsPath, _countof( assetsPath ) );
    m_assesPath = assetsPath;
}

DXSample::~DXSample()
{
}

void DXSample::OnTick()
{
    if ( !m_bIsInitialized )
	{
        return;
	}

	m_kTimer.Tick( [ & ]()
	{
		OnUpdate( m_kTimer );
	} );

    OnRender();
}

void DXSample::OnResuming()
{
    m_kTimer.ResetElapsedTime();

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

void DXSample::SetWidthAndHeight( uint32_t width, uint32_t height )
{
	m_width = max( width, 1 );
	m_height = max( height, 1 );
	m_aspectRatio = static_cast< float >( width ) / static_cast< float >( height );
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be founded, ppAdapter will be set to nullptr.
_Use_decl_annotations_
void DXSample::GetHardwareAdapter( IDXGIFactory4* pFactory,
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
                IID_PPV_ARGS( spAdapter.ReleaseAndGetAddressOf() ) ) );
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
        for ( UINT adapterIndex = 0; SUCCEEDED( pFactory->EnumAdapters1( adapterIndex, spAdapter.ReleaseAndGetAddressOf() ) ); ++adapterIndex )
        {
            DXGI_ADAPTER_DESC1 desc;
            spAdapter->GetDesc1( &desc );

            if ( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if ( SUCCEEDED( D3D12CreateDevice( spAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), nullptr ) ) )
            {
                break;
            }
        }
    }

#ifndef NDEBUG
    if ( !spAdapter.Get() )
    {
        // try WARP12
        if ( FAILED( pFactory->EnumWarpAdapter( IID_PPV_ARGS( spAdapter.ReleaseAndGetAddressOf() ) ) ) )
		{
			throw std::exception( "WARP12 not available. Enable the 'Graphics Tools' feature." );
		}
    }
#endif

    if ( !spAdapter.Get() )
    {
        throw std::runtime_error( "No Direct3D 12 device found" );
    }

    *ppAdapter = spAdapter.Detach();
}