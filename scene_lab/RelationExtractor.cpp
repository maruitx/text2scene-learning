#include "RelationExtractor.h"
#include "../common/geometry/CModel.h"
#include "../common/geometry/Scene.h"
#include "../t2scene/SceneSemGraph.h"
#include "../common/geometry/SuppPlane.h"

RelationExtractor::RelationExtractor()
{
}


RelationExtractor::~RelationExtractor()
{
}

QString RelationExtractor::getRelationConditionType(CModel *anchorModel, CModel *actModel)
{
	int anchorModelId = anchorModel->getID();
	int actModelId = actModel->getID();

	// test for support relation
	if (actModel->suppParentID == anchorModelId || anchorModel->suppParentID == actModelId)
	{
		return ConditionName[0];
	}
	
	// test for sibling relation
	if (actModel->suppParentID == anchorModel->suppParentID)
	{
		return ConditionName[1];
	}

	QString anchorCatName = anchorModel->getCatName();
	QString actCatName = actModel->getCatName();

	// test for proximity relation
	// do not consider proximity for "room"
	if (anchorCatName != "room" && actCatName != "room" && isInProximity(anchorModel, actModel))
	{
		return ConditionName[2];
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
	std::vector<QString> relationStrings;

	if (isInProximity(anchorModel, actModel))
	{
		relationStrings.push_back(PairRelStrings[PairRelation::Near]);  // near
	}

	MathLib::Vector3 refFront = anchorModel->getHorizonFrontDir();
	MathLib::Vector3 refUp = anchorModel->getVertUpDir();

	MathLib::Vector3 refPos = anchorModel->m_currOBBPos;   // OBB bottom center
	MathLib::Vector3 testPos = actModel->m_currOBBPos;

	MathLib::Vector3 fromRefToTestVec = testPos - refPos;
	fromRefToTestVec.normalize();

	// front or back side is not view dependent
	double frontDirDot = fromRefToTestVec.dot(refFront);

	double sideSectionVal = MathLib::Cos(15);
	if (frontDirDot > sideSectionVal)
	{
		QString refName = m_currSceneSemGraph->getCatName(anchorModel->getID());
		if (refName == "tv" || refName == "monitor" || refName.contains("table") || refName == "desk")  //  object requires to face to
		{
			MathLib::Vector3 testFront = actModel->getFrontDir();
			if (testFront.dot(refFront) < 0)  // test and ref should be opposite
			{
				relationStrings.push_back(PairRelStrings[PairRelation::Front]); // front
			}
		}
		else
		{
			relationStrings.push_back(PairRelStrings[PairRelation::Front]); // front
		}
	}
	else if (frontDirDot < -sideSectionVal && frontDirDot > -1)
	{
		relationStrings.push_back(PairRelStrings[PairRelation::Back]); // back
	}

	// left or right may be view dependent
	MathLib::Vector3 refRight = refFront.cross(refUp);
	double rightDirDot = fromRefToTestVec.dot(refRight);

	MathLib::Vector3 roomFront = m_currScene->getRoomFront();

	// if ref front is same to th room front (e.g. view inside 0, -1, 0)
	//bool useViewCentric = false;
	//if (refFront.dot(roomFront) > sideSectionVal  && useViewCentric)
	//{
	//	// use view-centric
	//	if (rightDirDot >= sideSectionVal) // right of obj
	//	{
	//		relationStrings.push_back(SSGPairRelStrings[5]); // left in view
	//	}
	//	else if (rightDirDot <= -sideSectionVal) // left of obj
	//	{
	//		relationStrings.push_back(SSGPairRelStrings[6]); // right in view
	//	}
	//}
	//else
	{
		if (rightDirDot >= sideSectionVal) // right of obj
		{
			relationStrings.push_back(PairRelStrings[PairRelation::RightSide]); // right
		}
		else if (rightDirDot <= -sideSectionVal)  // left of obj
		{
			relationStrings.push_back(PairRelStrings[PairRelation::LeftSide]); // left
		}
	}

	MathLib::Vector3 refOBBTopCenter = anchorModel->getModelTopCenter();
	MathLib::Vector3 fromRefTopToTestTop = actModel->getModelTopCenter() - refOBBTopCenter;
	fromRefTopToTestTop.normalize();

	if (fromRefTopToTestTop.dot(refUp) < 0)
	{
		SuppPlane* p = anchorModel->getSuppPlane(0);
		if (p->isCoverPos(testPos.x, testPos.y))
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
	relPos->theta = GetRotAngleR(anchorModelFrontDir, actModelFrontDir, m_currScene->getUprightVec());

	relPos->isValid = true;
}

