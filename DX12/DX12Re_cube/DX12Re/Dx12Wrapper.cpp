#include "Dx12Wrapper.h"
#include "Application.h"
#include "d3dx12.h"
#include <tchar.h>
#include <d3dcompiler.h>
#include <random>
#include <functional>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos;	//���W
	XMFLOAT2 uv;	//UV
};

Dx12Wrapper::Dx12Wrapper()
{
	HRESULT result = S_OK;

	//COM(Component Object Model)���C�u�������Ăяo���X���b�h�Ŏg�p���邽�߂ɏ�������
	//�X���b�h�̓������s���f����ݒ肵
	//�K�v�ɉ����ăX���b�h�̐V�����A�p�[�g�����g���쐬����B
	result = CoInitializeEx(nullptr, 0);

	//�\�Ȍ���V�����o�[�W�������g�����߂̃t�B�[�`���[���x���̐ݒ�
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	//�G���[���o�͂ɕ\��������(�f�o�b�O���C���[)
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

	InitVertex();

	InitCommand();

	InitSwapchain();

	result = LoadFromWICFile(L"img/virtual_cat2.bmp", 0, &metadata, Img);

	InitRTV();

	//�t�F���X�쐬
	fenceValue = 0;
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	//�R�}���h���X�g�̃N���[�Y
	commandList->Close();

	InitHeap();

	InitBox();

	InitRootSignature();

	InitGPS();

	InitCBV();
}

void Dx12Wrapper::Update(void)
{
	//�A���P�[�^���Z�b�g
	auto result = commandAllocator->Reset();
	//�R�}���h���X�g���Z�b�g
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
	commandList->SetDescriptorHeaps(1, &rgstDescHeap);
	auto handle = rgstDescHeap->GetGPUDescriptorHandleForHeapStart();
	commandList->SetGraphicsRootDescriptorTable(0, rgstDescHeap->GetGPUDescriptorHandleForHeapStart());
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(1, handle);
	
	auto heapStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//�|�C���^�̃n���h�����R�s�[
	auto bbIndex = swapChain->GetCurrentBackBufferIndex();
	//�I�t�Z�b�g
	heapStart.ptr += bbIndex * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�N���A�J���[�ݒ�
	float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	
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

	commandList->IASetVertexBuffers(0, 1, &vbView);
	commandList->IASetIndexBuffer(&ibView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	//���\�[�X�o���A�̐ݒ�
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
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

D3D12_VIEWPORT Dx12Wrapper::SetViewPort(void)
{
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Application::Instance().GetWindowSize().w;
	viewport.Height = Application::Instance().GetWindowSize().h;
	viewport.MaxDepth = 1.0f;	//�J��������̋���(�����ق�)
	viewport.MinDepth = 0.0f;	//�J��������̋���(�߂��ق�)

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
	//���_���̍쐬
	Vertex vertex[] = {
		{ { -0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f } },
	{ { -0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, -0.5f },{ 1.0f, 1.0f } },
	{ { 0.5f, 0.5f, -0.5f },{ 1.0f, 0.0f } },

	{ { 0.5f, -0.5f, 0.5f },{ 0.0f, 1.0f } },
	{ { 0.5f, 0.5f, 0.5f },{ 0.0f, 0.0f } },
	{ { -0.5f, -0.5f, 0.5f },{ 1.0f, 1.0f } },
	{ { -0.5f, 0.5f, 0.5f },{ 1.0f, 0.0f } },
	};

	IDXGIFactory4* factory = nullptr;
	auto result = CreateDXGIFactory(IID_PPV_ARGS(&factory));

	vertexBuffer = nullptr;
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	//CPU����GPU�֓]������p
		D3D12_HEAP_FLAG_NONE,	//���ʂȎw��Ȃ�
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex)),	//�T�C�Y
		D3D12_RESOURCE_STATE_GENERIC_READ,	//�悭�킩���
		nullptr,	//nullptr��OK
		IID_PPV_ARGS(&vertexBuffer)	//������
	);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(vertex);
	vbView.StrideInBytes = sizeof(Vertex);

	char* vertmap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)(&vertmap));
	memcpy(vertmap, &vertex[0], sizeof(vertex));
	vertexBuffer->Unmap(0, nullptr);

	index = {
		0, 1, 2, 1, 3, 2,
		2, 3, 4, 3, 5, 4,
		4, 5, 6, 5, 7, 6,
		6, 7, 0, 7, 1, 0,

		1, 7, 3, 7, 5, 3,
		6, 0, 4, 0, 2, 4
	};
	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(index.size() * sizeof(short)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)
	);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();	//�o�b�t�@�̏ꏊ
	ibView.Format = DXGI_FORMAT_R16_UINT;	//�t�H�[�}�b�g(short�Ȃ̂�R16)
	ibView.SizeInBytes = index.size() * sizeof(short);	//���T�C�Y

	unsigned short* indmap = nullptr;
	result = indexBuffer->Map(0, nullptr, (void**)(&indmap));
	memcpy(indmap, &index[0], sizeof(unsigned short) * index.size());
	indexBuffer->Unmap(0, nullptr);
}

void Dx12Wrapper::InitCommand(void)
{
	//�R�}���h�L���[�̍쐬
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

	//�R�}���h�A���P�[�^�̍쐬(�m�肽���̂̓R�}���h���X�g�̎�ʁA��1����)
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//�R�}���h���X�g�̍쐬(�m�肽���̂̓R�}���h���X�g�̎�ʁA��2����)
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

}

void Dx12Wrapper::InitSwapchain(void)
{
	auto wsize = Application::Instance().GetWindowSize();
	//�X���b�v�`�F�C���̍쐬
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
	//�����_�[�^�[�Q�b�g�̍쐬
	//�\����ʗp�������m��
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//�����_�[�^�[�Q�b�g�r���[
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;	//�\��ʂƗ���ʂԂ�
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descriptorHeap));

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

}

void Dx12Wrapper::InitHeap(void)
{
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

	auto result = dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&heapDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuffer)
	);

}

void Dx12Wrapper::InitBox(void)
{
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc = texBuffer->GetDesc();
	D3D12_BOX box = {};
	box.left = 0;
	box.top = 0;
	box.right = resDesc.Width;
	box.bottom = resDesc.Height;
	box.front = 0;
	box.back = 1;
	std::vector<unsigned char>data(4 * metadata.width * metadata.height);
	for (auto& i : data)
	{
		std::random_device device;
		auto n = std::bind(std::uniform_int_distribution<>(0, 255), std::mt19937_64(device()));
		i = n();
	}
	auto result = texBuffer->WriteToSubresource(
		0,
		&box,
		Img.GetPixels(),
		4 * metadata.width,
		4 * metadata.height
	);

}

void Dx12Wrapper::InitRootSignature(void)
{
	D3D12_DESCRIPTOR_RANGE descRange[2] = {};
	D3D12_ROOT_PARAMETER rootParam[2] = {};

	//�T���v��(uv��0�`1�𒴂����Ƃ��ǂ��������������邩�̐ݒ�)
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;	//���ʂȃt�B���^���g�p���Ȃ�
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//�G���J��Ԃ����(U����)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//�G���J��Ԃ����(V����)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	//�G���J��Ԃ����(W����)
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;	//MIPMAP����Ȃ�
	samplerDesc.MinLOD = 0.0f;	//MIPMAP�����Ȃ�
	samplerDesc.MipLODBias = 0.0f;	//MIPMAP�̃o�C�A�X
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	//�G�b�W�̐F(������)
	samplerDesc.ShaderRegister = 0;	//�g�p����V�F�[�_���W�X�^(�X���b�g)
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//�ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩
	samplerDesc.RegisterSpace = 0;	//0�ł���
	samplerDesc.MaxAnisotropy = 0;	//Filter��Anisotropy�̂Ƃ��̂ݗL��
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	//���ɔ�r���Ȃ�(�ł͂Ȃ���ɔے�)

	{
		//�e�N�X�`���p
		descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	//�V�F�[�_���\�[�X
		descRange[0].BaseShaderRegister = 0;	//���W�X�^�ԍ�(�͈͓��̋L�q�q�̐�)
		descRange[0].NumDescriptors = 1;	//�f�X�N���v�^�̐�(���W�X�^���)
		descRange[0].RegisterSpace = 0;	//���W�X�^���
		descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;	//(���[�g�V�O�l�`���̐擪����̋L�q�q���̃I�t�Z�b�g)

		rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];
		rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		//�萔�o�b�t�@�p
		descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	//�萔�o�b�t�@
		descRange[1].BaseShaderRegister = 0;
		descRange[1].NumDescriptors = 1;
		descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];
		rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 2;
	rsd.pParameters = &rootParam[0];
	rsd.NumStaticSamplers = 1;
	rsd.pStaticSamplers = &samplerDesc;

	//CreateRootSignature�ɓn�����Ƃ��ł���悤�V���A���C�Y��
	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error
	);

	//���[�g�V�O�l�`������
	result = dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);

}

void Dx12Wrapper::InitGPS(void)
{
	//���_���C�A�E�g
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


	//���_�V�F�[�_
	ID3DBlob* vertexShader = nullptr;
	//�s�N�Z���V�F�[�_
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
	//�p�C�v���C���X�e�[�g
	//�p�C�v���C���Ɋւ������i�[���Ă���
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layout;
	gpsDesc.InputLayout.NumElements = _countof(layout);
	//�V�F�[�_�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	//gpsDesc.HS
	//gpsDesc.DS
	//gpsDesc.GS
	//�����_�[�^�[�Q�b�g
	//�^�[�Q�b�g���Ɛݒ肷��t�H�[�}�b�g���͈�v�����Ă���
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//�[�x�X�e���V��(���ݒ�)
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat;
	//���X�^���C�U
	//�R���s���[�^������������摜���A�F�t���̏����ȓ_�ŕ\������
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;//����
	gpsDesc.SampleDesc.Quality = 0;//����
	gpsDesc.SampleMask = 0xffffffff;//�S��1
	gpsDesc.Flags;//�f�t�H���g��OK
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`
	pipelineState = nullptr;
	result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));

}

void Dx12Wrapper::InitCBV(void)
{
	auto wsize = Application::Instance().GetWindowSize();
	//�p�x
	auto angle = (XM_PI / 2.0f);
	//�s��
	TransformMaterices matrix;
	matrix.world = XMMatrixRotationY(angle);

	XMFLOAT3 eye(0, -1, -1.5f);
	XMFLOAT3 target(0, 0, 0);
	XMFLOAT3 up(0, 1, 0);

	view = XMMatrixLookAtLH(
		XMLoadFloat3(&eye),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up)
	);

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

	D3D12_DESCRIPTOR_HEAP_DESC rgstDesc{};
	rgstDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	rgstDesc.NodeMask = 0;
	rgstDesc.NumDescriptors = 2;
	rgstDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = dev->CreateDescriptorHeap(&rgstDesc, IID_PPV_ARGS(&rgstDescHeap));

	auto handle = rgstDescHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	dev->CreateConstantBufferView(&cbvDesc, handle);

	mappedMatrix = nullptr;
	result = cBuffer->Map(0, nullptr, (void**)&mappedMatrix);
	*mappedMatrix = matrix;

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
	signature->Release();
	rgstDescHeap->Release();
	cBuffer->Release();

	CoUninitialize();
}
