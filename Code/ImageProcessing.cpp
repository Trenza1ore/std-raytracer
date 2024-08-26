#include "ImageProcessing.h"

// Convert color from [0, 1] to [0, 255] range with Direct3D data conversion rule
int intensity2RGB(double color) {
	return std::clamp<int>(std::round(color * 255), 0, 255);
}

// Batch convert color from [0, 1] to [0, 255] range with Direct3D data conversion rule
void colorDataConvert(MyVector<float>& color) {
	for (int i = 0; i < 3; i++) {
		color[i] = std::clamp<float>(std::round(color[i] * 255), 0, 255);
	}
}

void NaiveToneMapping(ImagePPM& img, const MyMatrix<float>& R,
	const MyMatrix<float>& G, const MyMatrix<float>& B, double invertGamma) {

	// Use standard sRGB (with D65 illuminant) to XYZ matrices
	MyMatrix<double> RGB2XYZ(3, 3, 0.0), XYZ2RGB(3, 3, 0.0);
	RGB2XYZ[0] = { 0.4124564,  0.3575761,  0.1804375 };
	RGB2XYZ[1] = { 0.2126729,  0.7151522,  0.0721750 };
	RGB2XYZ[2] = { 0.0193339,  0.1191920,  0.9503041 };

	XYZ2RGB[0] = { 3.2404542, -1.5371385, -0.4985314 };
	XYZ2RGB[1] = { -0.9692660,  1.8760108,  0.0415560 };
	XYZ2RGB[2] = { 0.0556434, -0.2040259,  1.0572252 };

	int x, y, width = R.getRows(), height = R.getCols();
	std::vector<std::vector<MyVector<double>>> rgb(width,
		std::vector<MyVector<double>>(height));

	MyMatrix<double> L(width, height, 0.0);
	double minLuminance = FLT_MAX, maxLuminance = -FLT_MAX;
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			rgb[x][y] = { R[x][y], G[x][y], B[x][y] };
			MyVector<double> xyz = rgb[x][y].multiply(RGB2XYZ);
			double luminance = xyz[1];
			L[x][y] = luminance;
			if (luminance < minLuminance)
				minLuminance = luminance;
			if (luminance > maxLuminance)
				maxLuminance = luminance;
		}
	}

	double luminanceRange = maxLuminance - minLuminance;

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			L[x][y] -= minLuminance;
			L[x][y] /= luminanceRange;
		}
	}

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			float mappedLuminance = L[x][y] - minLuminance / luminanceRange;
			rgb[x][y] *= (mappedLuminance / L[x][y]);
			for (int i = 0; i < 3; i++) {
				double gammaCorrect = pow(rgb[x][y][i], invertGamma);
				img.setPixelValue(x, y, i, intensity2RGB(gammaCorrect));
			}
		}
	}
}

void ReinhardToneMapping(std::vector<ImagePPM>& imgs, const MyMatrix<float>& R,
	const MyMatrix<float>& G, const MyMatrix<float>& B, std::vector<double> Lw,
	double invertGamma) {

	// Use standard sRGB (with D65 illuminant) to XYZ matrices
	MyMatrix<double> RGB2XYZ(3, 3, 0.0), XYZ2RGB(3, 3, 0.0);
	RGB2XYZ[0] = { 0.4124564,  0.3575761,  0.1804375 };
	RGB2XYZ[1] = { 0.2126729,  0.7151522,  0.0721750 };
	RGB2XYZ[2] = { 0.0193339,  0.1191920,  0.9503041 };

	XYZ2RGB[0] = { 3.2404542, -1.5371385, -0.4985314 };
	XYZ2RGB[1] = { -0.9692660,  1.8760108,  0.0415560 };
	XYZ2RGB[2] = { 0.0556434, -0.2040259,  1.0572252 };

	int x, y, width = R.getRows(), height = R.getCols();
	std::vector<std::vector<MyVector<double>>> rgb(width,
		std::vector<MyVector<double>>(height));

	MyMatrix<double> L(width, height, 0.0);
	double avgLuminance = 0;
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			rgb[x][y] = { R[x][y], G[x][y], B[x][y] };
			MyVector<double> xyz = rgb[x][y].multiply(RGB2XYZ);
			L[x][y] = xyz[1];
			avgLuminance += xyz[1];
		}
	}
	avgLuminance /= (width * height);
	avgLuminance *= 9.6;
	avgLuminance = std::max((double)EPSILON, avgLuminance);

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			L[x][y] /= avgLuminance;
		}
	}

	int imgIdx = 0;

	for (double LwSq : Lw) {
		MyMatrix<double> mappedL = MyMatrix<double>(L);
		// Apply more advanced reinhard curve with L white
		if (LwSq > EPSILON) {
			LwSq *= LwSq;
			for (x = 0; x < width; x++) {
				for (y = 0; y < height; y++) {
					double L_ = L[x][y];
					mappedL[x][y] = L_ * (1 + L_ / LwSq) / (1 + L_);
				}
			}
		}
		// Apply a naive approach that normalizes luminance
		else if (LwSq < 0) {
			NaiveToneMapping(imgs[imgIdx], R, G, B, invertGamma);
		}
		// Apply basic reinhard curve
		else
		{
			for (x = 0; x < width; x++) {
				for (y = 0; y < height; y++) {
					mappedL[x][y] = L[x][y] / (1 + L[x][y]);
				}
			}
		}

		for (x = 0; x < width; x++) {
			for (y = 0; y < height; y++) {
				rgb[x][y] *= (mappedL[x][y] / L[x][y]);
				for (int i = 0; i < 3; i++) {
					double gammaCorrect = pow(rgb[x][y][i], invertGamma);
					imgs[imgIdx].setPixelValue(x, y, i, intensity2RGB(gammaCorrect));
				}
			}
		}
		imgIdx++;
	}
}