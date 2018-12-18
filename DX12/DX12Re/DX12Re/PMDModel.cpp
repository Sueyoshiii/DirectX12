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

	vertexNum = 0;
	fread(&vertexNum, sizeof(vertexNum), 1, fp);
	pmdvertex.resize(vertexNum);

	for (int i = 0; i < vertexNum; ++i)
	{
		fread(&pmdvertex[i], sizeof(PMDVertex), 1, fp);
	}

	indexNum = 0;
	fread(&indexNum, sizeof(indexNum), 1, fp);
	pmdindex.resize(indexNum);
	fread(pmdindex.data(), sizeof(unsigned short), pmdindex.size(), fp);

	materialNum = 0;
	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdmaterial.resize(materialNum);
	for (auto& material : pmdmaterial)
	{
		fread(&material, sizeof(PMDMaterial), 1, fp);
	}

	//一時的ボーン読み込み
	USHORT boneNum = 0;
	fread(&boneNum, sizeof(USHORT), 1, fp);

	fclose(fp);

	for (int i = 0; i < 17; ++i)
	{
		if (pmdmaterial[i].texture_file_name[0] != '\0')
		{
			DirectX::LoadFromWICFile(
				ChangeWString(
					SetTex("Model/", pmdmaterial[i].texture_file_name)).c_str(), 
				0, 
				&pmdmetadata, 
				img);
		}
	}
}

std::string PMDModel::SetTex(const std::string path1, const std::string path2)
{
	int pathIndex1 = path1.rfind("/");
	int pathIndex2 = path2.rfind("\\");
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderpath = path1.substr(0, pathIndex) + "/" + path2;

	return folderpath;
}

std::wstring PMDModel::ChangeWString(const std::string & st)
{
	auto bytesize = MultiByteToWideChar(
		CP_MACCP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		st.c_str(),
		-1,
		nullptr,
		0
	);

	std::wstring wstr;
	wstr.resize(bytesize);

	bytesize = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		st.c_str(),
		-1,
		&wstr[0],
		bytesize
	);

	return wstr;
}

PMDModel::~PMDModel()
{
}
