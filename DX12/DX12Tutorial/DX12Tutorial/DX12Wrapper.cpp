#include "DX12Wrapper.h"
#include "Application.h"
#include <DirectXMath.h>
#include <algorithm>
#include <d3dcompiler.h>
#include "d3dx12.h"
#include <tchar.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


using namespace DirectX;

//���_���
struct Vertex {
	XMFLOAT3 pos; //���_���W
};

DX12Wrapper::DX12Wrapper()
{
	D3D_FEATURE_LEVEL levels[] = {

		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	//�G���[���o�͂ɕ\��������
#ifdef _DEBUG
	ID3D12Debug *debug = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (FAILED(result))
		int i = 0;
	debug->EnableDebugLayer();
	debug->Release();
	debug = nullptr;
#endif

	//Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featurelevel;
	for (auto& l : levels)
	{
		if (D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&dev)) == S_OK)
		{
			featurelevel = l;
			break;
		}
	}

	
	//�R�}���h�L���[�I�u�W�F�N�g�̍쐬
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

	//�X���b�v�`�F�[���̐���
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
		IID_PPV_ARGS(&descriptorHeap)
	);

	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescH = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		//�e�f�X�N���v�^�̎g�p����T�C�Y���v�Z
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

	CreateVertices();
	InitShaders();

	ID3D12RootSignature* rootSignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
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
}

DX12Wrapper::~DX12Wrapper()
{
	commandAllocator->Release();
	commandQueue->Release();
	commandList->Release();
	descriptorHeap->Release();
	dev->Release();
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
	commandList->Reset(commandAllocator, nullptr);

	auto bbIdx = swapchain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier;
	ZeroMemory(&barrier, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffers[bbIdx];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//�o���A�ݒu
	commandList->ResourceBarrier(1, &barrier);

	//�����_�[�^�[�Q�b�g�̎w��
	auto descH = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	descH.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	commandList->OMSetRenderTargets(1, &descH, false, nullptr);

	float color[] = { 0.0, 1.0, 0.0, 1.0 };
	//�����_�[�^�[�Q�b�g�̃N���A
	commandList->ClearRenderTargetView(descH, color, 0, nullptr);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//�o���A�ݒu
	commandList->ResourceBarrier(1, &barrier);

	commandList->RSSetViewports(1, &SetViewPort());

	commandList->Close();

	ExecuteCommand();
	WaitExecute();

	swapchain->Present(0, 0);
}

//���_�����`���A���_�o�b�t�@�����
void DX12Wrapper::CreateVertices()
{
	Vertex vertices[] = {
		{ { -1, -1 , 0 } },
		{ { 0, 1, 0 } },
		{ { 1, 0, 0 } },
	};

	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&verticesBuff)
	);
	D3D12_RANGE range = { 0, 0 };
	Vertex* vbuffptr = nullptr;
	result = verticesBuff->Map(0, nullptr, (void**)&vbuffptr);
	std::copy(std::begin(vertices), std::end(vertices), vbuffptr);

	verticesBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = verticesBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(vertices);
	vbView.StrideInBytes = sizeof(Vertex);
}

void DX12Wrapper::InitShaders()
{
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vsBlob, nullptr);
	D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &psBlob, nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob);

	gpsDesc.InputLayout.NumElements;
	gpsDesc.InputLayout.pInputElementDescs;
	gpsDesc.pRootSignature;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.SampleMask = 0xffffff;
	ID3D12PipelineState* pipelineState = nullptr;
	result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));
}

D3D12_VIEWPORT DX12Wrapper::SetViewPort()
{
	D3D12_VIEWPORT viewPort;
	SecureZeroMemory(&viewPort, sizeof(viewPort));
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = Application::Instance().GetWindowSize().w;
	viewPort.Height = Application::Instance().GetWindowSize().h;
	viewPort.MaxDepth = 1;
	viewPort.MinDepth = 0;

	return viewPort;
}
