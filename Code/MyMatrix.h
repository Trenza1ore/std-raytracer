#pragma once

#include <vector>
#include <stdexcept>
#include <algorithm>

template <typename T>
class MyMatrix {
private:
	size_t rows, cols;

public:
	MyMatrix() : rows(1), cols(1), data() {};
	MyMatrix(size_t rows, size_t cols, T initial);
	MyMatrix(const MyMatrix<T>& other); // Copy constructor
	~MyMatrix() = default;

	// Access operators
	std::vector<T>& operator[](size_t index);
	const std::vector<T>& operator[](size_t index) const;

	// Matrix operations
	MyMatrix<T> operator+(const MyMatrix<T>& rhs) const;
	MyMatrix<T> operator-(const MyMatrix<T>& rhs) const;
	MyMatrix<T> operator*(const MyMatrix<T>& rhs) const;
	MyMatrix<T> multiplyElementWise(const MyMatrix<T>& rhs) const;
	MyMatrix<T> transpose() const;

	// Utility functions
	size_t getRows() const;
	size_t getCols() const;
	std::vector<std::vector<T>> data;
	void fill(T value);
	void fill(T value, size_t xStart, size_t yStart, size_t xEnd, size_t yEnd);

	// Casting
	MyMatrix<char> toCharMatrix() const;
};