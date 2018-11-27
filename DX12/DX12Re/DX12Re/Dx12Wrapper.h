#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <DirectXTex.h>

class PMDModel;

struct TransformMaterices {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX wvp;
};

class Dx12Wrapper
{
private:
	//デバイス
	ID3D12Device* dev;

	//デスクリプタヒープ
	//リソースの情報を格納した構造体
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

	//DirectXグラフィックスインストラクチャー
	//グラフィックスのランタイムに依存する必要のない、低レベルタスクを管理する。
	IDXGIFactory6* dxgi;

	//GPUで描画した画像を実際の画面に反映させるための機能、情報
	IDXGISwapChain4* swapChain;

	//コマンドキューにサブミットしたコマンドリストの完了を検知する
	ID3D12Fence* fence;

	//頂点バッファ
	//バッファってのは要素別にグループ化された完全型付きデータの集合のこと
	ID3D12Resource* vertexBuffer;

	//インデックスバッファ
	ID3D12Resource* indexBuffer;

	//テクスチャバッファ
	ID3D12Resource* texBuffer;

	//最終的に欲しいオブジェクト
	ID3D12RootSignature* rootSignature;

	//パイプラインステート
	ID3D12PipelineState* pipelineState;

	//ルートシグネチャを作るための材料
	ID3DBlob* signature;

	//エラー出たときの対処
	ID3DBlob* error;

	//RTV(レンダーターゲット)デスクリプタヒープ
	ID3D12DescriptorHeap* rtvDescHeap;

	//DSV(深度)デスクリプタヒープ
	ID3D12DescriptorHeap* dsvDescHeap;

	//その他(テクスチャ、定数)デスクリプタヒープ
	ID3D12DescriptorHeap* rgstDescHeap;

	//定数バッファ
	ID3D12Resource* cBuffer;

	//フェンス値
	UINT fenceValue;

	//レンダーターゲット
	std::vector<ID3D12Resource*> renderTarget;

	//インデックス配列
	std::vector<unsigned short> index;

	//モデル
	std::shared_ptr<PMDModel> model;

	//実行処理
	void ExecuteCommand(void);

	//待ち処理
	void WaitExecute(void);

	//ビューポート設定
	D3D12_VIEWPORT SetViewPort(void);

	//シザー矩形設定
	D3D12_RECT SetRect(void);

	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vbView;

	//インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView;

	//マップされた行列
	TransformMaterices* mappedMatrix;
	//ワールド
	DirectX::XMMATRIX world;
	//ビュー
	DirectX::XMMATRIX view;
	//プロジェクション
	DirectX::XMMATRIX projection;

	DirectX::TexMetadata metadata;

	DirectX::ScratchImage Img;

	//頂点初期化
	void InitVertex(void);
	//コマンド系初期化
	void InitCommand(void);
	//スワップチェイン作成
	void InitSwapchain(void);
	//レンダーターゲットビュー作成
	void InitRTV(void);
	//テクスチャ作成
	void InitTexture(void);
	//ルートシグネチャ作成
	void InitRootSignature(void);
	//グラフィックスパイプラインステート作成
	void InitGPS(void);
	//コンスタントバッファビュー作成
	void InitCBV(void);
public:
	Dx12Wrapper();
	~Dx12Wrapper();
	void Update(void);
};

