#include "stdafx.hpp"
#include "HelloWindow.hpp"
#include "Texture.hpp"

HelloWindow::HelloWindow( uint32_t width, uint32_t height, std::wstring title ) :
	DXSample( width, height, title ),
	m_frameIndex( 0 ),
	m_viewport( 0.0f, 0.0f, static_cast< float >( width ), static_cast< float >( height ) ),
	m_scissorRect( 0, 0, static_cast< LONG >( width ), static_cast< LONG >( height ) ),
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
	ThrowIfFailed( spFactory->CreateSwapChainForHwnd( m_spCommandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
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

		// Describe and create a shader resource view (SRV) heap for the texture.
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed( m_spDevice->CreateDescriptorHeap( &srvHeapDesc, IID_PPV_ARGS( &m_spSrvHeap ) ) );

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
	ThrowIfFailed( m_spDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS( &m_spBundleAllocator ) ) );
}

// Load the sample assets.
void HelloWindow::LoadAssets()
{
	// Create the root signature
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds,
		// the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if ( FAILED( m_spDevice->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[ 1 ];
		ranges[ 0 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC );

		CD3DX12_ROOT_PARAMETER1 rootParameters[ 1 ];
		rootParameters[ 0 ].InitAsDescriptorTable( 1, &ranges[ 0 ], D3D12_SHADER_VISIBILITY_PIXEL );

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1( _countof( rootParameters ), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );

		ComPtr< ID3DBlob > spSignature;
		ComPtr< ID3DBlob > spError;
		ThrowIfFailed( D3DX12SerializeVersionedRootSignature( &rootSignatureDesc, featureData.HighestVersion, &spSignature, &spError ) );
		ThrowIfFailed( m_spDevice->CreateRootSignature( 0, spSignature->GetBufferPointer(), spSignature->GetBufferSize(), IID_PPV_ARGS( &m_spRootSignature ) ) );
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr< ID3DBlob > spVertexShader;
		ComPtr< ID3DBlob > spPixelShader;

#if defined( _DEBUG )
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed( D3DCompileFromFile( GetAssetFullPath( L"assets\\shaders.hlsl" ).c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &spVertexShader, nullptr ) );
		ThrowIfFailed( D3DCompileFromFile( GetAssetFullPath( L"assets\\shaders.hlsl" ).c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &spPixelShader, nullptr ) );

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof( inputElementDescs ) };
		psoDesc.pRootSignature = m_spRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE( spVertexShader.Get() );
		psoDesc.PS = CD3DX12_SHADER_BYTECODE( spPixelShader.Get() );
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
		psoDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed( m_spDevice->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &m_spPipelineState ) ) );
	}

	// create the command list
	ThrowIfFailed( m_spDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_spCommandAllocator.Get(), m_spPipelineState.Get(), IID_PPV_ARGS( &m_spCommandList ) ) );

	// Create the vertex buffer.
	ComPtr< ID3D12Resource > spVertexBufferUploadHeap;
	{
		// Define the geometry for a triangle.
		std::vector< Vertex > vertices =
		{
			{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
			{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			{ { -0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		};

		const size_t vertexBufferSize = vertices.size() * sizeof( Vertex );

		// Note: using upload heaps to transfer static data like vertex buffers is not recommended.
		// Every time the GPU needs it, the upload heap will be marshalled over.
		// Please read up on Default Heap usage. An upload heap is used here for code simplicity and
		// because there are very few vertices to actually transfer.
		auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer( vertexBufferSize );
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ),
			D3D12_HEAP_FLAG_NONE,
			&uploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &spVertexBufferUploadHeap )
		) );

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange( 0, 0 ); // we do not intend to read from this resource on the CPU.
		ThrowIfFailed( spVertexBufferUploadHeap->Map( 0, &readRange, reinterpret_cast< void** >( &pVertexDataBegin ) ) );
		memcpy( pVertexDataBegin, vertices.data(), vertexBufferSize );
		spVertexBufferUploadHeap->Unmap( 0, nullptr );

		// create vertex buffer in default heap
		auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer( vertexBufferSize );
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &m_spVertexBuffer )
		) );
		m_spCommandList->CopyBufferRegion( m_spVertexBuffer.Get(), 0, spVertexBufferUploadHeap.Get(), 0, vertexBufferSize );
		m_spCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_spVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ) );

		// Initialize the vertex buffer view.
		m_spVertexBufferView.BufferLocation = m_spVertexBuffer->GetGPUVirtualAddress();
		m_spVertexBufferView.StrideInBytes = sizeof( Vertex );
		m_spVertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create Index Buffer
	ComPtr< ID3D12Resource > spIndexBufferUploadHeap;
	{
		std::vector< uint16_t > indices =
		{
			0, 1, 2,
			0, 2, 3
		};

		const size_t indexBufferSize = indices.size() * sizeof( uint16_t );

		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer( indexBufferSize ),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &spIndexBufferUploadHeap )
		) );

		// Copy the triangle data to the vertex buffer.
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange( 0, 0 ); // we do not intend to read from this resource on the CPU.
		ThrowIfFailed( spIndexBufferUploadHeap->Map( 0, &readRange, reinterpret_cast< void** >( &pIndexDataBegin ) ) );
		memcpy( pIndexDataBegin, indices.data(), indexBufferSize );
		spIndexBufferUploadHeap->Unmap( 0, nullptr );

		// create vertex buffer in default heap
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer( indexBufferSize ),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &m_spIndexBuffer )
		) );
		m_spCommandList->CopyBufferRegion( m_spIndexBuffer.Get(), 0, spIndexBufferUploadHeap.Get(), 0, indexBufferSize );
		m_spCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_spIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER ) );

		// Initialize the vertex buffer view.
		m_spIndexBufferView.BufferLocation = m_spIndexBuffer->GetGPUVirtualAddress();
		m_spIndexBufferView.SizeInBytes = indexBufferSize;
		m_spIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	}

	// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
	// the command list that references it has finished executing on the GPU.
	// We will flush the GPU at the end of this method to ensure the resouce is not prematurely destroyed.
	ComPtr< ID3D12Resource > spTextureUploadHeap;

	// Create the texture.
	{
		Texture2DPtr spTexture = TextureManager::CreateTexture2D( GetAssetFullPath( L"assets\\textures\\rickroll.jpg" ) );

		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = spTexture->width;
		textureDesc.Height = spTexture->height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &m_spTexture )
		) );

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize( m_spTexture.Get(), 0, 1 );

		// Create the GPU upload buffer.
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer( uploadBufferSize ),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &spTextureUploadHeap )
		) );

		// Copy data to the intermediate upload heap and then schedule a copy
		// from the upload heap to the Texture2D.
		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = spTexture->data;
		textureData.RowPitch = spTexture->width * spTexture->pixelSize;
		textureData.SlicePitch = textureData.RowPitch * spTexture->height;

		UpdateSubresources( m_spCommandList.Get(), m_spTexture.Get(), spTextureUploadHeap.Get(), 0, 0, 1, &textureData );
		m_spCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_spTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ) );

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		m_spDevice->CreateShaderResourceView( m_spTexture.Get(), &srvDesc, m_spSrvHeap->GetCPUDescriptorHandleForHeapStart() );
	}

	// Close the command list and execute it to begin the initial GPU setup.
	ThrowIfFailed( m_spCommandList->Close() );
	ID3D12CommandList* ppCommandLists[] = { m_spCommandList.Get() };
	m_spCommandQueue->ExecuteCommandLists( _countof( ppCommandLists ), ppCommandLists );

	// Create and record the bundle
	{
		ThrowIfFailed( m_spDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_spBundleAllocator.Get(), m_spPipelineState.Get(), IID_PPV_ARGS( &m_spBundle ) ) );
		m_spBundle->SetGraphicsRootSignature( m_spRootSignature.Get() );
		m_spBundle->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_spBundle->IASetVertexBuffers( 0, 1, &m_spVertexBufferView );
		m_spBundle->IASetIndexBuffer( &m_spIndexBufferView );
		m_spBundle->DrawIndexedInstanced( 6, 1, 0, 0, 0 );
		ThrowIfFailed( m_spBundle->Close() );
	}

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed( m_spDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &m_spFence ) ) );
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization
		m_hFenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
		if ( !m_hFenceEvent )
		{
			ThrowIfFailed( HRESULT_FROM_WIN32( GetLastError() ) );
		}

		// Wait for the command list to execute; we are reusing the same command
		// list in our main loop but for now, we just want to wait for setup to
		// complete before continuing.
		WaitForPreviousFrame();
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

	// Set necessary state.
	m_spCommandList->SetGraphicsRootSignature( m_spRootSignature.Get() );

	// Every command list only allow sets descriptor deap one time. 
	ID3D12DescriptorHeap* ppHeaps[] = { m_spSrvHeap.Get() };
	m_spCommandList->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );

	m_spCommandList->SetGraphicsRootDescriptorTable( 0, m_spSrvHeap->GetGPUDescriptorHandleForHeapStart() );
	m_spCommandList->RSSetViewports( 1, &m_viewport );
	m_spCommandList->RSSetScissorRects( 1, &m_scissorRect );

	// Indicate that the back buffer will be used as a render target.
	m_spCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_renderTargets[ m_frameIndex ].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET ) );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( m_spRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize );
	m_spCommandList->OMSetRenderTargets( 1, &rtvHandle, FALSE, nullptr );

	// Record command.
	const float clearColor[] = { 0.278f, 0.278f, 0.314f, 1.0f };
	m_spCommandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );

	m_spCommandList->ExecuteBundle( m_spBundle.Get() );

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