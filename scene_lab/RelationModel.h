#pragma once
#include "../common/utilities/utility.h"

class RelativePos
{
public:
	RelativePos() {};
	~RelativePos() {};

	MathLib::Vector3 pos;  // rel pos of actObj in anchor's unit frame
	double theta;
	MathLib::Matrix4d anchorAlignMat;  // transformation of anchorObj to unit frame
	MathLib::Matrix4d actAlignMat;  // transformation of actObj to anchorObj's unit frame,  anchorAlignMat*actInitTransMat
	double unitFactor;

	QString m_actObjName;
	QString m_anchorObjName;

	QString m_conditionName;
};

struct GaussianModel
{
	int dim;
	Eigen::VectorXd mean; 
	Eigen::MatrixXd covarMat;
	
	double weight;   // mixing weight
};

class PairwiseRelationModel
{
public:
	PairwiseRelationModel(const QString &anchorName, const QString &actName, const QString &conditionName, const QString & relationName = "general");
	~PairwiseRelationModel() {};

	void fitGMM();

	int m_numGauss;
	int m_numInstance;

	std::vector<RelativePos*> m_instances;
	std::vector<GaussianModel> m_gaussians;

	QString m_actObjName;
	QString m_anchorObjName;

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

