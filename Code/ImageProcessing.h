#pragma once

#include <algorithm>
#include <cmath>
#include <cfloat>
#include "SceneObject.h" // For my EPSILON definition
#include "MyVector.h"
#include "MyMatrix.h"
#include "ImagePPM.h"

// Convert color from [0, 1] to [0, 255] range with Direct3D data conversion rule
int intensity2RGB(double color);

// Batch convert color from [0, 1] to [0, 255] range with Direct3D data conversion rule
void colorDataConvert(MyVector<float>& color);

void NaiveToneMapping(ImagePPM& img, const MyMatrix<float>& R,
	const MyMatrix<float>& G, const MyMatrix<float>& B, double invertGamma);

void ReinhardToneMapping(std::vector<ImagePPM>& imgs, const MyMatrix<float>& R,
	const MyMatrix<float>& G, const MyMatrix<float>& B, std::vector<double> Lw,
	double invertGamma);