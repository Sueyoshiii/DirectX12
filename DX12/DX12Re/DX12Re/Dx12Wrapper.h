#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>
#include <DirectXTex.h>

class Dx12Wrapper
{
private:
	//�f�o�C�X
	ID3D12Device* dev;

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
	IDXGIFactory6* dxgi;

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

	//�ŏI�I�ɗ~�����I�u�W�F�N�g
	ID3D12RootSignature* rootSignature;

	//�p�C�v���C���X�e�[�g
	ID3D12PipelineState* pipelineState;

	//���[�g�V�O�l�`������邽�߂̍ޗ�
	ID3DBlob* signature;

	//�G���[�o���Ƃ��̑Ώ�
	ID3DBlob* error;

	//RTV(�����_�[�^�[�Q�b�g)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* rtvDescHeap;

	//DSV(�[�x)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* dsvDescHeap;

	//���̑�(�e�N�X�`���A�萔)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* rgstDescHeap;

	//�t�F���X�l
	UINT fenceValue;

	std::vector<ID3D12Resource*> renderTarget;

	//�C���f�b�N�X�z��
	std::vector<unsigned short> index;

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
public:
	Dx12Wrapper();
	~Dx12Wrapper();
	void Update(void);
};

