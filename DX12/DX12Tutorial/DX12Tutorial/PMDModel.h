#pragma once
#include <vector>
#include <DirectXMath.h>
#pragma pack(1)
struct PMDVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	unsigned short bone_num[2];
	unsigned char weight;
	unsigned char flag;
	short etc;
};
#pragma pack()

#pragma pack(1)
struct PMDMaterial {
	float diffuse_color[3]; //dr,dg,db(�����F)
	float alpha; //�����F�̕s�����x
	float specularity; //�X�y�L�����搔
	float specular_color[3]; //sr,sg,sb(����F)
	float mirror_color[3]; //mr,mg,mb(���F)
	unsigned char toon_index; //
	unsigned char edge_flag; //�֊s�A�e
	unsigned int face_vert_count; //�ʒ��_��
	char texture_file_name[20]; //�e�N�X�`���t�@�C����
};
#pragma pack()

struct PMDBone {
	std::vector<unsigned char>* boneName;
};

class PMDModel
{
private:
	//���_��
	unsigned int vertexNum;
	unsigned int indicesNum;
	unsigned int materialNum;
public:
	PMDModel(const char* filepath);
	~PMDModel();

	std::vector<PMDVertex> pmdvertices;
	std::vector<unsigned short> pmdindices;
	std::vector<PMDMaterial> pmdmaterices;

	unsigned int vertex_size;
};

