#include "MyMatrix.h"

template <typename T>
MyMatrix<T>::MyMatrix(size_t rows, size_t cols, T initial)
	: rows(rows), cols(cols), data(rows, std::vector<T>(cols, initial)) {}

template <typename T>
MyMatrix<T>::MyMatrix(const MyMatrix<T>& other) : rows(other.rows), cols(other.cols), data(other.data) {}

template <typename T>
std::vector<T>& MyMatrix<T>::operator[](size_t index) {
	return data.at(index);
}

template <typename T>
const std::vector<T>& MyMatrix<T>::operator[](size_t index) const {
	return data.at(index);
}

template <typename T>
MyMatrix<T> MyMatrix<T>::operator+(const MyMatrix<T>& rhs) const {
	if (rows != rhs.rows || cols != rhs.cols) {
		throw std::invalid_argument("Matrices must have the same dimensions for addition.");
	}

	MyMatrix result(rows, cols, T{});
	for (size_t i = 0; i < rows; ++i) {
		for (size_t j = 0; j < cols; ++j) {
			result[i][j] = data[i][j] + rhs[i][j];
		}
	}
	return result;
}

template <typename T>
MyMatrix<T> MyMatrix<T>::operator-(const MyMatrix<T>& rhs) const {
	// Similar to operator+, with subtraction
	if (rows != rhs.rows || cols != rhs.cols) {
		throw std::invalid_argument("Matrices must have the same dimensions for addition.");
	}

	MyMatrix result(rows, cols, T{});
	for (size_t i = 0; i < rows; ++i) {
		for (size_t j = 0; j < cols; ++j) {
			result[i][j] = data[i][j] - rhs[i][j];
		}
	}
	return result;
}

template <typename T>
MyMatrix<T> MyMatrix<T>::operator*(const MyMatrix<T>& rhs) const {
	if (cols != rhs.rows) {
		throw std::invalid_argument("Invalid dimensions for matrix multiplication.");
	}

	MyMatrix result(rows, rhs.cols, T{});
	for (size_t i = 0; i < result.rows; ++i) {
		for (size_t j = 0; j < result.cols; ++j) {
			for (size_t k = 0; k < cols; ++k) {
				result[i][j] += data[i][k] * rhs[k][j];
			}
		}
	}
	return result;
}

template <typename T>
MyMatrix<T> MyMatrix<T>::multiplyElementWise(const MyMatrix<T>& rhs) const {
	// Similar to operator+, with element-wise multiplication
	if (rows != rhs.rows || cols != rhs.cols) {
		throw std::invalid_argument("Matrices must have the same dimensions for addition.");
	}

	MyMatrix result(rows, cols, T{});
	for (size_t i = 0; i < rows; ++i) {
		for (size_t j = 0; j < cols; ++j) {
			result[i][j] = data[i][j] * rhs[i][j];
		}
	}
	return result;
}

template <typename T>
MyMatrix<T> MyMatrix<T>::transpose() const {
	MyMatrix result(cols, rows, T{});
	for (size_t i = 0; i < cols; ++i) {
		for (size_t j = 0; j < rows; ++j) {
			result[i][j] = data[j][i];
		}
	}
	return result;
}

template <typename T>
size_t MyMatrix<T>::getRows() const {
	return rows;
}

template <typename T>
size_t MyMatrix<T>::getCols() const {
	return cols;
}

template <typename T>
void MyMatrix<T>::fill(T value) {
	for (auto& row : data) {
		std::fill(row.begin(), row.end(), value);
	}
}

template <typename T>
void MyMatrix<T>::fill(T value, size_t xStart, size_t yStart, size_t xEnd, size_t yEnd) {
	for (size_t i = xStart; i <= xEnd; ++i) {
		for (size_t j = yStart; j <= yEnd; ++j) {
			data[i][j] = value;
		}
	}
}

template <typename T>
MyMatrix<char> MyMatrix<T>::toCharMatrix() const {
	MyMatrix<char> result(rows, cols, char{});
	for (size_t i = 0; i < rows; ++i) {
		for (size_t j = 0; j < cols; ++j) {
			T clampedValue = std::clamp<T>(data[i][j], T{}, static_cast<T>(255));
			result[i][j] = static_cast<char>(clampedValue);
		}
	}
	return result;
}


// Explicit instantiation for expected types
template class MyMatrix<float>;
template class MyMatrix<double>;
template class MyMatrix<char>;
