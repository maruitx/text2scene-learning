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

void RelationModelManager::addRelativePosFromCurrScene()
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
		relPos->m_anchorObjName = toQString(parts[0]);
		relPos->m_actObjName = toQString(parts[1]);
		relPos->m_conditionName = toQString(parts[2]);

		std::vector<std::string> subParts = PartitionString(parts[3], "_");
		relPos->m_sceneName = toQString(subParts[0]);
		relPos->m_anchorObjId = StringToInt(subParts[1]);
		relPos->m_actObjId = StringToInt(subParts[2]);

		relPos->m_instanceHash = QString("%1_%2_%3").arg(relPos->m_sceneName).arg(relPos->m_anchorObjId).arg(relPos->m_actObjId);;

		currLine = ifs.readLine();
		parts = PartitionString(currLine.toStdString(), ",");

		std::vector<float> pos = StringToFloatList(parts[0], "");
		relPos->pos = MathLib::Vector3(pos[0], pos[1], pos[2]);
		relPos->theta = pos[3];

		std::vector<float> transformVec = StringToFloatList(parts[1], "");
		relPos->anchorAlignMat = MathLib::Matrix4d(transformVec);

		transformVec = StringToFloatList(parts[2], "");
		relPos->actAlignMat = MathLib::Matrix4d(transformVec);

		m_relativePostions[relPos->m_instanceHash] = relPos;
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
		QString relationKey = relPos->m_anchorObjName + relPos->m_actObjName + relPos->m_conditionName;

		if (!m_relativeModels.count(relationKey))
		{
			PairwiseRelationModel *relativeModel = new PairwiseRelationModel(relPos->m_anchorObjName, relPos->m_actObjName, relPos->m_conditionName);
			
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
	for (auto iter = m_relativeModels.begin(); iter != m_relativeModels.end(); iter++)
	{
		PairwiseRelationModel *relModel = iter->second;
		relModel->fitGMM(15);
	}

	// 3. Close MATLAB engine
	engClose(matlabEngine);

	qDebug() << "Relative relations extracted";
}

void RelationModelManager::buildGroupRelationModels()
{

	// 1. Open MATLAB engine
	matlabEngine = engOpen(NULL);

	// fit GMM model
	for (auto iter = m_groupRelModels.begin(); iter != m_groupRelModels.end(); iter++)
	{
		GroupRelationModel *groupModel = iter->second;
		groupModel->fitGMMs();
	}

	// 3. Close MATLAB engine
	engClose(matlabEngine);

	qDebug() << "Relative relations extracted";
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
			QString groupKey = sgNode.nodeName + anchorObjName;

			if (!m_groupRelModels.count(groupKey))
			{
				GroupRelationModel *newGroupModel = new GroupRelationModel();
				newGroupModel->m_anchorObjName = anchorObjName;
				newGroupModel->m_relationName = sgNode.nodeName;

				collectInstanceForGroupModel(newGroupModel, sceneName, anchoNodeId, sgNode.activeNodeList);

				m_groupRelModels[groupKey] = newGroupModel;

			}

			else
				collectInstanceForGroupModel(m_groupRelModels[groupKey], sceneName, anchoNodeId, sgNode.activeNodeList);
		}
	}
}

void RelationModelManager::collectInstanceForGroupModel(GroupRelationModel *groupModel, const QString &sceneName, int anchorNodeId, const std::vector<int> &actNodeList)
{
	for (int t = 0; t < actNodeList.size(); t++)
	{
		QString instanceHash = QString("%1_%2_%3").arg(sceneName).arg(anchorNodeId).arg(actNodeList[t]);

		if (m_relativePostions.count(instanceHash))
		{
			RelativePos* relPos = m_relativePostions[instanceHash];
			QString relationKey = relPos->m_anchorObjName + relPos->m_actObjName + relPos->m_conditionName;

			if (!groupModel->m_pairwiseModels.count(relationKey))
			{
				PairwiseRelationModel *relativeModel = new PairwiseRelationModel(relPos->m_anchorObjName, relPos->m_actObjName, relPos->m_conditionName);

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
