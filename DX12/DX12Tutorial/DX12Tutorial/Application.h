#pragma once
#include <memory>

struct Size {
	Size() {}
	Size(int inw, int inh);
	int w;
	int h;
};

class DX12Wrapper;

	///�V���O���g���N���X
class Application
{
private:
	std::shared_ptr<DX12Wrapper> dx;
	//�����֎~
	Application();
	//�R�s�[�֎~
	Application(const Application&);
	//����֎~
	void operator=(const Application&);

	void InitWindow();

	//�E�B���h�E�̃n���h��
	HWND handle;
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
	void Initialize();
	//�E�B���h�E�̕\���ƃ��C�����[�v
	void Run();
	//�㏈��
	void Terminate();

	Size GetWindowSize();

	HWND GetWindowHandle()const;
};

