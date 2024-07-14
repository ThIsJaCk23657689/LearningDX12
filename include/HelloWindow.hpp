#pragma once
#include "DXSample.hpp"

using Microsoft::WRL::ComPtr;

class HelloWindow : public DXSample
{
public:
	HelloWindow( uint32_t width, uint32_t height, std::wstring title );

	virtual void OnInit();
	virtual void OnUpdate( const StepTimer& kTimer );
	virtual void OnRender();
	virtual void OnDestroy();

private:
	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	static const UINT FrameCount = 2;

	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 uv;
	};

	struct SceneConstantBuffer
	{
		DirectX::XMFLOAT4 offset = { 0, 0, 0, 0 };
		float padding[ 60 ]; // Padding so the constant buffer is 256-byte aligned.
	};
	static_assert( ( sizeof( SceneConstantBuffer ) % 256 ) == 0, "Constant Buffer size must be 256-byte aligned" );

	// Pipeline objects;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr< IDXGISwapChain3 > m_spSwapChain;
	ComPtr< ID3D12Device > m_spDevice;
	ComPtr< ID3D12CommandQueue > m_spCommandQueue;
	ComPtr< ID3D12DescriptorHeap > m_spRtvHeap;
	ComPtr< ID3D12DescriptorHeap > m_spSrvHeap;
	ComPtr< ID3D12Resource > m_renderTargets[ FrameCount ];
	ComPtr< ID3D12CommandAllocator > m_spCommandAllocator;
	ComPtr< ID3D12CommandAllocator > m_spBundleAllocator;
	ComPtr< ID3D12GraphicsCommandList > m_spCommandList;
	ComPtr< ID3D12GraphicsCommandList > m_spBundle;
	ComPtr< ID3D12PipelineState > m_spPipelineState;
	ComPtr< ID3D12RootSignature > m_spRootSignature;
	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;

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


	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_hFenceEvent;
	ComPtr < ID3D12Fence > m_spFence;
	UINT64 m_fenceValue;

};