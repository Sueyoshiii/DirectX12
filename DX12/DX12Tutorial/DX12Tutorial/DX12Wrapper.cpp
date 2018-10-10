#include "DX12Wrapper.h"
#include "Application.h"
#include <DirectXTex.h>
#include <Windows.h>
#include <algorithm>
#include <random>
#include <d3dcompiler.h>
#include "d3dx12.h"
#include <tchar.h>
#include "PMDModel.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

//頂点情報
struct Vertex {
	XMFLOAT3 pos; //頂点座標
	XMFLOAT2 uv;
};

DX12Wrapper::DX12Wrapper()
{
	D3D_FEATURE_LEVEL levels[] = {

		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	//エラーを出力に表示させる
#ifdef _DEBUG
	ID3D12Debug *debug = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (FAILED(result))
		int i = 0;
	debug->EnableDebugLayer();
	debug->Release();
	debug = nullptr;
#endif

	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featurelevel;
	for (auto& l : levels)
	{
		if (D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&dev)) == S_OK)
		{
			featurelevel = l;
			break;
		}
	}

	model.reset(new PMDModel("Model/初音ミク.pmd"));

	//コマンドキューオブジェクトの作成
	D3D12_COMMAND_QUEUE_DESC commandQDesc = {};
	commandQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQDesc.NodeMask = 0;
	commandQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	result = dev->CreateCommandQueue(
		&commandQDesc,
		IID_PPV_ARGS(&commandQueue)
	);

	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	//スワップチェーンの生成
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	auto wsize = Application::Instance().GetWindowSize();
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Width = wsize.w;
	swapchainDesc.Height = wsize.h;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.Flags = 0;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	result = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue,
		Application::Instance().GetWindowHandle(),
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(&swapchain)
	);

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	result = dev->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(&rtvDescHeap)
	);

	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescH = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
		//各デスクリプタの使用するサイズを計算
		auto rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		DXGI_SWAP_CHAIN_DESC swchDesc = {};
		swapchain->GetDesc(&swchDesc);
		backBuffers.resize(swchDesc.BufferCount);
		for (int i = 0; i < backBuffers.size(); ++i)
		{
			result = swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
			dev->CreateRenderTargetView(backBuffers[i], nullptr, cpuDescH);
			cpuDescH.ptr += rtvSize;
		}
	}
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	commandList->Close();
	
	fenceValue = 0;
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	InitVertices();
	InitShaders();
	InitTextureForDSV();
	InitDescriptorHeapForDSV();
	InitDepthView();
	InitTexture();
	InitConstants();
}

void DX12Wrapper::ExecuteCommand()
{
	ID3D12CommandList* commandList2[] = { commandList };
	commandQueue->ExecuteCommandLists(1, commandList2);
	commandQueue->Signal(fence, ++fenceValue);
}

void DX12Wrapper::WaitExecute()
{
	while (fence->GetCompletedValue() < fenceValue)
	{

	}
}

void DX12Wrapper::Update()
{
	commandAllocator->Reset();

	commandList->Reset(commandAllocator, pipelineState);
	commandList->SetPipelineState(pipelineState);
	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->RSSetViewports(1, &SetViewPort());
	commandList->RSSetScissorRects(1, &SetRect());

	auto bbIdx = swapchain->GetCurrentBackBufferIndex();

	static float angle = 0.0f;
	world = XMMatrixRotationY(angle);

	mappedMatrix->world = world;
	mappedMatrix->wvp = world * camera * projection;
	angle += 0.001f;

	D3D12_RESOURCE_BARRIER barrier;
	ZeroMemory(&barrier, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffers[bbIdx];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//バリア設置
	commandList->ResourceBarrier(1, &barrier);

	//レンダーターゲットの指定
	auto descH = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	descH.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	commandList->OMSetRenderTargets(1, &descH, false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());

	float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	//レンダーターゲットのクリア
	commandList->ClearRenderTargetView(descH, color, 0, nullptr);
	//深度バッファを毎フレームクリア
	commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(0, 1, &vbView);

	commandList->IASetIndexBuffer(&ibView);

	commandList->SetDescriptorHeaps(1, &srvDescHeap);

	commandList->SetGraphicsRootDescriptorTable(0, srvDescHeap->GetGPUDescriptorHandleForHeapStart());
	
	commandList->DrawIndexedInstanced(model->pmdindices.size(), 1, 0, 0, 0);
	//commandList->DrawInstanced(model->pmdvertices.size(), 1, 0, 0);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//バリア設置
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();

	ExecuteCommand();
	WaitExecute();

	swapchain->Present(0, 0);
}

//頂点情報を定義し、頂点バッファを作る
void DX12Wrapper::InitVertices()
{
	//Nの字になるよう配置
	//Vertex vertices[] = {
	//	XMFLOAT3(5.0f, -5.0f, -5.0f ), XMFLOAT2(1.0f, 1.0f),
	//	XMFLOAT3(-5.0f, -5.0f, -5.0f), XMFLOAT2(0.0f, 1.0f),
	//	XMFLOAT3(5.0f, 5.0f, -5.0f), XMFLOAT2(1.0f, 0.0f),
	//	XMFLOAT3(-5.0f, 5.0f, -5.0f), XMFLOAT2(0.0f, 0.0f),

	//	XMFLOAT3(-5.0f, -5.0f, 5.0f), XMFLOAT2(1.0f, 1.0f),
	//	XMFLOAT3(5.0f, -5.0f, 5.0f), XMFLOAT2(0.0f, 1.0f),
	//	XMFLOAT3(-5.0f, 5.0f, 5.0f), XMFLOAT2(1.0f, 0.0f),
	//	XMFLOAT3(5.0f, 5.0f, 5.0f), XMFLOAT2(0.0f, 0.0f),
	//};

	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(model->pmdvertices.size() * sizeof(model->pmdvertices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&verticesBuff)
	);
	PMDVertex* vbuffptr = nullptr;
	result = verticesBuff->Map(0, nullptr, (void**)&vbuffptr);
	std::copy(model->pmdvertices.begin(), model->pmdvertices.end(), vbuffptr);

	verticesBuff->Unmap(0, nullptr);

	vbView = {};
	vbView.BufferLocation = verticesBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = model->pmdvertices.size() * sizeof(model->pmdvertices[0]);
	vbView.StrideInBytes = sizeof(PMDVertex);

	//std::vector<unsigned short> indices = { 
	//	0, 1, 2, 1, 3, 2,
	//	1, 4, 3, 4, 6, 3,
	//	4, 5, 6, 5, 7, 6,
	//	5, 0, 7, 0, 2, 7,
	//	5, 4, 0, 4, 1, 0,
	//	2, 3, 7, 3, 6, 7
	//};
	auto indices = model->pmdindices;

	ID3D12Resource* indexBuff = nullptr;
	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);
	unsigned short* indexmap = nullptr;

	result = indexBuff->Map(0, nullptr, (void**)&indexmap);
	std::copy(indices.begin(), indices.end(), indexmap);
	indexBuff->Unmap(0, nullptr);
}

void DX12Wrapper::InitShaders()
{
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vsBlob, nullptr);
	D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &psBlob, nullptr);

	//ルートシグネチャ
	rootSignature = nullptr;

	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};
	D3D12_ROOT_PARAMETER rootParam = {};

	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[0].BaseShaderRegister = 0;
	descTblRange[0].NumDescriptors = 1;
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].BaseShaderRegister = 0;
	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.NumDescriptorRanges = 2;
	rootParam.DescriptorTable.pDescriptorRanges = descTblRange;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 1;
	rsd.pParameters = &rootParam;

	D3D12_STATIC_SAMPLER_DESC sampleDesc = {};
	sampleDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampleDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampleDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampleDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampleDesc.MaxLOD = D3D12_FLOAT32_MAX;
	sampleDesc.MinLOD = 0.0f;
	sampleDesc.MipLODBias = 0.0f;
	sampleDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	sampleDesc.ShaderRegister = 0;
	sampleDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	sampleDesc.RegisterSpace = 0;
	sampleDesc.MaxAnisotropy = 0;
	sampleDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &sampleDesc;
	rsd.NumStaticSamplers = 1;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error
	);
	result = dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));

	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{
			"POSITION", 0, 
			DXGI_FORMAT_R32G32B32_FLOAT, 0, 
			D3D12_APPEND_ALIGNED_ELEMENT, 
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		} , 
		{
			"NORMAL", 0,
			DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		//{
		//	"TEXCOORD", 0,
		//	DXGI_FORMAT_R32G32_FLOAT, 0,
		//	D3D12_APPEND_ALIGNED_ELEMENT,
		//	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		//}
	};


	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob);
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;
	gpsDesc.pRootSignature = rootSignature;
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.SampleMask = 0xffffff;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineState = nullptr;

	result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));
}

void DX12Wrapper::InitTexture()
{
	TexMetadata metadata = {};
	ScratchImage img;
	result = LoadFromWICFile(L"img/Site_Cat.png", WIC_FLAGS_NONE, &metadata, img);

	D3D12_HEAP_PROPERTIES heaprop = {};
	heaprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heaprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heaprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heaprop.CreationNodeMask = 1;
	heaprop.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = metadata.width;
	desc.Height = metadata.height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* texbuff = nullptr;
	result = dev->CreateCommittedResource(
		&heaprop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	std::random_device rd;
	std::mt19937_64 mt(rd());
	std::vector<unsigned char> data(256 * 256 * 4);
	//for (int i = 0; i < data.size(); i += 4)
	//{
	//	data[i] = mt() % 256;
	//	data[i + 1] = mt() % 256;
	//	data[i + 2] = mt() % 256;
	//	data[i + 3] = 255;
	//}
	//D3D12_BOX box = {};
	//box.left = 0;
	//box.right = 256;
	//box.top = 0;
	//box.bottom = 256;
	//box.front = 0;
	//box.back = 1;
	result = texbuff->WriteToSubresource(
		0,
		nullptr,
		img.GetPixels(),
		metadata.width * 4,
		img.GetPixelsSize()
	);

	commandAllocator->Reset();
	commandList->Reset(commandAllocator, nullptr);
	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			texbuff, 
			D3D12_RESOURCE_STATE_COPY_DEST, 
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		)
	);
	commandList->Close();
	//ExecuteCommand();
	//WaitExecute();

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&srvDescHeap));
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	dev->CreateShaderResourceView(texbuff, &srvDesc, srvDescHeap->GetCPUDescriptorHandleForHeapStart());

	img.Release();
}

void DX12Wrapper::InitConstants()
{
	Application& app = Application::Instance();
	auto wsize = app.GetWindowSize();
	auto angle = (XM_PI / 4.0f);
	TransformMaterices matrix;
	matrix.world = XMMatrixRotationY(angle);

	XMFLOAT3 eye(0, 10, -20);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	camera = XMMatrixLookAtLH(
		XMLoadFloat3(&eye), 
		XMLoadFloat3(&target), 
		XMLoadFloat3(&up));

	projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV2, 
		static_cast<float>(wsize.w) / static_cast<float>(wsize.h),
		0.1f,
		300.0f);

	matrix.wvp = matrix.world * camera * projection;

	size_t size = sizeof(matrix);

	size = (size + 0xff) & ~0xff;

	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cBuff)
	);
	mappedMatrix = nullptr;
	result = cBuff->Map(0, nullptr, (void**)&mappedMatrix);
	*mappedMatrix = matrix;
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	//XMMATRIX* m = nullptr;
	//result = cBuff->Map(0, nullptr, (void**)&m);
	//*m += matrix;D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = cBuff->GetGPUVirtualAddress();
	desc.SizeInBytes = size;
	auto handle = srvDescHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	dev->CreateConstantBufferView(&desc, handle);
}

void DX12Wrapper::InitTextureForDSV()
{
	depthResDesc = {};
	auto wsize = Application::Instance().GetWindowSize();
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = wsize.w;
	depthResDesc.Height = wsize.h;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
}

void DX12Wrapper::InitDescriptorHeapForDSV()
{
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer)
	);
}

void DX12Wrapper::InitDepthView()
{

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeap = nullptr;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = dev->CreateDescriptorHeap(
		&dsvHeapDesc,
		IID_PPV_ARGS(&dsvHeap)
	);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dev->CreateDepthStencilView(
		depthBuffer,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);
}

D3D12_VIEWPORT DX12Wrapper::SetViewPort()
{
	D3D12_VIEWPORT viewPort;
	SecureZeroMemory(&viewPort, sizeof(viewPort));
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = Application::Instance().GetWindowSize().w;
	viewPort.Height = Application::Instance().GetWindowSize().h;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	return viewPort;
}

D3D12_RECT DX12Wrapper::SetRect()
{
	D3D12_RECT scissorrect;
	SecureZeroMemory(&scissorrect, sizeof(scissorrect));
	scissorrect.left = 0;
	scissorrect.top = 0;
	scissorrect.right = Application::Instance().GetWindowSize().w;
	scissorrect.bottom = Application::Instance().GetWindowSize().h;
	return scissorrect;
}

DX12Wrapper::~DX12Wrapper()
{
	fence->Release();
	dev->Release();
	rtvDescHeap->Release();
	srvDescHeap->Release();
	commandAllocator->Release();
	commandList->Release();
	commandQueue->Release();
	dxgiFactory->Release();
	swapchain->Release();
	rootSignature->Release();
	pipelineState->Release();
	verticesBuff->Release();
	cBuff->Release();
	depthBuffer->Release();
	dsvHeap->Release();
}