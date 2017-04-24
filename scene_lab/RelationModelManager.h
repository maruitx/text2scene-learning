#pragma once
#include "RelationModel.h"

class GroupRelationModel;
class CScene;
class RelationExtractor;

struct SupportProb
{
	SupportProb() {
		beParentProb = 0; beChildProb=0; totalNum = 0; beParentNum = 0; beChildNum = 0;
	};

	double beParentProb;
	double beChildProb;  
	int totalNum;
	int beParentNum;
	int beChildNum;  // being a child while the parent is not "room"
};

class RelationModelManager
{
public:
	RelationModelManager(RelationExtractor *mExtractor);
	~RelationModelManager();

	void updateCurrScene(CScene *s);

	// extract relative pos
	void collectRelativePosInCurrScene();

	// load relative pos from file
	void loadRelativePosFromCurrScene();
	void buildRelativeRelationModels();

	void buildPairwiseRelationModels();
	void collectPairwiseInstanceFromCurrScene();

	void buildGroupRelationModels();
	void collectGroupInstanceFromCurrScene();
	void collectOccurrForGroupModel(GroupRelationModel *groupModel, const std::vector<int> &actNodeList);
	void collectRelPosForGroupModel(GroupRelationModel *groupModel, const QString &sceneName, int anchorNodeId, const std::vector<int> &actNodeList);

	void computeSimForPairwiseModels(std::map<QString, PairwiseRelationModel*> &pairModels, const std::vector<QString> &pairModelKeys, const std::vector<CScene*> &sceneList, bool isInGroup = false, const QString &filePath = "");
	void computeSimForPairModelInGroup(const std::vector<CScene*> &sceneList);

	void collectSupportRelationInCurrentScene();
	void buildSupportRelationModels();

	void saveRelativeRelationModels(const QString &filePath);
	void savePairwiseRelationModels(const QString &filePath);
	void saveGroupRelationModels(const QString &filePath);
	void saveSupportRelationModels(const QString &filePath);

	void savePairwiseModelSim(const QString &filePath);
	void saveGroupModelSim(const QString &filePath);

public:
	std::map<QString, PairwiseRelationModel*> m_relativeModels;  // relative models with general relations
	std::map<QString, PairwiseRelationModel*> m_pairwiseRelModels;  // pairwise relation models
	std::map<QString, GroupRelationModel*> m_groupRelModels;
	std::map<QString, SupportRelation*> m_supportRelations;
	std::map<QString, SupportProb> m_suppProbs; //

	std::vector<QString> m_pairRelModelKeys;

private:
	std::map<QString, RelativePos*> m_relativePostions;  // load from saved file for per scene

	RelationExtractor *m_relationExtractor;
	CScene *m_currScene;
};

