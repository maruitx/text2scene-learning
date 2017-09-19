#include "IO_3DS.h"


CIO_3DS::~CIO_3DS()
{

}

void CIO_3DS::read3DS(const QString &filename)
{
	m_pFile = lib3ds_file_open(filename.toStdString().c_str());

	if (m_pFile == NULL)
	{
		Simple_Message_Box("Cannot open file" + filename);
		return;
	}
	else
	{
		buildMeshModelFrom3DSNodes(m_pFile->nodes);
	}

}

void CIO_3DS::buildMeshModelFrom3DSNodes(Lib3dsNode *pNode)
{
	for (Lib3dsNode *p = pNode; p; p=p->next)
	{
		if (p->type == LIB3DS_NODE_MESH_INSTANCE)
		{
			extractMeshFrom3DSNode((Lib3dsMeshInstanceNode*)p);
		}

		buildMeshModelFrom3DSNodes(p->childs);
	}
}

int CIO_3DS::extractMeshFrom3DSNode(Lib3dsMeshInstanceNode *pNode)
{
	Lib3dsMesh* pM = lib3ds_file_mesh_for_node(m_pFile, (Lib3dsNode *)pNode);
	if (!pM || !pM->nvertices) {
		return -1;
	}

	float inv_matrix[4][4], M[4][4], tmp[3];
	lib3ds_matrix_copy(M, pNode->base.matrix);
	lib3ds_matrix_translate(M, -pNode->pivot[0], -pNode->pivot[1], -pNode->pivot[2]);
	lib3ds_matrix_copy(inv_matrix, pM->matrix);
	lib3ds_matrix_inv(inv_matrix);
	lib3ds_matrix_mult(M, M, inv_matrix);
	float sf = m_pFile->master_scale;


	int previousVertNum = m_mesh->m_vertices.size();

	// load vertices
	for (int i = 0; i < pM->nvertices; i++) {
		lib3ds_vector_transform(tmp, M, pM->vertices[i]);
		m_mesh->m_vertices.push_back(MathLib::Vector3(tmp[0], tmp[1], tmp[2])*sf);
	}

	// load faces
	for (int i = 0; i < pM->nfaces; i++)
	{
		Lib3dsFace *pF = &(pM->faces[i]);

		std::vector<int> vertIds(3);
		for (int j=0; j < 3;j++)
		{
			// For each mesh component, 3ds indices start at 0; should convert to the global index
			vertIds[j] = pF->index[j] + previousVertNum;
		}

		m_mesh->m_faces.push_back(vertIds);
	}

	return 0;
}
