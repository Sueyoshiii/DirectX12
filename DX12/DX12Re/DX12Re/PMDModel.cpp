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

	fclose(fp);
}


PMDModel::~PMDModel()
{
}
