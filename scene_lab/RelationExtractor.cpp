#include "RelationExtractor.h"
#include "../common/geometry/CModel.h"
#include "../common/geometry/Scene.h"
#include "../t2scene/SceneSemGraph.h"
#include "../common/geometry/SuppPlane.h"
#include "../scene_lab/RelationModel.h"

RelationExtractor::RelationExtractor()
{
}


RelationExtractor::~RelationExtractor()
{
}

QString RelationExtractor::extractRelationConditionType(CModel *anchorModel, CModel *actModel)
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

	// test for proximity relation
	if (isInProximity(anchorModel, actModel))
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
		relationStrings.push_back(PairRelStrings[9]);  // near
	}

	MathLib::Vector3 refFront = anchorModel->getHorizonFrontDir();
	MathLib::Vector3 refUp = anchorModel->getVertUpDir();

	MathLib::Vector3 refPos = anchorModel->getOBBInitPos();
	MathLib::Vector3 testPos = actModel->getOBBInitPos();

	MathLib::Vector3 fromRefToTestVec = testPos - refPos;
	fromRefToTestVec.normalize();

	// front or back side is not view dependent
	double frontDirDot = fromRefToTestVec.dot(refFront);


	double sideSectionVal = MathLib::Cos(30);
	if (frontDirDot > sideSectionVal)
	{
		QString refName = m_currSceneSemGraph->getCatName(anchorModel->getID());
		if (refName == "tv" || refName == "monitor" || refName.contains("table") || refName == "desk")  //  object requires to face to
		{
			MathLib::Vector3 testFront = actModel->getFrontDir();
			if (testFront.dot(refFront) < 0)  // test and ref should be opposite
			{
				relationStrings.push_back(PairRelStrings[7]); // front
			}
		}
		else
		{
			relationStrings.push_back(PairRelStrings[7]); // front
		}
	}
	else if (frontDirDot < -sideSectionVal && frontDirDot > -1)
	{
		relationStrings.push_back(PairRelStrings[8]); // back
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
			relationStrings.push_back(PairRelStrings[6]); // right
		}
		else if (rightDirDot <= -sideSectionVal)  // left of obj
		{
			relationStrings.push_back(PairRelStrings[5]); // left
		}
	}

	MathLib::Vector3 refOBBTopCenter = anchorModel->getModelTopCenter();
	MathLib::Vector3 fromRefTopToTestTop = actModel->getModelTopCenter() - refOBBTopCenter;

	if (fromRefTopToTestTop.dot(refUp) < 0)
	{
		SuppPlane* p = anchorModel->getSuppPlane(0);
		if (p->isCoverPos(testPos.x, testPos.y))
		{
			relationStrings.push_back(PairRelStrings[4]); // under
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

	double hausdorffDist = refOBB.HausdorffDist(testOBB);
	double connStrength = refOBB.ConnStrength_HD(testOBB);

	if (refOBB.IsIntersect(testOBB) || connStrength < 2.0)
	{
		return true;
	}
	else
		return false;
	
}

void RelationExtractor::extractRelativePosForModelPair(CModel *anchorModel, CModel *actModel, MathLib::Vector3 &pos, double &theta, MathLib::Matrix4d &transMat)
{

}

