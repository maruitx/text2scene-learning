#pragma once

#include "../common/utilities/utility.h"
#include <Eigen/Dense>

class GaussianModel
{
public:
	GaussianModel(int d, double w, const Eigen::VectorXd &m, const Eigen::MatrixXd &c);
	~GaussianModel();

	int dim;
	double weight;   // mixing weight
	Eigen::VectorXd mean;
	Eigen::MatrixXd covarMat;

};

class GaussianMixtureModel
{
public:
	GaussianMixtureModel(int n);
	~GaussianMixtureModel();

	std::vector<GaussianModel*> m_gaussians;

	Eigen::VectorXd m_probTh; // probability value that X percentiles of observations have passed, currently X = [20 50 80]
	int m_numGauss;
};

