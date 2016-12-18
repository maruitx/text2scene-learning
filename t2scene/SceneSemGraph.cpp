#include "SceneSemGraph.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"
#include "../common/geometry/SceneGraph.h"
#include "../scene_lab/modelDatabase.h"

SceneSemGraph::SceneSemGraph(CScene *s, ModelDatabase *db)
	:m_scene(s), m_modelDB(db)
{
	m_nodeNum = 0;
	m_edgeNum = 0;

	m_sceneFormat = m_scene->getSceneFormat();
	m_relGraph = m_scene->getSceneGraph();
}


SceneSemGraph::~SceneSemGraph()
{

}


void SceneSemGraph::addNode(const QString &nType, const QString &nName)
{
	SemNode newNode = SemNode(nType, nName, m_nodeNum);
	m_nodes.push_back(newNode);

	m_nodeNum++;
}


void SceneSemGraph::addEdge(int s, int t)
{
	SemEdge newEdge = SemEdge(s, t, m_edgeNum);

	m_edges.push_back(newEdge);

	// update node edge list
	//m_nodes[s].outEdgeNodeList.push_back(newEdge.edgeId);
	//m_nodes[t].inEdgeNodeList.push_back(newEdge.edgeId);

	m_edgeNum++;
}

void SceneSemGraph::generateGraph()
{
	// extract model as object node
	int modelNum = m_scene->getModelNum();

	for (int i = 0; i < modelNum;  i++)
	{
		QString modelNameStr = m_scene->getModelNameString(i);
		MetaModel *metaModel = m_modelDB->getMetaModelByNameString(modelNameStr);
		m_metaModelList.push_back(metaModel);

		QString objectNodeName = metaModel->getProcessedCatName();
		addNode(QString("object"), objectNodeName);
	}

	// extract low-level model attributes from ShapeNetSem annotation
	buildFromModelDBAnnotation();

	//
	extractRelationsFromRelationGraph();

	//
	loadAttributeNodeFromAnnotation();
}

void SceneSemGraph::buildFromModelDBAnnotation()
{

}

void SceneSemGraph::extractRelationsFromRelationGraph()
{
	for (int i = 0; i < m_relGraph->ESize(); i++)
	{
		// extract relationship node

		// insert edge and update node in/out edge list
	}
}

void SceneSemGraph::loadAttributeNodeFromAnnotation()
{

}


void SceneSemGraph::saveGraph()
{
	QString filename;

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (!outFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		QString currLine;


		// save scene meta data



		// save nodes in format: nodeId,nodeType,nodeName,inEdgeNodeList,outEdgeNodeList



		// save edges in format: edgeId,startId endId

		outFile.close();
	}
} 

