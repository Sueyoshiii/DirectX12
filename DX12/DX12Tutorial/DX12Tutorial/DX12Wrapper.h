#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

class DX12Wrapper
{
private:
	ID3D12Device* dev;
	ID3D12CommandQueue* commandqueue;
	IDXGIFactory6* dxgiFactory;
	IDXGISwapChain4* swapchain;
public:
	DX12Wrapper(HINSTANCE h, HWND hwnd);
	~DX12Wrapper();
};

