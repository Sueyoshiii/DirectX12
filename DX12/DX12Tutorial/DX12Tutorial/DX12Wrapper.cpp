#include "DX12Wrapper.h"
#include "Application.h"
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


DX12Wrapper::DX12Wrapper(HINSTANCE h, HWND hwnd)
{
	D3D_FEATURE_LEVEL levels[] = {

		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

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

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	
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

	//スワップチェーンの生成
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	auto wsize = Application::Instance().GetWindowSize();
	swapchainDesc = {};
	swapchainDesc.Width = wsize.w;
	swapchainDesc.Height = wsize.h;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapchainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	result = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue,
		Application::Instance().GetWindowHandle(),
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(swapchain)
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

	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescH = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//各デスクリプタの使用するサイズを計算
	auto rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DXGI_SWAP_CHAIN_DESC swchDesc = {};
	swapchain->GetDesc(&swchDesc);
	std::vector<ID3D12Resource*> backBuffers(swchDesc.BufferCount);
	for (int i = 0; i < backBuffers.size(); i++)
	{
		swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
		dev->CreateRenderTargetView(backBuffers[i], nullptr, cpuDescH);
		cpuDescH.ptr += rtvSize;
	}
}


DX12Wrapper::~DX12Wrapper()
{
	dev->Release();
}
