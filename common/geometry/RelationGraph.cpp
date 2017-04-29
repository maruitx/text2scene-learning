#include "RelationGraph.h"
#include "Scene.h"
#include "CModel.h"

RelationGraph::RelationGraph()
{
}

RelationGraph::RelationGraph(CScene *s):
m_scene(s), m_SuppThresh(0.02)
{
	m_nodeNum = m_scene->getModelNum();
	m_sceneMetric = m_scene->getSceneMetric();

	this->Initialize(m_nodeNum);
}

RelationGraph::~RelationGraph()
{
}

int RelationGraph::extractSupportRel()
{
	double dT = m_SuppThresh / m_sceneMetric;
	for (unsigned int i = 0; i < m_nodeNum; i++) {
		CModel *pMI = m_scene->getModel(i);
		for (unsigned int j = i + 1; j < m_nodeNum; j++) {
			CModel *pMJ = m_scene->getModel(j);
			bool roughOBB = false;
			if (pMI->IsSupport(pMJ, roughOBB, dT, m_scene->getUprightVec())) {
				this->InsertEdge(i, j, CT_VERT_SUPPORT);	// upright support
			}
		}
	}

	return 0;
}

void RelationGraph::correctSupportEdgeDir()
{
	// v1 should be support parent, v2 is support child
	for (unsigned int ei = 0; ei < this->ESize(); ei++) {
		Edge *currEdge = this->GetEdge(ei);
		if (currEdge->t == CT_VERT_SUPPORT) {
			CModel *pM1 = m_scene->getModel(currEdge->v1);
			CModel *pM2 = m_scene->getModel(currEdge->v2);
			double heightDiff = pM1->getOBB().BottomHeightDiff(pM2->getOBB(), m_scene->getUprightVec()); // M1_height - M2_height

			//if pM1 is higher than pM2, then pM2 is support parent of PM1
			if (heightDiff > m_SuppThresh/m_sceneMetric) {
				int tempId = currEdge->v1;
				currEdge->v1 = currEdge->v2;
				currEdge->v2 = tempId;
			}
			else if (heightDiff > -m_SuppThresh / m_sceneMetric && heightDiff < m_SuppThresh / m_sceneMetric) {
				// treat the large size object as parent
				double pM1Vol = pM1->getOBBVolume();
				double pM2Vol = pM2->getOBBVolume();

				if (pM1Vol < pM2Vol)
				{
					int tempId = currEdge->v1;
					currEdge->v1 = currEdge->v2;
					currEdge->v2 = tempId;
				}
			}
		}
	}
}

int RelationGraph::buildSuppEdgesFromFile()
{
	for (unsigned int i = 0; i < m_nodeNum; i++) {
		CModel *pMI = m_scene->getModel(i);
		std::vector<int> suppChildrenList = pMI->suppChindrenList;
		for (unsigned int j = 0; j < suppChildrenList.size(); j++) {
			int childId = suppChildrenList[j];
			CModel *pMJ = m_scene->getModel(childId);
			if (pMJ->parentContactNormal.dot(MathLib::Vector3(0,0,1)) > 0.95)
			{
				this->InsertEdge(i, childId, CT_VERT_SUPPORT);	// upright support, i is the support parent of j
			}
			else
			{
				this->InsertEdge(i, childId, CT_HORIZON_SUPPORT); // 
			}
		}
	}

	return 0;
}

void RelationGraph::updateGraph(int modelID)
{
	m_nodeNum = m_scene->getModelNum();

	std::vector<int> neighborEdges;
	this->GetAllNeigborEdgeList(modelID, neighborEdges);

	for (int i = 0; i < neighborEdges.size(); i++)
	{
		this->DeleteEdge(neighborEdges[i]);
	}

	updateSupportRel(modelID);
	pruneSupportRel();
}

void RelationGraph::updateGraph(int modelID, int suppModelID)
{
	std::vector<int> neighborEdges;
	this->GetAllNeigborEdgeList(modelID, neighborEdges);

	for (int i = 0; i < neighborEdges.size(); i++)
	{
		this->DeleteEdge(neighborEdges[i]);
	}

	// update support relationship
	CModel *pMJ = m_scene->getModel(modelID);
	CModel *pMI = m_scene->getModel(suppModelID);

	double dT = m_SuppThresh / m_sceneMetric;

	if (pMI->IsSupport(pMJ, false, dT, m_scene->getUprightVec())) {
		this->InsertEdge(suppModelID, modelID, CT_VERT_SUPPORT);	// upright support
	}
}

int RelationGraph::updateSupportRel(int modelID)
{
	double dT = m_SuppThresh / m_sceneMetric;
	CModel *pMJ = m_scene->getModel(modelID); 

	for (unsigned int i = 0; i < m_nodeNum; i++) {
		if ( i!=modelID)
		{
			CModel *pMI = m_scene->getModel(i);

			if (pMI->IsSupport(pMJ, false, dT, m_scene->getUprightVec())) {
				this->InsertEdge(i, modelID, CT_VERT_SUPPORT);	// upright support
			}
		}
	}

	return 0;
}

void RelationGraph::buildGraph()
{
	m_supportParentListForModels.clear();
	this->Initialize(m_nodeNum);

	if (m_scene->getSceneFormat() == "StanfordSceneDatabase")
	{
		buildSuppEdgesFromFile();
	}
	else{
		extractSupportRel();
		correctSupportEdgeDir();
		//pruneSupportRel();
	}
}

int RelationGraph::readGraph(const QString &filename)
{
	std::ifstream ifs(filename.toStdString());

	if (!ifs.is_open())
	{
		return -1;
	}

	char buf[MAX_STR_BUF_SIZE];
	ifs >> buf;
	while (!ifs.eof()) {
		switch (buf[0]) {
		case 'N':
			this->Clear();
			this->ReadVert(ifs);
			if (this->Size() != m_scene->getModelNum()) {
				Simple_Message_Box("Read SG: Number of models does not match!");
				return -1;
			}
			break;
		case 'E':
			this->ReadEdge(ifs);
			break;
		default:
			// eat up rest of the line
			ifs.getline(buf, MAX_STR_BUF_SIZE, '\n');
			break;
		}
		ifs >> buf;
	}
	return 0;
}

int RelationGraph::saveGraph(const QString &filename)
{
	std::ofstream  ofs(filename.toStdString());

	if (!ofs.is_open())
	{
		Simple_Message_Box(QString("Write SG: cannot open %1").arg(filename));
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////
	this->WriteVert(ofs);
	this->WriteEdge(ofs);
	//////////////////////////////////////////////////////////////////////////
	return 0;
}

void RelationGraph::drawGraph()
{
	if (m_nodeNum == 0)
	{
		return;
	}

	GLfloat red[] = { 1.0f, 0.3f, 0.3f, 1.0f };
	GLfloat blue[] = { 0.1f, 0.1f, 1.0f, 1.0f };
	GLfloat green[] = { 0.4f, 1.0f, 0.6f, 1.0f };
	GLubyte color[4] = { 0 };

	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);

	// Relations indicated with lines
	glDisable(GL_LIGHTING);
	glEnable(GL_LINE_STIPPLE);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	for (unsigned int i = 0; i < this->ESize(); i++) {
		const CUDGraph::Edge *e = this->GetEdge(i);
		switch (e->t) {
		case CT_VERT_SUPPORT:
			glLineWidth(5.0);
			glColor3f(1.0f, 0.3f, 0.3f); //red
			break;
		case CT_CONTACT:
			glLineWidth(5.0);
			glColor3f(0.8f, 0.5f, 0.1f);
			break;
		case CT_CONTAIN:
			glLineWidth(5.0);
			glColor3f(0.8f, 0.8f, 0.0f);
			break;
		case CT_PROXIMITY:
			glLineWidth(2.0);
			glColor3f(0.8, 0.1, 0.8);  // magenta
			break;
		case CT_SYMMETRY: case CT_PROX_SYM:
			glLineWidth(3.0);
			glColor3f(0.2, 0.8, 0.8);
			break;
		case CT_SUP_SYM: case CT_CONTACT_SYM:
			glLineWidth(3.0);
			glColor3f(0.2, 0.8, 0.2);
			break;
		case CT_WEAK:
			glLineWidth(1.0);
			glColor3f(0.4, 0.4, 0.9);
			break;
		}

		MathLib::Vector3 center[2];
		center[0] = m_scene->getModelOBBCenter(e->v1);
		center[1] = m_scene->getModelOBBCenter(e->v2);

		glBegin(GL_LINES);
		glVertex3dv(center[0].v);
		glVertex3dv(center[1].v);
		glEnd();

		// draw the end point of the edge
		QColor green(0, 255, 0);
		glPointSize(10);
		glColor4d(green.redF(), green.greenF(), green.blueF(), green.alphaF());
		glBegin(GL_POINTS);
		glVertex3d(center[1][0], center[1][1], center[1][2]);
		glEnd();
	}

	glPopAttrib();
}

void RelationGraph::collectSupportParentForModels()
{
	// build support parent list from computed support info
	// some model might be supported by multiple parents
	m_supportParentListForModels.clear(); 
	m_supportParentListForModels.resize(this->Size());

	for (unsigned int ei = 0; ei < this->ESize(); ei++) {
		Edge *currEdge = this->GetEdge(ei);
		if (currEdge->t == RelationGraph::CT_VERT_SUPPORT) {
			m_supportParentListForModels[currEdge->v2].push_back(currEdge->v1);
		}
	}
}

int RelationGraph::pruneSupportRel()
{
	// collect direct support info.
	collectSupportParentForModels();

	// check nodes with more than more supporters (if multiple nodes support this node)
	// filter those who doesn't contact
	for (unsigned int i = 0; i < m_supportParentListForModels.size(); i++)
	{
		if (m_supportParentListForModels[i].size() > 1)
		{
			CModel *pM1 = m_scene->getModel(i);
			for (int j = 0; j < m_supportParentListForModels[i].size(); j++)
			{
				CModel *pM2 =  m_scene->getModel(m_supportParentListForModels[i][j]);

				// use a relative large support threshold for conservative pruning
				if (!pM1->IsSupport(pM2, true, 10 * m_SuppThresh / m_sceneMetric, m_scene->getUprightVec()))
				{
					this->DeleteEdge(i, m_supportParentListForModels[i][j]);
				}
				else
				{
					//m_onTopList[i] = SuppList[i][j];
				}
			}
		}
	}

	return 0;
}




