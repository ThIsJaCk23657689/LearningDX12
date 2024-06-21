#include "stdafx.hpp"
#include "HelloWindow.hpp"

HelloWindow::HelloWindow( uint32_t width, uint32_t height, std::wstring title ) :
	DXSample( width, height, title ),
	m_frameIndex( 0 ),
	m_rtvDescriptorSize( 0 )
{
}

void HelloWindow::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

// Update frame-based values.
void HelloWindow::OnUpdate()
{
}

// Render the scene.
void HelloWindow::OnRender()
{
}

void HelloWindow::OnDestroy()
{
}

// load the rendering pipeline dependencies.
void HelloWindow::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	{
		ComPtr< ID3D12Debug > spDebugController;
		if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &spDebugController ) ) ) )
		{
			spDebugController->EnableDebugLayer();

			// enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr< IDXGIFactory4 > spFactory;
	ThrowIfFailed( CreateDXGIFactory2( dxgiFactoryFlags, IID_PPV_ARGS( &spFactory ) ) );

	// create device (not warp)
	ComPtr< IDXGIAdapter1 > spHardwareAdapter;
	GetHardwareAdapter( spFactory.Get(), &spHardwareAdapter );
	ThrowIfFailed( D3D12CreateDevice( spHardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &m_spDevice ) ) );

	// create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed( m_spDevice->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &m_spCommandQueue ) ) );

	// create swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	ComPtr< IDXGISwapChain1 > spSwapChain;
	ThrowIfFailed( spFactory->CreateSwapChainForHwnd( m_spCommandQueue.Get(), 
													  Win32App::GetHwnd(),
													  &swapChainDesc,
													  nullptr,
													  nullptr,
													  &spSwapChain ) );

	// this sample does not support fullscreen transitions.
	ThrowIfFailed( spFactory->MakeWindowAssociation( Win32App::GetHwnd(), DXGI_MWA_NO_ALT_ENTER ) );

	ThrowIfFailed( spSwapChain.As( &m_spSwapChain ) );
	m_frameIndex = m_spSwapChain->GetCurrentBackBufferIndex();

	// create descriptor heaps
	{
		// describe and create a render target view descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed( m_spDevice->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( &m_spRtvHeap ) ) );

		m_rtvDescriptorSize = m_spDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
	}

	// create frame resources
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( m_spRtvHeap->GetCPUDescriptorHandleForHeapStart() );

		// Crate a RTV for each frame.
		for ( UINT n = 0; n < FrameCount; ++n )
		{
			ThrowIfFailed( m_spSwapChain->GetBuffer( n, IID_PPV_ARGS( &m_renderTargets[ n ] ) ) );
			m_spDevice->CreateRenderTargetView( m_renderTargets[ n ].Get(), nullptr, rtvHandle );
			rtvHandle.Offset( 1, m_rtvDescriptorSize );
		}
	}

	ThrowIfFailed( m_spDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &m_spCommandAllocator ) ) );
}

void HelloWindow::LoadAssets()
{
	// create the command list


}