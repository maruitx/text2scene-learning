#include "RelationModelManager.h"
#include "RelationExtractor.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"
#include "../t2scene/SceneSemGraph.h"

#include "engine.h"
extern Engine *matlabEngine;

RelationModelManager::RelationModelManager(RelationExtractor *mExtractor)
	:m_relationExtractor(mExtractor)
{
	m_adjustFrontObjs.push_back("desk");
	m_adjustFrontObjs.push_back("bookcase");
	m_adjustFrontObjs.push_back("cabinet");
	m_adjustFrontObjs.push_back("dresser");
	m_adjustFrontObjs.push_back("monitor");
	m_adjustFrontObjs.push_back("tv");
	m_adjustFrontObjs.push_back("bed");
}

RelationModelManager::~RelationModelManager()
{
	for (auto it = m_relativePostions.begin(); it != m_relativePostions.end(); it++)
	{
		delete it->second;
	}

	for (auto it = m_pairwiseRelModels.begin(); it != m_pairwiseRelModels.end(); it++)
	{
		delete it->second;
	}

	for (auto it = m_groupRelModels.begin(); it!=m_groupRelModels.end(); it++)
	{
		delete it->second;
	}

	for (auto it=m_supportRelations.begin(); it!= m_supportRelations.end(); it++)
	{
		delete it->second;
	}
}

void RelationModelManager::updateCurrScene(CScene *s)
{
	m_currScene = s;
	m_relationExtractor->updateCurrScene(m_currScene);
}

void RelationModelManager::collectRelativePosInCurrScene()
{
	int modelNum = m_currScene->getModelNum();

	if (!m_currScene->loadModelBBAlignMat())
	{
		qDebug() << "RelationModelManager: compute model align mat first for "<< m_currScene->getSceneName();
		return;
	}

	for (int i = 0; i < modelNum; i++)
	{
		CModel *anchorModel = m_currScene->getModel(i);
		for (int j = 0; j < modelNum; j++)
		{
			if (i==j) continue;
			
			CModel *actModel = m_currScene->getModel(j);
			QString conditionName = m_relationExtractor->getRelationConditionType(anchorModel, actModel); 

			if (conditionName == "none") continue;

			RelativePos *relPos = new RelativePos();
			relPos->m_conditionName = conditionName;
			relPos->m_sceneName = m_currScene->getSceneName();

			m_relationExtractor->extractRelativePosForModelPair(anchorModel, actModel, relPos);

			// check whether observed relPos is valid
			if (relPos->isValid)
			{
				m_currScene->m_relPositions.push_back(relPos);
			}
		}
	}

	m_currScene->saveRelPositions();
}

void RelationModelManager::loadRelativePosFromCurrScene()
{
	QString filename = m_currScene->getFilePath() + "/" + m_currScene->getSceneName() + ".relPos";

	QFile inFile(filename);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly))
	{
		qDebug() << "RelationModelManager: cannot load relative position for scene " << m_currScene->getSceneName();
		return;
	}

	while (!ifs.atEnd())
	{
		QString currLine = ifs.readLine();
		std::vector<std::string> parts = PartitionString(currLine.toStdString(), ",");
		
		RelativePos *relPos = new RelativePos();
		relPos->m_instanceNameHash = toQString(parts[0]);
		std::vector<std::string> subParts = PartitionString(parts[0], "_");
		relPos->m_anchorObjName = toQString(subParts[0]);
		relPos->m_actObjName = toQString(subParts[1]);
		relPos->m_conditionName = toQString(subParts[2]);

		subParts = PartitionString(parts[1], "_");
		relPos->m_sceneName = toQString(subParts[0]);
		relPos->m_anchorObjId = StringToInt(subParts[1]);
		relPos->m_actObjId = StringToInt(subParts[2]);
		relPos->m_instanceIdHash = QString("%1_%2_%3").arg(relPos->m_sceneName).arg(relPos->m_anchorObjId).arg(relPos->m_actObjId);

		currLine = ifs.readLine();
		parts = PartitionString(currLine.toStdString(), ",");

		std::vector<float> pos = StringToFloatList(parts[0], "");
		relPos->pos = MathLib::Vector3(pos[0], pos[1], pos[2]);
		relPos->theta = pos[3];

		std::vector<float> transformVec = StringToFloatList(parts[1], "");
		relPos->anchorAlignMat = MathLib::Matrix4d(transformVec);

		transformVec = StringToFloatList(parts[2], "");
		relPos->actAlignMat = MathLib::Matrix4d(transformVec);

		m_relativePostions[relPos->m_instanceIdHash] = relPos;
	}

	inFile.close();

	qDebug() << "RelationModelManager: loaded relative position for scene " << m_currScene->getSceneName();
}

void RelationModelManager::buildRelativeRelationModels()
{
	// collect instance ids for relative models
	for(auto it = m_relativePostions.begin(); it!=m_relativePostions.end(); it++)
	{
		RelativePos *relPos = it->second;
		QString relationKey = relPos->m_instanceNameHash;

		if (!m_relativeModels.count(relationKey))
		{
			PairwiseRelationModel *relativeModel = new PairwiseRelationModel(relPos->m_anchorObjName, relPos->m_actObjName, relPos->m_conditionName, "general");
			
			relativeModel->m_instances.push_back(relPos);
			m_relativeModels[relationKey] = relativeModel;
		}
		else
			m_relativeModels[relationKey]->m_instances.push_back(relPos); 

		m_relativeModels[relationKey]->m_numInstance = m_relativeModels[relationKey]->m_instances.size();
	}

	// 1. Open MATLAB engine
	matlabEngine = engOpen(NULL);

	// fit GMM model
	int id = 0;
	int totalNum = m_relativeModels.size();
	for (auto iter = m_relativeModels.begin(); iter != m_relativeModels.end(); iter++)
	{
		PairwiseRelationModel *relModel = iter->second;
		relModel->fitGMM(15);

		std::cout << "Relative model fitted " << QString("%1/%2\r").arg(++id).arg(totalNum).toStdString();
	}

	// 3. Close MATLAB engine
	engClose(matlabEngine);

	qDebug() << "Relative relations extracted";
}

void RelationModelManager::buildPairwiseRelationModels()
{
	// 1. Open MATLAB engine
	matlabEngine = engOpen(NULL);

	// fit GMM model
	int id = 0;
	int totalNum = m_pairwiseRelModels.size();
	m_pairRelModelKeys.resize(totalNum);

	for (auto iter = m_pairwiseRelModels.begin(); iter != m_pairwiseRelModels.end(); iter++)
	{
		PairwiseRelationModel *relModel = iter->second;
		relModel->fitGMM(15);
		relModel->m_modelId = id;
		m_pairRelModelKeys[id] = relModel->m_relationKey;

		std::cout << "Pairwise model fitted " << QString("%1/%2\r").arg(++id).arg(totalNum).toStdString();
	}

	// 3. Close MATLAB engine
	engClose(matlabEngine);

	qDebug() << "Pairwise relations extracted";
}

void RelationModelManager::collectPairwiseInstanceFromCurrScene()
{
	SceneSemGraph *currSSG = m_currScene->m_ssg;
	QString sceneName = m_currScene->getSceneName();

	for (int i = 0; i < currSSG->m_nodeNum; i++)
	{
		SemNode &sgNode = currSSG->m_nodes[i];

		if (sgNode.nodeType.contains(SSGNodeTypeStrings[SSGNodeType::PairRel]))
		{
			int anchorNodeId = sgNode.anchorNodeList[0];
			int actNodeId = sgNode.activeNodeList[0];

			QString anchorObjName = currSSG->m_nodes[anchorNodeId].nodeName;
			QString actObjName = currSSG->m_nodes[actNodeId].nodeName;
			QString conditionName;

			QString instanceIdHash = QString("%1_%2_%3").arg(sceneName).arg(anchorNodeId).arg(actNodeId);

			// find condition name in observed relPos
			if (m_relativePostions.count(instanceIdHash))
			{
				RelativePos *relPos = m_relativePostions[instanceIdHash];
				conditionName = relPos->m_conditionName;
				QString relationKey = anchorObjName + "_" + actObjName + "_" + conditionName + "_" + sgNode.nodeName;

				if (!m_pairwiseRelModels.count(relationKey))
				{
					PairwiseRelationModel *newPairwiseModel = new PairwiseRelationModel(anchorObjName, actObjName, conditionName, sgNode.nodeName);
					newPairwiseModel->m_instances.push_back(relPos);
					m_pairwiseRelModels[relationKey] = newPairwiseModel;
				}
				else
				{
					m_pairwiseRelModels[relationKey]->m_instances.push_back(relPos);
				}

				m_pairwiseRelModels[relationKey]->m_numInstance++;
			}
		}
	}
}

void RelationModelManager::computeSimForPairwiseModels(std::map<QString, PairwiseRelationModel*> &pairModels, const std::vector<QString> &pairModelKeys, const std::vector<CScene*> &sceneList, bool isInGroup, const QString &filePath)
{
	qDebug() << "Computing similarity between pairwise models...";
	int sceneNum = sceneList.size();
	std::map<QString, int> sceneNameToIdMap;
	for (int i=0; i <sceneNum; i++)
	{
		QString sceneName = sceneList[i]->getSceneName();
		sceneNameToIdMap[sceneName] = i;
	}

	int relNum = PairRelNum;
	if (isInGroup)
	{
		relNum = 1;
	}

	for (int r=0;  r < relNum; r++)
	{
		QString relType = PairRelStrings[r];
		if (relNum == 1)
		{
			relType = "general";
		}

		for (int c=0; c< ConditionNum; c++)
		{
			QString conditionType = ConditionName[c];

			// collect pair models with same relation and condition type
			std::vector<int> pairModelIds;
			std::map<int, int> globalToLocalIdMap;

			int id = 0;
			for (auto iter = pairModels.begin(); iter != pairModels.end(); iter++)
			{
				PairwiseRelationModel *relModel = iter->second;

				if (relModel->m_relationName == relType && relModel->m_conditionName == conditionType)
				{
					relModel->computeObjNodeFeatures(sceneList, sceneNameToIdMap);
					pairModelIds.push_back(relModel->m_modelId);

					globalToLocalIdMap[relModel->m_modelId] = id;
					id++;
				}
			}

			std::vector<double> maxHeightToFloor(2,0), maxModelHeight(2,0), maxVolume(2, 0);  // max of avg of different models
			int featureDim = 3;  // used feature dimension
			std::vector<std::vector<double>> maxFeatures(2, std::vector<double>(featureDim, 0));

			// collect max value for normalization
			for (int m = 0; m < 2; m++)
			{
				for (int i = 0; i < pairModelIds.size(); i++)
				{
					QString modelKeyA = pairModelKeys[pairModelIds[i]];
					PairwiseRelationModel *relModelA = pairModels[modelKeyA];

					for (int d=0; d < featureDim; d++)
					{
						if (relModelA->m_avgObjFeatures[m][d] > maxFeatures[m][d]) maxFeatures[m][d] = relModelA->m_avgObjFeatures[m][d];
					}

					for (int j = 0; j < pairModelIds.size(); j++)
					{
						if (i == j) continue;
						QString modelKeyB = pairModelKeys[pairModelIds[j]];
						PairwiseRelationModel *relModelB = pairModels[modelKeyB];

						for (int d = 0; d < featureDim; d++)
						{
							if (relModelB->m_avgObjFeatures[m][d] > maxFeatures[m][d]) maxFeatures[m][d] = relModelB->m_avgObjFeatures[m][d];
						}
					}
				}
			}

			// compute similarity
			Eigen::MatrixXd simMat(pairModelIds.size(), pairModelIds.size());
			double featureWeights[] = { 1,1,1,0.5,0.5,0.5 };
			double catWeight = 0.3;			

			for (int i = 0; i < pairModelIds.size(); i++)
			{
				QString modelKeyA = pairModelKeys[pairModelIds[i]];
				PairwiseRelationModel *relModelA = pairModels[modelKeyA];
				relModelA->m_maxObjFeatures = maxFeatures;

				for (int j = 0; j < pairModelIds.size(); j++)
				{
					if (i == j) continue;

					QString modelKeyB = pairModelKeys[pairModelIds[j]];
					PairwiseRelationModel *relModelB = pairModels[modelKeyB];

					if (isAnchorFrontDirConsistent(relModelA->m_anchorObjName, relModelB->m_anchorObjName))
					{
						std::vector<double> simVal(2, 0);
						std::vector<double> geoSim(2, 0);
						std::vector<double> catSim(2, 0);

						if (relModelA->m_anchorObjName == relModelB->m_anchorObjName) catSim[0] = 1;
						if (relModelA->m_actObjName == relModelB->m_actObjName) catSim[1] = 1;

						for (int m = 0; m < 2; m++)
						{
							for (int d = 0; d < featureDim; d++)
							{
								geoSim[m] += featureWeights[d] * exp(-pow((relModelA->m_avgObjFeatures[m][d] - relModelB->m_avgObjFeatures[m][d]) / (maxFeatures[m][d] + 1e-3), 2));
							}

							geoSim[m] /= featureDim;
							simVal[m] = catWeight*catSim[m] + (1 - catWeight)*geoSim[m];
						}
						simMat(i, j) = simVal[0] * simVal[1];
					}
					else
					{
						simMat(i, j) = 0;
					}
				}
			}

			// sort and save similar relation model ids
			for (int i=0; i < pairModelIds.size(); i++)
			{
				std::vector<std::pair<double, int>> simPairs;
				for (int j=0; j < pairModelIds.size(); j++ )
				{
					if (i == j) continue;

					simPairs.push_back(std::make_pair(simMat(i, j), pairModelIds[j]));
				}
				std::sort(simPairs.begin(), simPairs.end()); // ascending order
				std::reverse(simPairs.begin(), simPairs.end()); // descending order
				
				QString modelKey = pairModelKeys[pairModelIds[i]];
				PairwiseRelationModel *relModel = pairModels[modelKey];
				int topSimNum = 20;
				for (int k = 0; k < simPairs.size()/2; k++)
				{
					if (k < topSimNum)
					{
						relModel->m_simModelIds.push_back(simPairs[k].second);
					}
				}
			}

			//// save sim Mat
			//if (!pairModelIds.empty() && !filePath.isEmpty())
			//{
			//	QString filename = filePath + QString("/%1_%2.simMat").arg(relType).arg(conditionType);
			//	std::ofstream ofs;
			//	ofs.open(filename.toStdString(), std::ios::out | std::ios::trunc);

			//	if (ofs.is_open())
			//	{
			//		for (int i = 0; i < pairModelIds.size(); i++)
			//		{
			//			QString modelKey = pairModelKeys[pairModelIds[i]];
			//			PairwiseRelationModel *relModel = pairModels[modelKey];

			//			ofs << relModel->m_relationKey.toStdString() << ",R_" << relModel->m_modelId << "," << i << "\n";
			//			int simModelNum = relModel->m_simModelIds.size();
			//			ofs << simModelNum << ",";

			//			if (simModelNum > 0)
			//			{
			//				for (int k = 0; k < simModelNum - 1; k++)
			//				{
			//					ofs << relModel->m_simModelIds[k] << " ";
			//				}
			//				ofs << relModel->m_simModelIds[simModelNum - 1] << "\n";
			//				for (int k = 0; k < simModelNum - 1; k++)
			//				{
			//					ofs << simMat(i, globalToLocalIdMap[relModel->m_simModelIds[k]]) << ",";
			//				}
			//				ofs << simMat(i, globalToLocalIdMap[relModel->m_simModelIds[simModelNum - 1]]) << "\n";
			//			}
			//			else
			//			{
			//				ofs << "-1\n";
			//			}
			//		}
			//		ofs.close();
			//	}
			//}
		}
	}
}

void RelationModelManager::computeSimForPairModelInGroup(const std::vector<CScene*> &sceneList)
{
	for (auto giter = m_groupRelModels.begin(); giter != m_groupRelModels.end(); giter++)
	{
		GroupRelationModel *groupModel = giter->second;
		computeSimForPairwiseModels(groupModel->m_pairwiseModels, groupModel->m_pairModelKeys, sceneList, true);
	}
}

bool RelationModelManager::isAnchorFrontDirConsistent(const QString &currAnchorName, const QString &dbAnchorName)
{
	if (std::find(m_adjustFrontObjs.begin(), m_adjustFrontObjs.end(), dbAnchorName) != m_adjustFrontObjs.end()
		&& std::find(m_adjustFrontObjs.begin(), m_adjustFrontObjs.end(), currAnchorName) == m_adjustFrontObjs.end())
	{
		return false;
	}

	if (std::find(m_adjustFrontObjs.begin(), m_adjustFrontObjs.end(), dbAnchorName) == m_adjustFrontObjs.end()
		&& std::find(m_adjustFrontObjs.begin(), m_adjustFrontObjs.end(), currAnchorName) != m_adjustFrontObjs.end())
	{
		return false;
	}

	return true;
}

void RelationModelManager::collectCoOccInCurrentScene()
{
	SceneSemGraph *currSSG = m_currScene->m_ssg;

	int modeNum = m_currScene->getModelNum();
	for (int i = 0; i < modeNum; i++)
	{
		CModel *currModel = m_currScene->getModel(i);

		if (!currModel->suppChindrenList.empty())
		{
			// collect unique support children list
			std::vector<int> uniqueIds;
			std::vector<QString> childCats;
			for (int j = 0; j < currModel->suppChindrenList.size(); j++)
			{
				int childModelId = currModel->suppChindrenList[j];
				CModel *childModel = m_currScene->getModel(childModelId);
				QString currChildCat = childModel->getCatName();

				if (std::find(childCats.begin(), childCats.end(), currChildCat) == childCats.end())
				{
					childCats.push_back(currChildCat);
					uniqueIds.push_back(childModelId);
				}
			}

			for (int j=0; j < uniqueIds.size(); j++)
			{
				CModel *firstModel = m_currScene->getModel(uniqueIds[j]);
				int firstParentModelId = firstModel->suppParentID;
				QString firstModelCatName = firstModel->getCatName();

				if (firstParentModelId == -1) continue;
				if (firstModelCatName == "room") continue;

				CModel *parentModel = m_currScene->getModel(firstParentModelId);
				QString parentCatName = parentModel->getCatName();

				for (int k = 0; k < uniqueIds.size(); k++)
				{
					if (k == j) continue;

					CModel *secondModel = m_currScene->getModel(uniqueIds[k]);
					int secondParentModelId = secondModel->suppParentID;
					QString secondModelCatName = secondModel->getCatName();

					if (firstParentModelId == secondParentModelId)
					{
						QString conditionType = "sibling";
						QString coOccKey = QString("%1_%2_%3_%4").arg(firstModelCatName).arg(secondModelCatName).arg(conditionType).arg(parentCatName);

						if (m_coOccModelsOnSameParent.count(coOccKey))
						{
							m_coOccModelsOnSameParent[coOccKey]->m_coOccNum++;
						}
						else
						{
							CoOccurrenceModel *newModel = new CoOccurrenceModel(firstModelCatName, secondModelCatName, conditionType, parentCatName);
							newModel->m_coOccNum++;
							m_coOccModelsOnSameParent[coOccKey] = newModel;
						}
					}
				}
			}
		}
	}


	//for (int i = 0; i < modeNum; i++)
	//{
	//	CModel *firstModel = m_currScene->getModel(i);
	//	int firstParentModelId = firstModel->suppParentID;
	//	QString firstModelCatName = firstModel->getCatName();

	//	if (firstParentModelId == -1) continue;
	//	if (firstModelCatName == "room") continue;

	//	CModel *parentModel = m_currScene->getModel(firstParentModelId);
	//	QString parentCatName = parentModel->getCatName();

	//	for (int j = 0; j < m_currScene->getModelNum(); j++)
	//	{
	//		if (i == j) continue;

	//		CModel *secondModel = m_currScene->getModel(j);
	//		int secondParentModelId = secondModel->suppParentID;
	//		QString secondModelCatName = secondModel->getCatName();

	//		if (firstParentModelId == secondParentModelId)
	//		{
	//			QString conditionType = "sibling";
	//			QString coOccKey = QString("%1_%2_%3_%4").arg(firstModelCatName).arg(secondModelCatName).arg(conditionType).arg(parentCatName);

	//			if (m_coOccModelsOnSameParent.count(coOccKey))
	//			{
	//				m_coOccModelsOnSameParent[coOccKey]->m_coOccNum++;
	//			}
	//			else
	//			{
	//				CoOccurrenceModel *newModel = new CoOccurrenceModel(firstModelCatName, secondModelCatName, conditionType, parentCatName);
	//				newModel->m_coOccNum++;
	//				m_coOccModelsOnSameParent[coOccKey] = newModel;
	//			}
	//		}
	//	}
	//}

	//std::vector<int> nodeCountedIndicator(modeNum, 0);
	//for (int i = 0; i < modeNum; i++)
	//{
	//	CModel *firstModel = m_currScene->getModel(i);
	//	int firstParentModelId = firstModel->suppParentID;
	//	QString firstModelCatName = firstModel->getCatName();
	//	CModel *parentModel = m_currScene->getModel(firstParentModelId);
	//	QString parentCatName = parentModel->getCatName();

	//	if (firstModelCatName == "room") continue;

	//	// add instance for first obj
	//	for (auto iter = m_coOccModelsOnSameParent.begin(); iter != m_coOccModelsOnSameParent.end(); iter++)
	//	{
	//		QString coOccKey = iter->first;
	//		if (coOccKey.left(firstModelCatName.length()) == firstModelCatName && coOccKey.right(parentCatName.length()) == parentCatName)
	//		{
	//			CoOccurrenceModel *currModel = iter->second;
	//			currModel->m_firstObjNum++;
	//		}
	//	}

	//	for (int j = 0; j < m_currScene->getModelNum(); j++)
	//	{
	//		if (i == j) continue;

	//		CModel *secondModel = m_currScene->getModel(j);
	//		int secondParentModelId = secondModel->suppParentID;
	//		QString secondModelCatName = secondModel->getCatName();

	//		if (nodeCountedIndicator[j] == 0)
	//		{
	//			// add instance for second obj
	//			for (auto iter = m_coOccModelsOnSameParent.begin(); iter != m_coOccModelsOnSameParent.end(); iter++)
	//			{
	//				QString coOccKey = iter->first;
	//				QString matchKey = secondModelCatName + "_sibling_" + parentCatName;
	//				if (coOccKey.right(matchKey.length()) == matchKey)
	//				{
	//					CoOccurrenceModel *currModel = iter->second;
	//					currModel->m_secondObjNum++;
	//				}
	//			}
	//			nodeCountedIndicator[j] = 1;
	//		}
	//	}
	//}
}

void RelationModelManager::addOccToCoOccFromSupportRelation()
{
	for (auto spIter = m_supportRelations.begin(); spIter!=m_supportRelations.end(); spIter++)
	{
		SupportRelation *currSuppRelation = spIter->second;
		QString childObjName = currSuppRelation->m_childName;
		QString parentObjName = currSuppRelation->m_parentName;

		for (auto iter = m_coOccModelsOnSameParent.begin(); iter!= m_coOccModelsOnSameParent.end(); iter++)
		{
			QString coOccKey = iter->first;
			if (coOccKey.left(childObjName.length()) == childObjName && coOccKey.right(parentObjName.length()) == parentObjName)
			{
				CoOccurrenceModel *currModel = iter->second;
				currModel->m_firstObjNum = currSuppRelation->m_childInstanceNum;
			}

			QString matchKey = childObjName + "_sibling_" + parentObjName;
			if (coOccKey.right(matchKey.length()) == matchKey)
			{
				CoOccurrenceModel *currModel = iter->second;
				currModel->m_secondObjNum = currSuppRelation->m_childInstanceNum;
			}
		}
	}
}

void RelationModelManager::computeOccToCoccOnSameParent()
{
	addOccToCoOccFromSupportRelation();

	for (auto iter = m_coOccModelsOnSameParent.begin(); iter != m_coOccModelsOnSameParent.end(); iter++)
	{
		CoOccurrenceModel *currModel = iter->second;
		currModel->computCoOccProb();
	}
}

void RelationModelManager::collectSupportRelationInCurrentScene()
{
	SceneSemGraph *currSSG = m_currScene->m_ssg;

	// count num of total object observation
	for (int i = 0; i < currSSG->m_nodeNum; i++)
	{
		SemNode &sgNode = currSSG->m_nodes[i];
		if (sgNode.nodeType == "object")
		{
			if (m_suppProbs.count(sgNode.nodeName))
			{
				m_suppProbs[sgNode.nodeName].totalNum++;
			}
			else
			{
				SupportProb newSupportProb;
				newSupportProb.totalNum++;
				m_suppProbs[sgNode.nodeName] = newSupportProb;
			}
		}
	}

	for (int i = 0; i < currSSG->m_nodeNum; i++)
	{
		SemNode &sgNode = currSSG->m_nodes[i];
		if (sgNode.nodeType == "object")
		{
			bool isSupportParent = false;

			// vertsupport and horizon support			
			for (int rt = 0; rt < 2; rt++)
			{
				QString supportType = PairRelStrings[rt];

				std::vector<int> actNodeList;
				for (int r = 0; r < sgNode.inEdgeNodeList.size(); r++)
				{
					int relNodeId = sgNode.inEdgeNodeList[r];
					SemNode &relNode = currSSG->m_nodes[relNodeId];

					if (relNode.nodeName == supportType)
					{
						actNodeList.push_back(relNode.activeNodeList[0]);
					}
				}

				std::vector<int> actNodeCountedIndicator(actNodeList.size(), 0); // whether the node has been counted
				for (int t = 0; t < actNodeList.size(); t++)
				{
					int actNodeId = actNodeList[t];
					SemNode &actNode = currSSG->m_nodes[actNodeId];
					QString actObjName = actNode.nodeName;

					// count obj num in current active list
					int currActObjNum = 0;
					for (int ai = 0; ai < actNodeList.size(); ai++)
					{
						int testNodeId = actNodeList[ai];
						SemNode &testNode = currSSG->m_nodes[testNodeId];
						if (actNodeCountedIndicator[ai] == 0 && actObjName == testNode.nodeName)
						{
							actNodeCountedIndicator[ai] = 1;
							currActObjNum++;
						}
					}
				
					if (currActObjNum)
					{
						SemNode &actNode = currSSG->m_nodes[actNodeList[t]];
						QString suppRelKey = sgNode.nodeName + "_" + actNode.nodeName + "_" + supportType;

						// add instance for joint
						if (!m_supportRelations.count(suppRelKey))
						{
							SupportRelation *newSuppRelation = new SupportRelation(sgNode.nodeName, actNode.nodeName, supportType);
							newSuppRelation->m_jointInstanceNum++;
							m_supportRelations[suppRelKey] = newSuppRelation;
						}
						else
						{
							m_supportRelations[suppRelKey]->m_jointInstanceNum++;
						}

						// add instance for support child
						for (auto iter = m_supportRelations.begin(); iter != m_supportRelations.end(); iter++)
						{
							SupportRelation *suppRel = iter->second;
							if (suppRel->m_childName == actNode.nodeName && suppRel->m_supportType == supportType)
							{
								suppRel->m_childInstanceNum++;
							}
						}
					}

					// count num of object being support child while the parent is not "room"
					if (sgNode.nodeName !="room")
					{
						m_suppProbs[actNode.nodeName].beChildNum++;
					}
				}

				if (!actNodeList.empty())
				{
					isSupportParent = true;

					// add instance for support parent
					for (auto iter = m_supportRelations.begin(); iter != m_supportRelations.end(); iter++)
					{
						SupportRelation *suppRel = iter->second;
						if (suppRel->m_parentName == sgNode.nodeName && suppRel->m_supportType == supportType)
						{
							suppRel->m_parentInstanceNum++;
						}
					}
				}
			}

			if (isSupportParent)
			{
				// count num of object being support parent
				m_suppProbs[sgNode.nodeName].beParentNum++;
			}
		}
	}
}

void RelationModelManager::buildSupportRelationModels()
{
	for (auto iter = m_supportRelations.begin(); iter != m_supportRelations.end(); iter++)
	{
		SupportRelation *suppRel = iter->second;
		suppRel->m_childProbGivenParent = suppRel->m_jointInstanceNum / (double)suppRel->m_parentInstanceNum;
		suppRel->m_parentProbGivenChild = suppRel->m_jointInstanceNum / (double)suppRel->m_childInstanceNum;
	}

	for (auto iter = m_suppProbs.begin(); iter!= m_suppProbs.end(); iter++)
	{
		SupportProb &suppParentProb = iter->second;
		suppParentProb.beParentProb = suppParentProb.beParentNum / (double) suppParentProb.totalNum;
		suppParentProb.beChildProb = suppParentProb.beChildNum / (double) suppParentProb.totalNum;
	}

	qDebug() << "Support relations extracted";
}

void RelationModelManager::buildGroupRelationModels()
{

	// 1. Open MATLAB engine
	matlabEngine = engOpen(NULL);

	// fit GMM model
	int id = 0;
	int totalNum = m_groupRelModels.size();
	for (auto iter = m_groupRelModels.begin(); iter != m_groupRelModels.end(); iter++)
	{
		GroupRelationModel *groupModel = iter->second;
		groupModel->computeOccurrence();
		groupModel->fitGMMs();

		std::cout << "Group model fitted " << QString("%1/%2\r").arg(++id).arg(totalNum).toStdString();

		addOccToCoOccForGroupModel(groupModel);
	}

	for (auto iter = m_coOccModelsInSameGroup.begin(); iter!=m_coOccModelsInSameGroup.end(); iter++)
	{
		CoOccurrenceModel *currModel = iter->second;
		currModel->computCoOccProb();
	}

	// 3. Close MATLAB engine
	engClose(matlabEngine);

	qDebug() << "Group relations extracted";
}

void RelationModelManager::collectGroupInstanceFromCurrScene()
{
	SceneSemGraph *currSSG = m_currScene->m_ssg;
	QString sceneName = m_currScene->getSceneName();

	for (int i=0; i< currSSG->m_nodeNum; i++)
	{
		SemNode &sgNode = currSSG->m_nodes[i];

		if (sgNode.nodeType.contains("group"))
		{
			int anchoNodeId = sgNode.anchorNodeList[0];

			QString anchorObjName = currSSG->m_nodes[anchoNodeId].nodeName;
			QString groupKey = sgNode.nodeName + "_" + anchorObjName;

			if (!m_groupRelModels.count(groupKey))
			{
				GroupRelationModel *newGroupModel = new GroupRelationModel(anchorObjName, sgNode.nodeName);
				collectRelPosForGroupModel(newGroupModel, sceneName, anchoNodeId, sgNode.activeNodeList);
				collectOccurrForGroupModel(newGroupModel, sgNode.activeNodeList);
				collectCoOccForGroupModel(newGroupModel, sgNode.activeNodeList);

				m_groupRelModels[groupKey] = newGroupModel;
			}
			else
			{
				collectRelPosForGroupModel(m_groupRelModels[groupKey], sceneName, anchoNodeId, sgNode.activeNodeList);
				collectOccurrForGroupModel(m_groupRelModels[groupKey], sgNode.activeNodeList);
				collectCoOccForGroupModel(m_groupRelModels[groupKey], sgNode.activeNodeList);
			}

			m_groupRelModels[groupKey]->m_numInstance++;
		}
	}
}

void RelationModelManager::collectOccurrForGroupModel(GroupRelationModel *groupModel, const std::vector<int> &actNodeList)
{
	SceneSemGraph *currSSG = m_currScene->m_ssg;

	std::vector<int> actNodeCountedIndicator(actNodeList.size(), 0); // whether the node has been counted

	for (int t=0; t < actNodeList.size(); t++)
	{
		int actNodeId = actNodeList[t];
		SemNode &actNode = currSSG->m_nodes[actNodeId];
		QString actObjName = actNode.nodeName;

		int currActObjNum = 0;
		// count obj num in current annotation
		for (int i=0; i < actNodeList.size(); i++)
		{
			int testNodeId = actNodeList[i];
			SemNode &testNode = currSSG->m_nodes[testNodeId];
			if (actNodeCountedIndicator[i] == 0 && actObjName == testNode.nodeName)
			{
				actNodeCountedIndicator[i] = 1;
				currActObjNum++;
			}
		}

		if (currActObjNum)
		{
			QString occurKey = QString("%1_%2").arg(actObjName).arg(currActObjNum); // e.g., two monitors

			if (!groupModel->m_occurModels[occurKey])
			{
				OccurrenceModel *occurModel = new OccurrenceModel(actObjName, currActObjNum);
				occurModel->m_numInstance++;
				groupModel->m_occurModels[occurKey] = occurModel;
			}
			else
			{
				groupModel->m_occurModels[occurKey]->m_numInstance++;
			}
		}	
	}
}

void RelationModelManager::collectCoOccForGroupModel(GroupRelationModel *groupModel, const std::vector<int> &actNodeList)
{
	SceneSemGraph *currSSG = m_currScene->m_ssg;

	std::vector<int> actNodeCountedIndicator(actNodeList.size(), 0); // whether the node has been counted
	std::vector<int> uniqueActNodeIds;

	for (int t = 0; t < actNodeList.size(); t++)
	{
		int actNodeId = actNodeList[t];
		SemNode &actNode = currSSG->m_nodes[actNodeId];
		QString actObjName = actNode.nodeName;

		int currActObjNum = 0;
		// count obj num in current annotation
		for (int i = 0; i < actNodeList.size(); i++)
		{
			int testNodeId = actNodeList[i];
			SemNode &testNode = currSSG->m_nodes[testNodeId];
			if (actNodeCountedIndicator[i] == 0 && actObjName == testNode.nodeName)
			{
				actNodeCountedIndicator[i] = 1;
				currActObjNum++;
			}
		}

		if (currActObjNum)
		{
			uniqueActNodeIds.push_back(actNodeId);
		}
	}

	for (int i = 0; i < uniqueActNodeIds.size(); i++)
	{
		SemNode &firstActNode = currSSG->m_nodes[uniqueActNodeIds[i]];
		for (int j = 0; j < uniqueActNodeIds.size(); j++)
		{
			if (i == j) continue;
			SemNode &secondActNode = currSSG->m_nodes[uniqueActNodeIds[j]];
			QString coOccKey = QString("%1_%2_%3_%4").arg(firstActNode.nodeName).arg(secondActNode.nodeName).arg(groupModel->m_relationName).arg(groupModel->m_anchorObjName);

			if (m_coOccModelsInSameGroup.count(coOccKey))
			{
				m_coOccModelsInSameGroup[coOccKey]->m_coOccNum++;
			}
			else
			{
				CoOccurrenceModel *newModel = new CoOccurrenceModel(firstActNode.nodeName, secondActNode.nodeName, groupModel->m_relationName, groupModel->m_anchorObjName);
				newModel->m_coOccNum++;
				m_coOccModelsInSameGroup[coOccKey] = newModel;
			}
		}
	}

	//std::vector<int> nodeCountedIndicator(uniqueActNodeIds.size(), 0);
	//for (int i = 0; i < uniqueActNodeIds.size(); i++)
	//{
	//	SemNode &firstActNode = currSSG->m_nodes[uniqueActNodeIds[i]];

	//	// add instance for first obj
	//	for (auto iter = m_coOccModelsInSameGroup.begin(); iter != m_coOccModelsInSameGroup.end(); iter++)
	//	{
	//		QString coOccKey = iter->first;
	//		QString matchKey = groupModel->m_relationName + "_" + groupModel->m_anchorObjName;
	//		if (coOccKey.left(firstActNode.nodeName.length()) == firstActNode.nodeName && coOccKey.right(matchKey.length()) == matchKey)
	//		{
	//			CoOccurrenceModel *currModel = iter->second;
	//			currModel->m_firstObjNum++;
	//		}
	//	}

	//	for (int j = 0; j < uniqueActNodeIds.size(); j++)
	//	{
	//		if (i == j) continue;
	//		SemNode &secondActNode = currSSG->m_nodes[uniqueActNodeIds[j]];
	//		if (nodeCountedIndicator[j] == 0)
	//		{
	//			// add instance for second obj
	//			for (auto iter = m_coOccModelsInSameGroup.begin(); iter != m_coOccModelsInSameGroup.end(); iter++)
	//			{
	//				QString coOccKey = iter->first;
	//				QString matchKey = secondActNode.nodeName + "_" + groupModel->m_relationName + "_" + groupModel->m_anchorObjName;
	//				if (coOccKey.right(matchKey.length()) == matchKey)
	//				{
	//					CoOccurrenceModel *currModel = iter->second;
	//					currModel->m_secondObjNum++;
	//				}
	//			}
	//			nodeCountedIndicator[j] = 1;
	//		}
	//	}
	//}
}

void RelationModelManager::collectRelPosForGroupModel(GroupRelationModel *groupModel, const QString &sceneName, int anchorNodeId, const std::vector<int> &actNodeList)
{
	for (int t = 0; t < actNodeList.size(); t++)
	{
		QString instanceIdHash = QString("%1_%2_%3").arg(sceneName).arg(anchorNodeId).arg(actNodeList[t]);

		if (m_relativePostions.count(instanceIdHash))
		{
			RelativePos* relPos = m_relativePostions[instanceIdHash];
			QString relationKey = relPos->m_anchorObjName + "_" + relPos->m_actObjName + "_" +relPos->m_conditionName + "_general";

			if (!groupModel->m_pairwiseModels.count(relationKey))
			{
				PairwiseRelationModel *relativeModel = new PairwiseRelationModel(relPos->m_anchorObjName, relPos->m_actObjName, relPos->m_conditionName, "general");

				relativeModel->m_instances.push_back(relPos);
				groupModel->m_pairwiseModels[relationKey] = relativeModel;
			}

			else
				groupModel->m_pairwiseModels[relationKey]->m_instances.push_back(relPos);

			groupModel->m_pairwiseModels[relationKey]->m_numInstance = groupModel->m_pairwiseModels[relationKey]->m_instances.size();
		}
	}
}

void RelationModelManager::addOccToCoOccForGroupModel(GroupRelationModel *groupModel)
{
	for (auto oiter = groupModel->m_occurModels.begin(); oiter!= groupModel->m_occurModels.end(); oiter++)
	{
		OccurrenceModel *currOccModel = oiter->second;
		QString currModelName = toQString(PartitionString(currOccModel->m_objName.toStdString(), "_")[0]);

		for (auto iter = m_coOccModelsInSameGroup.begin(); iter != m_coOccModelsInSameGroup.end(); iter++)
		{
			QString coOccKey = iter->first;
			QString matchKey = groupModel->m_relationName + "_" + groupModel->m_anchorObjName;
			if (coOccKey.left(currModelName.length()) == currModelName && coOccKey.right(matchKey.length()) == matchKey)
			{
				CoOccurrenceModel *currModel = iter->second;
				currModel->m_firstObjNum += currOccModel->m_numInstance;
			}

			matchKey = currModelName + "_" + groupModel->m_relationName + "_" + groupModel->m_anchorObjName;
			if (coOccKey.right(matchKey.length()) == matchKey)
			{
				CoOccurrenceModel *currModel = iter->second;
				currModel->m_secondObjNum += currOccModel->m_numInstance;
			}
		}
	}
}

void RelationModelManager::saveRelativeRelationModels(const QString &filePath, const QString &dbType)
{
	QString filename = filePath + QString("/Relative_%1.model").arg(dbType);

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		// save relative relations
		for (auto iter = m_relativeModels.begin(); iter != m_relativeModels.end(); iter++)
		{
			PairwiseRelationModel *relModel = iter->second;
			relModel->output(ofs);
		}

		outFile.close();
		qDebug() << "model saved";
	}
}

void RelationModelManager::savePairwiseRelationModels(const QString &filePath, const QString &dbType)
{
	QString filename = filePath + QString("/Pairwise_%1.model").arg(dbType);

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		// save relative relations
		for (auto iter = m_pairwiseRelModels.begin(); iter != m_pairwiseRelModels.end(); iter++)
		{
			PairwiseRelationModel *relModel = iter->second;
			relModel->output(ofs);
		}

		outFile.close();
		qDebug() << "model saved";
	}
}

void RelationModelManager::saveGroupRelationModels(const QString &filePath, const QString &dbType)
{
	QString filename = filePath +  QString("/Group_%1.model").arg(dbType);

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		for (auto giter = m_groupRelModels.begin(); giter != m_groupRelModels.end(); giter++)
		{
			GroupRelationModel *groupModel = giter->second;
			groupModel->output(ofs);
		}

		outFile.close();
		qDebug() << "model saved";
	}
}

void RelationModelManager::saveSupportRelationModels(const QString &filePath, const QString &dbType)
{
	QString filename = filePath + QString("/SupportRelation_%1.model").arg(dbType);

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		for (auto iter = m_supportRelations.begin(); iter != m_supportRelations.end(); iter++)
		{
			SupportRelation *suppRel = iter->second;
			suppRel->output(ofs);
		}

		outFile.close();
	}

	filename = filePath + QString("/SupportParent_%1.prob").arg(dbType);
	outFile.setFileName(filename);
	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		for (auto iter = m_suppProbs.begin(); iter!= m_suppProbs.end(); iter++)
		{
			SupportProb &suppProb = iter->second;
			ofs << iter->first << "," << suppProb.beParentProb << " " << suppProb.beChildProb << "\n";
		}

		outFile.close();
		qDebug() << "model saved";
	}
}

void RelationModelManager::savePairwiseModelSim(const QString &filePath, const QString &dbType)
{
	QString filename = filePath + QString("/Pairwise_%1.sim").arg(dbType);

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		// save relative relations
		for (auto iter = m_pairwiseRelModels.begin(); iter != m_pairwiseRelModels.end(); iter++)
		{
			PairwiseRelationModel *relModel = iter->second;
			ofs << relModel->m_relationKey << ","<<QString("R_%1").arg(relModel->m_modelId)<<"\n";
			
			int simModelNum = relModel->m_simModelIds.size();
			ofs << simModelNum << ",";

			if (simModelNum > 0)
			{
				for (int k = 0; k < simModelNum - 1; k++)
				{
					ofs << relModel->m_simModelIds[k] << " ";
				}
				ofs << relModel->m_simModelIds[simModelNum - 1] << "\n";
			}
			else
			{
				ofs << "-1\n";
			}

			// max features
			for (int m=0; m <2 ; m++)
			{
				for (int i = 0; i < relModel->m_maxObjFeatures[m].size(); i++)
				{
					ofs << relModel->m_maxObjFeatures[m][i] <<" ";
				}
				ofs << "\n";
			}

			// avg features
			for (int m = 0; m < 2; m++)
			{
				for (int i = 0; i < relModel->m_avgObjFeatures[m].size(); i++)
				{
					ofs << relModel->m_avgObjFeatures[m][i]<<" ";
				}
				ofs << "\n";
			}
		}

		outFile.close();
	}
}

void RelationModelManager::saveGroupModelSim(const QString &filePath, const QString &dbType)
{
	QString filename = filePath + QString("/Group_%1.sim").arg(dbType);

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		for (auto giter = m_groupRelModels.begin(); giter != m_groupRelModels.end(); giter++)
		{
			GroupRelationModel *groupModel = giter->second;
			ofs << groupModel->m_groupKey << "\n";

			for (auto iter = groupModel->m_pairwiseModels.begin(); iter!=groupModel->m_pairwiseModels.end(); iter++)
			{
				PairwiseRelationModel *relModel = iter->second;

				ofs << relModel->m_relationKey << "," << QString("R_%1").arg(relModel->m_modelId) << "\n";

				int simModelNum = relModel->m_simModelIds.size();
				ofs << simModelNum << ",";

				if (simModelNum > 0)
				{
					for (int k = 0; k < simModelNum - 1; k++)
					{
						ofs << relModel->m_simModelIds[k] << " ";
					}
					ofs << relModel->m_simModelIds[simModelNum - 1] << "\n";
				}
				else
				{
					ofs << "-1\n";
				}
			}
		}
	}
}

void RelationModelManager::saveCoOccurInGroupModels(const QString &filePath, const QString &dbType)
{
	QString filename = filePath + QString("/CoOccInGroup_%1.model").arg(dbType);
	saveCoOccurModels(filename, m_coOccModelsInSameGroup);

	qDebug() << "Co-Occurrence in group models saved.";
}

void RelationModelManager::saveCoOccurOnParentModels(const QString &filePath, const QString &dbType)
{
	QString filename = filePath + QString("/CoOccOnParent_%1.model").arg(dbType);
	saveCoOccurModels(filename, m_coOccModelsOnSameParent);

	qDebug() << "Co-Occurrence of sibling objects saved.";
}

void RelationModelManager::saveCoOccurModels(const QString &filename, std::map<QString, CoOccurrenceModel*> &models)
{
	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		for (auto iter = models.begin(); iter!=models.end(); iter++)
		{
			CoOccurrenceModel *currModel = iter->second;
			ofs << currModel->m_coOccurKey << "," << currModel->m_prob << "\n";
			ofs << currModel->m_coOccNum << "," << currModel->m_firstObjNum << ","  << currModel->m_secondObjNum<<"\n";
		}

		outFile.close();
	}
}

