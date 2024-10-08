#include "stdafx.hpp"
#include <system_error>
#include "Math.hpp"
#include "HelloWindow.hpp"
#include "Texture.hpp"

CD3DX12_HEAP_PROPERTIES HeapPropertiesFactory::m_upload( D3D12_HEAP_TYPE_UPLOAD );
CD3DX12_HEAP_PROPERTIES HeapPropertiesFactory::m_default( D3D12_HEAP_TYPE_DEFAULT );

HelloWindow::HelloWindow( uint32_t width, uint32_t height, std::wstring title ) :
	DXSample( width, height, title ),
	m_frameIndex( 0 ),
	m_rtvDescriptorSize( 0 ),
	m_srvDescriptorSize( 0 )
{
}

void HelloWindow::OnInit( uint32_t width, uint32_t height )
{
	// width and height are the same as the window size.
	SetWidthAndHeight( width, height );

	LoadPipeline();
	LoadAssets();
	InitImGui();

	m_bIsInitialized = true;
}

// Update frame-based values.
void HelloWindow::OnUpdate( const StepTimer& kTimer )
{
	float fCurrentTime = static_cast< float >( kTimer.GetTotalSeconds() );
	float fDeltaTime = static_cast< float >( kTimer.GetElapsedSeconds() );

	using namespace DirectX;
	// Model Matrix
	{
		XMMATRIX model = XMLoadFloat4x4( &m_kConstantBuffer.model );

		// Identity
		model = XMMatrixIdentity();

		// Rotate
		auto angle = static_cast< float >( Math::Radians( 50.0f ) ) * fCurrentTime;
		XMVECTOR rotationAxis = XMVectorSet( 0.5f, 1.0f, 0.0f, 0.0f );
		XMMATRIX rotationMatrix = XMMatrixRotationAxis( rotationAxis, angle );

		// create world matrix by first rotating the cube and then translating it.
		model = XMMatrixMultiply( model, rotationMatrix );

		// Trans
		model = XMMatrixMultiply( model, XMMatrixTranslation( 0.0, 0.0f, 0.0f ) );

		// must transpose the matrix before sending it to the GPU.
		model = XMMatrixTranspose( model );

		// Update back
		XMStoreFloat4x4( &m_kConstantBuffer.model, model );
	}

	// View Project Matrix
	{
		XMMATRIX view = XMMatrixIdentity();
		XMMATRIX proj = XMMatrixPerspectiveFovLH( static_cast< float >( Math::Radians( 45.0 ) ), m_aspectRatio, 0.1f, 1000.0f );

		XMVECTOR cameraPos = XMVectorSet( 0.0f, 0.0f, -3.0f, 0.0f );
		XMVECTOR cameraTarget = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
		XMVECTOR cameraUp = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
		view = XMMatrixLookAtLH( cameraPos, cameraTarget, cameraUp );

		XMMATRIX viewProj = XMMatrixMultiply( view, proj );
		
		// must transpose the matrix before sending it to the GPU.
		viewProj = XMMatrixTranspose( viewProj );
		
		XMStoreFloat4x4( &m_kConstantBuffer.viewProj, viewProj );
	}

	memcpy( m_pCbvDataBegin, &m_kConstantBuffer, sizeof( m_kConstantBuffer ) );
}

// Render the scene.
void HelloWindow::OnRender()
{
	// Don't try to render anything before the first Update.
	if ( m_kTimer.GetFrameCount() == 0 )
	{
		return;
	}

	RenderImGui();

	// Record all the command we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the commad list.
	ID3D12CommandList* ppCommandList[] = { m_spCommandList.Get() };
	m_spCommandQueue->ExecuteCommandLists( _countof( ppCommandList ), ppCommandList );

	// Present the frame.
	ThrowIfFailed( m_spSwapChain->Present( 1, 0 ) );

	MoveToNextFrame();
}

void HelloWindow::OnDestroy()
{
	// Ensure that GPU is no langer refernencing resource that are about to be cleaned up by the destructor.
	WaitForGpu();

	// Cleanup Imgui
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_fenceEvent.Close();
}

void HelloWindow::OnSizeChanged( uint32_t width, uint32_t height )
{
	if ( !m_spDevice || !m_bIsInitialized )
	{
		// if device is not created, means no resources need to be recreated.
		return;
	}

	if ( m_width == width && m_height == height )
	{
		// if the size is not changed, no need to recreate resources.
		return;
	}

	SetWidthAndHeight( width, height );
	CreateResources();
}

void HelloWindow::CreateDevice()
{
	UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr< ID3D12Debug > spDebugController;
		if ( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &spDebugController ) ) ) )
		{
			spDebugController->EnableDebugLayer();

			// enable additional debug layers.
			ComPtr< IDXGIInfoQueue > spDxgiInfoQueue;
			if ( SUCCEEDED( DXGIGetDebugInterface1( 0, IID_PPV_ARGS( &spDxgiInfoQueue ) ) ) )
			{
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

				spDxgiInfoQueue->SetBreakOnSeverity( DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true );
				spDxgiInfoQueue->SetBreakOnSeverity( DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true );
			}
		}
	}
#endif
	ThrowIfFailed( CreateDXGIFactory2( dxgiFactoryFlags, IID_PPV_ARGS( m_spDxgiFactory.ReleaseAndGetAddressOf() ) ) );

	// get adpater
	ComPtr< IDXGIAdapter1 > spHardwareAdapter;
	GetHardwareAdapter( m_spDxgiFactory.Get(), &spHardwareAdapter );
	
	// create DX12 device
	ThrowIfFailed( D3D12CreateDevice( spHardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( m_spDevice.ReleaseAndGetAddressOf() ) ) );
	
#ifndef NDEBUG
	// Configure debug device (if active).
	ComPtr< ID3D12InfoQueue > spD3dInfoQueue;
	if ( SUCCEEDED( m_spDevice.As( &spD3dInfoQueue ) ) )
	{
#ifdef _DEBUG
		spD3dInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, true );
		spD3dInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, true );
#endif
		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			// workaround for debug layer issues on hybrid-graphics systems
			D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = static_cast< UINT >( std::size( hide ) );
		filter.DenyList.pIDList = hide;
		spD3dInfoQueue->AddStorageFilterEntries( &filter );
	}
#endif

	// create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed( m_spDevice->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( m_spCommandQueue.ReleaseAndGetAddressOf() ) ) );

	// create descriptor heaps
	{
		// create descriptor heap for render target views
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed( m_spDevice->CreateDescriptorHeap( &rtvHeapDesc, IID_PPV_ARGS( m_spRtvHeap.ReleaseAndGetAddressOf() ) ) );

		// create descriptor heap for depth stencil view
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed( m_spDevice->CreateDescriptorHeap( &dsvHeapDesc, IID_PPV_ARGS( m_spDsvHeap.ReleaseAndGetAddressOf() ) ) );

		// Describe and create a shader resource view (SRV) heap for the texture
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 3; // 0: ImGui, 1: Texture, 2: Constant Buffer
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed( m_spDevice->CreateDescriptorHeap( &srvHeapDesc, IID_PPV_ARGS( m_spSrvHeap.ReleaseAndGetAddressOf() ) ) );

		m_rtvDescriptorSize = m_spDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
		m_srvDescriptorSize = m_spDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
	}

	// create a command allocator for each back buffer that will be rendered to
	{
		for ( UINT n = 0; n < FrameCount; ++n )
		{
			ThrowIfFailed( m_spDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( m_spCommandAllocator[ n ].ReleaseAndGetAddressOf() ) ) );
		}
	}
	// create a command allocator for bundle usage
	ThrowIfFailed( m_spDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS( m_spBundleAllocator.ReleaseAndGetAddressOf() ) ) );

	// create a command list for recording graphics commands.
	m_frameIndex = 0;
	ThrowIfFailed( m_spDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_spCommandAllocator[ m_frameIndex ].Get(), nullptr, IID_PPV_ARGS( m_spCommandList.ReleaseAndGetAddressOf() ) ) );
	ThrowIfFailed( m_spCommandList->Close() );

	// create a command list for bundle usage
	ThrowIfFailed( m_spDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_spBundleAllocator.Get(), nullptr, IID_PPV_ARGS( m_spBundle.ReleaseAndGetAddressOf() ) ) );
	ThrowIfFailed( m_spBundle->Close() );

	// create a fence for tracking GPU execution progress => 
	// fence wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed( m_spDevice->CreateFence( m_fenceValue[ m_frameIndex ], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( m_spFence.ReleaseAndGetAddressOf() ) ) );
		m_fenceValue[ m_frameIndex ]++;

		// Create an event handle to use for frame synchronization
		m_fenceEvent.Attach( CreateEventEx( nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE ) );
		if ( !m_fenceEvent.IsValid() )
		{
			throw std::system_error( std::error_code( static_cast< int >( GetLastError() ), std::system_category() ), "CreateEventEx" );
		}

		// Check Shader Model 6 support
		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
		if ( FAILED( m_spDevice->CheckFeatureSupport( D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof( shaderModel ) ) ) || 
			 ( shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0 ) )
		{
			throw std::runtime_error( "Shader Model 6.0 is not supported!" );
		}
	}

	// Initialize device dependent objects here (independent of window size).
}

// Allocate all memory resources that change on a window SizeChanged event.
void HelloWindow::CreateResources()
{
	// Wait until all previous GPU work is complete.
	WaitForGpu();

	// Release resources that are tied ti the swap chain and update fence values.
	for ( UINT n = 0; n < FrameCount; ++n )
	{
		m_renderTargets[ n ].Reset();
		m_fenceValue[ n ] = m_fenceValue[ m_frameIndex ];
	}

	constexpr DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
	const auto backBufferWidth = static_cast< UINT >( m_width );
	const auto backBufferHeight = static_cast< UINT >( m_height );

	// if the swap chain already exists, resize it, otherwise create one.
	if ( m_spSwapChain )
	{
		auto hr = m_spSwapChain->ResizeBuffers( FrameCount, backBufferWidth, backBufferHeight, backBufferFormat, 0 );
		if ( hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET )
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			OnDeviceLost();

			// Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method
			// and correctly set up the new device.
			return;
		}
		else
		{
			ThrowIfFailed( hr );
		}
	}
	else
	{
		// create swap chain
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Width = backBufferWidth;
		swapChainDesc.Height = backBufferHeight;
		swapChainDesc.Format = backBufferFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		ComPtr< IDXGISwapChain1 > spSwapChain;
		ThrowIfFailed( m_spDxgiFactory->CreateSwapChainForHwnd( m_spCommandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
																Win32App::GetHwnd(),
																&swapChainDesc,
																&fsSwapChainDesc,
																nullptr,
																&spSwapChain ) );

		ThrowIfFailed( spSwapChain.As( &m_spSwapChain ) );

		// this template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut
		ThrowIfFailed( m_spDxgiFactory->MakeWindowAssociation( Win32App::GetHwnd(), DXGI_MWA_NO_ALT_ENTER ) );
	}

	// Obtain the back buffers for this window which will be the final render targets
	// and create render target views for each of them.
	{
		// Crate a RTV for each frame.
		for ( UINT n = 0; n < FrameCount; ++n )
		{
			ThrowIfFailed( m_spSwapChain->GetBuffer( n, IID_PPV_ARGS( &m_renderTargets[ n ] ) ) );

			wchar_t rtvName[ 25 ] = {};
			swprintf_s( rtvName, L"Render Target %d", n );
			m_renderTargets[ n ]->SetName( rtvName );
			
			auto rtvRootHandle = m_spRtvHeap->GetCPUDescriptorHandleForHeapStart();
			const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvCurrentHandle( rtvRootHandle, static_cast< INT >( n ), m_rtvDescriptorSize );
			m_spDevice->CreateRenderTargetView( m_renderTargets[ n ].Get(), nullptr, rtvCurrentHandle );
		}
	}

	// Reset the frame index to the current back buffer.
	m_frameIndex = m_spSwapChain->GetCurrentBackBufferIndex();

	// allocate a 2-D surface as the depth / stencil buffer and create a dpeth / stencil view on this surface
	const CD3DX12_HEAP_PROPERTIES depthHeapProperties( D3D12_HEAP_TYPE_DEFAULT );
	D3D12_RESOURCE_DESC drvDesc = CD3DX12_RESOURCE_DESC::Tex2D( 
		depthBufferFormat, 
		backBufferWidth, 
		backBufferHeight, 
		1, // This depth stencil view has only one texture.
		1  // Use a single mipmap level.
	);
	drvDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	const CD3DX12_CLEAR_VALUE depthOptimizedClearValue( depthBufferFormat, 1.0f, 0u );
	ThrowIfFailed( m_spDevice->CreateCommittedResource( 
		&depthHeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&drvDesc, 
		D3D12_RESOURCE_STATE_DEPTH_WRITE, 
		&depthOptimizedClearValue, 
		IID_PPV_ARGS( m_spDepthStencil.ReleaseAndGetAddressOf() ) ) );
	m_spDepthStencil->SetName( L"Depth Stencil" );

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = depthBufferFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	auto dsvHandle = m_spDsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_spDevice->CreateDepthStencilView( m_spDepthStencil.Get(), &dsvDesc, dsvHandle );

	// TODO: Initialize window size-dependent objects here.
}

void HelloWindow::OnDeviceLost()
{
	for ( UINT n = 0; n < FrameCount; ++n )
	{
		m_spCommandAllocator[ n ].Reset();
		m_renderTargets[ n ].Reset();
	}
	m_spBundleAllocator.Reset();

	m_spDepthStencil.Reset();
	m_spFence.Reset();
	m_spBundle.Reset();
	m_spCommandList.Reset();
	m_spSwapChain.Reset();
	m_spRtvHeap.Reset();
	m_spDsvHeap.Reset();
	m_spSrvHeap.Reset();
	m_spCommandQueue.Reset();
	m_spDxgiFactory.Reset();

	LoadPipeline();
}

// load the rendering pipeline dependencies.
void HelloWindow::LoadPipeline()
{
	CreateDevice();
	CreateResources();
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


		// Create a descriptor range (descriptor table) and a root parameter.
		std::vector< CD3DX12_DESCRIPTOR_RANGE1 > ranges( 2 );
		ranges[ 0 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC );	// 建立一個 SRV 從 t0 開始
		ranges[ 1 ].Init( D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC );  // 建立一個 CBV 從 b0 開始


		std::vector< CD3DX12_ROOT_PARAMETER1 > rootParameters( 2 );
		rootParameters[ 0 ].InitAsDescriptorTable( 1, &ranges[ 0 ], D3D12_SHADER_VISIBILITY_PIXEL );
		rootParameters[ 1 ].InitAsDescriptorTable( 1, &ranges[ 1 ], D3D12_SHADER_VISIBILITY_VERTEX );


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

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Init_1_1( static_cast< UINT >( rootParameters.size() ), rootParameters.data(), 1, &sampler, rootSignatureFlags);

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
	ThrowIfFailed( m_spDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_spCommandAllocator[ m_frameIndex ].Get(), m_spPipelineState.Get(), IID_PPV_ARGS(&m_spCommandList)));

	// Create the vertex buffer.
	ComPtr< ID3D12Resource > spVertexBufferUploadHeap;
	{
		// Define the geometry for a triangle.
		std::vector< Vertex > vertices =
		{
			// front face
			{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
			{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			{ { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },

			// back face
			{ { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
			{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
			{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
			
			// right face
			{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
			{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },

			// left face
			{ { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
			{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
			{ { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },

			// top face
			{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			{ { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
			
			// bottom face
			{ { -0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
			{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
			{ {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		};

		const size_t vertexBufferSize = vertices.size() * sizeof( Vertex );

		// Note: using upload heaps to transfer static data like vertex buffers is not recommended.
		// Every time the GPU needs it, the upload heap will be marshalled over.
		// Please read up on Default Heap usage. An upload heap is used here for code simplicity and
		// because there are very few vertices to actually transfer.
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( vertexBufferSize );
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			HeapPropertiesFactory::GetUploadHeapProperties(),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &spVertexBufferUploadHeap )
		) );

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange( 0, 0 ); // we do not intend to read from this resource on the CPU.
		ThrowIfFailed( spVertexBufferUploadHeap->Map( 0, &readRange, reinterpret_cast< void** >( &pVertexDataBegin ) ) );
		memcpy( pVertexDataBegin, vertices.data(), vertexBufferSize );
		spVertexBufferUploadHeap->Unmap( 0, nullptr );

		// create vertex buffer in default heap
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			HeapPropertiesFactory::GetDefaultHeapProperties(),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &m_spVertexBuffer )
		) );
		m_spCommandList->CopyBufferRegion( m_spVertexBuffer.Get(), 0, spVertexBufferUploadHeap.Get(), 0, vertexBufferSize );

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition( m_spVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
		m_spCommandList->ResourceBarrier( 1, &barrier );

		// Initialize the vertex buffer view.
		m_spVertexBufferView.BufferLocation = m_spVertexBuffer->GetGPUVirtualAddress();
		m_spVertexBufferView.StrideInBytes = sizeof( Vertex );
		m_spVertexBufferView.SizeInBytes = static_cast< UINT >( vertexBufferSize );
	}

	// Create Index Buffer
	ComPtr< ID3D12Resource > spIndexBufferUploadHeap;
	{
		std::vector< uint16_t > indices =
		{
			0,  1,  2,
			0,  2,  3,

			4,  5,  6,
			4,  6,  7,

			8,  9, 10,
			8, 10, 11,

			12, 13, 14,
			12, 14, 15,

			16, 17, 18,
			16, 18, 19,

			20, 21, 22,
			20, 22, 23,
		};

		const size_t indexBufferSize = indices.size() * sizeof( uint16_t );

		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( indexBufferSize );
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			HeapPropertiesFactory::GetUploadHeapProperties(),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &spIndexBufferUploadHeap )
		) );

		// Copy the triangle data to the vertex buffer.
		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE readRange( 0, 0 ); // we do not intend to read from this resource on the CPU.
		ThrowIfFailed( spIndexBufferUploadHeap->Map( 0, &readRange, reinterpret_cast< void** >( &pIndexDataBegin ) ) );
		memcpy( pIndexDataBegin, indices.data(), indexBufferSize );
		spIndexBufferUploadHeap->Unmap( 0, nullptr );

		// create vertex buffer in default heap
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			HeapPropertiesFactory::GetDefaultHeapProperties(),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &m_spIndexBuffer )
		) );
		m_spCommandList->CopyBufferRegion( m_spIndexBuffer.Get(), 0, spIndexBufferUploadHeap.Get(), 0, indexBufferSize );
		
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition( m_spIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER );
		m_spCommandList->ResourceBarrier( 1, &barrier );

		// Initialize the vertex buffer view.
		m_spIndexBufferView.BufferLocation = m_spIndexBuffer->GetGPUVirtualAddress();
		m_spIndexBufferView.SizeInBytes = static_cast< UINT >( indexBufferSize );
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
		textureDesc.Width = static_cast< UINT >( spTexture->width );
		textureDesc.Height = static_cast< UINT >( spTexture->height );
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			HeapPropertiesFactory::GetDefaultHeapProperties(),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &m_spTexture )
		) );

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize( m_spTexture.Get(), 0, 1 );

		// Create the GPU upload buffer.
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( uploadBufferSize );
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			HeapPropertiesFactory::GetUploadHeapProperties(),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
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
		
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition( m_spTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
		m_spCommandList->ResourceBarrier( 1, &barrier );

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle( m_spSrvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_srvDescriptorSize );
		m_spDevice->CreateShaderResourceView( m_spTexture.Get(), &srvDesc, srvHandle );
	}

	// Close the command list and execute it to begin the initial GPU setup.
	ThrowIfFailed( m_spCommandList->Close() );
	ID3D12CommandList* ppCommandLists[] = { m_spCommandList.Get() };
	m_spCommandQueue->ExecuteCommandLists( _countof( ppCommandLists ), ppCommandLists );

	// Create the contant buffer
	{
		const UINT constantBufferSize = sizeof( SceneConstantBuffer );

		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( constantBufferSize );
		ThrowIfFailed( m_spDevice->CreateCommittedResource(
			HeapPropertiesFactory::GetUploadHeapProperties(),
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &m_spConstantBuffer )
		) );

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_spConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle( m_spSrvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_srvDescriptorSize );
		m_spDevice->CreateConstantBufferView( &cbvDesc, srvHandle );

		// Map and initialize the constant buffer. we don't unmap this untill the app close.
		// Keeping mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange( 0, 0 ); // we do not intend to read from this resource on the CPU.
		ThrowIfFailed( m_spConstantBuffer->Map( 0, &readRange, reinterpret_cast< void** >( &m_pCbvDataBegin ) ) );
		memcpy( m_pCbvDataBegin, &m_kConstantBuffer, sizeof( m_kConstantBuffer ) );
	}

	// Create and record the bundle
	{
		ThrowIfFailed( m_spBundleAllocator->Reset() );
		ThrowIfFailed( m_spBundle->Reset( m_spBundleAllocator.Get(), m_spPipelineState.Get() ) );
		m_spBundle->SetGraphicsRootSignature( m_spRootSignature.Get() );
		m_spBundle->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		m_spBundle->IASetVertexBuffers( 0, 1, &m_spVertexBufferView );
		m_spBundle->IASetIndexBuffer( &m_spIndexBufferView );
		m_spBundle->DrawIndexedInstanced( 36, 1, 0, 0, 0 );
		ThrowIfFailed( m_spBundle->Close() );
	}

	WaitForGpu();
}

void HelloWindow::InitImGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); ( void )io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;        // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// 嘗試抓取 ImGui 的 Font Texture 在 Descriptor Heap 中的位置
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle( m_spSrvHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_srvDescriptorSize );
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle( m_spSrvHeap->GetGPUDescriptorHandleForHeapStart(), 0, m_srvDescriptorSize );

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init( Win32App::GetHwnd() );
	ImGui_ImplDX12_Init( m_spDevice.Get(), FrameCount, 
						 DXGI_FORMAT_R8G8B8A8_UNORM, 
						 m_spSrvHeap.Get(), 
						 srvCpuHandle,
						 srvGpuHandle );
}

void HelloWindow::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated
	// command lists have finished execution on the GPU; apps should use
	// fences to determine GPU execution progress.
	ThrowIfFailed( m_spCommandAllocator[ m_frameIndex ]->Reset() );

	// However, when ExecuteCommandList() is called on a particular command list,
	// that command list can be reset at any time and must be before re-recording
	ThrowIfFailed( m_spCommandList->Reset( m_spCommandAllocator[ m_frameIndex ].Get(), m_spPipelineState.Get() ) );

	// Every command list only allow sets descriptor deap one time. 
	ID3D12DescriptorHeap* ppHeaps[] = { m_spSrvHeap.Get() };
	m_spCommandList->SetDescriptorHeaps( _countof( ppHeaps ), ppHeaps );

	// Set necessary state.
	m_spCommandList->SetGraphicsRootSignature( m_spRootSignature.Get() );

	// 從 Descriptor Heap 中取得 index 為 1 的 Gpu Handle => 這是設計用來給我們方塊貼上 Texture
	CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle( m_spSrvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_srvDescriptorSize );
	CD3DX12_GPU_DESCRIPTOR_HANDLE constantHandle( m_spSrvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_srvDescriptorSize );
	m_spCommandList->SetGraphicsRootDescriptorTable( 0, textureHandle ); // 當時我們已經定義好說 Root Signature 的第 0 個參數是一個 Descriptor Table （SRV）
	m_spCommandList->SetGraphicsRootDescriptorTable( 1, constantHandle );

	// Indicate that the back buffer will be used as a render target.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition( m_renderTargets[ m_frameIndex ].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
	m_spCommandList->ResourceBarrier( 1, &barrier );

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( m_spRtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize );
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle( m_spDsvHeap->GetCPUDescriptorHandleForHeapStart() );
	m_spCommandList->OMSetRenderTargets( 1, &rtvHandle, FALSE, &dsvHandle );

	const float clearColor[ 4 ] = { m_clearColor.x * m_clearColor.w, m_clearColor.y * m_clearColor.w, m_clearColor.z * m_clearColor.w, m_clearColor.w };
	m_spCommandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );
	m_spCommandList->ClearDepthStencilView( dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr );

	// Set the viewport and scissor rect.
	const CD3DX12_VIEWPORT viewport( 0.0f, 0.0f, static_cast< float >( m_width ), static_cast< float >( m_height ), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH );
	const CD3DX12_RECT scissorRect( 0, 0, static_cast< LONG >( m_width ), static_cast< LONG >( m_height ) );
	m_spCommandList->RSSetViewports( 1, &viewport );
	m_spCommandList->RSSetScissorRects( 1, &scissorRect );

	// Draw the scene
	m_spCommandList->ExecuteBundle( m_spBundle.Get() );

	// Render ImGui
	ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), m_spCommandList.Get() );

	// Indicate that the back buffer will now be used to present.
	barrier = CD3DX12_RESOURCE_BARRIER::Transition( m_renderTargets[ m_frameIndex ].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );
	m_spCommandList->ResourceBarrier( 1, &barrier );

	ThrowIfFailed( m_spCommandList->Close() );
}

void HelloWindow::RenderImGui()
{
	// Start the Dear imgui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if ( m_showDemoWindow )
	{
		ImGui::ShowDemoWindow( &m_showDemoWindow );
	}

	{
		ImGuiIO& io = ImGui::GetIO(); ( void ) io;
		ImGui::Begin( "Hello, World" );
		ImGui::Checkbox( "Demo Window", &m_showDemoWindow );
		ImGui::ColorEdit3( "clear color", ( float* ) &m_clearColor );
		ImGui::Text( "Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate );
		ImGui::End();
	}

	ImGui::Render();
}

// Wait for pending GPU work to complete.
void HelloWindow::WaitForGpu()
{
	if ( !m_spCommandQueue || !m_spFence || !m_fenceEvent.IsValid() )
	{
		return;
	}

	// Scheudle a Signal command in the queue.
	ThrowIfFailed( m_spCommandQueue->Signal( m_spFence.Get(), m_fenceValue[ m_frameIndex ] ) );

	// Wait untill the fence has been processed.
	ThrowIfFailed( m_spFence->SetEventOnCompletion( m_fenceValue[ m_frameIndex ], m_fenceEvent.Get() ) );
	std::ignore = WaitForSingleObjectEx( m_fenceEvent.Get(), INFINITE, FALSE );

	// Increment the fence value for the current frame.
	m_fenceValue[ m_frameIndex ]++;
}

// Prepare to render the next frame.
void HelloWindow::MoveToNextFrame()
{
	// Scheudle a Signal command in the queue.
	const UINT64 currentFenceValue = m_fenceValue[ m_frameIndex ];
	ThrowIfFailed( m_spCommandQueue->Signal( m_spFence.Get(), currentFenceValue ) );

	// Update the frame index.
	m_frameIndex = m_spSwapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if ( m_spFence->GetCompletedValue() < m_fenceValue[ m_frameIndex ] )
	{
		ThrowIfFailed( m_spFence->SetEventOnCompletion( m_fenceValue[ m_frameIndex ], m_fenceEvent.Get() ) );
		std::ignore = WaitForSingleObjectEx( m_fenceEvent.Get(), INFINITE, FALSE );
	}

	// Set the fence value for the next frame.
	m_fenceValue[ m_frameIndex ] = currentFenceValue + 1;
}
