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
	short etc;					//�p�f�B���O(�{���ɂ���ł������͓�)
};
#pragma pack()

#pragma pack(1)
struct PMDMaterial {
	DirectX::XMFLOAT3 diffuse_color;	//dr,dg,db(�����F)
	float alpha;						//�����F�̕s�����x
	float specularity;					//�X�y�L�����搔
	DirectX::XMFLOAT3 specular_color;	//sr,sg,sb(����F)
	DirectX::XMFLOAT3 mirror_color;		//mr,mg,mb(���F)
	unsigned char toon_index;			//
	unsigned char edge_flag;			//�֊s�A�e
	unsigned int face_vert_count;		//�ʒ��_��
	char texture_file_name[20];			//�e�N�X�`���t�@�C����
};
#pragma pack()

class PMDModel
{
private:
	unsigned int vertex_size;

	//���_��
	unsigned int vertexNum;

	//�C���f�b�N�X��
	unsigned int indexNum;
public:
	PMDModel(const char* filepath);
	~PMDModel();

	//���_�W����
	std::vector<PMDVertex> pmdvertex;

	//�C���f�b�N�X�W����
	std::vector<unsigned short> pmdindex;

	//�}�e���A���W����
	std::vector<PMDMaterial> pmdmaterial;
};

