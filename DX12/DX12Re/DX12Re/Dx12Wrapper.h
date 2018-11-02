#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

class Dx12Wrapper
{
private:
	//�f�o�C�X
	ID3D12Device* dev;

	//�f�X�N���v�^�q�[�v
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

	//
	IDXGIFactory6* dxgi;

	//GPU�ŕ`�悵���摜�����ۂ̉�ʂɔ��f�����邽�߂̋@�\�A���
	IDXGISwapChain4* swapChain;

	//�R�}���h�L���[�ɃT�u�~�b�g�����R�}���h���X�g�̊��������m����
	ID3D12Fence* fence;

	//���_�o�b�t�@
	ID3D12Resource* vertexBuffer;

	UINT fenceValue;

	std::vector<ID3D12Resource*> renderTarget;

	//���s����
	void ExecuteCommand(void);
	//�҂�����
	void WaitExecute(void);
public:
	Dx12Wrapper();
	~Dx12Wrapper();
	void Update(void);
};

