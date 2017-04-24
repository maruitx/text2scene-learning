#include "GaussianMixtureModel.h"

GaussianModel::GaussianModel(int d, double w, const Eigen::VectorXd &m, const Eigen::MatrixXd &c)
	:dim(d), weight(w), mean(m), covarMat(c)
{
}

GaussianModel::~GaussianModel()
{
}



GaussianMixtureModel::GaussianMixtureModel(int n)
	:m_numGauss(n)
{
	m_gaussians.resize(m_numGauss);
}

GaussianMixtureModel::~GaussianMixtureModel()
{
	for (int i = 0; i < m_numGauss; i++)
	{
		delete m_gaussians[i];
	}

	m_gaussians.clear();
}

