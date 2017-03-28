#include "RelationModel.h"
//#include <mlpack/core.hpp>
//#include <mlpack/methods/gmm/gmm.hpp>
#include "engine.h"
extern Engine *matlabEngine;

PairwiseRelationModel::PairwiseRelationModel(const QString &anchorName, const QString &actName, const QString &conditionName, const QString & relationName /*= "general"*/)
	:m_anchorObjName(anchorName), m_actObjName(actName), m_conditionName(conditionName), m_relationName(relationName)

{

}

void PairwiseRelationModel::fitGMM()
{
	// prepare data points
	int instanceNum = m_instances.size();

	if (instanceNum < 30)
	{
		return;
	}

	double *observations = new double[instanceNum*3];
	double *alignMats = new double[instanceNum * 16];

	for (int i=0; i< instanceNum; i++)
	{
		RelativePos &relPos = m_instances[i];
		observations[3 * i] = relPos.pos.x;
		observations[3 * i +1] = relPos.pos.y;
		observations[3 * i +2] = relPos.pos.z;

		MathLib::Matrix4d alignM = relPos.actAlignMat;
		for (int j=0; j < 16; j++)
		{
			alignMats[16 * i + j] = alignM.M[j];
		}
	}

	const int BUFSIZE = 1024;
	char buffer[BUFSIZE + 1];
	buffer[BUFSIZE] = '\0';
	engOutputBuffer(matlabEngine, buffer, BUFSIZE);

	// clear all variables (optional)
	engEvalString(matlabEngine, "clc;clear;"); 

	mxArray *observationArray = mxCreateDoubleMatrix(3, instanceNum, mxREAL);
	memcpy((void *)mxGetPr(observationArray), (void *)observations, sizeof(double)*3*instanceNum);
	engPutVariable(matlabEngine, "X", observationArray); // put into matlab

	mxArray *alignMatsArray = mxCreateDoubleMatrix(16, instanceNum, mxREAL);
	memcpy((void *)mxGetPr(alignMatsArray), (void *)alignMats, sizeof(double) * 16 * instanceNum);
	engPutVariable(matlabEngine, "alignMats", alignMatsArray); // put into matlab

	engEvalString(matlabEngine, "cd \'C:\\Ruim\\Graphics\\T2S_MPC\\text2scene-learning\\scene_lab'");
	engEvalString(matlabEngine, "[mu, sigma, numK] = fitGMMWithAIC(X, 4, alignMats)");

	printf("%s\n", buffer); // get error messages or prints (optional)

	mxArray *mu = engGetVariable(matlabEngine, "mu");
	mxArray *sigma = engGetVariable(matlabEngine, "sigma");
	mxArray *k = engGetVariable(matlabEngine, "numK");

	int numGauss = mxGetScalar(k);

	
}

GroupRelationModel::GroupRelationModel()
{

}


GroupRelationModel::~GroupRelationModel()
{

}
