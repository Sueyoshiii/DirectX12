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
	ID3D12DescriptorHeap* rtvDescHeap;
	ID3D12DescriptorHeap* srvDescHeap;
	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList3* commandList;
	ID3D12CommandQueue* commandQueue;
	IDXGIFactory6* dxgiFactory;
	IDXGISwapChain4* swapchain;
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pipelineState;
	ID3D12Resource* verticesBuff;
	ID3D12Resource* cBuff;

	std::vector<ID3D12Resource*> backBuffers;

	HRESULT result;

	void CreateVertices();
	void InitShaders();
	void InitTexture();
	void InitConstants();
	D3D12_VIEWPORT SetViewPort();
	D3D12_RECT SetRect();
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	D3D12_SHADER_RESOURCE_VIEW_DESC texView;
public:
	DX12Wrapper();
	~DX12Wrapper();
	void ExecuteCommand();
	void WaitExecute();
	void Update();
};

