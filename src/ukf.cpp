#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>
#include "tools.h"

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 2;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.3;
  
  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.
  
  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.
  
  Hint: one or more values initialized above might be wildly off...
  */
  is_initialized_ = false;

  time_us_ = 0.0;

  // State space dimension
  n_x_ = 5;

  // Augmented state dimension
  n_aug_ = 7;

  // sigma point spreading parameter
  lambda_ = 3 - n_x_;

  // Weights vector
  weights_ = VectorXd(2 * n_aug_ + 1);


  // Predicted sigma points
  Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);

}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
	if ((meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_) ||
		(meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_)) {
		
		if (!is_initialized_) {
			// Initiate the state vector
			x_ << 1, 1, 1, 1, 0.1;

			// Initiate the Covariance Matrix
			P_ << 0.15, 0, 0, 0, 0,
				  0, 0.15, 0, 0, 0,
				  0,    0, 1, 0, 0,
				  0,    0, 0, 1, 0,
				  0,    0, 0, 0, 1;

			// Use the first measurement info as the first state vector
			time_us_ = meas_package.timestamp_;

			if (meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_) {
				x_(0) = meas_package.raw_measurements_(0)* cos(meas_package.raw_measurements_(1));
				x_(1) = meas_package.raw_measurements_(0) * sin(meas_package.raw_measurements_(1));
			}
			else if (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_) {

				x_(0) = meas_package.raw_measurements_(0);
				x_(1) = meas_package.raw_measurements_(1);
			}

			is_initialized_ = true;

			return;
		}
		
		// After initialization run the prediction and update loop for UKF estimations

		float delta_t = (meas_package.timestamp_ - time_us_) / 1000000.0;
		time_us_ = meas_package.timestamp_;

		Prediction(delta_t);

		if (meas_package.sensor_type_ == MeasurementPackage::RADAR){
				UpdateRadar(meas_package);
		}
		else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {

				UpdateLidar(meas_package);
		}
	}
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
	// Generate the augmented state vector
	VectorXd x_aug = VectorXd(n_aug_);

	x_aug.head(n_x_) = x_;
	x_aug(5) = 0;
	x_aug(6) = 0;

	// Generate the augmented covariance matrix
	MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
	P_aug.fill(0.0);

	P_aug.topLeftCorner(n_x_, n_x_) = P_;
	P_aug(5,5) = std_a_ * std_a_;
	P_aug(6,6) = std_yawdd_ * std_yawdd_;

	// Square root of P_aug matrix
	MatrixXd L = P_aug.llt().matrixL();

	// Generate the augmented sigma points
	MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

	Xsig_aug.col(0) = x_aug;
	for (int i = 0; i< n_aug_; i++)
	{
		Xsig_aug.col(i + 1) = x_aug + sqrt(lambda_ + n_aug_) * L.col(i);
		Xsig_aug.col(i + 1 + n_aug_) = x_aug - sqrt(lambda_ + n_aug_) * L.col(i);
	}

	// Sigma point prediction
	//predict sigma points
	for (int i = 0; i < 2 * n_aug_ + 1; i++)
	{
		//extract values for better readability
		double p_x = Xsig_aug(0, i);
		double p_y = Xsig_aug(1, i);
		double v = Xsig_aug(2, i);
		double yaw = Xsig_aug(3, i);
		double yawd = Xsig_aug(4, i);
		double nu_a = Xsig_aug(5, i);
		double nu_yawdd = Xsig_aug(6, i);

		//predicted state values
		double px_p, py_p;

		//avoid division by zero
		if (fabs(yawd) > 0.001) {
			px_p = p_x + v / yawd * (sin(yaw + yawd * delta_t) - sin(yaw));
			py_p = p_y + v / yawd * (cos(yaw) - cos(yaw + yawd * delta_t));
		}
		else {
			px_p = p_x + v * delta_t * cos(yaw);
			py_p = p_y + v * delta_t * sin(yaw);
		}

		double v_p = v;
		double yaw_p = yaw + yawd * delta_t;
		double yawd_p = yawd;

		//add noise
		px_p = px_p + 0.5 * nu_a * delta_t * delta_t * cos(yaw);
		py_p = py_p + 0.5 * nu_a * delta_t * delta_t * sin(yaw);
		v_p = v_p + nu_a * delta_t;

		yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
		yawd_p = yawd_p + nu_yawdd * delta_t;

		//write predicted sigma point into right column
		Xsig_pred_(0, i) = px_p;
		Xsig_pred_(1, i) = py_p;
		Xsig_pred_(2, i) = v_p;
		Xsig_pred_(3, i) = yaw_p;
		Xsig_pred_(4, i) = yawd_p;
	}

	// Predic the mean and the covariance matrix
	// Set Weights
	double weight_0 = lambda_ / (lambda_ + n_aug_);

	weights_(0) = weight_0;

	for (int i = 1; i < 2 * n_aug_ + 1; i++) {  //2n+1 weights
		double weight = 0.5 / (n_aug_ + lambda_);
		weights_(i) = weight;
	}

	//predicted state mean
	x_.fill(0.0);             
	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points
		x_ = x_ + weights_(i) * Xsig_pred_.col(i);
	}

	//predicted state covariance matrix
	P_.fill(0.0);             
	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points

												// state difference
		VectorXd x_diff = Xsig_pred_.col(i) - x_;
		//angle normalization
		while (x_diff(3)> M_PI) x_diff(3) -= 2.*M_PI;
		while (x_diff(3)<-M_PI) x_diff(3) += 2.*M_PI;

		P_ = P_ + weights_(i) * x_diff * x_diff.transpose();
	}

}


/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
	int n_z = 2;
	VectorXd Z = meas_package.raw_measurements_;

	MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);


	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
									
		Zsig(0, i) = Xsig_pred_(0, i);
		Zsig(1, i) = Xsig_pred_(1, i);                             

	}

	//mean predicted measurement
	VectorXd z_pred = VectorXd(n_z);

	z_pred.fill(0.0);

	for (int i = 0; i < 2 * n_aug_ + 1; i++) {
		z_pred = z_pred + weights_(i) * Zsig.col(i);
	}

	//innovation covariance matrix S
	MatrixXd S = MatrixXd(n_z, n_z);

	S.fill(0.0);

	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
												//residual
		VectorXd z_diff = Zsig.col(i) - z_pred;

		S = S + weights_(i) * z_diff * z_diff.transpose();
	}
	//add measurement noise covariance matrix
	MatrixXd R = MatrixXd(n_z, n_z);

	R << std_laspx_ * std_laspx_, 0,
		0, std_laspy_*std_laspy_;
	S = S + R;

	// Update the state and the covariance matrix
	MatrixXd Tc = MatrixXd(n_x_, n_z);
	Tc.fill(0.0);

	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

		//residual
		VectorXd z_diff = Zsig.col(i) - z_pred;


		// state difference
		VectorXd x_diff = Xsig_pred_.col(i) - x_;


		Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
	}

	//Kalman gain K;
	MatrixXd K = Tc * S.inverse();

	//residual
	VectorXd z_diff = Z - z_pred;

	//update state mean and covariance matrix
	x_ = x_ + K * z_diff;
	P_ = P_ - K * S* K.transpose();


}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */

	VectorXd Z = meas_package.raw_measurements_;

	int n_z = 3;

	MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);



	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

											   // extract values for better readibility
		double p_x = Xsig_pred_(0, i);
		double p_y = Xsig_pred_(1, i);
		double v = Xsig_pred_(2, i);
		double yaw = Xsig_pred_(3, i);

		double v1 = cos(yaw)*v;
		double v2 = sin(yaw)*v;

		// measurement model
		Zsig(0, i) = sqrt(p_x*p_x + p_y * p_y);                        //r
		Zsig(1, i) = atan2(p_y, p_x);                                 //phi
		Zsig(2, i) = (p_x*v1 + p_y * v2) / sqrt(p_x*p_x + p_y * p_y);   //r_dot
	}

	//mean predicted measurement
	VectorXd z_pred = VectorXd(n_z);
	z_pred.fill(0.0);
	for (int i = 0; i < 2 * n_aug_ + 1; i++) {
		z_pred = z_pred + weights_(i) * Zsig.col(i);
	}

	//innovation covariance matrix S
	MatrixXd S = MatrixXd(n_z, n_z);
	S.fill(0.0);
	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
											   //residual
		VectorXd z_diff = Zsig.col(i) - z_pred;

		//angle normalization
		while (z_diff(1)> M_PI) z_diff(1) -= 2.*M_PI;
		while (z_diff(1)<-M_PI) z_diff(1) += 2.*M_PI;

		S = S + weights_(i) * z_diff * z_diff.transpose();
	}
	//add measurement noise covariance matrix
	MatrixXd R = MatrixXd(n_z, n_z);
	R << std_radr_ * std_radr_, 0, 0,
		0, std_radphi_*std_radphi_, 0,
		0, 0, std_radrd_*std_radrd_;
	S = S + R;

	// Update the state and the covariance matrix
	MatrixXd Tc = MatrixXd(n_x_, n_z);
	Tc.fill(0.0);

	for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

											   //residual
		VectorXd z_diff = Zsig.col(i) - z_pred;
		//angle normalization
		while (z_diff(1)> M_PI) z_diff(1) -= 2.*M_PI;
		while (z_diff(1)<-M_PI) z_diff(1) += 2.*M_PI;

		// state difference
		VectorXd x_diff = Xsig_pred_.col(i) - x_;
		//angle normalization
		while (x_diff(3)> M_PI) x_diff(3) -= 2.*M_PI;
		while (x_diff(3)<-M_PI) x_diff(3) += 2.*M_PI;

		Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
	}

	//Kalman gain K;
	MatrixXd K = Tc * S.inverse();

	//residual
	VectorXd z_diff = Z - z_pred;

	//angle normalization
	while (z_diff(1)> M_PI) z_diff(1) -= 2.*M_PI;
	while (z_diff(1)<-M_PI) z_diff(1) += 2.*M_PI;

	//update state mean and covariance matrix
	x_ = x_ + K * z_diff;
	P_ = P_ - K * S* K.transpose();

}