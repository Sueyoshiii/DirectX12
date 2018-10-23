#pragma once
#include <vector>
#include <DirectXMath.h>
#include <d3d12.h>
#include <DirectXTex.h>
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

//struct PMDBone {
//	char boneName;
//	unsigned short parentIndex;
//	unsigned short tailIndex;
//	unsigned char boneType;
//	unsigned short IKboneIndex;
//	
//};

class PMDModel
{
private:
	//���_��
	unsigned int vertexNum;
	unsigned int indicesNum;
	unsigned int materialNum;

	std::string SetTex(const std::string path1, const std::string path2);
	std::wstring ChangeWString(const std::string& st);
public:
	PMDModel(const char* filepath);
	~PMDModel();

	std::vector<PMDVertex> pmdvertices;
	std::vector<unsigned short> pmdindices;
	std::vector<PMDMaterial> pmdmaterices;

	DirectX::ScratchImage scImage;
	DirectX::TexMetadata texMeta;

	unsigned int vertex_size;
};

