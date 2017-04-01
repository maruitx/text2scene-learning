#include "RelationModel.h"
#include "engine.h"
#include "../common/utilities/eigenMat.h"

extern Engine *matlabEngine;

PairwiseRelationModel::PairwiseRelationModel(const QString &anchorName, const QString &actName, const QString &conditionName, const QString & relationName /*= "general"*/)
	:m_anchorObjName(anchorName), m_actObjName(actName), m_conditionName(conditionName), m_relationName(relationName)

{

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
	engEvalString(matlabEngine, "[isFitSuccess, mus, sigmas, weights, numComp] = fitGMMWithAIC(X, 4, alignMats);");

	printf("%s\n", buffer); // get error messages or prints (optional)

	mxArray *isFitSuccess = engGetVariable(matlabEngine, "isFitSuccess");
	mxArray *mus = engGetVariable(matlabEngine, "mus");
	mxArray *sigmas = engGetVariable(matlabEngine, "sigmas");
	mxArray *weights = engGetVariable(matlabEngine, "weights");
	mxArray *numComp = engGetVariable(matlabEngine, "numComp");

	int isGuassFitted = mxGetScalar(isFitSuccess);
	if (!isGuassFitted)
	{
		m_numGauss = 0;
		return;
	}

	int numGauss = mxGetScalar(numComp);
	Eigen::Map<Eigen::MatrixXd> gaussMus = fromMATd(mus);
	std::vector<Eigen::Map<Eigen::MatrixXd>> gaussSigmas = from3DimMATd(sigmas);
	Eigen::Map<Eigen::MatrixXd> gaussMixWeights = fromMATd(weights);

	for (int i=0; i < numGauss; i++)
	{
		GaussianModel newGauss;
		newGauss.dim = gaussMus.cols();

		newGauss.mean = gaussMus.row(i);
		newGauss.covarMat = gaussSigmas[i];
		
		newGauss.weight = gaussMixWeights(i);
		m_gaussians.push_back(newGauss);
	}

	m_numGauss = m_gaussians.size();
}

void PairwiseRelationModel::output(QTextStream &ofs)
{
	ofs << m_anchorObjName << "," << m_actObjName << "," << m_conditionName << "\n";
	ofs << m_numGauss << " " << m_numInstance << "\n";

	if (m_numGauss > 0)
	{
		for (int i = 0; i < m_numGauss; i++)
		{
			GaussianModel & currGauss = m_gaussians[i];
			ofs << currGauss.weight << ",";
			ofs << currGauss.mean(0) << " " << currGauss.mean(1) << " " << currGauss.mean(2) << " " << currGauss.mean(3) << ",";
			ofs << GetTransformationString(currGauss.covarMat) << "\n";
		}
	}

	else
	{
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

GroupRelationModel::GroupRelationModel()
{

}

GroupRelationModel::~GroupRelationModel()
{

}

void GroupRelationModel::fitGMMs()
{
	for (auto iter = m_pairwiseModels.begin(); iter!=m_pairwiseModels.end(); iter++)
	{
		PairwiseRelationModel *relModel = iter->second;
		relModel->fitGMM(15);
	}
}

void GroupRelationModel::output(QTextStream &ofs)
{

}
