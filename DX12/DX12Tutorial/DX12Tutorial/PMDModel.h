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
	float diffuse_color[3]; //dr,dg,db(減衰色)
	float alpha; //減衰色の不透明度
	float specularity; //スペキュラ乗数
	float specular_color[3]; //sr,sg,sb(光沢色)
	float mirror_color[3]; //mr,mg,mb(環境色)
	unsigned char toon_index; //
	unsigned char edge_flag; //輪郭、影
	unsigned int face_vert_count; //面頂点数
	char texture_file_name[20]; //テクスチャファイル名
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
	//頂点数
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

