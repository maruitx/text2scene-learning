#include "SceneSemGraph.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"
#include "../common/geometry/RelationGraph.h"
#include "../scene_lab/modelDatabase.h"

SceneSemGraph::SceneSemGraph(CScene *s, ModelDatabase *db)
	:m_scene(s), m_modelDB(db)
{
	m_sceneFormat = m_scene->getSceneFormat();
	m_relGraph = m_scene->getSceneGraph();
}

SceneSemGraph::~SceneSemGraph()
{
	m_metaModelList.clear();
}

void SceneSemGraph::generateGraph()
{
	// extract model as object node
	int modelNum = m_scene->getModelNum();

	for (int i = 0; i < modelNum;  i++)
	{
		QString modelNameStr = m_scene->getModelNameString(i);

		if (m_modelDB->isModelInDB(modelNameStr))
		{
			MetaModel *metaModel = m_modelDB->getMetaModelByNameString(modelNameStr);
			metaModel->setTransMat(m_scene->getModelInitTransMat(i));
			m_metaModelList.push_back(metaModel);

			QString objectNodeName = metaModel->getProcessedCatName();
			addNode(QString(SSGNodeType[0]), objectNodeName);
		}
		else
		{
			std::cout << "SceneSemGraph: cannot find model: " << modelNameStr.toStdString() << " in ShapeNetDB-"<<m_modelDB->getMetaFileType().toStdString()<<"\n";
		}

	}

	// extract low-level model attributes from ShapeNetSem annotation
	buildFromModelDBAnnotation();

	// add relationships
	extractRelationsFromRelationGraph();

	// add attributes
	loadAttributeNodeFromAnnotation();

	std::cout << "SceneSemGraph: graph generated.\n";
}

void SceneSemGraph::buildFromModelDBAnnotation()
{

}

void SceneSemGraph::extractRelationsFromRelationGraph()
{
	if (m_relGraph == NULL)
	{
		std::cout << "SceneSemGraph: relation graph is not generated.\n";
		return;
	}

	for (int i = 0; i < m_relGraph->ESize(); i++)
	{
		// extract relationship node
		
		RelationGraph::Edge *relEdge =  m_relGraph->GetEdge(i);

		switch (relEdge->t)
		{
			// e.g. table --> support --> laptop
		case RelationGraph::CT_SUPPORT:
			addNode(SSGNodeType[2], SSGPairRelStrings[0]);
			addEdge(relEdge->v1, m_nodeNum-1);
			addEdge(m_nodeNum-1, relEdge->v2);
			
		case RelationGraph::CT_CONTAIN:

		default:
			break;
		}


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

		std::cout << "SceneSemGraph: graph saved.\n";
	}

	else
	{
		std::cout << "SceneSemGraph: fail to save graph .\n";
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

void SceneSemGraph::saveNodeStringToLabelIDMap(const QString &filename)
{
	if (!m_catStringToLabelIDMap.empty())
	{
		return;
	}

	// generate node string to label id map
	// category labels
	int labelID = 0;
	for (auto it = m_modelDB->dbCategories.begin(); it != m_modelDB->dbCategories.end(); it++)
	{
		m_catStringToLabelIDMap[it->first] = labelID;
		labelID++;
	}

	// save the map
	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		// category labels
		for (auto it = m_catStringToLabelIDMap.begin(); it != m_catStringToLabelIDMap.end(); it++)
		{
			ofs << it->first << " " << it->second << " 0\n";
		}

		// single attribute labels
		//labelID = 1000;
		for (int i = 0; i < SingleAttriNum; i++)
		{
			ofs << SSGSingleAttriStrings[i] << " " << labelID << " 1\n";
			labelID++;
		}


		// pair-wise relationship labels
		//labelID = 2000;
		for (int i = 0; i < PairRelNum; i++)
		{
			ofs << SSGPairRelStrings[i] << " " << labelID << " 2\n";
			labelID++;
		}


		// group relationship labels
		//labelID = 3000;
		for (int i = 0; i < GroupRelNum; i++)
		{
			ofs << SSGGroupRelStrings[i] << " " << labelID << " 3\n";
			labelID++;
		}

		// group attribute labels
		//labelID = 4000;
		for (int i = 0; i < GroupAttriNum; i++)
		{
			ofs << SSGGroupAttrStings[i] << " " << labelID << " 4\n";
			labelID++;
		}

		outFile.close();

		std::cout << "SceneSemGraph: node labels saved.\n";
	}

	else
	{
		std::cout << "SceneSemGraph: fail to save node labels.\n";
	}
}

void SceneSemGraph::saveGMTNodeAttributeFile(const QString &filename)
{
	// save node label and attributes file
	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
	{
		ofs << "#\n";

		int nodeLabel = 0;

		// category labels
		for (auto it = m_catStringToLabelIDMap.begin(); it != m_catStringToLabelIDMap.end(); it++)
		{
			//Label definitions :
			//
			//= L =
			//	l  0      '  d   1                t  0.1           w  1         i 1  c  1
			//	^label	 ^# of attributes    ^1.threshold     ^1.weight   ^del  ^ins

			ofs << "=L=\n";
			ofs << "l "<< nodeLabel << " ' d 1 t 0 w 1 i 1 c 1" <<"\n";
			nodeLabel++;
		}

		// single attribute labels
		//nodeLabel = 1000;
		for (int i = 0; i < SingleAttriNum; i++)
		{			
			ofs << "=L=\n";
			ofs << "l " << nodeLabel << " ' d 1 t 0 w 0.8 i 1 c 1" << "\n";
			nodeLabel++;
		}


		// pair-wise relationship labels
		//nodeLabel = 2000;
		for (int i = 0; i < PairRelNum; i++)
		{			
			ofs << "=L=\n";
			ofs << "l " << nodeLabel << " ' d 1 t 0 w 0.6 i 1 c 1" << "\n";
			nodeLabel++;
		}


		// group relationship labels
		//nodeLabel = 3000;
		for (int i = 0; i < GroupRelNum; i++)
		{			
			ofs << "=L=\n";
			ofs << "l " << nodeLabel << " ' d 1 t 0 w 0.6 i 1 c 1" << "\n";
			nodeLabel++;
		}

		// group attribute labels
		//nodeLabel = 4000;
		for (int i = 0; i < GroupAttriNum; i++)
		{			
			ofs << "=L=\n";
			ofs << "l " << nodeLabel << " ' d 1 t 0 w 0.6 i 1 c 1" << "\n";
			nodeLabel++;
		}

		ofs << "=E=\n";

		outFile.close();

		std::cout << "SceneSemGraph: GMT attribute file saved.\n";
	}

	else
	{
		std::cout << "SceneSemGraph: fail to save GMT attribute file.\n";
	}
}

