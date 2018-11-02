#include "Dx12Wrapper.h"
#include "Application.h"
#include "d3dx12.h"
#include <DirectXMath.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos;	//���W
};

Dx12Wrapper::Dx12Wrapper()
{
	HRESULT result = S_OK;
	//�\�Ȍ���V�����o�[�W�������g�����߂̃t�B�[�`���[���x���̐ݒ�
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	//�G���[���o�͂ɕ\��������(�f�o�b�O�G���[)
#ifdef _DEBUG
	ID3D12Debug *debug = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (FAILED(result))
		int i = 0;
	debug->EnableDebugLayer();
	debug->Release();
	debug = nullptr;
#endif

	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
	for (auto l : levels)
	{
		result = D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&dev));
		if (SUCCEEDED(result))
		{
			level = l;
			break;
		}
	}

	//���_���̍쐬
	Vertex vertex[] = {
		{ {0.0f, 1.0f, 0.0f} },
		{ {1.0f, 0.0f, 0.0f} },
		{ {0.0f, -1.0f, 0.0f} }
	};

	IDXGIFactory4* factory = nullptr;
	result = CreateDXGIFactory(IID_PPV_ARGS(&factory));

	//�R�}���h�L���[�̍쐬
	D3D12_COMMAND_QUEUE_DESC commandQDesc = {};
	commandQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQDesc.NodeMask = 0;
	commandQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	result = dev->CreateCommandQueue(
		&commandQDesc,
		IID_PPV_ARGS(&commandQueue)
	);

	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgi));

	//�X���b�v�`�F�C���̍쐬
	auto wsize = Application::Instance().GetWindowSize();
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Width = wsize.w;
	swapchainDesc.Height = wsize.h;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.Stereo = false;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.BufferCount = 2;
	result = dxgi->CreateSwapChainForHwnd(
		commandQueue,
		Application::Instance().hwnd,
		&swapchainDesc, 
		nullptr, 
		nullptr, 
		(IDXGISwapChain1**)(&swapChain));


	//�����_�[�^�[�Q�b�g�̍쐬
	//�\����ʗp�������m��
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//�����_�[�^�[�Q�b�g�r���[
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;					//�\��ʂƗ���ʂԂ�
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descriptorHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE descHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	swapChain->GetDesc(&swcDesc);
	auto renderTargetNum = swcDesc.BufferCount;
	//�����_�[�^�[�Q�b�g���Ԃ�m��
	renderTarget.resize(renderTargetNum);
	//�f�X�N���v�^1������̃T�C�Y���擾
	auto descSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < renderTargetNum; ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget[i]));
		dev->CreateRenderTargetView(renderTarget[i], nullptr, descHandle);
		descHandle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//�R�}���h�A���P�[�^�̍쐬(�m�肽���̂̓R�}���h���X�g�̎�ʁA��1����)
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//�R�}���h���X�g�̍쐬(�m�肽���̂̓R�}���h���X�g�̎�ʁA��2����)
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

	fenceValue = 0;
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	//�R�}���h���X�g�̃N���[�Y
	commandList->Close();

	//�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = Application::Instance().GetWindowSize().w;
	desc.Height = Application::Instance().GetWindowSize().h;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	vertexBuffer = nullptr;
	result = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)
		);
	char* pData = nullptr;

	///Map�����܂������Ȃ�
	result = vertexBuffer->Map(0, nullptr, (void**)(&pData));
	//memcpy(pData, vertex, sizeof(vertex));
	//vertexBuffer->Unmap(0, nullptr);
}


Dx12Wrapper::~Dx12Wrapper()
{
	dev->Release();
	descriptorHeap->Release();
	commandList->Release();
	commandQueue->Release();
	commandAllocator->Release();
	dxgi->Release();
	swapChain->Release();
	fence->Release();
	vertexBuffer->Release();
}

void Dx12Wrapper::Update(void)
{
	auto heapStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//�|�C���^�̃n���h�����R�s�[
	auto bbIndex = swapChain->GetCurrentBackBufferIndex();
	//�I�t�Z�b�g
	heapStart.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�N���A�J���[�ݒ�
	float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };

	//�A���P�[�^���Z�b�g
	auto result = commandAllocator->Reset();
	//�R�}���h���X�g���Z�b�g
	result = commandList->Reset(commandAllocator, nullptr);

	//���\�[�X�o���A�̐ݒ�
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTarget[bbIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	commandList->ResourceBarrier(1, &barrier);

	//�����_�[�^�[�Q�b�g�ݒ�
	commandList->OMSetRenderTargets(1, &heapStart, false, nullptr);
	//�����_�[�^�[�Q�b�g�̃N���A
	commandList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	//���\�[�X�o���A�̐ݒ�
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);

	//�R�}���h���X�g�̃N���[�Y
	commandList->Close();

	ExecuteCommand();

	WaitExecute();

	//Present:�����_�����O���ăX���b�v����
	swapChain->Present(1, 0);
}

void Dx12Wrapper::ExecuteCommand(void)
{
	//����commandQueue�ɓ�����ExecuteCommnadLists�Łu���s�v����
	ID3D12CommandList* commandList_2[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(commandList_2), commandList_2);
	commandQueue->Signal(fence, ++fenceValue);
}

void Dx12Wrapper::WaitExecute(void)
{
	//�u�҂����v�̎���
	while (fence->GetCompletedValue() != fenceValue)
	{
		//�������Ȃ�
	}
}