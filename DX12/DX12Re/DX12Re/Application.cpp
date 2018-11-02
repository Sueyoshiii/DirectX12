#include "Application.h"
#include <tchar.h>
#include <iostream>
#include "Dx12Wrapper.h"

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

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
	//規定の処理を行う
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

	//ウィンドウ破棄のタイミングでループを抜ける無限ループ
	while (true)
	{
		//OSからのメッセージをmsgに格納
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//仮想キー関連の変換
			TranslateMessage(&msg);
			//処理されなかったメッセージをOSに返す
			DispatchMessage(&msg);
		}

		//アプリケーションが終わるときにWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}
		dx->Update();
	}
}

void Application::Terminate(void)
{
	//使わないので登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);
}

void Application::InitWindow(void)
{
	w.cbSize = sizeof(WNDCLASSEX);						//何のために設定するのか謎
	w.lpfnWndProc = (WNDPROC)WindowProcedure;			//コールバック関数の指定
	w.lpszClassName = _T("DirectXTest");				//アプリケーションクラス名
	w.hInstance = GetModuleHandle(0);					//ハンドルの取得
	RegisterClassEx(&w);								//アプリケーションクラス

	rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };	//ウィンドウサイズ決め
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);	//関数を使っての補正

	hwnd = CreateWindow(
		w.lpszClassName,		//クラス名指定
		_T("DirectX12"),		//タイトルバー
		WS_OVERLAPPEDWINDOW,	//タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,			//表示X座標をOSにお任せ
		CW_USEDEFAULT,			//表示Y座標をOSにお任せ
		rect.right - rect.left,	//ウィンドウ幅
		rect.bottom - rect.top,	//ウィンドウ高
		nullptr,				//親ウィンドウハンドル
		nullptr,				//メニューハンドル
		w.hInstance,			//呼び出しアプリケーションハンドル
		nullptr					//追加パラメータ
	);

	//失敗したときにキャッチする
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
