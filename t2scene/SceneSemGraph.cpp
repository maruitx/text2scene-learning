#include "SceneSemGraph.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"
#include "../common/geometry/OBB.h"
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
	for (int i = 0; i < m_metaModelList.size(); i++)
	{
		// since we create a new instance of MetaModel
		delete m_metaModelList[i];
	}
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
			DBMetaModel *metaModel = m_modelDB->getMetaModelByNameString(modelNameStr);

			DBMetaModel *newMetaModelInstance = new DBMetaModel(metaModel);
			newMetaModelInstance->setTransMat(m_scene->getModelInitTransMat(i));

			if (m_scene->modelHasOBB(i))
			{
				newMetaModelInstance->position = m_scene->getOBBInitPos(i);
			}

			m_metaModelList.push_back(newMetaModelInstance);

			QString objectNodeName = metaModel->getProcessedCatName();
			addNode(QString(SSGNodeType[0]), objectNodeName);
		}
		else
		{
			std::cout << "SceneSemGraph: cannot find model: " << modelNameStr.toStdString() << " in ShapeNetDB-"<<m_modelDB->getMetaFileType().toStdString()<<"\n";

			// add model to DB
			DBMetaModel *newMetaModel = new DBMetaModel();
			CModel *m = m_scene->getModel(i);
			newMetaModel->frontDir = m->getFrontDir() * m_scene->getSceneMetric();
			newMetaModel->upDir = m->getUpDir()* m_scene->getSceneMetric();
			newMetaModel->position = m->getOBBInitPos();
			newMetaModel->setCatName("unknown");
			newMetaModel->setIdStr(modelNameStr);
			newMetaModel->setTransMat(m_scene->getModelInitTransMat(i));

			newMetaModel->dbID = m_modelDB->dbMetaModels.size();
			//m_modelDB->dbMetaModels[modelNameStr] = newMetaModel;

			m_metaModelList.push_back(newMetaModel);			

			QString objectNodeName = "unknown";
			addNode(QString(SSGNodeType[0]), objectNodeName);
		}
	}

	// extract low-level model attributes from ShapeNetSem annotation
	buildFromModelDBAnnotation();

	// add relationships
	extractRelationsFromRelationGraph();
	extractSpatialSideRel();

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
		case RelationGraph::CT_VERT_SUPPORT:
			addNode(SSGNodeType[2], SSGPairRelStrings[0]);
			// in RG, an edge is (desk, monitor), in SSG will be (monitor, support), (support, desk)
			//addEdge(relEdge->v1, m_nodeNum-1);   
			//addEdge(m_nodeNum-1, relEdge->v2);
			addEdge(relEdge->v2, m_nodeNum - 1);
			addEdge(m_nodeNum - 1, relEdge->v1);
			
		case RelationGraph::CT_CONTAIN:

		default:
			break;
		}


		// insert edge and update node in/out edge list
	}
}


void SceneSemGraph::extractSpatialSideRel()
{
	if (!m_scene->hasSupportHierarchyBuilt())
	{
		m_scene->buildSupportHierarchy();
	}

	// collect obj ids with support level 0
	int roomId = m_scene->getRoomID();

	std::vector<int> groundModelIds;
	std::vector<int> wallModelIds;

	for (int i = 0; i < m_scene->getModelNum(); i++)
	{
		CModel *m = m_scene->getModel(i);

		if (m->supportLevel == 0 && m->parentContactNormal == MathLib::Vector3(0, 0, 1))
		{
			groundModelIds.push_back(i);
		}

		if (m->supportLevel == 0 && m->parentContactNormal.dot(MathLib::Vector3(0, 0, 1)) == 0)
		{
			wallModelIds.push_back(i);
		}
	}

	if (groundModelIds.empty())
	{
		qDebug() << "No models supported by the room to extract side relation\n";
		return;
	}

	// between ground objects
	for (int i = 0; i < groundModelIds.size()-1; i++)
	{
		int refModelId = groundModelIds[i];

		for (int j = i + 1; j < groundModelIds.size(); j++)
		{
			int testModelId = groundModelIds[j];
			extractSpatialSideRelForModelPair(refModelId, testModelId);		
		}
	}
}

void SceneSemGraph::extractSpatialSideRelForModelPair(int refModelId, int testModelId)
{
	std::vector<QString> sideRels = computeSpatialSideRelForModelPair(refModelId, testModelId);

	for (int r = 0; r < sideRels.size(); r++)
	{
		QString currRel = sideRels[r];
		addNode(SSGNodeType[2], currRel);

		// SSG edge direction will be (testModel, relation), (relation, refModel)
		addEdge(testModelId, m_nodeNum - 1);
		addEdge(m_nodeNum - 1, refModelId);
	}

	// inverse the ref and test, and compute the inverse relationship
	sideRels.clear();
	int newRefModelId = testModelId;
	int newTestNodeId = refModelId;
	sideRels = computeSpatialSideRelForModelPair(newRefModelId, newTestNodeId);

	for (int r = 0; r < sideRels.size(); r++)
	{
		QString currRel = sideRels[r];
		addNode(SSGNodeType[2], currRel);

		// SSG edge direction will be (testModel, relation), (relation, refModel)
		addEdge(newTestNodeId, m_nodeNum - 1);
		addEdge(m_nodeNum - 1, newRefModelId);
	}
}


std::vector<QString> SceneSemGraph::computeSpatialSideRelForModelPair(int refModelId, int testModelId)
{
	std::vector<QString> relationStrings;

	double distTh = 1.0;

	double sceneMetric = m_scene->getSceneMetric();
	MathLib::Vector3 sceneUpDir = m_scene->getUprightVec();


	CModel *refModel = m_scene->getModel(refModelId);
	CModel *testModel = m_scene->getModel(testModelId);

	COBB refOBB = refModel->getOBB();
	COBB testOBB = testModel->getOBB();

	double hausdorffDist = refOBB.HausdorffDist(testOBB);
	double connStrength = refOBB.ConnStrength_HD(testOBB);

	MathLib::Vector3 contactDir;
	if (refOBB.IsIntersect(testOBB) || connStrength < 2.0)
	{
		relationStrings.push_back(SSGPairRelStrings[9]);  // near
	}

	MathLib::Vector3 refFront = refModel->getHorizonFrontDir();
	MathLib::Vector3 refUp = refModel->getVertUpDir();

	MathLib::Vector3 refPos = refModel->getOBBInitPos();
	MathLib::Vector3 testPos = testModel->getOBBInitPos();

	MathLib::Vector3 fromRefToTestVec = testPos - refPos;
	fromRefToTestVec.normalize();

	// front or back side is not view dependent
	double frontDirDot = fromRefToTestVec.dot(refFront);


	double sideSectionVal = MathLib::Cos(30);
	if (frontDirDot > sideSectionVal)
	{
		QString refName = m_metaModelList[refModelId]->getProcessedCatName();
		if (refName == "tv" || refName == "monitor")  //  object requires to face to
		{
			MathLib::Vector3 testFront = testModel->getFrontDir();
			if (testFront.dot(refFront) < 0)  // test and ref should be opposite
			{
				relationStrings.push_back(SSGPairRelStrings[7]); // front
			}
		}
		else
		{
			relationStrings.push_back(SSGPairRelStrings[7]); // front
		}
	}
	else if (frontDirDot < -sideSectionVal && frontDirDot > -1)
	{
		relationStrings.push_back(SSGPairRelStrings[8]); // back
	}

	// left or right may be view dependent
	MathLib::Vector3 refRight = refFront.cross(refUp);
	double rightDirDot = fromRefToTestVec.dot(refRight);

	MathLib::Vector3 roomFront = m_scene->getRoomFront();

	// if ref front is same to th room front (e.g. view inside 0, -1, 0)
	bool useViewCentric = false;
	if (refFront.dot(roomFront) > sideSectionVal  && useViewCentric)
	{
		// use view-centric
		if (rightDirDot >= sideSectionVal) // right of obj
		{
			relationStrings.push_back(SSGPairRelStrings[5]); // left in view
		}
		else if (rightDirDot <= -sideSectionVal) // left of obj
		{
			relationStrings.push_back(SSGPairRelStrings[6]); // right in view
		}
	}
	else
	{
		if (rightDirDot >= sideSectionVal) // right of obj
		{
			relationStrings.push_back(SSGPairRelStrings[6]); // right
		}
		else if (rightDirDot <= -sideSectionVal)  // left of obj
		{
			relationStrings.push_back(SSGPairRelStrings[5]); // left
		}
	}

	return relationStrings;
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

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		ofs << m_sceneFormat << "\n";

		// save scene meta data
		int modelNum = m_metaModelList.size();
		ofs << "modelCount " << modelNum << "\n";

		for (int i = 0; i < modelNum; i++)
		{
			ofs << "newModel "<< i << " " << m_metaModelList[i]->getIdStr() << "\n";
			ofs << "transform " << GetTransformationString(m_metaModelList[i]->getTransMat()) << "\n";
			ofs << "position " << m_metaModelList[i]->position[0] << " " << m_metaModelList[i]->position[1] << " " << m_metaModelList[i]->position[2] << "\n";
			ofs << "frontDir " << m_metaModelList[i]->frontDir[0] << " " << m_metaModelList[i]->frontDir[1] << " " << m_metaModelList[i]->frontDir[2] << "\n";
			ofs << "upDir " << m_metaModelList[i]->upDir[0] << " " << m_metaModelList[i]->upDir[1] << " " << m_metaModelList[i]->upDir[2] << "\n";
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

			DBMetaModel *newMetaModel = new DBMetaModel(modelNameString);

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


