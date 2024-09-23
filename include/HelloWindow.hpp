#pragma once
#include "DXSample.hpp"

using Microsoft::WRL::ComPtr;

class HelloWindow : public DXSample
{
public:
	HelloWindow( uint32_t width, uint32_t height, std::wstring title );

	virtual void OnInit( uint32_t width, uint32_t height );
	virtual void OnUpdate( const StepTimer& kTimer );
	virtual void OnRender();
	virtual void OnDestroy();

	virtual void OnSizeChanged( uint32_t width, uint32_t height );

private:
	virtual void CreateDevice();
	virtual void CreateResources();
	virtual void OnDeviceLost();

	void LoadPipeline();
	void LoadAssets();
	void InitImGui();
	void PopulateCommandList();
	void RenderImGui();
	void WaitForGpu();
	void MoveToNextFrame();

	// In the sample we overload the meaning of FrameCount to mean both the maximum
	// number of frames that will be queued to the GPU at a time, as well as the number
	// of back buffers in the DXGI swap chain. For the majority of applications, this is
	// convenient and works well. However, there will be certain cases where an application
	// may want to queue up more frames than there are back buffers available.
	// It should be noted that excessive buffering of frames dependent our user input may result
	// in noticeable latency in yout application.
	static const UINT FrameCount = 2;

	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 uv;
	};

	struct SceneConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 viewProj;
		float padding[ 32 ]; // Padding so the constant buffer is 256-byte aligned.
	};
	static_assert( ( sizeof( SceneConstantBuffer ) % 256 ) == 0, "Constant Buffer size must be 256-byte aligned" );

	// Pipeline objects;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr< ID3D12Device > m_spDevice;
	ComPtr< IDXGIFactory4 > m_spDxgiFactory;
	ComPtr< ID3D12CommandQueue > m_spCommandQueue;
	ComPtr< ID3D12DescriptorHeap > m_spRtvHeap;
	ComPtr< ID3D12DescriptorHeap > m_spDsvHeap;
	ComPtr< ID3D12DescriptorHeap > m_spSrvHeap;
	ComPtr< ID3D12CommandAllocator > m_spCommandAllocator[ FrameCount ];
	ComPtr< ID3D12CommandAllocator > m_spBundleAllocator;
	ComPtr< ID3D12GraphicsCommandList > m_spCommandList;
	ComPtr< ID3D12GraphicsCommandList > m_spBundle;
	ComPtr< ID3D12PipelineState > m_spPipelineState;
	ComPtr< ID3D12RootSignature > m_spRootSignature;
	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;

	// Backbuffer / Renderiing resources
	ComPtr< IDXGISwapChain3 > m_spSwapChain;
	ComPtr< ID3D12Resource > m_renderTargets[ FrameCount ];
	ComPtr< ID3D12Resource > m_depthStencil;

	// App resources.
	ComPtr< ID3D12Resource > m_spVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_spVertexBufferView;

	ComPtr< ID3D12Resource > m_spIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_spIndexBufferView;

	// Texture
	ComPtr< ID3D12Resource > m_spTexture;

	// Constant Buffer
	ComPtr< ID3D12Resource > m_spConstantBuffer;
	D3D12_INDEX_BUFFER_VIEW m_spConstantBufferView;
	SceneConstantBuffer m_kConstantBuffer;
	UINT8* m_pCbvDataBegin = nullptr;

	// Scene State
	bool m_showDemoWindow = false;
	DirectX::XMFLOAT4 m_clearColor = { 0.45f, 0.55f, 0.60f, 1.0f };

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_hFenceEvent;
	ComPtr < ID3D12Fence > m_spFence;
	UINT64 m_fenceValue[ FrameCount ] = { 0, 0 };

};