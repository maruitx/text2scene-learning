#include "Scene.h"
#include "RelationGraph.h"
#include "SuppPlane.h"
//#include "../action/Skeleton.h"
#include "../utilities/utility.h"
//#include "../utilities/rng.h"
#include "../utilities/mathlib.h"
#include "CMesh.h"
//#include "ICP.h"

#include "../t2scene/SceneSemGraph.h"

#include <QFileDialog>
#include <QTextStream>

const double InchToMeterFactor = 0.0254;

CScene::CScene()
{
	m_modelNum = 0;
	m_uprightVec = MathLib::Vector3(0, 0, 1.0);
	m_metric = 1.0;
	m_floorHeight = 0;

	m_roomID = -1;

	m_relationGraph = NULL;
	m_hasRelGraph = false;
	m_hasSupportHierarchy = false;

	m_ssg = NULL;

	m_showSceneGaph = false;
	m_showModelOBB = false;
	m_showSuppPlane = false;
	m_showModelFrontDir = false;
	m_showSuppChildOBB = true;
}

CScene::~CScene()
{
	for (int i = 0; i < m_modelNum; i++)
	{
		delete m_modelList[i];
	}

	m_modelList.clear();
	m_modelNameIdMap.clear();

	if (m_relationGraph!=NULL)
	{
		delete m_relationGraph;
	}

	m_showSceneGaph = false;
	m_showModelOBB = false;
	m_showSuppPlane = false;
}

void CScene::loadStanfordScene(const QString &filename, int metaDataOnly, int obbOnly, int meshAndOBB)
{

	QFile inFile(filename);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	QFileInfo sceneFileInfo(inFile.fileName());
	m_sceneFileName = sceneFileInfo.baseName();   // scene_01.txt
	m_sceneFilePath = sceneFileInfo.absolutePath();  

	int cutPos = sceneFileInfo.absolutePath().lastIndexOf("/");  
	m_sceneDBPath = sceneFileInfo.absolutePath().left(cutPos);  

	if (metaDataOnly)
	{
		std::cout << "\nLoading scene meta data: " << m_sceneFileName.toStdString() << "...\n";
	}
	else
	{
		std::cout << "\nLoading scene: " << m_sceneFileName.toStdString() << "...\n";
	}
	

	QString databaseType;
	QString modelFileName;

	ifs >> databaseType;

	m_sceneFormat = databaseType;

	if (!obbOnly)
	{
		if (meshAndOBB)
		{
			std::cout << "\tloading objects with obb...\n";
			
		}
		else
		{
			std::cout << "\tloading objects without obb...\n";
		}
	}
	else
	{
		std::cout << "\tloading objects with obb only...\n";
	}

	if (m_sceneFormat == QString("StanfordSceneDatabase") || m_sceneFormat == QString("SceneNNConversionOutput"))
	{
		m_metric = InchToMeterFactor; // convert from 1 inch to 1 meter

		if (m_sceneFormat == QString("SceneNNConversionOutput"))
		{
			m_metric = 1.0;
		}

		int currModelID = -1;

		m_modelDBPath = m_sceneDBPath + "/models";  

		while (!ifs.atEnd())
		{
			QString currLine = ifs.readLine();
			if (currLine.contains("modelCount "))
			{
				m_modelNum = StringToIntegerList(currLine.toStdString(), "modelCount ")[0];
				m_modelList.resize(m_modelNum);
			}

			if (currLine.contains("newModel "))
			{
				std::vector<std::string> parts = PartitionString(currLine.toStdString(), " ");
				int modelIndex = StringToInt(parts[1]);

				CModel *newModel = new CModel();
				newModel->setSceneMetric(m_metric);
				newModel->setSceneUpRightVec(m_uprightVec);

				QString modelNameString = parts[2].c_str();
				newModel->loadModel(m_modelDBPath + "/" + modelNameString + ".obj", 1.0, metaDataOnly, obbOnly, meshAndOBB, m_sceneFormat);
				
				currModelID += 1;
				newModel->setID(currModelID);
				m_modelList[currModelID] = newModel;

				m_modelCatNameList.push_back(newModel->getCatName());

				if (modelNameString.contains("room"))
				{
					m_roomID = currModelID;
				}
			}

			if (m_sceneFormat == QString("StanfordSceneDatabase"))
			{
				if (currLine.contains("parentIndex "))
				{
					std::vector<std::string> parts = PartitionString(currLine.toStdString(), "parentIndex ");
					int parentId = StringToInt(parts[0]);
					m_modelList[currModelID]->suppParentID = parentId;
				}

				if (currLine.contains("children "))
				{
					std::vector<int> intList = StringToIntegerList(currLine.toStdString(), "children ");
					for (int i = 0; i < intList.size(); i++)
					{
						int childId = intList[i];
						m_modelList[currModelID]->suppChindrenList.push_back(childId);
					}
				}

				if (currLine.contains("parentContactPosition "))
				{
					std::vector<float> floatList = StringToFloatList(currLine.toStdString(), "parentContactPosition ");
					m_modelList[currModelID]->parentContactPos = MathLib::Vector3(floatList[0], floatList[1], floatList[2]);
				}

				if (currLine.contains("parentContactNormal "))
				{
					std::vector<int> intList =  StringToIntegerList(currLine.toStdString(), "parentContactNormal ");
					m_modelList[currModelID]->parentContactNormal = MathLib::Vector3(intList[0], intList[1], intList[2]);
				}
			}

			if (currLine.contains("transform "))
			{
				std::vector<float> transformVec = StringToFloatList(currLine.toStdString(), "transform ");
				MathLib::Matrix4d transMat(transformVec);

				//if (databaseType == "SceneNNConversionOutput")
				//{
				//	MathLib::Matrix4d rotMat = GetRotMat(MathLib::Vector3(1, 0, 0), -MathLib::ML_PI_2);
				//	transMat = rotMat*transMat;
				//}
				
				m_modelList[currModelID]->setInitTransMat(transMat);
				m_modelList[currModelID]->transformModel(transMat);

				std::cout << "\t\t" << currModelID + 1 << "/" << m_modelNum << " models loaded\r";
			}
		}
	}

	std::cout << "\n";

	m_relationGraph = new RelationGraph(this);

	QString graphFilename = m_sceneFilePath + "/" + m_sceneFileName + ".sg";

	if (m_relationGraph->readGraph(graphFilename) != -1)
	{
		m_hasRelGraph = true;

		buildSupportHierarchy();
		
		std::cout << "\tstructure graph loaded\n";
	}

	if (metaDataOnly)
	{
		std::cout << "Scene loaded\n";
		return;
	}

	computeAABB();
	buildModelDislayList();

	std::cout << "Scene loaded\n";
}

void CScene::loadTsinghuaScene(const QString &filename, int obbOnly /*= 0*/)
{
	QFile inFile(filename);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	QFileInfo sceneFileInfo(inFile.fileName());
	m_sceneFileName = sceneFileInfo.baseName();   // Bedroom_0.th
	m_sceneFilePath = sceneFileInfo.absolutePath();

	int cutPos = sceneFileInfo.absolutePath().lastIndexOf("/");
	m_sceneDBPath = sceneFileInfo.absolutePath().left(cutPos);

	m_metric = 1.0;
	m_uprightVec = MathLib::Vector3(0, 0, 1);

	int currModelID = 0;

	while (!ifs.atEnd())
		//for (int i = 0; i < 8; i++)
		{

			{
				QString modelName;
				ifs >> modelName;
				if (!modelName.isEmpty())
				{
					QString modelFullName = m_sceneDBPath + "/" + m_sceneFileName + "/" + modelName;

					CModel *newModel = new CModel();
					newModel->setSceneMetric(m_metric);
					newModel->setSceneUpRightVec(m_uprightVec);

					newModel->loadModel(modelFullName);
					newModel->setID(currModelID++);

					m_modelList.push_back(newModel);
				}
			}
		}

	computeAABB();
	buildModelDislayList();

	std::cout << "Scene loaded\n";

}

void CScene::buildRelationGraph()
{
	std::cout << "\tstart build relation graph for "<< m_sceneFileName.toStdString()<< "...";

	// build OBB if not exist
	for (int i = 0; i < m_modelList.size(); i++)
	{
		CModel *m = m_modelList[i];
		if( !m->hasOBB())
		{
			std::cout << "\t no OBB found, computing OBB first\n";
			return;
		}
	}

	QString graphFilename = m_sceneFilePath + "/" + m_sceneFileName + ".sg";

	m_relationGraph->buildGraph();
	m_relationGraph->saveGraph(graphFilename);

	buildSupportHierarchy();
	m_hasRelGraph = true;
	std::cout << " done.\n";
}

void CScene::buildModelDislayList(int showDiffColor /*= 1*/, int showFaceCluster /*= 0*/)
{
	foreach(CModel *m, m_modelList)
	{
		m->buildDisplayList(showDiffColor, showFaceCluster);
	}
}

void CScene::draw()
{
	if (m_showSceneGaph && m_hasRelGraph)
	{
		m_relationGraph->drawGraph();

		foreach(CModel *m, m_modelList)
		{
			m->draw(0, 1, m_showSuppPlane, m_showModelFrontDir, m_showSuppChildOBB);  // only show obb
		}
	}
	else
	{
		foreach(CModel *m, m_modelList)
		{
			m->draw(1, m_showModelOBB, m_showSuppPlane, m_showModelFrontDir, m_showSuppChildOBB);
		}
	}
}

void CScene::computeAABB()
{
	// init aabb
	m_AABB = m_modelList[0]->getAABB();

	// update scene bbox
	for (int i = 1; i < m_modelList.size(); i++)
	{
		if (!m_modelList[i]->isVisible())
		{
			continue;
		}

		CAABB curbox = m_modelList[i]->getAABB();
		m_AABB.Merge(curbox);
	}
}

bool CScene::isSegIntersectModel(MathLib::Vector3 &startPt, MathLib::Vector3 &endPt, int modelID, double radius)
{
	return m_modelList[modelID]->isSegIntersectMesh(startPt, endPt, radius);
}

void CScene::prepareForIntersect()
{
		for (int i = 0; i < m_modelNum; i++)
		{
			m_modelList[i]->prepareForIntersect();
		}
}

//void CScene::buildModelSuppPlane()
//{
//	for (int i = 0; i < m_modelNum; i++)
//	{
//		m_modelList[i]->buildSuppPlane();
//
//		QString catName = m_modelList[i]->getCatName();
//		if (catName.contains("room"))
//		{
//			//m_floor = m_modelList[i]->getSuppPlane(0);
//			if (m_modelList[i]->getModelFileName() == "room01")
//			{
//				m_floorHeight = 0.074;
//			}
//
//		}
//	}
//}

double CScene::getFloorHeight()
{
	//return m_floor->GetZ();
	return m_floorHeight;
}

MathLib::Matrix4d CScene::getModelInitTransMatWithSceneMetric(int modelID)
{
	MathLib::Matrix4d currTransMat = m_modelList[modelID]->getInitTransMat();
	double scaleFactor = m_metric / InchToMeterFactor;
	MathLib::Matrix4d scaledMat = currTransMat*scaleFactor;
	scaledMat.M[15] = 1.0;

	return scaledMat;
}

MathLib::Vector3 CScene::getOBBInitPostWithSceneMetric(int modelID)
{
	MathLib::Vector3 currInitPos = m_modelList[modelID]->getOBBInitPos();
	double scaleFactor = m_metric / InchToMeterFactor;
	MathLib::Vector3 scaledPos = currInitPos*scaleFactor;

	return scaledPos;
}

std::vector<int> CScene::getModelIdWithCatName(QString s, bool usingSynset)
{
	std::vector<int> idList;

	for (int i = 0; i < m_modelNum; i++)
	{
		QString synS;
		QString tempS = s;

		// find candidates in the scene
		if (s == "coffeetable")
		{
			synS = "sidetable";
		}

		if (s == "officechair" || s=="diningchair")
		{
			synS = "chair";
			tempS = "";
		}

		//if (s == "tv")
		//{
		//	synS = "monitor";
		//}

		if (s == "diningtable")
		{
			synS = "officedesk";
		}

		if (s == "officedesk")
		{
			synS = "diningtable";
		}

		if ((!s.isEmpty() && m_modelCatNameList[i] == s)) 
		{
			idList.push_back(i);
		}
		//else if (usingSynset && !synS.isEmpty() && m_modelCatNameList[i].contains(synS) && !m_modelCatNameList[i].contains(s))
		else if (usingSynset && !synS.isEmpty() && m_modelCatNameList[i].contains(synS) && !(m_modelCatNameList[i] == tempS))
		{
			idList.push_back(i);
		}
	}

	return idList;
}

//void CScene::insertModel(QString modelFileName)
//{
//	CModel *m = new CModel();
//	m->loadModel(m_sceneFilePath + "/" + modelFileName + ".obj", m_metric, 0, m_sceneFormat);
//	m->setID(m_modelNum);
//	m->setJobName(m_jobName);
//
//	m->buildSuppPlane();
//	m->buildDisplayList();
//	m->prepareForIntersect();
//
//	m_modelList.push_back(m);
//	m_modelCatNameList.push_back(modelFileName);
//	m_modelNum++;
//
//	updateSeneAABB(m->getAABB());
//	m_relationGraph->InsertNode(0);
//}
//
//void CScene::insertModel(CModel *m)
//{
//	m->setID(m_modelNum);
//	m->setJustInserted(true);
//
//	m->buildSuppPlane();
//	m->buildDisplayList();
//	m->prepareForIntersect();
//
//	m_modelList.push_back(m);
//	m_modelCatNameList.push_back(m->getCatName());
//	m_modelNum++;
//
//	updateSeneAABB(m->getAABB());
//	m_relationGraph->InsertNode(0);
//}

//TO DO: fix support relationship after insert model
void CScene::buildSupportHierarchy()
{
	if (m_sceneFormat != "StanfordSceneDatabase")
	{
		if (m_relationGraph->IsEmpty()) return;

		m_relationGraph->collectSupportParentForModels();
		std::vector<std::vector<int>> parentList = m_relationGraph->getSupportParentListForModels();

		// init all support to be -1
		foreach(CModel *m, m_modelList)
		{
			m->suppParentID = -1;
			m->supportLevel = -10;
			m->suppChindrenList.clear();
		}

		// collect parent-child relationship
		for (int i = 0; i < m_relationGraph->Size(); i++)
		{
			std::vector<int> parentIDs = parentList[i];

			if (!parentIDs.empty())
			{
				int parentId = parentIDs[0];  // use the first parent in the list
				m_modelList[i]->suppParentID = parentId;
				m_modelList[parentId]->suppChindrenList.push_back(i);
			}
		}
	}

	buildSupportLevels();

	m_hasSupportHierarchy = true;

	//// update grid on support plane
	//for (int i = 0; i < m_modelNum; i++)
	//{
	//	int childNum = m_modelList[i]->suppChindrenList.size();
	//	if (childNum > 0)
	//	{
	//		for (int j = 0; j < childNum; j++)
	//		{
	//			int childID = m_modelList[i]->suppChindrenList[j];
	//		
	//			m_modelList[childID]->parentSuppPlaneID = findPlaneSuppPlaneID(childID, i);
	//		}			
	//	}
	//}
}


void CScene::buildSupportLevels()
{
	// init support level
	double minHeight = 1e6;
	std::vector<double> modelBottomHeightList(m_modelNum);

	int roomId = getRoomID();
	for (int i = 0; i < m_modelNum; i++)
	{
		if (i == roomId)
		{
			m_modelList[i]->supportLevel = -1;
		}

		if (m_modelList[i]->suppParentID == roomId)
		{
			// find models that are support by the floor
			m_modelList[i]->supportLevel = 0;
			CModel *currModel = m_modelList[i];
			setSupportChildrenLevel(currModel);
		}
	}

	// if support level is still not set, it may be hang on the wall
	// set support level of these models to be 1 and recursively set children's level
	for (int i = 0; i < m_modelNum; i++)
	{
		if (m_modelList[i]->supportLevel == -10) // still unset
		{
			m_modelList[i]->supportLevel = 0;

			setSupportChildrenLevel(m_modelList[i]);
		}
	}
}

//
//int CScene::findPlaneSuppPlaneID(int childModelID, int parentModelID)
//{
//	double childModelBottomHeight = m_modelList[childModelID]->getOBBBottomHeight();
//
//	int suppPlaneNum = m_modelList[parentModelID]->getSuppPlaneNum();
//	double minDist = 1e6;
//	int closestPlaneID = -1;
//
//	for (int sid = 0; sid < suppPlaneNum; sid++)
//	{
//		double dist = abs(m_modelList[parentModelID]->getSuppPlane(sid)->GetZ() - childModelBottomHeight);
//
//		if (dist < minDist)
//		{
//			minDist = dist;
//			closestPlaneID = sid;
//		}
//	}
//
//	return closestPlaneID;
//}
//
void CScene::setSupportChildrenLevel(CModel *m)
{
	if (m->suppChindrenList.size() == 0)
	{
		return;
	}

	else
	{
		for (int i = 0; i < m->suppChindrenList.size(); i++)
		{
			m_modelList[m->suppChindrenList[i]]->supportLevel = m->supportLevel + 1;

			// set children's support level recursively
			setSupportChildrenLevel(m_modelList[m->suppChindrenList[i]]);
		}
	}
} 


void CScene::updateRelationGraph(int modelID)
{
	// scene graph should only be updated after model in inserted AND be transformed to new location
	m_relationGraph->updateGraph(modelID);
	//buildSupportHierarchy();  //update
}

void CScene::updateRelationGraph(int modelID, int suppModelID, int suppPlaneID)
{
	// only update graph linking with support model
	m_relationGraph->updateGraph(modelID, suppModelID);
//	updateSupportHierarchy(modelID, suppModelID, suppPlaneID);

}

void CScene::loadSSG()
{
	if (m_ssg!=NULL)
	{
		delete m_ssg;
	}

	QString ssgFileName = m_sceneFilePath + "/" + m_sceneFileName + ".ssg";
	m_ssg = new SceneSemGraph(ssgFileName);
}

int CScene::getRoomID()
{
	// if already know room id
	if (m_roomID != -1)
	{
		return m_roomID;
	}

	// set room ID
	for (int i = 0; i < m_modelList.size(); i++)
	{
		if (m_modelCatNameList[i].contains("room"))
		{
			m_roomID = i;
			break;
		}
	}

	return m_roomID;
}

MathLib::Vector3 CScene::getRoomFront()
{
	if (m_roomID != -1)
	{
		return m_modelList[m_roomID]->getFrontDir();;
	}

	else
	{
		int id = getRoomID();
		return m_modelList[id]->getFrontDir();
	}
}

std::vector<double> CScene::getUVonBBTopPlaneForModel(int modelId)
{
	CModel *currModel = m_modelList[modelId];
	int parentId = currModel->suppParentID;

	if (parentId == -1)
	{
		return std::vector<double>(2, 0.5);
	}

	CModel *parentModel = m_modelList[parentId];

	std::vector<double> uv;

	//if (parentModel->hasSuppPlane())
	//{
	//	SuppPlane *suppPlane = parentModel->getSuppPlane(0);	
	//	MathLib::Vector3 pos = currModel->getModelPosOBB();  // in real world pos
	//	//MathLib::Vector3 pos = currModel->parentContactPos;
	//	
	//	uv = suppPlane->GetUVForPoint(pos);

	//	return uv;
	//}
	//else
	//{
	//	return std::vector<double>(2, 0.5);
	//}

	MathLib::Vector3 pos = currModel->getModelPosOBB();  // in real world pos
	uv = parentModel->m_bbTopPlane->GetUVForPoint(pos);

	return uv;

}

double CScene::getHightToBBTopPlaneForModel(int modelId)
{
	CModel *currModel = m_modelList[modelId];
	int parentId = currModel->suppParentID;

	if (parentId == -1)
	{
		return -1;
	}

	CModel *parentModel = m_modelList[parentId];
	double d;

	SuppPlane *suppPlane = parentModel->m_bbTopPlane;
	MathLib::Vector3 pos = currModel->getModelPosOBB();

	d = suppPlane->GetPointToPlaneDist(pos); // in real world unit

	double scaleFactor = m_metric / InchToMeterFactor;
	d = d*scaleFactor;

	return d;

	//if (parentModel->hasSuppPlane())
	//{
	//	SuppPlane *suppPlane = parentModel->getSuppPlane(0);
	//	MathLib::Vector3 pos = currModel->getModelPosOBB();

	//	d = suppPlane->GetPointToPlaneDist(pos); // in real world unit

	//	double scaleFactor = m_metric / InchToMeterFactor;
	//	d = d*scaleFactor;

	//	return d;
	//}
	//else
	//{
	//	return 0;
	//}
}

std::vector<MathLib::Vector3> CScene::getParentSuppPlaneCorners(int modelId)
{
	CModel *currModel = m_modelList[modelId];
	int parentId = currModel->suppParentID;

	if (parentId == -1)
	{
		return std::vector<MathLib::Vector3>(4, MathLib::Vector3(0, 0, 0));
	}

	CModel *parentModel = m_modelList[parentId];
	if (parentModel->hasSuppPlane())
	{
		SuppPlane *suppPlane = parentModel->getSuppPlane(0);

		std::vector<MathLib::Vector3> corners = suppPlane->GetCorners();
		return corners;
	}
	else
	{
		return std::vector<MathLib::Vector3>(4, MathLib::Vector3(0, 0, 0));
	}
}


std::vector<MathLib::Vector3> CScene::getCurrModelSuppPlaneCornersWithSceneMetric(int modelId)
{
	CModel *currModel = m_modelList[modelId];

	if (currModel->hasSuppPlane())
	{
		SuppPlane *suppPlane = currModel->getSuppPlane(0);
		std::vector<MathLib::Vector3> corners = suppPlane->GetCorners();

		double scaleFactor = m_metric / InchToMeterFactor;
		for (int i=0;  i <corners.size(); i++)
		{
			corners[i] = corners[i] * scaleFactor;
		}

		return corners;
	}
	else
	{
		return std::vector<MathLib::Vector3>(4, MathLib::Vector3(0, 0, 0));
	}
}

std::vector<MathLib::Vector3> CScene::getCurrModelBBTopPlaneCornersWithSceneMetric(int modelId)
{
	std::vector<MathLib::Vector3> corners = m_modelList[modelId]->m_bbTopPlane->GetCorners();

	double scaleFactor = m_metric / InchToMeterFactor;
	for (int i = 0; i < corners.size(); i++)
	{
		corners[i] = corners[i] * scaleFactor;
	}

	return corners;
}

void CScene::computeModelBBAlignMat()
{
	for (int i = 0; i < m_modelNum; i++)
	{
		m_modelList[i]->computeBBAlignMat();
	}

	saveModelBBAlignMat();
}

bool CScene::loadModelBBAlignMat()
{
	QString filename = m_sceneFilePath + "/" + m_sceneFileName + ".alignMat";

	QFile inFile(filename);
	QTextStream ifs(&inFile);

	if (inFile.open(QIODevice::ReadOnly))
	{
		for (int i = 0; i < m_modelNum; i++)
		{
			QString currLine = ifs.readLine();

			if (currLine.contains("nan"))
			{
				qDebug() << "invalid matrix";
			}

			std::vector<float> transformVec = StringToFloatList(currLine.toStdString(), "");
			MathLib::Matrix4d transMat(transformVec);

			m_modelList[i]->m_WorldBBToUnitBoxMat = transMat;
		}

		return true;
	}
	else
	{
		return false;
	}
}

void CScene::saveModelBBAlignMat()
{
	QString filename = m_sceneFilePath + "/" + m_sceneFileName + ".alignMat";

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		for (int i = 0; i < m_modelNum; i++)
		{
			ofs << GetTransformationString(m_modelList[i]->m_WorldBBToUnitBoxMat) << "\n";
		}

		outFile.close();
	}
}

void CScene::saveRelPositions()
{
	QString filename = m_sceneFilePath + "/" + m_sceneFileName + ".relPos";

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		for (int i = 0; i < m_relPositions.size(); i++)
		{
			RelativePos *relPos = m_relPositions[i];
			ofs << relPos->m_instanceNameHash << "," << relPos->m_instanceIdHash <<"\n";
			ofs << relPos->pos.x << " " << relPos->pos.y << " " << relPos->pos.z << " " << relPos->theta << "," 
				<< GetTransformationString(relPos->anchorAlignMat) << ","
				<< GetTransformationString(relPos->actAlignMat) << "\n";
		}

		outFile.close();
	}
}



//void CScene::computeTransformation(const std::vector<MathLib::Vector3> &source, const std::vector<MathLib::Vector3> &target, Eigen::Matrix4d &mat)
//{
//	Eigen::Matrix3Xd source_vertices;
//	Eigen::Matrix3Xd target_vertices;
//
//	source_vertices.resize(3, source.size());
//	target_vertices.resize(3, target.size());
//
//	for (int i = 0; i < source.size(); i++){
//		source_vertices(0, i) = (double)source[i].x;
//		source_vertices(1, i) = (double)source[i].y;
//		source_vertices(2, i) = (double)source[i].z;
//	}
//
//	for (int i = 0; i < target.size(); i++){
//		target_vertices(0, i) = (double)target[i].x;
//		target_vertices(1, i) = (double)target[i].y;
//		target_vertices(2, i) = (double)target[i].z;
//	}
//
//	Eigen::Affine3d affine = RigidMotionEstimator::point_to_point(source_vertices, target_vertices);
//	mat = affine.matrix().cast<double>();;
//}
//
//Eigen::Matrix4d  CScene::computeModelTransMat(int sourceModelId, QString tarFileName, QString sceneDBType)
//{
//	// load original model
//	CModel *tempModel = new CModel();
//	tempModel->loadModel(tarFileName, 1.0, 1, sceneDBType);
//
//	int sampleNum = 20;
//	CMesh *currMesh = m_modelList[sourceModelId]->getMesh();
//	CMesh *tempMesh = tempModel->getMesh();
//
//	sampleNum = std::min(sampleNum, currMesh->getVertsNum());
//
//	std::vector<int> randList = GetRandIntList(sampleNum, currMesh->getVertsNum());
//	std::vector<MathLib::Vector3> sourceVerts = tempMesh->getVerts(randList);
//	std::vector<MathLib::Vector3> targetVerts = currMesh->getVerts(randList);
//
//	int vid;
//	// find a vertex different from v0 to compute the scale
//	for (vid = 1; vid < sampleNum; vid++)
//	{
//		if (randList[vid] != randList[0])
//		{
//			std::abs((sourceVerts[0] - sourceVerts[vid]).magnitude()) > 0.01;
//			break;
//		}
//	}
//
//	double scaleFactor = (targetVerts[0] - targetVerts[vid]).magnitude() / (sourceVerts[0] - sourceVerts[vid]).magnitude();
//	
//	
//	
//	for (int i = 0; i < sourceVerts.size(); i++)
//	{
//		sourceVerts[i] = sourceVerts[i] * scaleFactor;
//	}
//
//	Eigen::Matrix4d eigenTransMat;
//	computeTransformation(sourceVerts, targetVerts, eigenTransMat);
//
//	for (int i = 0; i < 3; i++)
//	{
//		for (int j = 0; j < 3; j++)
//		{
//			eigenTransMat(i, j) *= scaleFactor;
//		}
//	}
//
//	delete tempModel;
//
//	return eigenTransMat;
//}

//void CScene::updateSupportHierarchy(int modelID, int suppModelID, int suppPlaneID)
//{
//	// recover grid value in previous support plane
//	int preParentID = m_modelList[modelID]->suppParentID;
//	if (preParentID!=-1)
//	{
//		int preSuppPlaneID = m_modelList[modelID]->parentSuppPlaneID;
//		if (preSuppPlaneID != -1)
//		{
//			SuppPlane *p = m_modelList[preParentID]->getSuppPlane(preSuppPlaneID);
//			p->recoverGrid(m_modelList[modelID]->suppGridPos);		
//		}
//	}
//
//	 //update support parent
//	if (!m_modelList[suppModelID]->isSupportChild(modelID))
//	{
//		m_modelList[modelID]->suppParentID = modelID;
//
//		m_modelList[suppModelID]->suppChindrenList.push_back(modelID);
//		m_modelList[modelID]->supportLevel = m_modelList[suppModelID]->supportLevel + 1;
//	}
//
//	// set new grid value using new obb location
//	if (m_modelList[suppModelID]->hasSuppPlane())
//	{
//		SuppPlane *p = m_modelList[suppModelID]->getSuppPlane(suppPlaneID);
//		m_modelList[modelID]->parentSuppPlaneID = suppPlaneID;
//
//		COBB obb = m_modelList[modelID]->getOBB();
//		std::vector<MathLib::Vector3> bottomCorners;
//		bottomCorners.push_back(obb.V(2));
//		bottomCorners.push_back(obb.V(6));
//		bottomCorners.push_back(obb.V(7));
//		bottomCorners.push_back(obb.V(3));
//
//		p->updateGrid(bottomCorners, 1);
//	}
//}




