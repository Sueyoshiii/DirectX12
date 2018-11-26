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
};
#pragma pack()

class PMDModel
{
private:
	//頂点数
	unsigned int vertexNum;
public:
	PMDModel(const char* filepath);
	~PMDModel();
};

