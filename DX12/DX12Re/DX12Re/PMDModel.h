#pragma once
#include <vector>
#include <DirectXTex.h>

#pragma pack(1)
struct PMDVertex
{
	DirectX::XMFLOAT3 pos;		//座標
	DirectX::XMFLOAT3 normal;	//法線ベクトル(nx, ny, nz)
	DirectX::XMFLOAT2 uv;		//UV座標(MMDは頂点UV)
	unsigned short bone_num[2];	//ボーン番号1,2
	unsigned char bone_weight;	//ボーン1に与える影響度
	unsigned char edge_flag;	//0:通常, 1:エッジ無効
	short etc;					//パディング(本当にこれでいいかは謎)
};
#pragma pack()

#pragma pack(1)
struct PMDMaterial {
	DirectX::XMFLOAT3 diffuse_color;	//dr,dg,db(減衰色)
	float alpha;						//減衰色の不透明度
	float specularity;					//スペキュラ乗数
	DirectX::XMFLOAT3 specular_color;	//sr,sg,sb(光沢色)
	DirectX::XMFLOAT3 mirror_color;		//mr,mg,mb(環境色)
	unsigned char toon_index;			//
	unsigned char edge_flag;			//輪郭、影
	unsigned int face_vert_count;		//面頂点数
	char texture_file_name[20];			//テクスチャファイル名
};
#pragma pack()

class PMDModel
{
private:
	unsigned int vertex_size;

	//頂点数
	unsigned int vertexNum;

	//インデックス数
	unsigned int indexNum;
public:
	PMDModel(const char* filepath);
	~PMDModel();

	//頂点集合体
	std::vector<PMDVertex> pmdvertex;

	//インデックス集合体
	std::vector<unsigned short> pmdindex;

	//マテリアル集合体
	std::vector<PMDMaterial> pmdmaterial;
};

