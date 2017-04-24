#pragma once

#include "../common/utilities/utility.h"
#include "../scene_lab/RelationModel.h"

class CScene;
class CModel;
class SceneSemGraph;

const QString ConditionName[] = {"parentchild", "sibling", "proximity"};
const QString PairRelStrings[] = { "vertsupport", "horizonsupport", "contain", "above", "under", "leftside", "rightside", "front", "back", "near"};
const QString GroupRelStrings[] = { "around", "aligned", "grouped", "stacked", "scattered" };

enum PairRelation
{
	VertSupp = 0,
	HorizonSupp,
	Contain,
	Above,
	Under,
	LeftSide,
	RightSide,
	Front,
	Back,
	Near	
};

enum GroupRelation
{
	Around = 0,
	Aligned,
	Grouped,
	Stacked,
	Scattered
};

class RelationExtractor
{
public:
	RelationExtractor();
	~RelationExtractor();

	void updateCurrScene(CScene *s) { m_currScene = s; };
	void updateCurrSceneSemGraph(SceneSemGraph *sg) { m_currSceneSemGraph = sg; };

	QString getRelationConditionType(CModel *anchorModel, CModel *actModel);
	void extractRelativePosForModelPair(CModel *anchorModel, CModel *actModel, RelativePos *relPos);

	std::vector<QString> extractSpatialSideRelForModelPair(int anchorModelId, int actModelId);
	std::vector<QString> extractSpatialSideRelForModelPair(CModel *anchorModel, CModel *actModel);

	bool isInProximity(CModel *anchorModel, CModel *actModel);

private:
	CScene *m_currScene;
	SceneSemGraph *m_currSceneSemGraph;


};

