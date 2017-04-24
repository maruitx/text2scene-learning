#include "RelationModel.h"
#include "engine.h"
#include "../common/utilities/eigenMat.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"

extern Engine *matlabEngine;

PairwiseRelationModel::PairwiseRelationModel(const QString &anchorName, const QString &actName, const QString &conditionName, const QString & relationName /*= "general"*/)
	:m_anchorObjName(anchorName), m_actObjName(actName), m_conditionName(conditionName), m_relationName(relationName)

{
	m_relationKey = m_anchorObjName + "_" + m_actObjName + "_" + m_conditionName + "_" + m_relationName;
	m_numInstance = 0;
	m_numGauss = 0;
	
	m_GMM = NULL;
}

PairwiseRelationModel::~PairwiseRelationModel()
{
	if (m_GMM != NULL)
	{
		delete m_GMM;
	}
}

void PairwiseRelationModel::fitGMM(int instanceTh)
{
	// prepare data points
	if (m_numInstance < instanceTh)
	{
		m_numGauss = 0;
		return;
	}

	//double *observations = new double[m_numInstance*3];
	//double *alignMats = new double[m_numInstance * 16];

	//for (int i=0; i< m_numInstance; i++)
	//{
	//	RelativePos &relPos = m_instances[i];
	//	observations[3 * i] = relPos->pos.x;
	//	observations[3 * i +1] = relPos->pos.y;
	//	observations[3 * i +2] = relPos->pos.z;

	//	MathLib::Matrix4d alignM = relPos->anchorAlignMat;  // world to unit bb mat
	//	for (int j=0; j < 16; j++)
	//	{
	//		alignMats[16 * i + j] = alignM.M[j];
	//	}
	//}

	//mxArray *observationArray = mxCreateDoubleMatrix(3, m_numInstance, mxREAL);
	//memcpy((void *)mxGetPr(observationArray), (void *)observations, sizeof(double) * 3 * m_numInstance);
	//mxArray *alignMatsArray = mxCreateDoubleMatrix(16, m_numInstance, mxREAL);
	//memcpy((void *)mxGetPr(alignMatsArray), (void *)alignMats, sizeof(double) * 16 * m_numInstance);

	Eigen::MatrixXd observations(4, m_numInstance);
	Eigen::MatrixXd alignMats(16, m_numInstance);
	for (int i = 0; i< m_numInstance; i++)
	{
		RelativePos *relPos = m_instances[i];
		observations(0, i) = relPos->pos.x;
		observations(1, i) = relPos->pos.y;
		observations(2, i) = relPos->pos.z;
		observations(3, i) = relPos->theta;

		MathLib::Matrix4d alignM = relPos->anchorAlignMat;  // world to unit bb mat
		for (int j = 0; j < 16; j++)
		{
			alignMats(j, i) = alignM.M[j];
		}
	}

	mxArray *observationArray = toMAT(observations);
	mxArray *alignMatsArray = toMAT(alignMats);

	const int BUFSIZE = 1024;
	char buffer[BUFSIZE + 1];
	buffer[BUFSIZE] = '\0';
	engOutputBuffer(matlabEngine, buffer, BUFSIZE);

	// clear all variables (optional)
	engEvalString(matlabEngine, "clc;clear;"); 
	engPutVariable(matlabEngine, "X", observationArray); // put into matlab
	engPutVariable(matlabEngine, "alignMats", alignMatsArray); // put into matlab

	engEvalString(matlabEngine, "cd \'C:\\Ruim\\Graphics\\T2S_MPC\\text2scene-learning\\scene_lab'");
	engEvalString(matlabEngine, "[isFitSuccess, mus, sigmas, weights, numComp, pTh] = fitGMMWithAIC(X, 4, alignMats);");

	printf("%s\n", buffer); // get error messages or prints (optional)

	mxArray *isFitSuccess = engGetVariable(matlabEngine, "isFitSuccess");
	mxArray *mus = engGetVariable(matlabEngine, "mus");
	mxArray *sigmas = engGetVariable(matlabEngine, "sigmas");
	mxArray *weights = engGetVariable(matlabEngine, "weights");
	mxArray *numComp = engGetVariable(matlabEngine, "numComp");
	mxArray *pTh = engGetVariable(matlabEngine, "pTh");

	int isGuassFitted = mxGetScalar(isFitSuccess);
	if (!isGuassFitted)
	{
		m_numGauss = 0;
		return;
	}

	m_numGauss = mxGetScalar(numComp);
	Eigen::Map<Eigen::MatrixXd> probTh = fromMATd(pTh);
	Eigen::Map<Eigen::MatrixXd> gaussMus = fromMATd(mus);
	std::vector<Eigen::Map<Eigen::MatrixXd>> gaussSigmas = from3DMATd(sigmas, m_numGauss);
	Eigen::Map<Eigen::MatrixXd> gaussMixWeights = fromMATd(weights);

	m_GMM = new GaussianMixtureModel(m_numGauss);
	m_GMM->m_probTh = probTh.row(0);

	for (int i=0; i < m_numGauss; i++)
	{
		int gaussDim = gaussMus.cols();
		double gaussWeight = gaussMixWeights(i);

		Eigen::VectorXd mean = gaussMus.row(i);
		Eigen::MatrixXd covarMat = gaussSigmas[i];
		
		GaussianModel* newGauss = new GaussianModel(gaussDim, gaussWeight, mean, covarMat);

		m_GMM->m_gaussians[i] = newGauss;
	}
}

void PairwiseRelationModel::output(QTextStream &ofs)
{
	ofs << m_relationKey <<"\n";

	ofs << m_numGauss << " " << m_numInstance;
	if (m_numGauss > 0)
	{
		ofs<< " " << m_GMM->m_probTh[0] << " " << m_GMM->m_probTh[1] << " " << m_GMM->m_probTh[2] << "\n";

		for (int i = 0; i < m_numGauss; i++)
		{
			GaussianModel *currGauss = m_GMM->m_gaussians[i];
			ofs << currGauss->dim << ",";
			ofs << currGauss->weight << ",";
			ofs << currGauss->mean(0) << " " << currGauss->mean(1) << " " << currGauss->mean(2) << " " << currGauss->mean(3) << ",";
			ofs << GetTransformationString(currGauss->covarMat) << "\n";
		}
	}
	else
	{
		ofs << " 0 0 0\n";

		for (int i = 0; i < m_numInstance; i++)
		{
			RelativePos *relPos = m_instances[i];
			if (i < m_numInstance - 1)
			{
				ofs << relPos->pos.x << " " << relPos->pos.y << " " << relPos->pos.z << " " << relPos->theta << ",";
			}
			else
				ofs << relPos->pos.x << " " << relPos->pos.y << " " << relPos->pos.z << " " << relPos->theta << "\n";
		}
	}
}

void PairwiseRelationModel::computeObjNodeFeatures(const std::vector<CScene*> &sceneList, std::map<QString, int> &sceneNameToIdMap)
{
	int instanceNum = m_instances.size();
	int featureDim = 6;
	m_avgObjFeatures.resize(2, std::vector<double>(featureDim, 0));

	std::vector<int> modelIds(2);

	for (int i =0; i < instanceNum; i++)
	{
		RelativePos *relPos = m_instances[i];
		int sceneId = sceneNameToIdMap[relPos->m_sceneName];
		CScene *currScene = sceneList[sceneId];

		modelIds[0] = relPos->m_anchorObjId;
		modelIds[1] = relPos->m_actObjId;

		for (int m = 0; m < 2; m++)
		{
			CModel *currModel = currScene->getModel(modelIds[m]);
			double heightToFloor = currModel->getOBBBottomHeight() - currScene->getFloorHeight();
			m_avgObjFeatures[m][0] += heightToFloor;
			
			double modelHeight = currModel->getOBBHeight();
			m_avgObjFeatures[m][1] += modelHeight;

			double modelVolume = currModel->getOBBVolume();
			m_avgObjFeatures[m][2] += modelVolume;

			double xRange = currModel->getHorizonShortRange();  // treat x as horizon short range of OBB
			double yRange = currModel->getHorizonLongRange();
			double xzRatio = modelHeight/xRange;
			double yzRatio = modelHeight/yRange;
			double xyRatio = yRange / xRange;

			m_avgObjFeatures[m][3] += xzRatio;
			m_avgObjFeatures[m][4] += yzRatio;
			m_avgObjFeatures[m][5] += xyRatio;
		}
	}

	for (int m = 0; m < 2; m++)
	{
		for (int d=0; d < featureDim; d++)
		{
			m_avgObjFeatures[m][d] /= instanceNum;
		}
	}
}

OccurrenceModel::OccurrenceModel(const QString &objName, int objNum)
	:m_objName(objName), m_objNum(objNum)
{
	m_occurKey = QString("%1_%2").arg(m_objName).arg(m_objNum);
	m_numInstance = 0;
	m_occurProb = 0;
}

GroupRelationModel::GroupRelationModel(const QString &anchorObjName, const QString &relationName)
	:m_anchorObjName(anchorObjName), m_relationName(relationName)
{
	m_groupKey = m_relationName + "_" + m_anchorObjName;
	m_numInstance = 0;
}

GroupRelationModel::~GroupRelationModel()
{
	for (auto iter = m_occurModels.begin(); iter != m_occurModels.end(); iter++)
	{
		delete iter->second;
	}

	for (auto iter = m_pairwiseModels.begin(); iter != m_pairwiseModels.end(); iter++)
	{
		delete iter->second;
	}
}

void GroupRelationModel::computeOccurrence()
{
	for (auto iter = m_occurModels.begin(); iter!= m_occurModels.end(); iter++)
	{
		OccurrenceModel *occurModel = iter->second;
		occurModel->m_occurProb = occurModel->m_numInstance / (double) m_numInstance;
	}
}

void GroupRelationModel::fitGMMs()
{
	int id = 0;
	m_pairModelKeys.resize(m_pairwiseModels.size());
	for (auto iter = m_pairwiseModels.begin(); iter!=m_pairwiseModels.end(); iter++)
	{
		PairwiseRelationModel *relModel = iter->second;
		relModel->fitGMM(15);
		relModel->m_modelId = id;

		m_pairModelKeys[id] = relModel->m_relationKey;
		id++;
	}
}

void GroupRelationModel::output(QTextStream &ofs)
{
	ofs <<m_groupKey << "," << m_numInstance <<"\n";
	ofs << "occurrence " << m_occurModels.size() << "\n";
	for (auto iter = m_occurModels.begin(); iter!= m_occurModels.end(); iter++)
	{
		OccurrenceModel *occurModel = iter->second;
		ofs << occurModel->m_occurKey << "," << occurModel->m_occurProb << "\n";
	}

	ofs << "pairwise " << m_pairwiseModels.size() << "\n";

	for (auto iter = m_pairwiseModels.begin(); iter != m_pairwiseModels.end(); iter++)
	{
		ofs << m_relationName << ",";
		PairwiseRelationModel *relModel = iter->second;
		relModel->output(ofs);
	}
}

SupportRelation::SupportRelation(const QString &parentName, const QString &childName, const QString &supportType)
	:m_parentName(parentName), m_childName(childName), m_supportType(supportType), m_parentProbGivenChild(0), m_childProbGivenParent(0)
{
	m_suppRelKey =	m_parentName + "_" + m_childName + "_" + m_supportType;

	m_childInstanceNum = 0;
	m_parentInstanceNum = 0;
	m_jointInstanceNum = 0;
}

SupportRelation::~SupportRelation()
{

}

void SupportRelation::computeSupportProb()
{
	m_childProbGivenParent = m_jointInstanceNum / m_parentInstanceNum;
	m_parentProbGivenChild = m_jointInstanceNum / m_childInstanceNum;
}

void SupportRelation::output(QTextStream &ofs)
{
	ofs << m_suppRelKey << "\n";
	ofs << m_childProbGivenParent << " " << m_parentProbGivenChild << "\n";
}
