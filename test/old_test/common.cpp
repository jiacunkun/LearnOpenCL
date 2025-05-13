#include "common.h"
#include <math.h>

QString	g_szCurPath = "";

double AngleToRadian(int nAngle)
{
	return M_PI * nAngle / 180.0;
}

int RadianToAngle(double fRadian)
{
	return (int)(180.0 * fRadian / M_PI);
}

int GetAngleFromPoints(float x1, float y1, float x2, float y2, float x3, float y3)
{
	int nDirection = 1;
	if (y3 > 0) {
		nDirection = (x2 < x1) ? -1 : 1;
	}
	else {
		nDirection = (x2 > x1) ? -1 : 1;
	}
	double ac = sqrt((x1 - x3)*(x1 - x3) + (y1 - y3)*(y1 - y3));// b
	double bc = sqrt((x2 - x3)*(x2 - x3) + (y2 - y3)*(y2 - y3));// a
	double ab = sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));// c

	double radian = nDirection * acos((ac*ac + bc*bc - ab*ab) / (2 * ac*bc));
	
	return RadianToAngle(radian);
}

int Random(int nMinValue, int nMaxValue)
{
	int minInteger = nMinValue;
	int maxInteger = nMaxValue;
	int randInteger = rand()*rand();
	int diffInteger = maxInteger - minInteger;
	int resultInteger = randInteger % diffInteger + minInteger;

	return resultInteger;
}
