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
	// Record all the command we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the commad list.
	ID3D12CommandList* ppCommandList[] = { m_spCommandList.Get() };
	m_spCommandQueue->ExecuteCommandLists( _countof( ppCommandList ), ppCommandList );

	// Present the frame.
	ThrowIfFailed( m_spSwapChain->Present( 1, 0 ) );

	WaitForPreviousFrame();
}

void HelloWindow::OnDestroy()
{
	// Ensure that GPU is no langer refernencing resource that are about to be cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle( m_hFenceEvent );
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

// Load the sample assets.
void HelloWindow::LoadAssets()
{
	// create the command list
	ThrowIfFailed( m_spDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_spCommandAllocator.Get(), nullptr, IID_PPV_ARGS( &m_spCommandList ) ) );

	// Command list are created in the recording state, but there is nothing to record yet.
	// The main loop expects it to be closed, so close it now.
	ThrowIfFailed( m_spCommandList->Close() );

	// Create synchronization objects.
	{
		ThrowIfFailed( m_spDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_spFence ) ) );
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization
		m_hFenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
		if ( !m_hFenceEvent )
		{
			ThrowIfFailed( HRESULT_FROM_WIN32( GetLastError() ) );
		}
	}
}

void HelloWindow::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated
	// command lists have finished execution on the GPU; apps should use
	// fences to determine GPU execution progress.
	ThrowIfFailed( m_spCommandAllocator->Reset() );

	// However, when ExecuteCommandList() is called on a particular command list,
	// that command list can be reset at any time and must be before re-recording
	ThrowIfFailed( m_spCommandList->Reset( m_spCommandAllocator.Get(), m_spPipelineState.Get() ) );

	// Indicate that the back buffer will be used as a render target.
	m_spCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_renderTargets[ m_frameIndex ].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET ) );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( m_spRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize );

	// Record command.
	const float clearColor[] = { 0.1478f, 0.2576f, 0.1258f, 1.0f };
	m_spCommandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );

	// Indicate that the back buffer will now be used to present.
	m_spCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_renderTargets[ m_frameIndex ].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT ) );

	ThrowIfFailed( m_spCommandList->Close() );
}

void HelloWindow::WaitForPreviousFrame()
{
	// WATTING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFramebuffing
	// sample illustrates how to use fences for efficient resource usage and to maximize GPU utillization

	// Signal and increment the fence value
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed( m_spCommandQueue->Signal( m_spFence.Get(), fence ) );
	m_fenceValue++;

	// Wait untill the previous frame is finished.
	if ( m_spFence->GetCompletedValue() < fence )
	{
		ThrowIfFailed( m_spFence->SetEventOnCompletion( fence, m_hFenceEvent ) );
		WaitForSingleObject( m_hFenceEvent, INFINITE );
	}

	m_frameIndex = m_spSwapChain->GetCurrentBackBufferIndex();
}