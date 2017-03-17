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

	for (int i = 0; i < modelNum; i++)
	{
		CModel *anchorModel = m_currScene->getModel(i);
		for (int j = 0; j < modelNum; j++)
		{
			if (i==j) continue;
			
			CModel *actModel = m_currScene->getModel(i);
			QString conditionName = m_relationExtractor->extractRelationConditionType(anchorModel, actModel); 

			if (conditionName == "none") continue;

			RelativePos relPos;
			relPos.actObjName;
			relPos.anchorObjName;

			m_relationExtractor->extractRelativePosForModelPair(anchorModel, actModel, relPos.pos, relPos.theta, relPos.transMat);

			m_relativePostions.push_back(relPos);
		}
	}
}
