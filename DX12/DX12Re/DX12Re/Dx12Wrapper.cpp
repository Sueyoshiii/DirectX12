#include "Dx12Wrapper.h"
#include "Application.h"
#include "PMDModel.h"
#include "d3dx12.h"
#include <tchar.h>
#include <d3dcompiler.h>
#include <random>
#include <functional>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos;	//座標
	XMFLOAT2 uv;	//UV
};

Dx12Wrapper::Dx12Wrapper()
{
	HRESULT result = S_OK;

	//COM(Component Object Model)ライブラリを呼び出しスレッドで使用するために初期化し
	//スレッドの同時実行モデルを設定し
	//必要に応じてスレッドの新しいアパートメントを作成する。
	result = CoInitializeEx(nullptr, 0);

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
	//Direct3Dデバイスの初期化
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

	//モデル初期化
	model.reset(new PMDModel("model/初音ミク.pmd"));

	InitVertex();

	InitCommand();

	InitSwapchain();

	InitRTV();

	//フェンス作成
	fenceValue = 0;
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	//コマンドリストのクローズ
	commandList->Close();

	InitTexture();

	InitRootSignature();

	InitGPS();

	InitCBV();

	InitMaterial();
}

void Dx12Wrapper::Update(void)
{
	//アロケータリセット
	auto result = commandAllocator->Reset();
	//コマンドリストリセット
	result = commandList->Reset(commandAllocator, pipelineState);

	commandList->RSSetViewports(1, &SetViewPort());
	commandList->RSSetScissorRects(1, &SetRect());

	static float angle = 0.0f;
	world = XMMatrixRotationY(angle);

	mappedMatrix->world = world;
	mappedMatrix->wvp = world * view * projection;
	angle += 0.01f;

	commandList->SetPipelineState(pipelineState);
	commandList->SetGraphicsRootSignature(rootSignature);

	auto handle = rgstDescHeap->GetGPUDescriptorHandleForHeapStart();

	commandList->SetDescriptorHeaps(1, &rgstDescHeap);

	commandList->SetGraphicsRootDescriptorTable(0, rgstDescHeap->GetGPUDescriptorHandleForHeapStart());
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(1, handle);
	
	auto heapStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//ポインタのハンドルをコピー
	auto bbIndex = swapChain->GetCurrentBackBufferIndex();
	//オフセット
	heapStart.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//クリアカラー設定
	float clearColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };

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
	commandList->OMSetRenderTargets(
		1,
		&heapStart,
		false,
		&dsvDescHeap->GetCPUDescriptorHandleForHeapStart()
	);
	//レンダーターゲットのクリア
	commandList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(
		dsvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,
		0,
		0,
		nullptr
	);

	commandList->IASetVertexBuffers(0, 1, &vbView);
	commandList->IASetIndexBuffer(&ibView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	unsigned int offset = 0;
	auto matHandle = matDescHeap->GetGPUDescriptorHandleForHeapStart();
	//ID3D12DescriptorHeap* matDescHeaps[] = { matDescHeap };
	commandList->SetDescriptorHeaps(1, &matDescHeap);
	for (auto& m : model->pmdmaterial)
	{
		commandList->SetGraphicsRootDescriptorTable(2, matHandle);
		matHandle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		auto idxcnt = m.face_vert_count;
		commandList->DrawIndexedInstanced(idxcnt, 1, offset, 0, 0);
		offset += idxcnt;
	}

	//commandList->DrawIndexedInstanced(model->pmdindex.size(), 1, 0, 0, 0);

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

void Dx12Wrapper::InitVertex(void)
{
	//頂点情報の作成
	//Vertex vertex[] = {
	//	{ { -0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f } },
	//	{ { -0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f } },
	//	{ { 0.5f, -0.5f, -0.5f },{ 1.0f, 1.0f } },
	//	{ { 0.5f, 0.5f, -0.5f },{ 1.0f, 0.0f } },

	//	{ { 0.5f, -0.5f, 0.5f },{ 0.0f, 1.0f } },
	//	{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 0.0f } },
	//	{ { -0.5f, -0.5f, 0.5f },{ 1.0f, 1.0f } },
	//	{ { -0.5f, 0.5f, 0.5f },{ 1.0f, 0.0f } },
	//};

	IDXGIFactory4* factory = nullptr;
	auto result = CreateDXGIFactory(IID_PPV_ARGS(&factory));

	vertexBuffer = nullptr;
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPUからGPUへ転送する用
		D3D12_HEAP_FLAG_NONE,	//特別な指定なし
		&CD3DX12_RESOURCE_DESC::Buffer(model->pmdvertex.size() * sizeof(model->pmdvertex[0])),	//サイズ
		D3D12_RESOURCE_STATE_GENERIC_READ,	//よくわからん
		nullptr,	//nullptrでOK
		IID_PPV_ARGS(&vertexBuffer)	//いつもの
	);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = model->pmdvertex.size() * sizeof(model->pmdvertex[0]);
	vbView.StrideInBytes = sizeof(PMDVertex);

	char* vertmap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)(&vertmap));
	memcpy(vertmap, &model->pmdvertex[0], model->pmdvertex.size() * sizeof(model->pmdvertex[0]));
	vertexBuffer->Unmap(0, nullptr);

	//index = {
	//	0, 1, 2, 1, 3, 2,
	//	2, 3, 4, 3, 5, 4,
	//	4, 5, 6, 5, 7, 6,
	//	6, 7, 0, 7, 1, 0,

	//	1, 7, 3, 7, 5, 3,
	//	6, 0, 4, 0, 2, 4
	//};
	index = model->pmdindex;
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(index.size() * sizeof(index[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)
	);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();	//バッファの場所
	ibView.Format = DXGI_FORMAT_R16_UINT;	//フォーマット(shortなのでR16)
	ibView.SizeInBytes = index.size() * sizeof(index[0]);	//総サイズ

	unsigned short* indmap = nullptr;
	result = indexBuffer->Map(0, nullptr, (void**)(&indmap));
	memcpy(indmap, &index[0], index.size() * sizeof(index[0]));
	indexBuffer->Unmap(0, nullptr);
}

void Dx12Wrapper::InitCommand(void)
{
	//コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC commandQDesc = {};
	commandQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQDesc.NodeMask = 0;
	commandQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	auto result = dev->CreateCommandQueue(
		&commandQDesc,
		IID_PPV_ARGS(&commandQueue)
	);

	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgi));

	//コマンドアロケータの作成(知りたいのはコマンドリストの種別、第1引数)
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドリストの作成(知りたいのはコマンドリストの種別、第2引数)
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

}

void Dx12Wrapper::InitSwapchain(void)
{
	auto wsize = Application::Instance().GetWindowSize();
	//スワップチェインの作成
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
	auto result = dxgi->CreateSwapChainForHwnd(
		commandQueue,
		Application::Instance().hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(&swapChain)
	);

}

void Dx12Wrapper::InitRTV(void)
{
	//レンダーターゲットの作成
	//表示画面用メモリ確保
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//レンダーターゲットビュー
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;	//表画面と裏画面ぶん
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descriptorHeap));

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

}

void Dx12Wrapper::InitTexture(void)
{
	auto result = LoadFromWICFile(L"model/eye2.bmp", 0, &metadata, Img);

	D3D12_DESCRIPTOR_HEAP_DESC rgstDesc = {};
	rgstDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	rgstDesc.NodeMask = 0;
	rgstDesc.NumDescriptors = 2;
	rgstDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = dev->CreateDescriptorHeap(&rgstDesc, IID_PPV_ARGS(&rgstDescHeap));

	//D3D12_RESOURCE_DESC resDesc = {};
	//resDesc = texBuffer->GetDesc();
	//D3D12_BOX box = {};
	//box.left = 0;
	//box.top = 0;
	//box.right = resDesc.Width;
	//box.bottom = resDesc.Height;
	//box.front = 0;
	//box.back = 1;
	//std::vector<unsigned char>data(4 * metadata.width * metadata.height);
	//for (auto& i : data)
	//{
	//	std::random_device device;
	//	auto n = std::bind(std::uniform_int_distribution<>(0, 255), std::mt19937_64(device()));
	//	i = n();
	//}

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC heapDesc = {};
	heapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	heapDesc.Alignment = 0;
	heapDesc.Width = metadata.width;
	heapDesc.Height = metadata.height;
	heapDesc.DepthOrArraySize = 1;
	heapDesc.MipLevels = 0;
	heapDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	heapDesc.SampleDesc.Count = 1;
	heapDesc.SampleDesc.Quality = 0;
	heapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	heapDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	texBuffer = nullptr;
	result = dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&heapDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuffer)
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	dev->CreateShaderResourceView(
		texBuffer,
		&srvDesc,
		rgstDescHeap->GetCPUDescriptorHandleForHeapStart()
	);

	result = texBuffer->WriteToSubresource(
		0,
		nullptr,
		Img.GetPixels(),
		4 * metadata.width,
		4 * metadata.height
	);

	auto wsize = Application::Instance().GetWindowSize();
	D3D12_RESOURCE_DESC depthResDesc = {};
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

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	depthBuffer = nullptr;
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer)
	);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescHeap));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dev->CreateDepthStencilView(
		depthBuffer, 
		&dsvDesc, 
		dsvDescHeap->GetCPUDescriptorHandleForHeapStart()
	);
}

void Dx12Wrapper::InitRootSignature(void)
{
	D3D12_DESCRIPTOR_RANGE descRange[3] = {};
	D3D12_ROOT_PARAMETER rootParam[3] = {};

	//サンプラ(uvが0〜1を超えたときどういう扱いをするかの設定)
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;	//特別なフィルタを使用しない
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//絵が繰り返される(U方向)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//絵が繰り返される(V方向)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//絵が繰り返される(W方向)
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;	//MIPMAP上限なし
	samplerDesc.MinLOD = 0.0f;	//MIPMAP下限なし
	samplerDesc.MipLODBias = 0.0f;	//MIPMAPのバイアス
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	//エッジの色(黒透明)
	samplerDesc.ShaderRegister = 0;	//使用するシェーダレジスタ(スロット)
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//どのくらいのデータをシェーダに見せるか
	samplerDesc.RegisterSpace = 0;	//0でいい
	samplerDesc.MaxAnisotropy = 0;	//FilterがAnisotropyのときのみ有効
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	//常に否定

	{
		//テクスチャ用
		descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	//シェーダリソース
		descRange[0].BaseShaderRegister = 0;	//レジスタ番号(範囲内の記述子の数)
		descRange[0].NumDescriptors = 1;	//デスクリプタの数(レジスタ空間)
		descRange[0].RegisterSpace = 0;	//レジスタ空間
		descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;	//(ルートシグネチャの先頭からの記述子内のオフセット)

		rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];
		rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		//定数バッファ用
		descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	//定数バッファ
		descRange[1].BaseShaderRegister = 0;
		descRange[1].NumDescriptors = 1;
		descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];
		rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		//マテリアル用
		descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		descRange[2].BaseShaderRegister = 1;
		descRange[2].NumDescriptors = 1;
		descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[2];
		rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = _countof(rootParam);
	rsd.pParameters = &rootParam[0];
	rsd.NumStaticSamplers = 1;
	rsd.pStaticSamplers = &samplerDesc;

	//ルートシグネチャを作るための材料
	ID3DBlob* signature;
	//エラー出たときの対処
	ID3DBlob* error;

	//CreateRootSignatureに渡すことができるようシリアライズ化
	auto result = D3D12SerializeRootSignature(
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

}

void Dx12Wrapper::InitGPS(void)
{
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
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		//{
		//	"TEXCOORD",
		//	0,
		//	DXGI_FORMAT_R32G32_FLOAT,
		//	0,
		//	D3D12_APPEND_ALIGNED_ELEMENT,
		//	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		//	0
		//}
	};


	//頂点シェーダ
	ID3DBlob* vertexShader = nullptr;
	//ピクセルシェーダ
	ID3DBlob* pixelShader = nullptr;
	auto result = D3DCompileFromFile(
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
	//深度ステンシル(未設定)
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//DSV必須
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を返す
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;//必須(DSV)
	//ラスタライザ
	//コンピュータが扱う文字や画像を、色付きの小さな点で表現する
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;//いる
	gpsDesc.SampleDesc.Quality = 0;//いる
	gpsDesc.SampleMask = 0xffffffff;//全部1
	gpsDesc.Flags;//デフォルトでOK
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineState = nullptr;
	result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));

}

void Dx12Wrapper::InitCBV(void)
{
	//角度
	auto angle = (XM_PI / 2.0f);
	//行列
	TransformMaterices matrix = {};
	matrix.world = XMMatrixRotationY(angle);

	XMFLOAT3 eye(0, 10, -15.0f);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up)
	);

	auto wsize = Application::Instance().GetWindowSize();
	projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
		static_cast<float>(wsize.w) / static_cast<float>(wsize.h),
		0.1f,
		300.0f
	);

	matrix.wvp = matrix.world * view * projection;

	size_t size = sizeof(matrix);
	size = (size + 0xff) & ~0xff;
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cBuffer)
	);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = cBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	auto handle = rgstDescHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	dev->CreateConstantBufferView(&cbvDesc, handle);

	mappedMatrix = nullptr;
	result = cBuffer->Map(0, nullptr, (void**)&mappedMatrix);
	*mappedMatrix = matrix;

}

void Dx12Wrapper::InitMaterial(void)
{
	//マテリアルのサイズ
	size_t size = sizeof(Material);
	//256で
	size = (size + 0xff) & ~0xff;
	//マテリアル
	auto mats = model->pmdmaterial;

	
	//マテリアルバッファ作成
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,	
		&CD3DX12_RESOURCE_DESC::Buffer(size * mats.size()),	
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&matBuffer));

	//ヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.NumDescriptors = mats.size();
	descHeapDesc.NodeMask = 0;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&matDescHeap));

	//ビュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = matBuffer->GetGPUVirtualAddress();
	desc.SizeInBytes = size;
	auto handle = matDescHeap->GetCPUDescriptorHandleForHeapStart();
	std::vector<Material> tempMat(mats.size());
	int cnt = 0;
	for (auto& m : mats)
	{
		tempMat[cnt++].diffuse = m.diffuse_color;

		dev->CreateConstantBufferView(&desc, handle);
		desc.BufferLocation += size;
		handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	char* matmap = nullptr;
	result = matBuffer->Map(0, nullptr, (void**)&matmap);
	memcpy(matmap, &tempMat[0], tempMat.size() * sizeof(Material));
	//matBuffer->Unmap(0, nullptr);

/*	HRESULT result = S_OK;

	D3D12_HEAP_PROPERTIES materialHeapProp = {};
	materialHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	materialHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	materialHeapProp.CreationNodeMask = 1;
	materialHeapProp.VisibleNodeMask = 1;
	materialHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	int midx = 0;

	material.resize(model->pmdmaterial.size());
	matBuffer.resize(model->pmdmaterial.size());

	for (auto& m : matBuffer)
	{
		result = dev->CreateCommittedResource(
			&materialHeapProp,
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m)
		);

		MaterialRGBA diffuse(
			model->pmdmaterial[midx].diffuse_color.x,
			model->pmdmaterial[midx].diffuse_color.y,
			model->pmdmaterial[midx].diffuse_color.z,
			0
		);
		MaterialRGBA specular(
			model->pmdmaterial[midx].specular_color.x,
			model->pmdmaterial[midx].specular_color.y,
			model->pmdmaterial[midx].specular_color.z,
			0
		);
		float ambient = 0.5f;
		MaterialRGBA ambientData(ambient, ambient, ambient, ambient);

		Material sendMat(diffuse, specular, ambientData);

		result = m->Map(0, nullptr, (void**)&material[midx]);
		
		*material[midx] = sendMat;

		m->Unmap(0, nullptr);

		++midx;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC matDesc = {};
	matDesc.SizeInBytes = size;

	//ヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.NumDescriptors = mats.size();
	descHeapDesc.NodeMask = 0;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&matDescHeap));

	auto handle = matDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto h_size = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	unsigned int idx = 0;
	for (auto& m : matBuffer)
	{
		//バッファの場所を取得
		matDesc.BufferLocation = m->GetGPUVirtualAddress();

		//定数バッファの生成
		dev->CreateConstantBufferView(&matDesc, handle);

		//シェーダリソースビュー
		if (std::strlen(model->pmdmaterial[idx].texture_file_name) > 0)
		{
			//テクスチャあるとき
			D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = 1;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			dev->CreateShaderResourceView(texBuffer, &desc, handle);
		}
		//テクスチャない場合、白テクスチャを作る

		//ハンドルをずらす
		handle.ptr += h_size;
		++idx;
	}
	*/
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
	indexBuffer->Release();
	texBuffer->Release();
	matBuffer->Release();
	cBuffer->Release();
	rootSignature->Release();
	pipelineState->Release();
	rgstDescHeap->Release();

	CoUninitialize();
}
