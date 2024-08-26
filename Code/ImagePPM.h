#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>

class ImagePPM {
private:

public:
	ImagePPM(int w, int h);
	ImagePPM();
	~ImagePPM();

	bool read(const std::string& filename);
	bool write(const std::string& filename) const;

	unsigned char getPixelValue(int x, int y, int channel) const;
	void setPixelValue(int x, int y, int channel, unsigned char value);

	// Set a rectangular region with a specific color
	void setRegion(int xStart, int yStart, int xEnd, int yEnd, const unsigned char color[3]);
	void setRegion(int xStart, int yStart, int xEnd, int yEnd, int channel, const unsigned char color);

	void drawTriangle(float coordinates[6], const unsigned char colors[9]);

	// Get and Set for entire channels
	std::vector<unsigned char> getChannel(int channel) const;
	void setChannel(int channel, const std::vector<unsigned char>& values);

	// Get and Set for all channels
	std::vector<unsigned char> getAllPixels() const;
	void setAllPixels(const std::vector<unsigned char>& values);


	int width, height;
	std::vector<unsigned char> data;
	std::string comment;
};