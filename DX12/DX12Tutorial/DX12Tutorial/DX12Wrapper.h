#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

class DX12Wrapper
{
private:
	ID3D12Device* dev;
	ID3D12DescriptorHeap* descriptorHeap;
	ID3D12CommandQueue* commandQueue;
	IDXGIFactory6* dxgiFactory;
	IDXGISwapChain4* swapchain;

	HRESULT result;
public:
	DX12Wrapper();
	~DX12Wrapper();
};

