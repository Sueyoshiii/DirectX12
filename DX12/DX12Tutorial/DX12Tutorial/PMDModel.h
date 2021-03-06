#pragma once
#include <vector>
#include <DirectXMath.h>
#include <d3d12.h>
#include <DirectXTex.h>
#include <memory>

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
	float diffuse_color[3]; //dr,dg,db(ΈF)
	float alpha; //ΈFΜs§Ύx
	float specularity; //XyLζ
	float specular_color[3]; //sr,sg,sb(υςF)
	float mirror_color[3]; //mr,mg,mb(Β«F)
	unsigned char toon_index; //
	unsigned char edge_flag; //ΦsAe
	unsigned int face_vert_count; //ΚΈ_
	char texture_file_name[20]; //eNX`t@CΌ
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
	//Έ_
	unsigned int vertexNum;
	unsigned int indicesNum;
	unsigned int materialNum;

	//tH_Όo
	std::string SetTex(const std::string path1, const std::string path2);
	//string¨wstringΦΜΟ·
	std::wstring ChangeWString(const std::string& st);

	DirectX::ScratchImage img;
public:
	PMDModel(const char* filepath);
	~PMDModel();

	std::weak_ptr<ID3D12Device> dev;

	std::vector<PMDVertex> pmdvertices;
	std::vector<unsigned short> pmdindices;
	std::vector<PMDMaterial> pmdmaterices;

	unsigned int vertex_size;

	DirectX::TexMetadata pmdmetadata = {};
};

