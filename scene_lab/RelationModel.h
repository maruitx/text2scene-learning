#pragma once
#include "../common/utilities/utility.h"
#include "GaussianMixtureModel.h"

class CScene;

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
	QString m_instanceNameHash;  // anchorObjName_actObjName_conditionName


	int m_actObjId;
	int m_anchorObjId;
	QString m_sceneName;
	QString m_instanceIdHash;  // sceneName_anchorObjId_actObjId


	bool isValid;
};


class PairwiseRelationModel
{
public:
	PairwiseRelationModel(const QString &anchorName, const QString &actName, const QString &conditionName, const QString &relationName);
	~PairwiseRelationModel();

	void fitGMM(int instanceTh);
	void output(QTextStream &ofs);
	void computeObjNodeFeatures(const std::vector<CScene*> &sceneList, std::map<QString, int> &sceneNameToIdMap);

	int m_numGauss;
	GaussianMixtureModel *m_GMM;

	int m_numInstance;
	std::vector<RelativePos*> m_instances;

	QString m_relationKey;  // anchorName_actName_conditionName_relationName

	QString m_actObjName;
	QString m_anchorObjName;

	QString m_conditionName; // parent-child, sibling, proximity, or text-specified relation
	QString m_relationName;  // none, left, right, etc.

	std::vector<std::vector<double>> m_avgObjFeatures;  // heightToFloor, modelHeight, modelVolume of anchor and act obj
	std::vector<std::vector<double>> m_maxObjFeatures;

	int m_modelId;
	std::vector<int> m_simModelIds; // list of similar relation model ids, sorted from high sim to low
};

class OccurrenceModel
{
public:
	OccurrenceModel(const QString &objName, int objNum);

	int m_objNum;
	int m_numInstance;   // number of observations
	double m_occurProb;

	QString m_objName; // active object name
	//QString m_conditionName; // condition to the anchor object

	QString m_occurKey; // objName_objNum
};

class GroupRelationModel
{
public: 
	GroupRelationModel(const QString &anchorObjName, const QString &relationName);
	~GroupRelationModel();

	void computeOccurrence();
	void fitGMMs();
	void output(QTextStream &ofs);

public:
	std::map<QString, PairwiseRelationModel*> m_pairwiseModels;  // relation-conditioned relative model
	std::vector<QString> m_pairModelKeys;

	std::map<QString, OccurrenceModel*> m_occurModels;

	QString m_anchorObjName;
	QString m_relationName;

	QString m_groupKey;  // relationName_anchorObjName
	int m_numInstance;
};


class SupportRelation
{
public:

	SupportRelation(const QString &parentName, const QString &childName, const QString &supportType);
	~SupportRelation();
	void computeSupportProb();
	void output(QTextStream &ofs);

	double m_childProbGivenParent;
	double m_parentProbGivenChild;

	int m_jointInstanceNum;
	int m_parentInstanceNum;
	int m_childInstanceNum;

	QString m_parentName;
	QString m_childName;
	QString m_supportType;  // vertsupport, horizonsupport

	QString m_suppRelKey;


};