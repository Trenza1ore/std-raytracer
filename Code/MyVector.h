#pragma once

#include <vector>
#include <cmath>
#include <stdexcept>
#include <initializer_list>
#include <iostream>
#include <string>
#include "MyMatrix.h"

template <typename T>
class MyVector {
public:
	// Constructors
	MyVector();
	explicit MyVector(size_t size);
	explicit MyVector(std::vector<T> data);
	MyVector(std::initializer_list<T> values);
	MyVector(const MyVector<T>& vec);

	MyVector<T>& operator=(std::initializer_list<T> values);
	MyVector<T>& operator=(const MyVector<T>& vec);

	// Vector operations
	MyVector<T> operator+(const MyVector<T>& other) const;
	MyVector<T> operator-(const MyVector<T>& other) const;
	MyVector<T> operator*(const T scalar) const;
	MyVector<T> operator*(const MyVector<T>& other) const;
	MyVector<T> operator/(const T scalar) const;
	MyVector<T> operator/(const MyVector<T>& other) const;
	MyVector<T>& operator+=(const MyVector<T>& other);
	MyVector<T>& operator-=(const MyVector<T>& other);
	MyVector<T>& operator*=(const T scalar);
	MyVector<T>& operator*=(const MyVector<T>& other);
	MyVector<T>& operator/=(const T scalar);
	MyVector<T>& operator/=(const MyVector<T>& other);
	T dot(const MyVector<T>& other) const;
	MyVector<T> cross(const MyVector<T>& other) const;

	// Method for multiplying this vector by a given matrix
	MyVector<T> multiply(const MyMatrix<T>& matrix) const;

	// Normalization
	MyVector<T> normalize() const;

	T length() const;

	// Access operators
	T& operator[](size_t index);
	const T& operator[](size_t index) const;

	// Size of the vector
	size_t size() const;
	const size_t vecSize;

	std::vector<T> data;
};

// Compute rotation matrix for rotating at euler angles (X, Y, Z)
template <typename T>
MyMatrix<T> rotationMatrix(const MyVector<T> euler) {
	T sinX = sin(euler[0]), sinY = sin(euler[1]), sinZ = sin(euler[2]);
	T cosX = cos(euler[0]), cosY = cos(euler[1]), cosZ = cos(euler[2]);
	MyMatrix<T> rotMat(3, 3, 0.0);
	rotMat[0] = { cosZ * cosY  , cosZ * sinY * sinX - sinZ * cosX  , cosZ * sinY * cosX + sinZ * sinX };
	rotMat[1] = { sinZ * cosY  , sinZ * sinY * sinX + cosZ * cosX  , sinZ * sinY * cosX - cosZ * sinX };
	rotMat[2] = { -sinY        , cosY * sinX                       , cosY * cosX };

	return rotMat;
};

// Print vectors out
template <typename T>
void printVec(const MyVector<T>& vec, std::string name) {
	std::cout << name << ": ( " << vec[0];
	for (int i = 1; i < vec.size(); i++) {
		std::cout << ", " << vec[i];
	}
	std::cout << " )" << std::endl;
};