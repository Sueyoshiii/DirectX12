#pragma once
#include <Windows.h>
#include <memory>

struct Size
{
	Size() {}
	int w, h;
	Size(int inw, int inh) : w(inw), h(inh) {}
};

class Dx12Wrapper;

class Application
{
private:
	//生成禁止
	Application();
	//コピー禁止
	Application(const Application&);
	//代入禁止
	void operator=(const Application&);

	std::shared_ptr<Dx12Wrapper> dx;


	//画面サイズ用構造体
	RECT rect;
public:
	//インスタンスを渡す
	static Application& Instance()
	{
		static Application instance;
		return instance;
	}
	~Application();

	//初期化
	void Initialize(void);
	//ウィンドウの表示とメインループ
	void Run(void);
	//後処理
	void Terminate(void);

	//ウィンドウの初期化
	void InitWindow(void);

	//ウィンドウサイズの取得
	Size GetWindowSize(void);

	WNDCLASSEX w;

	//ウィンドウハンドル
	HWND hwnd;
};

