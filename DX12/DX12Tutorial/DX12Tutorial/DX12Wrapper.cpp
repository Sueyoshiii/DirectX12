#include "DX12Wrapper.h"
#include "Application.h"

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

	D3D_FEATURE_LEVEL featurelevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
	for (auto& l : levels)
	{
		if (D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&dev)) == S_OK)
		{
			featurelevel = l;
			break;
		}
	}

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.BufferCount = 2;
	auto wsize = Application::Instance().GetWindowSize();
	swapchainDesc = {};
	swapchainDesc.Width = wsize.w;
	swapchainDesc.Height = wsize.h;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	
	//result = dxgiFactory->CreateSwapChain(dev, )
}


DX12Wrapper::~DX12Wrapper()
{
	dev->Release();
}
