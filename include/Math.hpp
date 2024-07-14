#pragma once
#include <cmath>

// M_PI is not defined in C++ standard
#define M_PI 3.14159265358979323846

class Math
{
public:
	static double Radians( double degrees )
	{
		return degrees * M_PI / 180.0;
	}

	static double Degrees( double radians )
	{
		return radians * 180.0 / M_PI;
	}

};