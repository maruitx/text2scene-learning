#include "RelationModelManager.h"
#include "RelationExtractor.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"



RelationModelManager::RelationModelManager(ModelDatabase *mDB, RelationExtractor *mExtractor)
	:m_modelDB(mDB), m_relationExtractor(mExtractor)
{

}


RelationModelManager::~RelationModelManager()
{

}


void RelationModelManager::collectRelativePosInCurrScene()
{
	int modelNum = m_currScene->getModelNum();

	if (!m_currScene->loadModelBBAlignMat())
	{
		qDebug() << "RelationModelManager: compute model align mat first for "<< m_currScene->getSceneName();
	}

	for (int i = 0; i < modelNum; i++)
	{
		CModel *anchorModel = m_currScene->getModel(i);
		for (int j = 0; j < modelNum; j++)
		{
			if (i==j) continue;
			
			CModel *actModel = m_currScene->getModel(i);
			QString conditionName = m_relationExtractor->getRelationConditionType(anchorModel, actModel); 

			if (conditionName == "none") continue;

			RelativePos relPos;
			relPos.conditionName = conditionName;

			m_relationExtractor->extractRelativePosForModelPair(anchorModel, actModel, relPos);

			m_currScene->m_relPositions.push_back(relPos);
		}
	}

	m_currScene->saveRelPositions();
}
