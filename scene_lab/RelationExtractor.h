#pragma once

#include "../common/utilities/utility.h"

class CScene;
class CModel;
class SceneSemGraph;

const QString ConditionName[] = {"parentchild", "sibling", "proximity"};
const QString PairRelStrings[] = { "vert_support", "horizon_support", "contain", "above", "under", "leftside", "rightside", "front", "back", "near"};
const QString GroupRelStrings[] = { "around", "aligned", "grouped", "stacked", "scattered" };

class RelationExtractor
{
public:
	RelationExtractor();
	~RelationExtractor();

	void updateCurrScene(CScene *s) { m_currScene = s; };
	void updateCurrSceneSemGraph(SceneSemGraph *sg) { m_currSceneSemGraph = sg; };

	QString extractRelationConditionType(CModel *anchorModel, CModel *actModel);
	void extractRelativePosForModelPair(CModel *anchorModel, CModel *actModel, MathLib::Vector3 &pos, double &theta, MathLib::Matrix4d &transMat);

	std::vector<QString> extractSpatialSideRelForModelPair(int anchorModelId, int actModelId);
	std::vector<QString> extractSpatialSideRelForModelPair(CModel *anchorModel, CModel *actModel);

	bool isInProximity(CModel *anchorModel, CModel *actModel);

private:
	CScene *m_currScene;
	SceneSemGraph *m_currSceneSemGraph;


};

