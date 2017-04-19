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

}

RelationModelManager::~RelationModelManager()
{
	for (auto it = m_relativePostions.begin(); it != m_relativePostions.end(); it++)
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

		std::cout << "Relative model fitted " << QString("%1/%2\r\n").arg(id++).arg(totalNum).toStdString();
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

		std::cout << "Pairwise model fitted " << QString("%1/%2\r\n").arg(id++).arg(totalNum).toStdString();
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

		if (sgNode.nodeType.contains(SSGNodeType[2]))
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

void RelationModelManager::computeSimForPairwiseModels(std::map<QString, PairwiseRelationModel*> &pairModels, const std::vector<QString> &pairModelKeys, const std::vector<CScene*> &sceneList, const QString &filePath)
{
	int sceneNum = sceneList.size();
	std::map<QString, int> sceneNameToIdMap;
	for (int i=0; i <sceneNum; i++)
	{
		QString sceneName = sceneList[i]->getSceneName();
		sceneNameToIdMap[sceneName] = i;
	}

	int relNum = 10;
	if (filePath.isEmpty())
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

		for (int c=0; c< 3; c++)
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
			// collect max value for normalization
			Eigen::MatrixXd simMat(pairModelIds.size(), pairModelIds.size());
			double featureWeights[] = { 1,1,1,0.5,0.5,0.5 };

			for (int i = 0; i < pairModelIds.size(); i++)
			{
				QString modelKeyA = pairModelKeys[pairModelIds[i]];
				PairwiseRelationModel *relModelA = pairModels[modelKeyA];

				for (int j = 0; j < pairModelIds.size(); j++)
				{
					if (i == j) continue;

					std::vector<double> simVal(2, 0);
					for (int m = 0; m < 2; m++)
					{
						QString modelKeyB = pairModelKeys[pairModelIds[j]];
						PairwiseRelationModel *relModelB = pairModels[modelKeyB];

						for (int d=0; d<featureDim; d++)
						{
							simVal[m] += featureWeights[d]*exp(-pow((relModelA->m_avgObjFeatures[m][d] - relModelB->m_avgObjFeatures[m][d])/ (maxFeatures[m][d] + 1e-3), 2));
						}
					}
					simMat(i, j) = simVal[0] * simVal[1];
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

			// save sim Mat
			if (!pairModelIds.empty() && !filePath.isEmpty())
			{
				QString filename = filePath + QString("/%1_%2.simMat").arg(relType).arg(conditionType);
				std::ofstream ofs;
				ofs.open(filename.toStdString(), std::ios::out | std::ios::trunc);

				if (ofs.is_open())
				{
					for (int i = 0; i < pairModelIds.size(); i++)
					{
						QString modelKey = pairModelKeys[pairModelIds[i]];
						PairwiseRelationModel *relModel = pairModels[modelKey];

						ofs << relModel->m_relationKey.toStdString() << ",R_" << relModel->m_modelId << "," << i << "\n";
						int simModelNum = relModel->m_simModelIds.size();
						ofs << simModelNum << ",";

						if (simModelNum > 0)
						{
							for (int k = 0; k < simModelNum - 1; k++)
							{
								ofs << relModel->m_simModelIds[k] << " ";
							}
							ofs << relModel->m_simModelIds[simModelNum - 1] << "\n";
							for (int k = 0; k < simModelNum - 1; k++)
							{
								ofs << simMat(i, globalToLocalIdMap[relModel->m_simModelIds[k]]) << ",";
							}
							ofs << simMat(i, globalToLocalIdMap[relModel->m_simModelIds[simModelNum - 1]]) << "\n";
						}
						else
						{
							ofs << "-1\n";
						}
					}
					ofs.close();
				}
			}
		}
	}
}

void RelationModelManager::computeSimForPairModelInGroup(const std::vector<CScene*> &sceneList)
{
	for (auto giter = m_groupRelModels.begin(); giter != m_groupRelModels.end(); giter++)
	{
		GroupRelationModel *groupModel = giter->second;
		computeSimForPairwiseModels(groupModel->m_pairwiseModels, groupModel->m_pairModelKeys, sceneList);
	}
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

		std::cout << "Group model fitted " << QString("%1/%2\r\n").arg(id++).arg(totalNum).toStdString();
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

				m_groupRelModels[groupKey] = newGroupModel;
			}
			else
			{
				collectRelPosForGroupModel(m_groupRelModels[groupKey], sceneName, anchoNodeId, sgNode.activeNodeList);
				collectOccurrForGroupModel(m_groupRelModels[groupKey], sgNode.activeNodeList);				
			}

			m_groupRelModels[groupKey]->m_numInstance++;
		}
	}
}

void RelationModelManager::collectOccurrForGroupModel(GroupRelationModel *groupModel, const std::vector<int> &actNodeList)
{
	SceneSemGraph *currSSG = m_currScene->m_ssg;

	std::vector<int> actNodeIndicator(actNodeList.size(), 0); // whether the node has been counted

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
			if (actNodeIndicator[i] == 0 && actObjName == testNode.nodeName)
			{
				actNodeIndicator[i] = 1;
				currActObjNum++;
			}
		}

		if (currActObjNum)
		{
			QString occurKey = QString("%1_%2").arg(actObjName).arg(currActObjNum);

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

void RelationModelManager::saveRelativeRelationModels(const QString &filePath)
{
	QString filename = filePath + "/Relative.model";

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
	}
}

void RelationModelManager::savePairwiseRelationModels(const QString &filePath)
{
	QString filename = filePath + "/Pairwise.model";

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
	}
}

void RelationModelManager::saveGroupRelationModels(const QString &filePath)
{
	QString filename = filePath + "/Group.model";

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
	}
}

void RelationModelManager::savePairwiseModelSim(const QString &filePath)
{
	QString filename = filePath + "/Pairwise.sim";

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
		}

		outFile.close();
	}
}

void RelationModelManager::saveGroupModelSim(const QString &filePath)
{
	QString filename = filePath + "/Group.sim";

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
