#include "PMDModel.h"



PMDModel::PMDModel(const char* filepath)
{
	auto fp = fopen(filepath, "rb");
	const unsigned int vertexCount = 9036;
	const unsigned int vertex_size = 38;
	std::vector<char> pmdvertex(vertexCount * vertex_size);
	fread(pmdvertex.data(), pmdvertex.size(), 1, fp);
}


PMDModel::~PMDModel()
{
}
