#pragma once
#include "../common/utilities/utility.h"

struct RelativePos
{
	double x, y, z, theta;
	QString actObjName;
	QString anchorObjName;
};

struct GaussianModel
{
	double mean, stdVar;
	double weight;
};

struct RelativeModel
{
	std::vector<GaussianModel> gaussians;
	QString actObjName;
	QString anchorObjName;

	std::vector<int> instanceIds;

};

struct OccurrenceModel
{
	std::map<QString, double> m_objOccurProbs;
};

class RelationModel
{
public:
	RelationModel();
	~RelationModel();

public:

	std::vector<RelativeModel> m_relativeModels;  // relation-conditioned relative model
	OccurrenceModel m_occurrenceModel;  // relation-conditioned occurrence model

	std::vector<QString> actObjNames;
	QString anchorObjName;

	QString m_relationName;
};

