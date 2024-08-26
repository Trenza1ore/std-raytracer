#pragma once

#include <string>
#include <vector>
#include <random>
#include "MyMatrix.h"
#include "MyVector.h"

class Ray {
public:
	Ray() : o(3), d(3) {}

	Ray(const MyVector<float>& origin, const MyVector<float>& direction)
	: o(origin), d(direction) {}

	MyVector<float> at(float t) const {
		return o + (d * t);
	}

	MyVector<float> o;
	MyVector<float> d;
};