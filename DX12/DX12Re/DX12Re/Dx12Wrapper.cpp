#include "Dx12Wrapper.h"
#include "Application.h"
#include "d3dx12.h"
#include <tchar.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define TEX_SIZE_W 720
#define TEX_SIZE_H 720

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos;	//座標
	XMFLOAT2 uv;	//UV
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

	//エラーを出力に表示させる(デバッグレイヤー)
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
		{ {-0.5f, -0.9f, 0.0f}, {0.0f, 0.0f} },
		{ {-0.5f, 0.9f, 0.0f}, {1.0f, 0.0f} },
		{ {0.5f, -0.9f, 0.0f}, {0.0f, 1.0f} },
		{ {0.5f, 0.9f, 0.0f}, {1.0f, 1.0f} }
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
		(IDXGISwapChain1**)(&swapChain)
	);


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

	vertexBuffer = nullptr;
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPUからGPUへ転送する用
		D3D12_HEAP_FLAG_NONE,	//特別な指定なし
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex)),	//サイズ
		D3D12_RESOURCE_STATE_GENERIC_READ,	//よくわからん
		nullptr,	//nullptrでOK
		IID_PPV_ARGS(&vertexBuffer)	//いつもの
		);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(vertex);
	vbView.StrideInBytes = sizeof(Vertex);

	char* vertmap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)(&vertmap));
	memcpy(vertmap, &vertex[0], sizeof(vertex));
	vertexBuffer->Unmap(0, nullptr);

	index = { 0, 1, 2, 1, 3, 2 };
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(index.size() * sizeof(short)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)
	);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();	//バッファの場所
	ibView.Format = DXGI_FORMAT_R16_UINT;	//フォーマット(shortなのでR16)
	ibView.SizeInBytes = index.size() * sizeof(short);	//総サイズ

	unsigned short* indmap = nullptr;
	result = indexBuffer->Map(0, nullptr, (void**)(&indmap));
	memcpy(indmap, &index[0], sizeof(unsigned short) * index.size());
	indexBuffer->Unmap(0, nullptr);

	//頂点シェーダ
	ID3DBlob* vertexShader = nullptr;
	//ピクセルシェーダ
	ID3DBlob* pixelShader = nullptr;
	result = D3DCompileFromFile(
		L"Shader.hlsl", 
		nullptr, 
		nullptr, 
		"BasicVS", 
		"vs_5_0", 
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, 
		&vertexShader, 
		nullptr
	);
	result = D3DCompileFromFile(
		L"Shader.hlsl", 
		nullptr, 
		nullptr, 
		"BasicPS", 
		"ps_5_0", 
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 
		0, 
		&pixelShader, 
		nullptr);

	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.NumDescriptorRanges = 1;

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//CreateRootSignatureに渡すことができるようシリアライズ化
	result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error
	);

	//ルートシグネチャ生成
	result = dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);

	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};

	//テクスチャオブジェクト生成
	{
		result = LoadFromWICFile(L"img/virtual_cat.png", WIC_FLAGS_NONE, &metaData, Img);

		D3D12_HEAP_PROPERTIES heapprop = {};
		heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask = 1;
		heapprop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC heapDesc = {};
		heapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		heapDesc.Alignment = 0;
		heapDesc.Width = TEX_SIZE_W;
		heapDesc.Height = TEX_SIZE_H;
		heapDesc.DepthOrArraySize = 1;
		heapDesc.MipLevels = 0;
		heapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		heapDesc.SampleDesc.Count = 1;
		heapDesc.SampleDesc.Quality = 0;
		heapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		heapDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		result = dev->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&heapDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&texBuffer)
		);

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc = texBuffer->GetDesc();
		D3D12_BOX box = {};
		box.left = 0;
		box.top = 0;
		box.right = resDesc.Width;
		box.bottom = resDesc.Height;
		box.front = 0;
		box.back = 1;
		//result = texBuffer->WriteToSubresource(
		//	0,
		//	&box,
		//	&,
		//	4 * 720,
		//	4 * 720 * 720
		//);

		//テクスチャオブジェクトの生成は１回だけすればいいから恐らく初期化処理内でいいだろう
		//しかしPDFだとこの後バリア＆フェンス待ちの配置がくる
		//これはUpdateじゃなくてもいいのか?
		commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				texBuffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		);
		commandList->Close();
		commandQueue->ExecuteCommandLists(
			1,
			(ID3D12CommandList* const*)&commandList
		);

		//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		//srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		//srvDesc.Texture2D.MipLevels = 1;
		//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		Img.Release();
	}

	//パイプラインステート
	//パイプラインに関わる情報を格納していく
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layout;
	gpsDesc.InputLayout.NumElements = _countof(layout);
	//シェーダ系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	//gpsDesc.HS
	//gpsDesc.DS
	//gpsDesc.GS
	//レンダーターゲット
	//ターゲット数と設定するフォーマット数は一致させておく
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat;
	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;//いる
	gpsDesc.SampleDesc.Quality = 0;//いる
	gpsDesc.SampleMask = 0xffffffff;//全部1
	gpsDesc.Flags;//デフォルトでOK
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形
	pipelineState = nullptr;
	result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));
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
	rootSignature->Release();
	pipelineState->Release();
}

void Dx12Wrapper::Update(void)
{
	//アロケータリセット
	auto result = commandAllocator->Reset();
	//コマンドリストリセット
	result = commandList->Reset(commandAllocator, nullptr);

	commandList->SetPipelineState(pipelineState);
	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->RSSetViewports(1, &SetViewPort());
	commandList->RSSetScissorRects(1, &SetRect());
	auto heapStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//ポインタのハンドルをコピー
	auto bbIndex = swapChain->GetCurrentBackBufferIndex();
	//オフセット
	heapStart.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//クリアカラー設定
	float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };

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

	commandList->IASetVertexBuffers(0, 1, &vbView);
	commandList->IASetIndexBuffer(&ibView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	//リソースバリアの設定
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
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

D3D12_VIEWPORT Dx12Wrapper::SetViewPort(void)
{
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Application::Instance().GetWindowSize().w;
	viewport.Height = Application::Instance().GetWindowSize().h;
	viewport.MaxDepth = 1.0f;	//カメラからの距離(遠いほう)
	viewport.MinDepth = 0.0f;	//カメラからの距離(近いほう)

	return viewport;
}

D3D12_RECT Dx12Wrapper::SetRect(void)
{
	D3D12_RECT scissorrect;
	scissorrect.left = 0;
	scissorrect.top = 0;
	scissorrect.right = Application::Instance().GetWindowSize().w;
	scissorrect.bottom = Application::Instance().GetWindowSize().h;
	return scissorrect;
}
