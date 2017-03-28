#pragma once
#include "../common/utilities/utility.h"

struct RelativePos
{
	MathLib::Vector3 pos;  // rel pos of actObj in anchor's unit frame
	double theta;
	MathLib::Matrix4d actAlignMat;  // transformation of actObj to anchorObj's unit frame,  anchorAlignMat*inverse(actInitTransMat)
	double unitFactor;

	QString m_actObjName;
	QString m_anchorObjName;

	QString m_conditionName;
};

struct GaussianModel
{
	double mean, stdVar;
	double weight;
};

class PairwiseRelationModel
{
public:
	PairwiseRelationModel(const QString &anchorName, const QString &actName, const QString &conditionName, const QString & relationName = "general");
	~PairwiseRelationModel() {};

	void fitGMM();

	std::vector<GaussianModel> gaussians;

	QString m_actObjName;
	QString m_anchorObjName;

	std::vector<RelativePos> m_instances;

	QString m_conditionName; // parent-child, sibling, proximity, or text-specified relation
	QString m_relationName;  // none, left, right, etc.
};

struct OccurrenceModel
{
	std::map<QString, double> m_objOccurProbs;
};

class GroupRelationModel
{
public: 
	GroupRelationModel();
	~GroupRelationModel();

public:
	std::vector<PairwiseRelationModel*> m_pairwiseModels;  // relation-conditioned relative model
	OccurrenceModel m_occurrenceModel;  // relation-conditioned occurrence model

	std::vector<QString> m_actObjNames;
	QString m_anchorObjName;

	QString m_relationName;
};

