#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

class Dx12Wrapper
{
private:
	//デバイス
	ID3D12Device* dev;

	//デスクリプタヒープ
	ID3D12DescriptorHeap* descriptorHeap;

	//コマンドリスト
	//GPUに命令を送る際、情報を蓄える仕組み
	ID3D12GraphicsCommandList* commandList;

	//コマンドキュー
	//描画に関する複数呼び出しを一括に行うための仕組み
	ID3D12CommandQueue* commandQueue;

	//コマンドアロケータ
	//コマンドリストがハードウェアネイティブな描画コマンドに変換され、
	//グラフィックスメモリ上に確保される領域を確保する仕組み
	ID3D12CommandAllocator* commandAllocator;

	//
	IDXGIFactory6* dxgi;

	//GPUで描画した画像を実際の画面に反映させるための機能、情報
	IDXGISwapChain4* swapChain;

	//コマンドキューにサブミットしたコマンドリストの完了を検知する
	ID3D12Fence* fence;

	//頂点バッファ
	ID3D12Resource* vertexBuffer;

	UINT fenceValue;

	std::vector<ID3D12Resource*> renderTarget;

	//実行処理
	void ExecuteCommand(void);
	//待ち処理
	void WaitExecute(void);
public:
	Dx12Wrapper();
	~Dx12Wrapper();
	void Update(void);
};

