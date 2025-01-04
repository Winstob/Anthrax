/* ---------------------------------------------------------------- *\
 * quaternion.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-17
\* ---------------------------------------------------------------- */

#include "quaternion.hpp"

#include <cmath>
#include <iostream>

namespace Anthrax
{

Quaternion::Quaternion(float x, float y, float z, float w)
{
	x_ = x;
	y_ = y;
	z_ = z;
	w_ = w;
	return;
}


Quaternion::Quaternion(float yaw, float pitch, float roll)
{
	float cy = cos(yaw / 2.0);
	float sy = sin(yaw / 2.0);
	float cp = cos(pitch / 2.0);
	float sp = sin(pitch / 2.0);
	float cr = cos(roll / 2.0);
	float sr = sin(roll / 2.0);

	w_ = cy * cp * cr + sy * sp * sr;
	x_ = cy * cp * sr - sy * sp * cr;
	y_ = cy * sp * cr + sy * cp * sr;
	z_ = sy * cp * cr - cy * sp * sr;
	return;
}


void Quaternion::copy(const Quaternion &other)
{
	x_ = other.x_;
	y_ = other.y_;
	z_ = other.z_;
	w_ = other.w_;
	return;
}


float Quaternion::operator[](size_t i)
{
	switch (i)
	{
		case 0:
			return x_;
		case 1:
			return y_;
		case 2:
			return z_;
		case 3:
			return w_;
		default:
			throw std::runtime_error("Quaternion indexed out of bounds!");
	}
}


void Quaternion::normalize()
{
	float d = sqrt(x_*x_ + y_*y_ + z_*z_ + w_*w_);
	x_ /= d;
	y_ /= d;
	z_ /= d;
	w_ /= d;
	return;
}

/******************************************\
 * Ordering of returned vector is:
 *	Yaw
 *	Pitch
 *	Roll
\******************************************/
std::vector<float> Quaternion::eulerAngles()
{
	std::vector<float> euler_angles(3);
	euler_angles[0] = atan2(x_+z_, w_-y_) + atan2(z_-x_, y_+w_);
	euler_angles[1] = acos((w_-y_)*(w_-y_) + (x_+z_)*(x_+z_) - 1.0) - PI/2.0;
	euler_angles[2] = atan2(x_+z_, w_-y_) - atan2(z_-x_, y_+w_);
	return euler_angles;
}

} // namespace Anthrax
