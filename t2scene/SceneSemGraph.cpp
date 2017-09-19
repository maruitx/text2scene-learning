#include "SceneSemGraph.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"
#include "../common/geometry/OBB.h"
#include "../common/geometry/SuppPlane.h"
#include "../common/geometry/RelationGraph.h"
#include "../scene_lab/modelDatabase.h"
#include "../scene_lab/RelationExtractor.h"


SceneSemGraph::SceneSemGraph(CScene *s, ModelDatabase *db, RelationExtractor *relationExtractor)
	:m_scene(s), m_modelDB(db), m_relationExtractor(relationExtractor)
{
	m_relationExtractor->updateCurrScene(s);
	m_relationExtractor->updateCurrSceneSemGraph(this);

	m_sceneFormat = m_scene->getSceneFormat();
	m_relGraph = m_scene->getSceneGraph();
}

SceneSemGraph::SceneSemGraph(const QString &s)
	: m_fullFilename(s)
{
	loadGraph(m_fullFilename);
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
	m_modelNum = m_scene->getModelNum();

	for (int i = 0; i < m_modelNum;  i++)
	{
		QString modelNameStr = m_scene->getModelNameString(i);

		if (m_modelDB != NULL && m_modelDB->isModelInDB(modelNameStr))
		{
			DBMetaModel *metaModel = m_modelDB->getMetaModelByNameString(modelNameStr);
			CModel *m = m_scene->getModel(i);
			DBMetaModel *newMetaModelInstance = new DBMetaModel(metaModel);

			// add extra info to meta model
			//newMetaModelInstance->setTransMat(m_scene->getModelInitTransMat(i));
			newMetaModelInstance->setTransMat(m_scene->getModelInitTransMatWithSceneMetric(i));

			if (m_scene->modelHasOBB(i))
			{
				newMetaModelInstance->position = m->getModelPosOBB();
			}
			newMetaModelInstance->parentId = m_scene->getSuppParentId(i);
			newMetaModelInstance->onSuppPlaneUV = m_scene->getUVonBBTopPlaneForModel(i);
			newMetaModelInstance->positionToSuppPlaneDist = m_scene->getHightToBBTopPlaneForModel(i);

			if (m->m_bbTopPlane != NULL)
			{
				newMetaModelInstance->suppPlaneCorners = m_scene->getCurrModelBBTopPlaneCornersWithSceneMetric(i);
			}

			m_metaModelList.push_back(newMetaModelInstance);

			QString objectNodeName = metaModel->getProcessedCatName();
			addNode(QString(SSGNodeTypeStrings[SSGNodeType::Object]), objectNodeName);
		}
		else
		{
			//std::cout << "SceneSemGraph: cannot find model: " << modelNameStr.toStdString() << " in ShapeNetDB-"<<m_modelDB->getMetaFileType().toStdString()<<"\n";

			// add model to DB
			DBMetaModel *newMetaModel = new DBMetaModel();
			CModel *m = m_scene->getModel(i);
			//QString modelCatName = "unknown";
			QString modelCatName = m->getCatName();

			if (m->getNameStr().contains("room"))
			{
				modelCatName = "room";
			}

			newMetaModel->frontDir = m->getFrontDir() * m_scene->getSceneMetric();
			newMetaModel->upDir = m->getUpDir()* m_scene->getSceneMetric();
			//newMetaModel->position = m->getOBBInitPos();
			newMetaModel->position = m->getModelPosOBB();

			newMetaModel->setCatName(modelCatName);
			newMetaModel->setIdStr(modelNameStr);
			//newMetaModel->setTransMat(m_scene->getModelInitTransMat(i));
			newMetaModel->setTransMat(m_scene->getModelInitTransMatWithSceneMetric(i));


			newMetaModel->parentId = m->suppParentID;
			newMetaModel->onSuppPlaneUV = m_scene->getUVonBBTopPlaneForModel(i);
			newMetaModel->positionToSuppPlaneDist = m_scene->getHightToBBTopPlaneForModel(i);

			if (m->m_bbTopPlane!=NULL)
			{
				newMetaModel->suppPlaneCorners = m_scene->getCurrModelBBTopPlaneCornersWithSceneMetric(i);
			}

			//// add current instance to DB
			//newMetaModel->dbID = m_modelDB->dbMetaModels.size();
			//m_modelDB->dbMetaModels[modelNameStr] = newMetaModel;

			m_metaModelList.push_back(newMetaModel);			
			
			addNode(QString(SSGNodeTypeStrings[SSGNodeType::Object]), modelCatName);
		}
	}

	// extract low-level model attributes from ShapeNetSem annotation
	addModelDBAnnotation();

	// add relationships
	addRelationsFromRelationGraph();
	addSpatialSideRel();

	// add attributes
	addGroupAttributeFromAnnotation();
	splitSpecialGroupRelationToPairRelations();

	parseNodeNeighbors();

	std::cout << "SceneSemGraph: graph generated.\n";
}

void SceneSemGraph::addModelDBAnnotation()
{
	for (int i = 0; i < m_metaModelList.size(); i++)
	{
		DBMetaModel *currDBModel = m_metaModelList[i];

		for (int a = 0; a < currDBModel->m_attributes.size(); a++)
		{
			addNode(QString(SSGNodeTypeStrings[SSGNodeType::Attribute]), currDBModel->m_attributes[a]);
			addEdge(m_nodeNum-1, i);
		}
	}
}

void SceneSemGraph::addRelationsFromRelationGraph()
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
			addNode(SSGNodeTypeStrings[SSGNodeType::PairRel], PairRelStrings[0]);

			// in RG, an edge is (desk, monitor), in SSG will be (monitor, support), (support, desk)
			addEdge(relEdge->v2, m_nodeNum - 1);
			addEdge(m_nodeNum - 1, relEdge->v1);
			break;
			
		case RelationGraph::CT_HORIZON_SUPPORT:
			addNode(SSGNodeTypeStrings[SSGNodeType::PairRel], PairRelStrings[1]);

			// in RG, an edge is (desk, monitor), in SSG will be (monitor, support), (support, desk)
			addEdge(relEdge->v2, m_nodeNum - 1);
			addEdge(m_nodeNum - 1, relEdge->v1);
			break;

		case RelationGraph::CT_CONTAIN:

		default:
			break;
		}


		// insert edge and update node in/out edge list
	}
}

void SceneSemGraph::addSpatialSideRel()
{
	if (!m_scene->hasSupportHierarchyBuilt())
	{
		m_scene->buildSupportHierarchy();
	}

	for (int i=0; i < m_modelNum -1; i++)
	{
		for (int j=i+1; j<m_modelNum; j++)
		{
			addSpatialSideRelForModelPair(i, j);
		}
	}
}

void SceneSemGraph::addSpatialSideRelForModelPair(int refModelId, int testModelId)
{
	std::vector<QString> sideRels = m_relationExtractor->extractSpatialSideRelForModelPair(refModelId, testModelId);

	for (int r = 0; r < sideRels.size(); r++)
	{
		QString currRel = sideRels[r];
		addNode(SSGNodeTypeStrings[SSGNodeType::PairRel], currRel);

		// SSG edge direction will be (testModel, relation), (relation, refModel)
		addEdge(testModelId, m_nodeNum - 1);
		addEdge(m_nodeNum - 1, refModelId);
	}

	// inverse the ref and test, and compute the inverse relationship
	sideRels.clear();
	int newRefModelId = testModelId;
	int newTestNodeId = refModelId;

	sideRels = m_relationExtractor->extractSpatialSideRelForModelPair(newRefModelId, newTestNodeId);

	for (int r = 0; r < sideRels.size(); r++)
	{
		QString currRel = sideRels[r];
		addNode(SSGNodeTypeStrings[SSGNodeType::PairRel], currRel);

		// SSG edge direction will be (testModel, relation), (relation, refModel)
		addEdge(newTestNodeId, m_nodeNum - 1);
		addEdge(m_nodeNum - 1, newRefModelId);
	}
}

void SceneSemGraph::addGroupAttributeFromAnnotation()
{
	QString sceneANNPath = "C:/Ruim/Graphics/T2S_MPC/text2scene/t2s-evol/SceneDB/ANN/";

	QString sceneName = m_scene->getSceneName();

	QString annFileName = sceneANNPath + sceneName + ".snn";

	QFile inFile(annFileName);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return;
	}

	//std::vector<GroupAnnotation> annotations;

	while (!ifs.atEnd())
	{
		QString currLine = ifs.readLine(); // format line

		if (!currLine.contains("SceneNN") && !currLine.contains("Stanford") 
			&& !currLine.contains("model") && !currLine.contains("Model")
			&& !currLine.contains("new") && !currLine.contains("transform"))
		{
			// filter noise in currLine

			currLine = currLine.replace('.', ',');

			int pos = currLine.indexOf(", ");
			if (pos != -1)
			{
				currLine.replace(pos, 2, ',');
			}

			QStringList parts = currLine.split(",");

			for (int i = 0; i < parts.size(); i++)
			{
				bool isNumber = false;
				parts[i].toInt(&isNumber);

				if (isNumber)
				{
					int groupNum = i;
					for (int g = 0; g < groupNum; g++)
					{
						GroupAnnotation ann;
						ann.name = parts[g].remove(" ");
						if (parts[groupNum]=="")
						{
							ann.anchorModelId = -1;
						}
						else
						{
							ann.anchorModelId = parts[groupNum].toInt();
						}

						QString actString = parts[groupNum + 1];
						QStringList actStringList = actString.split(" ");

						int actNodeNum = 0;

						for (int t = 0; t < actStringList.size(); t++)
						{
							if (actStringList[t] !=" ")
							{
								ann.actModelIds.push_back(actStringList[t].toInt());
								actNodeNum++;
							}							
						}

						addNode(SSGNodeTypeStrings[SSGNodeType::GroupRelAnno], ann.name);
						addEdge(m_nodeNum - 1, ann.anchorModelId); // messy --> table

						for (int t = 0; t < actNodeNum; t++)
						{
							if (ann.actModelIds[t]!=0)
							{
								addEdge(ann.actModelIds[t], m_nodeNum - 1); // plates -->  messy
							}							
						}
					}
					break;
				}
			}

		}
	}

	inFile.close();

	qDebug() << QString("Annotation for scene %1 is loaded\n").arg(sceneName);
}

void SceneSemGraph::splitSpecialGroupRelationToPairRelations()
{
	for (int i=0; i < m_nodeNum; i++)
	{
		SemNode &currNode = m_nodes[i];
		if (currNode.nodeType == SSGNodeTypeStrings[SSGNodeType::GroupRelAnno])
		{
			if (currNode.nodeName == "surround")
			{
				if (!currNode.outEdgeNodeList.empty())
				{
					int anchorNodeId = currNode.outEdgeNodeList[0];

					// add pairwise node to each act obj
					for (int j=0; j < currNode.inEdgeNodeList.size(); j++)
					{
						int actNodeId = currNode.inEdgeNodeList[j];
						addNode(SSGNodeTypeStrings[SSGNodeType::PairRel], PairRelStrings[PairRelation::PairAround]);

						addEdge(actNodeId, m_nodeNum - 1);
						addEdge(m_nodeNum - 1, anchorNodeId);
					}
				}
			}

			if (currNode.nodeName == "aligned")
			{
				// find same obj in same cats and use the first one as anchor
				std::map<QString, std::vector<int>> objIds;
				for (int j=0; j < currNode.inEdgeNodeList.size(); j++)
				{
					int actNodeId = currNode.inEdgeNodeList[j];
					SemNode& actNode = m_nodes[actNodeId];

					objIds[actNode.nodeName].push_back(actNodeId);
				}

				for (auto it=objIds.begin(); it!=objIds.end(); it++)
				{
					int anchorNodeId = it->second[0];
					for (int k=1;k<it->second.size(); k++)
					{
						int actNodeId = it->second[k];

						addNode(SSGNodeTypeStrings[SSGNodeType::PairRel], PairRelStrings[PairRelation::PairAligned]);

						addEdge(actNodeId, m_nodeNum - 1);
						addEdge(m_nodeNum - 1, anchorNodeId);
					}
				}
			}
		}
	}
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

			if (!m_metaModelList[i]->suppPlaneCorners.empty())
			{
				ofs << "bbTopPlane " << m_metaModelList[i]->suppPlaneCorners[0][0] << " " << m_metaModelList[i]->suppPlaneCorners[0][1] << " " << m_metaModelList[i]->suppPlaneCorners[0][2] << " "
					<< m_metaModelList[i]->suppPlaneCorners[1][0] << " " << m_metaModelList[i]->suppPlaneCorners[1][1] << " " << m_metaModelList[i]->suppPlaneCorners[1][2] << " "
					<< m_metaModelList[i]->suppPlaneCorners[2][0] << " " << m_metaModelList[i]->suppPlaneCorners[2][1] << " " << m_metaModelList[i]->suppPlaneCorners[2][2] << " "
					<< m_metaModelList[i]->suppPlaneCorners[3][0] << " " << m_metaModelList[i]->suppPlaneCorners[3][1] << " " << m_metaModelList[i]->suppPlaneCorners[3][2] << "\n";
			}

			ofs << "parentId " << m_metaModelList[i]->parentId << "\n";
			ofs << "parentPlaneUVH " << m_metaModelList[i]->onSuppPlaneUV[0] << " " << m_metaModelList[i]->onSuppPlaneUV[1] << " " << m_metaModelList[i]->positionToSuppPlaneDist << "\n";
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
	int currModelID = -1;
	QString currLine;
	while (!ifs.atEnd() && !currLine.contains("nodeNum"))
	{
		currLine = ifs.readLine();

		//	load model info
		if (currLine.contains("modelCount "))
		{
			m_modelNum = StringToIntegerList(currLine.toStdString(), "modelCount ")[0];
		}

		if (currLine.contains("newModel "))
		{
			std::vector<std::string> parts = PartitionString(currLine.toStdString(), " ");
			int modelIndex = StringToInt(parts[1]);
			QString modelNameString = QString(parts[2].c_str());

			DBMetaModel *newMetaModel = new DBMetaModel(modelNameString);
			m_metaModelList.push_back(newMetaModel);

			currModelID++;
		}

		if (currLine.contains("transform "))
		{
			std::vector<float> transformVec = StringToFloatList(currLine.toStdString(), "transform ");  // transformation vector in stanford scene file is column-wise
			MathLib::Matrix4d transMat(transformVec);
			transMat = transMat.transpose();
			m_metaModelList[currModelID]->setTransMat(transMat);
		}
	}

	//	load nodes
	if (currLine.contains("nodeNum "))
	{
		int nodeNum = StringToIntegerList(currLine.toStdString(), "nodeNum ")[0];
		for (int i = 0; i < nodeNum; i++)
		{
			currLine = ifs.readLine();
			std::vector<std::string> parts = PartitionString(currLine.toStdString(), ",");

			// object node
			if (toQString(parts[1]) == "object")
			{
				if (parts.size() > 2)
				{
					addNode(toQString(parts[1]), toQString(parts[2]));
				}
				else
				{
					addNode(toQString(parts[1]), "noname");
				}
			}
			else
			{
				addNode(toQString(parts[1]), toQString(parts[2]));
			}
		}
	}

	currLine = ifs.readLine();

	// load edges
	if (currLine.contains("edgeNum "))
	{
		int edgeNum = StringToIntegerList(currLine.toStdString(), "edgeNum ")[0];
		for (int i = 0; i < edgeNum; i++)
		{
			currLine = ifs.readLine();
			std::vector<int> parts = StringToIntegerList(currLine.toStdString(), "", ",");
			addEdge(parts[1], parts[2]);
		}
	}

	parseNodeNeighbors();
	
	inFile.close();
}

QString SceneSemGraph::getCatName(int modelId)
{
	return m_metaModelList[modelId]->getProcessedCatName();
}


