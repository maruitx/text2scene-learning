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

void SceneSemGraph::generateGraph()
{
	// extract model as object node
	int modelNum = m_scene->getModelNum();

	for (int i = 0; i < modelNum;  i++)
	{
		QString modelNameStr = m_scene->getModelNameString(i);
		MetaModel *metaModel = m_modelDB->getMetaModelByNameString(modelNameStr);
		metaModel->setTransMat(m_scene->getModelInitTransMat(i));
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
	QString scenePath = m_scene->getFilePath();
	QString sceneName = m_scene->getSceneName();

	QString filename = scenePath + "/" + sceneName + ".ssg";

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		ofs << m_sceneFormat << "\n";

		// save scene meta data
		int modelNum = m_metaModelList.size();
		ofs << "modelCount " << modelNum << "\n";

		for (int i = 0; i < modelNum; i++)
		{
			ofs << "newModel "<< i << " " << m_metaModelList[i]->getIdStr() << "\n";
			ofs << "transform " << GetTransformationString(m_metaModelList[i]->getTransMat()) << "\n";
		}

		// save nodes in format: nodeId,nodeType,nodeName,inEdgeNodeList,outEdgeNodeList
		ofs << "nodeNum " << m_nodeNum << "\n";
		for (int i = 0; i < m_nodeNum; i++)
		{
			ofs << i << "," << m_nodes[i].nodeType << "," << m_nodes[i].nodeName << ","
				<< GetIntString(m_nodes[i].inEdgeNodeList, " ") << "," 
				<< GetIntString(m_nodes[i].outEdgeNodeList, " ") << "\n";
		}


		// save edges in format: edgeId,startId endId
		ofs << "edgeNum " << m_edgeNum << "\n";
		for (int i = 0; i < m_edgeNum; i++)
		{
			ofs << i << "," << m_edges[i].sourceNodeId << "," << m_edges[i].targetNodeId << "\n";
		}

		outFile.close();

		Simple_Message_Box("Semantic graph generated.");
	}

	else
	{
		Simple_Message_Box("Fail to generate semantic graph ");
	}
} 

void SceneSemGraph::loadGraph(const QString &filename)
{
	QFile inFile(filename);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	ifs >> m_sceneFormat;

	// load model info
	QString currLine = ifs.readLine();

	if (currLine.contains("modelCount "))
	{
		m_modelNum = StringToIntegerList(currLine.toStdString(), "modelCount ")[0];
	}

	for (int i = 0; i < m_modelNum; i++)
	{
		currLine = ifs.readLine();
		if (currLine.contains("newModel "))
		{			
			std::vector<std::string> parts = PartitionString(currLine.toStdString(), " ");
			int modelIndex = StringToInt(parts[1]);
			QString modelNameString = QString(parts[2].c_str());

			MetaModel *newMetaModel = new MetaModel(modelNameString);

			currLine = ifs.readLine();
			
			if (currLine.contains("transform "))
			{
				std::vector<float> transformVec = StringToFloatList(currLine.toStdString(), "transform ");
				MathLib::Matrix4d transMat(transformVec);
				newMetaModel->setTransMat(transMat);
			}

			m_metaModelList.push_back(newMetaModel);
		}
	}

	
	inFile.close();
}

