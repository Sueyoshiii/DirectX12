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

struct MaterialRGBA
{
	MaterialRGBA() {}
	MaterialRGBA(float r, float g, float b, float a) : red(r), green(g), blue(b), alpha(a) {}
	float red;
	float green;
	float blue;
	float alpha;
};

struct Material
{
	//Material() {}
	//Material(MaterialRGBA& dif, MaterialRGBA& spe, MaterialRGBA& amb) :
	//	diffuse(dif), specular(spe), ambient(amb) {}
	//MaterialRGBA diffuse;
	//MaterialRGBA specular;
	//MaterialRGBA ambient;
	DirectX::XMFLOAT3 diffuse;
	DirectX::XMFLOAT3 specular;
	DirectX::XMFLOAT3 ambient;
};

class Dx12Wrapper
{
private:
	//�f�o�C�X
	ID3D12Device * dev;

	//�f�X�N���v�^�q�[�v
	//���\�[�X�̏����i�[�����\����
	ID3D12DescriptorHeap* descriptorHeap;

	//�R�}���h���X�g
	//GPU�ɖ��߂𑗂�ہA����~����d�g��
	ID3D12GraphicsCommandList* commandList;

	//�R�}���h�L���[
	//�`��Ɋւ��镡���Ăяo�����ꊇ�ɍs�����߂̎d�g��
	ID3D12CommandQueue* commandQueue;

	//�R�}���h�A���P�[�^
	//�R�}���h���X�g���n�[�h�E�F�A�l�C�e�B�u�ȕ`��R�}���h�ɕϊ�����A
	//�O���t�B�b�N�X��������Ɋm�ۂ����̈���m�ۂ���d�g��
	ID3D12CommandAllocator* commandAllocator;

	//DirectX�O���t�B�b�N�X�C���X�g���N�`���[
	//�O���t�B�b�N�X�̃����^�C���Ɉˑ�����K�v�̂Ȃ��A�჌�x���^�X�N���Ǘ�����B
	IDXGIFactory5* dxgi;

	//GPU�ŕ`�悵���摜�����ۂ̉�ʂɔ��f�����邽�߂̋@�\�A���
	IDXGISwapChain4* swapChain;

	//�R�}���h�L���[�ɃT�u�~�b�g�����R�}���h���X�g�̊��������m����
	ID3D12Fence* fence;

	//���_�o�b�t�@
	//�o�b�t�@���Ă̂͗v�f�ʂɃO���[�v�����ꂽ���S�^�t���f�[�^�̏W���̂���
	ID3D12Resource* vertexBuffer;

	//�C���f�b�N�X�o�b�t�@
	ID3D12Resource* indexBuffer;

	//�e�N�X�`���o�b�t�@
	ID3D12Resource* texBuffer;

	//�}�e���A���o�b�t�@
	//ID3D12Resource* matBuffer;
	ID3D12Resource* matBuffer;

	//�萔�o�b�t�@
	ID3D12Resource* cBuffer;

	//�t�F���X�l
	UINT fenceValue;

	//�[�x�o�b�t�@
	ID3D12Resource* depthBuffer;

	//�ŏI�I�ɗ~�����I�u�W�F�N�g
	ID3D12RootSignature* rootSignature;

	//�p�C�v���C���X�e�[�g
	ID3D12PipelineState* pipelineState;

	//RTV(�����_�[�^�[�Q�b�g)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* rtvDescHeap;

	//DSV(�[�x)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* dsvDescHeap;

	//�}�e���A���f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* matDescHeap;

	//���̑�(�e�N�X�`���A�萔)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* rgstDescHeap;

	//�����_�[�^�[�Q�b�g
	std::vector<ID3D12Resource*> renderTarget;

	//�C���f�b�N�X�z��
	std::vector<unsigned short> index;

	//�}�e���A���p�z��
	std::vector<Material*> material;

	//���f��
	std::shared_ptr<PMDModel> model;

	//���s����
	void ExecuteCommand(void);

	//�҂�����
	void WaitExecute(void);

	//�r���[�|�[�g�ݒ�
	D3D12_VIEWPORT SetViewPort(void);

	//�V�U�[��`�ݒ�
	D3D12_RECT SetRect(void);

	//���_�o�b�t�@�r���[
	D3D12_VERTEX_BUFFER_VIEW vbView;

	//�C���f�b�N�X�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW ibView;

	//�}�b�v���ꂽ�s��
	TransformMaterices* mappedMatrix;
	//���[���h
	DirectX::XMMATRIX world;
	//�r���[
	DirectX::XMMATRIX view;
	//�v���W�F�N�V����
	DirectX::XMMATRIX projection;

	DirectX::TexMetadata metadata;

	DirectX::ScratchImage Img;

	//���_������
	void InitVertex(void);
	//�R�}���h�n������
	void InitCommand(void);
	//�X���b�v�`�F�C���쐬
	void InitSwapchain(void);
	//�����_�[�^�[�Q�b�g�r���[�쐬
	void InitRTV(void);
	//�e�N�X�`���쐬
	void InitTexture(void);
	//���[�g�V�O�l�`���쐬
	void InitRootSignature(void);
	//�O���t�B�b�N�X�p�C�v���C���X�e�[�g�쐬
	void InitGPS(void);
	//�R���X�^���g�o�b�t�@�r���[�쐬
	void InitCBV(void);
	//�}�e���A���쐬
	void InitMaterial(void);
public:
	Dx12Wrapper();
	~Dx12Wrapper();
	void Update(void);
};

