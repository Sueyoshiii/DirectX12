#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

class DX12Wrapper
{
private:
	UINT fenceValue;
	ID3D12Fence * fence;
	ID3D12Device* dev;
	ID3D12DescriptorHeap* descriptorHeap;
	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList3* commandList;
	ID3D12CommandQueue* commandQueue;
	IDXGIFactory6* dxgiFactory;
	IDXGISwapChain4* swapchain;
	ID3D12RootSignature* rootSignature;

	std::vector<ID3D12Resource*> backBuffers;

	HRESULT result;

	ID3D12Resource* verticesBuff;
	//頂点情報を定義し、頂点バッファを作る
	void CreateVertices();
	void InitShaders();
	D3D12_VIEWPORT SetViewPort();
public:
	DX12Wrapper();
	~DX12Wrapper();
	void ExecuteCommand();
	void WaitExecute();
	void Update();
};

