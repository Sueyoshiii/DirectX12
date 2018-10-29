#include "PMDModel.h"
#include "iostream"
#include <tchar.h>
#include "DX12Wrapper.h"



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

	for (int i = 0; i < 17; ++i)
	{
		if (pmdmaterices[i].texture_file_name[0] != '\0')
		{
			DirectX::LoadFromWICFile(ChangeWString(SetTex("Model/", pmdmaterices[i].texture_file_name)).c_str(), 0, &pmdmetadata, img);
		}
	}
}

std::string PMDModel::SetTex(const std::string path1, const std::string path2)
{
	int pathIndex1 = path1.rfind("/");
	int pathIndex2 = path1.rfind("\\");
	int pathIndex = max(pathIndex1, pathIndex2);

	std::string folderPath = path1.substr(0, pathIndex) + "/" + path2;

	return folderPath;
}

std::wstring PMDModel::ChangeWString(const std::string& st)
{
	auto byteSize = MultiByteToWideChar(CP_MACCP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, st.c_str(), -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(byteSize);

	byteSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, st.c_str(), -1, &wstr[0], byteSize);

	return wstr;
}

PMDModel::~PMDModel()
{
}