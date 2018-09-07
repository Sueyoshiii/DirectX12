#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include "Application.h"
#include "DX12Wrapper.h"
using namespace std;

HWND hwnd;
constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;

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
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

Application::Application()
{
	rect.right = WINDOW_WIDTH;
	rect.left = 0;
	rect.bottom = WINDOW_HEIGHT;
	rect.top = 0;
}

Application::Application(const Application &)
{
}

Application::~Application()
{
}

void Application::InitWindow()
{
	//�E�B���h�E�̏����i�[����\����
	WNDCLASSEX wnd = {};
	//�T�C�Y�n��(�����m���Ă�͂��̏��Ȃ̂ŉ��̂��߂ɂ���Ă邩�͓�)
	wnd.cbSize = sizeof(WNDCLASSEX);
	//�R�[���o�b�N�֐��w��
	wnd.lpfnWndProc = (WNDPROC)WindowProcedure;
	//�A�v���P�[�V�����N���X��
	wnd.lpszClassName = _T("DirectX12");
	//�n���h���擾
	wnd.hInstance = GetModuleHandle(0);
	//�A�v���P�[�V�����N���X(OS�ɑ΂���\��)
	RegisterClassEx(&wnd);

	//��ʃT�C�Y�w��
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	//�E�B���h�E�쐬
	hwnd = CreateWindow(
		wnd.lpszClassName, //OS����������n���h����
		_T("DX12"), //�^�C�g���o�[�̃e�L�X�g
		WS_OVERLAPPEDWINDOW, //�E�B���h�E
		CW_USEDEFAULT, //�����W
		CW_USEDEFAULT, //����W
		rect.right - rect.left, //�E�B���h�E��
		rect.bottom - rect.top, //�E�B���h�E����
		nullptr, //�e�E�B���h�E
		nullptr, //���j���[�n���h��
		wnd.hInstance, //�Ăяo������P�[�V�����n���h��
		nullptr); //�ǉ��p�����[�^
}


void Application::Initialize()
{
	InitWindow();

	if (handle = nullptr)
	{
		LPVOID messageBuffer = nullptr;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer,
			0,
			nullptr);
		OutputDebugString((TCHAR*)messageBuffer);
		cout << (TCHAR*)messageBuffer << endl;
		LocalFree(messageBuffer);
	}

	dx = std::make_shared<DX12Wrapper>(nullptr, handle);
}

void Application::Run()
{
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}
}

void Application::Terminate()
{
	WNDCLASSEX wnd = {};
	UnregisterClass(wnd.lpszClassName, wnd.hInstance);
}

Size Application::GetWindowSize()
{
	return Size();
}

