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
	void addRelativePosFromCurrScene();

	void buildRelationModels();
	void saveRelationModels(const QString &filePath);

public:
	std::map<QString, PairwiseRelationModel*> m_relativeModels;  // relative models with general relations
	std::map<QString, PairwiseRelationModel*> m_pairwiseRelModels;  // pairwise relation models
	std::map<QString, GroupRelationModel*> m_groupRelModels;

private:
	std::vector<RelativePos*> m_relativePostions;  // load from saved file for per scene

	RelationExtractor *m_relationExtractor;
	CScene *m_currScene;
};

