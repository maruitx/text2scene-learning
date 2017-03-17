#pragma once
#include "RelationModel.h"

class GroupRelationModel;
class CScene;
class ModelDatabase;
class RelationExtractor;

class RelationModelManager
{
public:
	RelationModelManager(ModelDatabase *mDB, RelationExtractor *mExtractor);
	~RelationModelManager();

	void updateCurrScene(CScene *s) { m_currScene = s; };

	void collectRelativePosInCurrScene();

public:
	std::map<QString, PairwiseRelationModel*> m_pairwiseRelModels;
	std::map<QString, GroupRelationModel*> m_groupRelModels;

private:
	std::vector<RelativePos> m_relativePostions;

	RelationExtractor *m_relationExtractor;
	ModelDatabase *m_modelDB;
	CScene *m_currScene;
};

