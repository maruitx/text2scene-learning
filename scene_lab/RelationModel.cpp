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

	for (int i=0; i< instanceNum; i++)
	{
		observations[3 * i] = m_instances[i].pos.x;
		observations[3 * i +1] = m_instances[i].pos.y;
		observations[3 * i +2] = m_instances[i].pos.z;
	}

	const int BUFSIZE = 1024;
	char buffer[BUFSIZE + 1];
	buffer[BUFSIZE] = '\0';
	engOutputBuffer(matlabEngine, buffer, BUFSIZE);

	mxArray *observationArray = mxCreateDoubleMatrix(3, instanceNum, mxREAL);
	memcpy((void *)mxGetPr(observationArray), (void *)observations, sizeof(double)*3*instanceNum);
	engPutVariable(matlabEngine, "X", observationArray); // put into matlab

	engEvalString(matlabEngine, "clc;clear;"); // clear all variables (optional)
	engEvalString(matlabEngine, "cd \'C:\\Ruim\\Graphics\\T2S_MPC\\text2scene-learning\\scene_lab'");
	engEvalString(matlabEngine, "[mu, sigma, numK] = fitGMMWithAIC(X, 4)");

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
