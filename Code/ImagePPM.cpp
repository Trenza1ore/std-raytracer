#include "ImagePPM.h"

float edgeFunc(float x0, float y0, float x1, float y1, float x2, float y2) {
	return ((x2 - x0) * (y1 - y0)) - ((y2 - y0) * (x1 - x0));
}

bool inTriangle(float point[2], float triangle[6]) {
	// Create shorter names for readability
	float   x0 = triangle[0], y0 = triangle[1], x1 = triangle[2],
		y1 = triangle[3], x2 = triangle[4], y2 = triangle[5],
		xp = point[0], yp = point[1];
	// Use Barycentric Technique for checking if a point is inside a triangle
	bool    b0, b1, b2;
	b0 = edgeFunc(x0, y0, x1, y1, xp, yp) < 0.0f;
	b1 = edgeFunc(x1, y1, x2, y2, xp, yp) < 0.0f;
	b2 = edgeFunc(x2, y2, x0, y0, xp, yp) < 0.0f;
	return (b0 == b1) && (b1 == b2);
}

ImagePPM::ImagePPM(int w, int h) : width(w), height(h), data(w* h * 3, 0) {}
ImagePPM::ImagePPM() : width(0), height(0), data(1, 0) {}

ImagePPM::~ImagePPM() {}

bool ImagePPM::read(const std::string& filename) {
	std::ifstream infile(filename, std::ios::binary);
	if (!infile.is_open()) {
		return false;
	}

	std::string header;
	int maxColor;
	infile >> header;

	std::getline(infile, comment);
	std::getline(infile, comment);

	if (comment[0] != '#') {
		std::istringstream line(comment);
		line >> width >> height;
		comment = "No comment";
	}
	else {
		infile >> width >> height;
	}

	infile >> maxColor;
	infile.ignore();

	if (header != "P6" || maxColor != 255) {
		infile.close();
		return false;
	}

	data.resize(width * height * 3);
	infile.read(reinterpret_cast<char*>(data.data()), data.size());

	infile.close();
	return true;
}

bool ImagePPM::write(const std::string& filename) const {
	std::ofstream outfile(filename, std::ios::binary);
	if (!outfile.is_open()) {
		return false;
	}

	outfile << "P6\n" << "# Rendered by Hugo's ray-caster v1.0 for CGR CW2, Nov 2023\n" << width << " " << height << "\n255\n";
	outfile.write(reinterpret_cast<const char*>(data.data()), data.size());

	outfile.close();
	return true;
}

unsigned char ImagePPM::getPixelValue(int x, int y, int channel) const {
	return data[(y * width + x) * 3 + channel];
}

void ImagePPM::setPixelValue(int x, int y, int channel, unsigned char value) {
	data[(y * width + x) * 3 + channel] = value;
}

void ImagePPM::setRegion(int xStart, int yStart, int xEnd, int yEnd, const unsigned char color[3]) {
	if (xStart < 0 || xEnd > width || yStart < 0 || yEnd > height) {
		throw std::out_of_range("Region coordinates are out of the image bounds.");
	}
	for (int y = yStart; y < yEnd; ++y) {
		for (int x = xStart; x < xEnd; ++x) {
			int index = (y * width + x) * 3;
			data[index] = color[0];     // Red
			data[index + 1] = color[1]; // Green
			data[index + 2] = color[2]; // Blue
		}
	}
}

void ImagePPM::setRegion(int xStart, int yStart, int xEnd, int yEnd, int channel, const unsigned char color) {
	if (xStart < 0 || xEnd > width || yStart < 0 || yEnd > height || channel > 2 || channel < 0) {
		throw std::out_of_range("Channel index or region coordinates out of bounds.");
	}
	for (int y = yStart; y < yEnd; ++y) {
		for (int x = xStart; x < xEnd; ++x) {
			int index = (y * width + x) * 3;
			data[index + channel] = color;
		}
	}
}

void ImagePPM::drawTriangle(float coordinates[6], const unsigned char colors[9]) {
	// Create shorter names for readability
	float   x0 = coordinates[0], y0 = coordinates[1], x1 = coordinates[2],
		y1 = coordinates[3], x2 = coordinates[4], y2 = coordinates[5];

	// Implement back-face culling by not drawing "clockwise" triangles like OpenGL
	//if ((x0 * (y1 - y2) + x1 * (y2 - y1) + x2 * (y0 - y1)) > 0) return;

	float point[2], w[3], area;
	int xMin, yMin, xMax, yMax, x, y, i;

	area = edgeFunc(x0, y0, x1, y1, x2, y2);

	xMin = static_cast<int>(std::lrint(std::clamp(std::min(std::min(x0, x1), x2), 0.0f, static_cast<float>((width)))));
	xMax = static_cast<int>(std::lrint(std::clamp(std::max(std::max(x0, x1), x2), 0.0f, static_cast<float>((width)))));
	yMin = static_cast<int>(std::lrint(std::clamp(std::min(std::min(y0, y1), y2), 0.0f, static_cast<float>((height)))));
	yMax = static_cast<int>(std::lrint(std::clamp(std::max(std::max(y0, y1), y2), 0.0f, static_cast<float>((height)))));

	for (y = yMin; y < yMax; y++) {
		point[1] = y + 0.5f;
		for (x = xMin; x < xMax; x++) {
			point[0] = x + 0.5f;
			if (inTriangle(point, coordinates)) {
				// Edge function for barycentric coordinates, normalize weight
				w[0] = edgeFunc(x1, y1, x2, y2, point[0], point[1]) / area;
				w[1] = edgeFunc(x2, y2, x0, y0, point[0], point[1]) / area;
				w[2] = edgeFunc(x0, y0, x1, y1, point[0], point[1]) / area;

				unsigned char r = 0, g = 0, b = 0;
				for (i = 0; i < 3; i++) {
					int flattenIndex = i * 3; // index for 1D colors array
					r += w[i] * colors[flattenIndex++];
					g += w[i] * colors[flattenIndex++];
					b += w[i] * colors[flattenIndex];
				}

				i = (y * width + x) * 3;
				data[i++] = r;  // Red
				data[i++] = g;  // Green
				data[i] = b;    // Blue
			}
		}
	}
}

std::vector<unsigned char> ImagePPM::getChannel(int channel) const {
	std::vector<unsigned char> channelData(width * height);
	for (int i = 0; i < width * height; ++i) {
		channelData[i] = data[i * 3 + channel];
	}
	return channelData;
}

void ImagePPM::setChannel(int channel, const std::vector<unsigned char>& values) {
	if (values.size() != static_cast<size_t>(width * height)) {
		throw std::invalid_argument("Channel data size does not match image dimensions.");
	}
	for (int i = 0; i < width * height; ++i) {
		data[i * 3 + channel] = values[i];
	}
}

std::vector<unsigned char> ImagePPM::getAllPixels() const {
	std::vector<unsigned char> clone(data);
	return clone;
}

void ImagePPM::setAllPixels(const std::vector<unsigned char>& values) {
	if (values.size() != data.size()) {
		throw std::invalid_argument("Input data size does not match image data size.");
	}
	data = values;
}