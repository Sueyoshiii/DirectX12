#include "PMDModel.h"


PMDModel::PMDModel(const char* filepath)
{
	struct PMDHeader {
		char magic[3];		//�uPmd�v�Ƃ���3�o�C�g
		float version;		//�o�[�W�������(4�o�C�g)
		char model_name[20];//���f����(20�o�C�g)
		char comment[256];	//�R�����g(256�o�C�g)
	};
	auto fp = fopen(filepath, "rb");
	PMDHeader pmdheader = {};
	//�擪3������ʕ��Ƃ���fread����
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
