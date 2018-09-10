#pragma once
#include <memory>

struct Size {
	Size() {}
	Size(int inw, int inh);
	int w;
	int h;
};

class DX12Wrapper;

	///シングルトンクラス
class Application
{
private:
	std::shared_ptr<DX12Wrapper> dx;
	//生成禁止
	Application();
	//コピー禁止
	Application(const Application&);
	//代入禁止
	void operator=(const Application&);

	void InitWindow();

	//ウィンドウのハンドル
	HWND handle;
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
	void Initialize();
	//ウィンドウの表示とメインループ
	void Run();
	//後処理
	void Terminate();

	Size GetWindowSize();

	HWND GetWindowHandle()const;
};

