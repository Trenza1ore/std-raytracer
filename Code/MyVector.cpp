#include "MyVector.h"
#include "MyMatrix.h"

template <typename T>
MyVector<T>::MyVector() : data(3), vecSize(3) {}

template <typename T>
MyVector<T>::MyVector(size_t size) : data(size), vecSize(size) {}

template<typename T>
MyVector<T>::MyVector(std::vector<T> data) : vecSize(data.size())
{
	this->data = data;
}

template <typename T>
MyVector<T>::MyVector(std::initializer_list<T> values) : data(values), vecSize(values.size()) {}

template<typename T>
MyVector<T>::MyVector(const MyVector<T>& vec) : data(vec.vecSize), vecSize(vec.vecSize)
{
	data = vec.data;
}

template <typename T>
MyVector<T>& MyVector<T>::operator=(std::initializer_list<T> values) {
	/*if (vecSize != values.size()) {
		throw std::runtime_error("Vectors must be the same size for assignment.");
	}*/
	data = values;
	return *this;
}

template <typename T>
MyVector<T>& MyVector<T>::operator=(const MyVector<T>& vec) {
	/*if (vecSize != vec.vecSize) {
		throw std::runtime_error("Vectors must be the same size for assignment.");
	}*/
	data = vec.data;
	return *this;
}

template <typename T>
MyVector<T> MyVector<T>::operator+(const MyVector<T>& other) const {
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for addition.");
	}*/
	MyVector<T> result(vecSize);
	for (size_t i = 0; i < vecSize; ++i) {
		result[i] = data[i] + other.data[i];
	}
	return result;
}

template <typename T>
MyVector<T> MyVector<T>::operator-(const MyVector<T>& other) const {
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for subtraction.");
	}*/
	MyVector<T> result(vecSize);
	for (size_t i = 0; i < vecSize; ++i) {
		result[i] = data[i] - other.data[i];
	}
	return result;
}

template <typename T>
MyVector<T> MyVector<T>::operator*(const T scalar) const {
	MyVector<T> result(vecSize);
	for (size_t i = 0; i < vecSize; ++i) {
		result[i] = data[i] * scalar;
	}
	return result;
}

template <typename T>
MyVector<T> MyVector<T>::operator*(const MyVector<T>& other) const {
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for multiplication.");
	}*/
	MyVector<T> result(vecSize);
	for (size_t i = 0; i < vecSize; ++i) {
		result[i] = data[i] * other.data[i];
	}
	return result;
}

template <typename T>
MyVector<T> MyVector<T>::operator/(const T scalar) const {
	MyVector<T> result(vecSize);
	for (size_t i = 0; i < vecSize; ++i) {
		result[i] = data[i] / scalar;
	}
	return result;
}

template<typename T>
MyVector<T> MyVector<T>::operator/(const MyVector<T>& other) const
{
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for division.");
	}*/
	MyVector<T> result(vecSize);
	for (size_t i = 0; i < vecSize; ++i) {
		result[i] = data[i] / other.data[i];
	}
	return result;
}

template <typename T>
MyVector<T>& MyVector<T>::operator+=(const MyVector<T>& other) {
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for addition.");
	}*/
	for (size_t i = 0; i < vecSize; ++i) {
		data[i] = data[i] + other.data[i];
	}
	return *this;
}

template <typename T>
MyVector<T>& MyVector<T>::operator-=(const MyVector<T>& other) {
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for addition.");
	}*/
	for (size_t i = 0; i < vecSize; ++i) {
		data[i] = data[i] - other.data[i];
	}
	return *this;
}

template <typename T>
MyVector<T>& MyVector<T>::operator*=(const T scalar) {
	for (size_t i = 0; i < vecSize; ++i) {
		data[i] = data[i] * scalar;
	}
	return *this;
}

template <typename T>
MyVector<T>& MyVector<T>::operator*=(const MyVector<T>& other) {
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for multiplication.");
	}*/
	for (size_t i = 0; i < vecSize; ++i) {
		data[i] = data[i] * other.data[i];
	}
	return *this;
}

template <typename T>
MyVector<T>& MyVector<T>::operator/=(const T scalar) {
	for (size_t i = 0; i < vecSize; ++i) {
		data[i] = data[i] / scalar;
	}
	return *this;
}

template<typename T>
MyVector<T>& MyVector<T>::operator/=(const MyVector<T>& other)
{
	/*if (vecSize != other.vecSize) {
		throw std::invalid_argument("Vectors must be the same size for division.");
	}*/
	for (size_t i = 0; i < vecSize; ++i) {
		data[i] = data[i] / other.data[i];
	}
	return *this;
}


template <typename T>
T MyVector<T>::dot(const MyVector<T>& other) const {
	/*if (this->vecSize != other.vecSize) {
		throw std::invalid_argument("Dot product is not defined for vectors of different sizes.");
	}*/

	T result = T{};
	for (size_t i = 0; i < this->vecSize; ++i) {
		result += this->data[i] * other.data[i];
	}
	return result;
}

template <typename T>
MyVector<T> MyVector<T>::cross(const MyVector<T>& other) const {
	/*if (this->vecSize != 3 || other.vecSize != 3) {
		throw std::invalid_argument("Cross product is only defined for 3-dimensional vectors.");
	}*/

	return MyVector<T>({
		this->data[1] * other.data[2] - this->data[2] * other.data[1],  // x
		this->data[2] * other.data[0] - this->data[0] * other.data[2],  // y
		this->data[0] * other.data[1] - this->data[1] * other.data[0]   // z
		});
}

template <typename T>
MyVector<T> MyVector<T>::multiply(const MyMatrix<T>& matrix) const {
	size_t colNum = matrix.getCols(), rowNum = matrix.getRows();
	/*if (vecSize != colNum) {
		throw std::invalid_argument("Vector size must match the number of columns of the matrix.");
	}*/

	MyVector<T> result(rowNum);
	for (size_t i = 0; i < rowNum; ++i) {
		result[i] = 0; // Initialize the element
		for (size_t j = 0; j < vecSize; ++j) {
			result[i] += matrix[i][j] * data[j];
		}
	}
	return result;
}

template <typename T>
MyVector<T> MyVector<T>::normalize() const {
	T norm = sqrt(this->dot(*this));
	/*if (norm <= 0) throw std::runtime_error("Cannot normalize zero-length vector."); */
	return (*this / norm);
}

template <typename T>
T MyVector<T>::length() const {
	T norm = sqrt(this->dot(*this));
	return norm;
}

template <typename T>
T& MyVector<T>::operator[](size_t index) {
	return data.at(index);
}

template <typename T>
const T& MyVector<T>::operator[](size_t index) const {
	return data.at(index);
}

template <typename T>
size_t MyVector<T>::size() const {
	return vecSize;
}

// Explicit template instantiation for expected types
template class MyVector<float>;
template class MyVector<double>;