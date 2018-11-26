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
	//�����֎~
	Application();
	//�R�s�[�֎~
	Application(const Application&);
	//����֎~
	void operator=(const Application&);

	std::shared_ptr<Dx12Wrapper> dx;


	//��ʃT�C�Y�p�\����
	RECT rect;
public:
	//�C���X�^���X��n��
	static Application& Instance()
	{
		static Application instance;
		return instance;
	}
	~Application();

	//������
	void Initialize(void);
	//�E�B���h�E�̕\���ƃ��C�����[�v
	void Run(void);
	//�㏈��
	void Terminate(void);

	//�E�B���h�E�̏�����
	void InitWindow(void);

	//�E�B���h�E�T�C�Y�̎擾
	Size GetWindowSize(void);

	WNDCLASSEX w;

	//�E�B���h�E�n���h��
	HWND hwnd;
};

