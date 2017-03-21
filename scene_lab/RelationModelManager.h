#pragma once
#include "RelationModel.h"

class GroupRelationModel;
class CScene;
class RelationExtractor;

class RelationModelManager
{
public:
	RelationModelManager(RelationExtractor *mExtractor);
	~RelationModelManager();

	void updateCurrScene(CScene *s);

	void collectRelativePosInCurrScene();
	void addRelativePosForCurrScene();

	void buildRelationModels();

public:
	std::vector<PairwiseRelationModel*> m_pairwiseRelModels;
	std::vector<GroupRelationModel*> m_groupRelModels;



private:
	std::vector<RelativePos> m_relativePostions;  // load from saved file for per scene

	RelationExtractor *m_relationExtractor;
	CScene *m_currScene;
};

