#pragma once
#include <vector>
#include <DirectXMath.h>

struct PMDVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	unsigned short bone_num[2];
	unsigned char weight;
	unsigned char flag;
	short etc;
};

class PMDModel
{
private:
	//í∏ì_êî
	unsigned int vertexNum;
	unsigned int indicesNum;
public:
	PMDModel(const char* filepath);
	~PMDModel();

	std::vector<PMDVertex> pmdvertices;
	std::vector<unsigned short> pmdindices;

	unsigned int vertex_size;
};

