#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include "Application.h"
#include "DX12Wrapper.h"
using namespace std;

HWND hwnd;
constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;

//コールバック関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//ウィンドウが破棄された場合
	if (msg == WM_DESTROY)
	{
		//OSに対してアプリケーションの終了を伝える
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
	//ウィンドウの情報を格納する構造体
	WNDCLASSEX wnd = {};
	//サイズ渡し(正直知ってるはずの情報なので何のためにやってるかは謎)
	wnd.cbSize = sizeof(WNDCLASSEX);
	//コールバック関数指定
	wnd.lpfnWndProc = (WNDPROC)WindowProcedure;
	//アプリケーションクラス名
	wnd.lpszClassName = _T("DirectX12");
	//ハンドル取得
	wnd.hInstance = GetModuleHandle(0);
	//アプリケーションクラス(OSに対する予告)
	RegisterClassEx(&wnd);

	//画面サイズ指定
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウ作成
	hwnd = CreateWindow(
		wnd.lpszClassName, //OSが式罰するハンドル名
		_T("DX12"), //タイトルバーのテキスト
		WS_OVERLAPPEDWINDOW, //ウィンドウ
		CW_USEDEFAULT, //左座標
		CW_USEDEFAULT, //上座標
		rect.right - rect.left, //ウィンドウ幅
		rect.bottom - rect.top, //ウィンドウ高さ
		nullptr, //親ウィンドウ
		nullptr, //メニューハンドル
		wnd.hInstance, //呼び出し青売りケーションハンドル
		nullptr); //追加パラメータ
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

