#include "RelationExtractor.h"
#include "../common/geometry/CModel.h"
#include "../common/geometry/Scene.h"
#include "../t2scene/SceneSemGraph.h"
#include "../common/geometry/SuppPlane.h"

RelationExtractor::RelationExtractor(double angleTh)
	:m_angleThreshold(angleTh)
{
	m_rightAdjustObjNames.push_back("desk");
	m_rightAdjustObjNames.push_back("bookcase");
	m_rightAdjustObjNames.push_back("cabinet");
	m_rightAdjustObjNames.push_back("dresser");
	m_rightAdjustObjNames.push_back("monitor");
	m_rightAdjustObjNames.push_back("tv");
	m_rightAdjustObjNames.push_back("bed");
}


RelationExtractor::~RelationExtractor()
{
}

QString RelationExtractor::getRelationConditionType(CModel *anchorModel, CModel *actModel)
{
	int anchorModelId = anchorModel->getID();
	int actModelId = actModel->getID();

	// test for support relation
	if (actModel->suppParentID == anchorModelId)
	{
		return ConditionName[ConditionType::Pc];
	}

	if (anchorModel->suppParentID == actModelId)
	{
		return ConditionName[ConditionType::Cp];
	}
	
	// test for sibling relation
	if (actModel->suppParentID == anchorModel->suppParentID)
	{
		return ConditionName[ConditionType::Sib];
	}

	QString anchorCatName = anchorModel->getCatName();
	QString actCatName = actModel->getCatName();

	// test for proximity relation
	// do not consider proximity for "room"
	if (anchorCatName != "room" && actCatName != "room" && isInProximity(anchorModel, actModel))
	{
		return ConditionName[ConditionType::Prox];
	}

	return QString("none");
}

std::vector<QString> RelationExtractor::extractSpatialSideRelForModelPair(int anchorModelId, int actModelId)
{
	CModel *anchorModel = m_currScene->getModel(anchorModelId);
	CModel *actModel = m_currScene->getModel(actModelId);

	return extractSpatialSideRelForModelPair(anchorModel, actModel);
}

std::vector<QString> RelationExtractor::extractSpatialSideRelForModelPair(CModel *anchorModel, CModel *actModel)
{
	double distThreshold = 0.1 / m_currScene->getSceneMetric();

	std::vector<QString> relationStrings;

	QString conditionType = getRelationConditionType(anchorModel, actModel);

	if (conditionType == "none") return relationStrings;

	bool isGroundSib = false;  // both models are ground
	if (anchorModel->isGroundObj() && actModel->isGroundObj())
	{
		isGroundSib = true;
	}

	// add near to proximity objs
	bool isNear = isInProximity(anchorModel, actModel);
	if (isNear && isGroundSib)
	{
		relationStrings.push_back(PairRelStrings[PairRelation::Near]);  // near
	}

	double sideSectionVal = MathLib::Cos(m_angleThreshold);
	MathLib::Vector3 refFront = anchorModel->getHorizonFrontDir();
	MathLib::Vector3 refUp = anchorModel->getVertUpDir();

	MathLib::Vector3 refPos = anchorModel->m_currOBBPos;   // OBB bottom center
	MathLib::Vector3 testPos = actModel->m_currOBBPos;

	MathLib::Vector3 fromRefPosToTestPosVec = testPos - refPos;
	fromRefPosToTestPosVec[2] = 0; // project to XY plane
	fromRefPosToTestPosVec.normalize();

	// add front and back to sibling or near objs
	if (conditionType == ConditionName[ConditionType::Sib]&&isGroundSib || conditionType == ConditionName[ConditionType::Prox])
	{
		double posFrontDot = fromRefPosToTestPosVec.dot(refFront); // diff to ref's front dir using the posVec
		if (posFrontDot > sideSectionVal)
		{
			QString refName = m_currScene->getModelCatName(anchorModel->getID());
			relationStrings.push_back(PairRelStrings[PairRelation::Front]); // front
		}
		else if (posFrontDot < -sideSectionVal && posFrontDot > -1)
		{
			relationStrings.push_back(PairRelStrings[PairRelation::Back]); // back
		}
	}

	bool isOnCenter = false;
	double toBottomHight = testPos.z - refPos.z;
	double xyPlaneDist = std::sqrt(std::pow(testPos.x-refPos.x, 2) + std::pow(testPos.y-refPos.y, 2));

	if (xyPlaneDist < distThreshold && toBottomHight > 2*distThreshold)
	{
		isOnCenter = true;
		if (conditionType == ConditionName[ConditionType::Pc])
		{
			relationStrings.push_back(PairRelStrings[PairRelation::OnCenter]); // right
		}
	}

	MathLib::Vector3 refRight = refFront.cross(refUp);

	// adjust Right for certain models
	QString refName = m_currScene->getModelCatName(anchorModel->getID());
	bool isAnchorRightAdjust = false;
	if(std::find(m_rightAdjustObjNames.begin(), m_rightAdjustObjNames.end(), refName) != m_rightAdjustObjNames.end())
	{
		refRight = -refRight;
		isAnchorRightAdjust = true;
	}

	MathLib::Vector3 refBackCent = anchorModel->getOBBBackCenter();   // OBB bottom center
	MathLib::Vector3 testBackCent = actModel->getOBBBackCenter();

	MathLib::Vector3 fromRefBackToTestBackVec = testBackCent - refBackCent;
	fromRefBackToTestBackVec[2] = 0; // project to XY plane
	fromRefBackToTestBackVec.normalize();

	double posRightDirDot = fromRefPosToTestPosVec.dot(refRight); // diff to ref's right dir using the posVec
	double backRightDirDot = fromRefBackToTestBackVec.dot(refRight); // diff to ref's right dir using the backCentVec

	SuppPlane *anchorBBTopPlane = anchorModel->m_bbTopPlane;
	SuppPlane *actBBTopPlane = actModel->m_bbTopPlane;
	
	MathLib::Vector3 actFront = actModel->getHorizonFrontDir();
	double rightAngleTh = MathLib::ML_PI / 3;
	if (posRightDirDot >= sideSectionVal || backRightDirDot >= sideSectionVal) // right of obj
	{
		if (conditionType == ConditionName[ConditionType::Pc] && !isOnCenter)
		{
			relationStrings.push_back(PairRelStrings[PairRelation::OnRight]); // right
		}
		else if(anchorBBTopPlane!=NULL&& !anchorBBTopPlane->isCoverPos(testPos.x, testPos.y)
			&& actBBTopPlane != NULL&&!actBBTopPlane->isCoverPos(refPos.x, refPos.y)
			&&(conditionType == ConditionName[ConditionType::Sib]&&isNear || conditionType == ConditionName[ConditionType::Prox]))
		{
			if (isGroundSib) // ensure the front dir is consistent for ground obj
			{
				double angle = GetRotAngleR(refFront, actFront, MathLib::Vector3(0,0,1));
				if (isAnchorRightAdjust && angle <= 0 && angle >= -rightAngleTh)
				{
					relationStrings.push_back(PairRelStrings[PairRelation::RightSide]); // ensure the front dir is consistent for ground obj
				}
				else if(!isAnchorRightAdjust && angle >= 0 && angle <= rightAngleTh)
				{
					relationStrings.push_back(PairRelStrings[PairRelation::RightSide]); // ensure the front dir is consistent for ground obj

				}
			}
			//else
			//{
			//	relationStrings.push_back(PairRelStrings[PairRelation::RightSide]); // right
			//}
		}
	}
	else if (posRightDirDot < -sideSectionVal || backRightDirDot < -sideSectionVal)  // left of obj
	{
		if (conditionType == ConditionName[ConditionType::Pc] && !isOnCenter)
		{
			relationStrings.push_back(PairRelStrings[PairRelation::OnLeft]); // right
		}
		else if(anchorBBTopPlane != NULL && !anchorBBTopPlane->isCoverPos(testPos.x, testPos.y)
			&& actBBTopPlane != NULL && !actBBTopPlane->isCoverPos(refPos.x, refPos.y)
			&& (conditionType == ConditionName[ConditionType::Sib] && isNear || conditionType == ConditionName[ConditionType::Prox]))
		{
			if (isGroundSib)
			{
				double angle = GetRotAngleR(refFront, actFront, MathLib::Vector3(0, 0, 1));
				if (isAnchorRightAdjust && angle >= 0 && angle <= rightAngleTh)
				{
					relationStrings.push_back(PairRelStrings[PairRelation::LeftSide]); // ensure the front dir is consistent for ground obj
				}
				else if (!isAnchorRightAdjust && angle <= 0 && angle >= -rightAngleTh)
				{
					relationStrings.push_back(PairRelStrings[PairRelation::LeftSide]); // ensure the front dir is consistent for ground obj
				}
			}
		}
	}

	MathLib::Vector3 refOBBTopCenter = anchorModel->getModelTopCenter();
	MathLib::Vector3 fromRefTopToTestTop = actModel->getModelTopCenter() - refOBBTopCenter;
	fromRefTopToTestTop.normalize();

	if (fromRefTopToTestTop.dot(refUp) < 0)
	{
		if (anchorBBTopPlane != NULL&&anchorBBTopPlane->isCoverPos(testPos.x, testPos.y) && conditionType != ConditionName[ConditionType::Pc])
		{
			relationStrings.push_back(PairRelStrings[PairRelation::Under]); // under
		}
	}

	return relationStrings;
}

bool RelationExtractor::isInProximity(CModel *anchorModel, CModel *actModel)
{
	double sceneMetric = m_currScene->getSceneMetric();
	MathLib::Vector3 sceneUpDir = m_currScene->getUprightVec();

	COBB refOBB = anchorModel->getOBB();
	COBB testOBB = actModel->getOBB();

	double closestBBDist = refOBB.ClosestDist_Approx(testOBB);

	if (closestBBDist < 0.5 / sceneMetric)
	{
		return true;
	}
	else
		return false;
	
}

void RelationExtractor::extractRelativePosForModelPair(CModel *anchorModel, CModel *actModel, RelativePos *relPos)
{
	relPos->m_anchorObjName = anchorModel->getCatName();
	relPos->m_actObjName = actModel->getCatName();

	relPos->m_actObjId = actModel->getID();
	relPos->m_anchorObjId = anchorModel->getID();

	relPos->m_instanceNameHash = QString("%1_%2_%3").arg(relPos->m_anchorObjName).arg(relPos->m_actObjName).arg(relPos->m_conditionName);
	relPos->m_instanceIdHash = QString("%1_%2_%3").arg(relPos->m_sceneName).arg(relPos->m_anchorObjId).arg(relPos->m_actObjId);

	// first transform actModel into the scene and then bring it back using anchor model's alignMat
	relPos->anchorAlignMat = anchorModel->m_WorldBBToUnitBoxMat;

	if (relPos->anchorAlignMat.M[0] >1e10)
	{
		relPos->isValid = false;
		return;
	}

	relPos->actAlignMat = relPos->anchorAlignMat*actModel->getInitTransMat();

	MathLib::Vector3 actModelInitPos = actModel->getOBBInitPos(); // init pos when load the file
	relPos->pos = relPos->actAlignMat.transform(actModelInitPos);

	MathLib::Vector3 anchorModelFrontDir = anchorModel->getFrontDir();
	MathLib::Vector3 actModelFrontDir = actModel->getFrontDir();
	relPos->theta = GetRotAngleR(anchorModelFrontDir, actModelFrontDir, m_currScene->getUprightVec())/MathLib::ML_PI;

	relPos->isValid = true;
}

