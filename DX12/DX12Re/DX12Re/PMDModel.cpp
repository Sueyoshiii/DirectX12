#include "PMDModel.h"


PMDModel::PMDModel(const char* filepath)
{
	struct PMDHeader {
		char magic[3];		//「Pmd」という3バイト
		float version;		//バージョン情報(4バイト)
		char model_name[20];//モデル名(20バイト)
		char comment[256];	//コメント(256バイト)
	};
	auto fp = fopen(filepath, "rb");
	PMDHeader pmdheader = {};
	//先頭3文字を別物としてfreadする
	fread(&pmdheader.magic, sizeof(pmdheader.magic), 1, fp);
	fread(&pmdheader.version, sizeof(pmdheader) - sizeof(pmdheader.magic) - 1, 1, fp);

	fread(&vertexNum, sizeof(vertexNum), 1, fp);
	vertex_size = 38;
	pmdvertex.resize(vertexNum);

	for (int i = 0; i < vertexNum; ++i)
	{
		fread(&pmdvertex[i], vertex_size, 1, fp);
	}

	indexNum = 0;
	fread(&indexNum, sizeof(indexNum), 1, fp);
	pmdindex.resize(indexNum);
	fread(pmdindex.data(), sizeof(unsigned short), pmdindex.size(), fp);

	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdmaterial.resize(materialNum);

	for (auto& material : pmdmaterial)
	{
		fread(&material.diffuse_color, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&material.alpha, sizeof(float), 1, fp);
		fread(&material.specularity, sizeof(float), 1, fp);
		fread(&material.specular_color, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&material.mirror_color, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&material.toon_index, sizeof(unsigned char), 1, fp);
		fread(&material.edge_flag, sizeof(unsigned char), 1, fp);

		fread(&material.face_vert_count, sizeof(unsigned int), 1, fp);
		fread(&material.texture_file_name[0], sizeof(material.texture_file_name), 1, fp);
	}

	//一時的ボーン読み込み
	USHORT boneNum = 0;
	fread(&boneNum, sizeof(USHORT), 1, fp);

	fclose(fp);
}


PMDModel::~PMDModel()
{
}
