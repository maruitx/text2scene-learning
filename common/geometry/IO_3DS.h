#pragma once
#include "../third_party/Lib3DS/include/lib3ds.h"
#include "CMesh.h"
#include "../utilities/utility.h"


class CIO_3DS
{
public:
	CIO_3DS(CMesh *mesh) :m_mesh(mesh) {};
	~CIO_3DS();

	void read3DS(const QString &filename);

private:

	void buildMeshModelFrom3DSNodes(Lib3dsNode *pNode);
	int extractMeshFrom3DSNode(Lib3dsMeshInstanceNode *pNode);

	CMesh *m_mesh;
	Lib3dsFile *m_pFile;
};

