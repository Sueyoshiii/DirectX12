#include "Dx12Wrapper.h"
#include "Application.h"
#include "d3dx12.h"
#include <DirectXMath.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos;	//座標
};

Dx12Wrapper::Dx12Wrapper()
{
	HRESULT result = S_OK;
	//可能な限り新しいバージョンを使うためのフィーチャーレベルの設定
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	//エラーを出力に表示させる(デバッグエラー)
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

	//頂点情報の作成
	Vertex vertex[] = {
		{ {0.0f, 1.0f, 0.0f} },
		{ {1.0f, 0.0f, 0.0f} },
		{ {0.0f, -1.0f, 0.0f} }
	};

	IDXGIFactory4* factory = nullptr;
	result = CreateDXGIFactory(IID_PPV_ARGS(&factory));

	//コマンドキューの作成
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

	//スワップチェインの作成
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


	//レンダーターゲットの作成
	//表示画面用メモリ確保
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//レンダーターゲットビュー
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;					//表画面と裏画面ぶん
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descriptorHeap));

	D3D12_CPU_DESCRIPTOR_HANDLE descHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	swapChain->GetDesc(&swcDesc);
	auto renderTargetNum = swcDesc.BufferCount;
	//レンダーターゲット数ぶん確保
	renderTarget.resize(renderTargetNum);
	//デスクリプタ1個あたりのサイズを取得
	auto descSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < renderTargetNum; ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget[i]));
		dev->CreateRenderTargetView(renderTarget[i], nullptr, descHandle);
		descHandle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//コマンドアロケータの作成(知りたいのはコマンドリストの種別、第1引数)
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドリストの作成(知りたいのはコマンドリストの種別、第2引数)
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

	fenceValue = 0;
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	//コマンドリストのクローズ
	commandList->Close();

	//ヒーププロパティ
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

	///Mapがうまくいかない
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
	//ポインタのハンドルをコピー
	auto bbIndex = swapChain->GetCurrentBackBufferIndex();
	//オフセット
	heapStart.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//クリアカラー設定
	float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };

	//アロケータリセット
	auto result = commandAllocator->Reset();
	//コマンドリストリセット
	result = commandList->Reset(commandAllocator, nullptr);

	//リソースバリアの設定
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTarget[bbIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	commandList->ResourceBarrier(1, &barrier);

	//レンダーターゲット設定
	commandList->OMSetRenderTargets(1, &heapStart, false, nullptr);
	//レンダーターゲットのクリア
	commandList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	//リソースバリアの設定
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier);

	//コマンドリストのクローズ
	commandList->Close();

	ExecuteCommand();

	WaitExecute();

	//Present:レンダリングしてスワップする
	swapChain->Present(1, 0);
}

void Dx12Wrapper::ExecuteCommand(void)
{
	//情報をcommandQueueに投げてExecuteCommnadListsで「実行」する
	ID3D12CommandList* commandList_2[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(commandList_2), commandList_2);
	commandQueue->Signal(fence, ++fenceValue);
}

void Dx12Wrapper::WaitExecute(void)
{
	//「待つ処理」の実装
	while (fence->GetCompletedValue() != fenceValue)
	{
		//何もしない
	}
}