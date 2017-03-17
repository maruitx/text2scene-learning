#pragma once
#include "../common/utilities/utility.h"

struct RelativePos
{
	MathLib::Vector3 pos;
	double theta;

	MathLib::Matrix4d transMat;  // transformation of anchor to unit box

	QString actObjName;
	QString anchorObjName;
};

struct GaussianModel
{
	double mean, stdVar;
	double weight;
};

struct PairwiseRelationModel
{
	std::vector<GaussianModel> gaussians;

	QString actObjName;
	QString anchorObjName;

	std::vector<int> instanceIds;

	QString conditionName; // parent-child, sibling, proximity, or text-specified relation
	QString relationName;
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

	std::vector<QString> actObjNames;
	QString anchorObjName;

	QString m_relationName;
};

