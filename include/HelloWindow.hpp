#pragma once
#include "DXSample.hpp"

using Microsoft::WRL::ComPtr;

class HelloWindow : public DXSample
{
public:
	HelloWindow( uint32_t width, uint32_t height, std::wstring title );

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();

private:
	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	static const UINT FrameCount = 2;

	// Pipeline objects;
	ComPtr< IDXGISwapChain3 > m_spSwapChain;
	ComPtr< ID3D12Device > m_spDevice;
	ComPtr< ID3D12CommandQueue > m_spCommandQueue;
	ComPtr< ID3D12DescriptorHeap > m_spRtvHeap;
	ComPtr< ID3D12Resource > m_renderTargets[ FrameCount ];
	ComPtr< ID3D12CommandAllocator > m_spCommandAllocator;
	ComPtr< ID3D12GraphicsCommandList > m_spCommandList;
	ComPtr< ID3D12PipelineState > m_spPipelineState;
	UINT m_rtvDescriptorSize;

	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_hFenceEvent;
	ComPtr < ID3D12Fence > m_spFence;
	UINT64 m_fenceValue;

};