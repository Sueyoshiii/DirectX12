#include "Application.h"
#include <tchar.h>
#include <iostream>
#include "Dx12Wrapper.h"

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

//�R�[���o�b�N�֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//�E�B���h�E���j�����ꂽ�ꍇ
	if (msg == WM_DESTROY)
	{
		//OS�ɑ΂��ăA�v���P�[�V�����̏I����`����
		PostQuitMessage(0);
		return 0;
	}
	//�K��̏������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

Application::Application()
{
}


Application::~Application()
{
}

void Application::Initialize(void)
{
	InitWindow();

	dx = std::make_shared<Dx12Wrapper>();
}

void Application::Run(void)
{
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	//�E�B���h�E�j���̃^�C�~���O�Ń��[�v�𔲂��閳�����[�v
	while (true)
	{
		//OS����̃��b�Z�[�W��msg�Ɋi�[
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//���z�L�[�֘A�̕ϊ�
			TranslateMessage(&msg);
			//��������Ȃ��������b�Z�[�W��OS�ɕԂ�
			DispatchMessage(&msg);
		}

		//�A�v���P�[�V�������I���Ƃ���WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}
		dx->Update();
	}
}

void Application::Terminate(void)
{
	//�g��Ȃ��̂œo�^����
	UnregisterClass(w.lpszClassName, w.hInstance);
}

void Application::InitWindow(void)
{
	w.cbSize = sizeof(WNDCLASSEX);						//���̂��߂ɐݒ肷��̂���
	w.lpfnWndProc = (WNDPROC)WindowProcedure;			//�R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DirectXTest");				//�A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(0);					//�n���h���̎擾
	RegisterClassEx(&w);								//�A�v���P�[�V�����N���X

	rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };	//�E�B���h�E�T�C�Y����
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);	//�֐����g���Ă̕␳

	hwnd = CreateWindow(
		w.lpszClassName,		//�N���X���w��
		_T("DirectX12"),		//�^�C�g���o�[
		WS_OVERLAPPEDWINDOW,	//�^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,			//�\��X���W��OS�ɂ��C��
		CW_USEDEFAULT,			//�\��Y���W��OS�ɂ��C��
		rect.right - rect.left,	//�E�B���h�E��
		rect.bottom - rect.top,	//�E�B���h�E��
		nullptr,				//�e�E�B���h�E�n���h��
		nullptr,				//���j���[�n���h��
		w.hInstance,			//�Ăяo���A�v���P�[�V�����n���h��
		nullptr					//�ǉ��p�����[�^
	);

	//���s�����Ƃ��ɃL���b�`����
	if (hwnd == nullptr)
	{
		LPVOID messageBuffer = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer,
			0,
			nullptr
		);
		OutputDebugString((TCHAR*)messageBuffer);
		std::cout << (TCHAR*)messageBuffer << std::endl;
		LocalFree(messageBuffer);
	}
}

Size Application::GetWindowSize(void)
{
	return Size(rect.right, rect.bottom);
}
