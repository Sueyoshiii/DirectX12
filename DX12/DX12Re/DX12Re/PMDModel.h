#pragma once
#include <vector>
#include <DirectXTex.h>

#pragma pack(1)
struct PMDVertex
{
	DirectX::XMFLOAT3 pos;		//���W
	DirectX::XMFLOAT3 normal;	//�@���x�N�g��(nx, ny, nz)
	DirectX::XMFLOAT2 uv;		//UV���W(MMD�͒��_UV)
	unsigned short bone_num[2];	//�{�[���ԍ�1,2
	unsigned char bone_weight;	//�{�[��1�ɗ^����e���x
	unsigned char edge_flag;	//0:�ʏ�, 1:�G�b�W����
};
#pragma pack()

class PMDModel
{
private:
	//���_��
	unsigned int vertexNum;
public:
	PMDModel(const char* filepath);
	~PMDModel();
};

