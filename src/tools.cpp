#include <iostream>
#include "tools.h"

using Eigen::VectorXd;
using Eigen::MatrixXd;
using std::vector;

Tools::Tools() {}

Tools::~Tools() {}

VectorXd Tools::CalculateRMSE(const vector<VectorXd> &estimations,
                              const vector<VectorXd> &ground_truth) {
  /**
  TODO:
    * Calculate the RMSE here.
  */
	VectorXd rmse(4);
	rmse << 0, 0, 0, 0;

	if (estimations.size() != ground_truth.size() || estimations.size() == 0)
	{
		cout << " Invalid estimation or ground_truth data" << endl;
		return rmse;
	}

	for (unsigned int i = 0; i < estimations.size(); ++i)
	{
		VectorXd error = estimations[i] - ground_truth[i];

		error = error.array()*error.array();
		rmse += error;
	}

	rmse = rmse / (estimations.size());
	rmse = rmse.array().sqrt();

	return rmse;
}