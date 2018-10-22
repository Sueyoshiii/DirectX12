#include "PMDModel.h"
#include "iostream"


PMDModel::PMDModel(const char* filepath)
{
	struct PMDHeader {
		char magic[3];
		float version;
		char model_name[20];
		char comment[256];
	};
	auto fp = fopen(filepath, "rb");
	PMDHeader pmdheader = {};
	//æ“ª3•¶š‚ğ•Ê•¨‚Æ‚µ‚Äfread‚·‚é
	fread(&pmdheader.magic, sizeof(pmdheader.magic), 1, fp);
	fread(&pmdheader.version, sizeof(pmdheader) - sizeof(pmdheader.magic) - 1, 1, fp);

	fread(&vertexNum, sizeof(vertexNum), 1, fp);

	vertex_size = 38;
	pmdvertices.resize(vertexNum);

	for (int i = 0; i < vertexNum; ++i)
	{
		fread(&pmdvertices[i], vertex_size, 1, fp);
	}

	indicesNum = 0;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	pmdindices.resize(indicesNum);
	fread(pmdindices.data(), sizeof(unsigned short), pmdindices.size(), fp);

	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdmaterices.resize(materialNum);
	fread_s(pmdmaterices.data(), sizeof(PMDMaterial) * materialNum, sizeof(PMDMaterial), materialNum, fp);

	//for (auto& material : pmdmaterices)
	//{
	//	fread(&material, 46, 1, fp);
	//	fread(&material.face_vert_count, 24, 1, fp);
	//}

	fclose(fp);
}


PMDModel::~PMDModel()
{
}