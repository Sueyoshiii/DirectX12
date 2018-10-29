#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <memory>

class PMDModel;

struct TransformMaterices {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX wvp;
};

struct Material {
	Material() {}
	Material(DirectX::XMFLOAT4& diff) : diffuse(DirectX::XMFLOAT4(diff.x, diff.y, diff.z, diff.w)) {}
	Material(float r, float g, float b, float a) : diffuse(DirectX::XMFLOAT4(r, g, b, a)) {}
	DirectX::XMFLOAT4 diffuse;	//拡散反射
	DirectX::XMFLOAT4 specular;	//鏡面反射
	DirectX::XMFLOAT4 ambient;	//環境光成分
};

class DX12Wrapper
{
private:
	UINT fenceValue;
	ID3D12Fence * fence;
	ID3D12Device* dev;
	ID3D12DescriptorHeap* rtvDescHeap;
	ID3D12DescriptorHeap* srvDescHeap;
	ID3D12DescriptorHeap* dsvHeap;
	ID3D12DescriptorHeap* materialHeap;
	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList3* commandList;
	ID3D12CommandQueue* commandQueue;
	IDXGIFactory6* dxgiFactory;
	IDXGISwapChain4* swapchain;
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pipelineState;
	ID3D12Resource* verticesBuff;
	ID3D12Resource* constantBuff;
	ID3D12Resource* depthBuffer;
	ID3D12Resource* texbuff;
	ID3D12Resource* whiteTexBuff;
	//ID3D12Resource* materialBuff;

	std::shared_ptr<PMDModel> model;
	std::vector<ID3D12Resource*> backBuffers;

	HRESULT result;

	DirectX::TexMetadata metadata;
	DirectX::ScratchImage img;

	void InitVertices(void);
	void InitShaders(void);
	void InitTexture(void);
	void InitConstants(void);
	void InitTextureForDSV(void);
	void InitDescriptorHeapForDSV(void);
	void InitDepthView(void);


	D3D12_VIEWPORT SetViewPort();
	D3D12_RECT SetRect();
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	D3D12_SHADER_RESOURCE_VIEW_DESC texView;
	D3D12_RESOURCE_DESC depthResDesc;

	TransformMaterices* mappedMatrix;
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX camera;
	DirectX::XMMATRIX projection;

	//白テクスチャ生成
	void CreateWhite(void);
	////黒テクスチャ生成
	//void CreateBlack(void);
public:
	DX12Wrapper();
	~DX12Wrapper();
	void ExecuteCommand(void);
	void WaitExecute(void);
	void Update(void);
};

