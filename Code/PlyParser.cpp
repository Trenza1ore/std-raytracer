#define _USE_MATH_DEFINES

#include "PlyParser.h"

PlyParser::PlyParser(const std::string& filename) : filePath(filename), vertexCount(0), faceCount(0) {}

std::vector<MyVector<float>> PlyParser::loadModel(MyVector<float>& scale, MyVector<float>& offset, MyVector<float> euler) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filePath << std::endl;
		return {};
	}

	std::string line;
	getline(file, line); // ply
	getline(file, line); // format ascii 1.0
	bool stopReadingProperty = false;
	while (getline(file, line)) {
		// Break at the end of header
		if (line.substr(0, 10) == "end_header") {
			break;
		}

		// Extract comments
		if (line.substr(0, 7) == "comment") {
			comment = line.substr(8);
		}

		// Extract vertex and face counts from the header
		std::istringstream iss(line);
		std::string attribute;
		while (iss >> attribute) {
			if (attribute == "element") {
				std::string type;
				iss >> type;
				if (type == "vertex") {
					iss >> vertexCount;
				}
				else if (type == "face") {
					iss >> faceCount;
					stopReadingProperty = true;
				}
				else if (type == "texture") {
					int temp;
					iss >> temp;
					if (temp != faceCount) {
						std::string errMsg = "Numbers of faces and texture coordinates do not match: " +
							std::to_string(faceCount) + " != " + std::to_string(temp);
						throw std::runtime_error(errMsg);
					}
					uvTextured = true;
				}
			}
			else if (attribute == "property" && (!stopReadingProperty)) {
				std::string property;
				iss >> property; // skip data type, assume float
				iss >> property;
				switch (property[0]) {
				case 'x': case 'y': case 'z':
					break;
				case 's': case 't':
					// Only need to read in ST coordinates once
					if (!stTextured) {
						stTextured = true;
						needST = true;
						uniqueST = std::vector<MyVector<float>>(vertexCount, MyVector<float>(2));
						printVec(stScale, "S-T coordinates scale");
					}
					else {
						needST = false;
					}
					stopReadingProperty = true;
					break;
				default:
					throw std::runtime_error("[!] PLYParser: Unknown property for vertex: " + property);
					break;
				}
			}
		}
	}

	// Convert degree to radians
	MyVector<double> eulerDouble = { euler[0], euler[1], euler[2] };
	eulerDouble *= M_PI;
	eulerDouble /= 180.0f;

	vertices = std::vector<MyVector<float>>(vertexCount, MyVector<float>(3));

	MyMatrix<double> rotMat = rotationMatrix(eulerDouble);

	for (size_t i = 0; i < vertexCount; i++) {
		// Retrieve vertex coordinates then apply these transformations: 
		// 1. scale, 2. rotate, 3. offset
		MyVector<double> temp = parseVertex(file, i).multiply(rotMat);
		// If blender UV mapping format is detected
		vertices[i] = { (float)temp[0], (float)temp[1], (float)temp[2] };
		vertices[i] *= scale;
		vertices[i] += offset;
	}

	return parseFaces(file, faceCount);
}

MyVector<double> PlyParser::parseVertex(std::ifstream& file, int i) {
	std::string line;
	std::getline(file, line);
	std::istringstream iss(line);

	MyVector<double> vertex(3);
	iss >> vertex[0] >> vertex[1] >> vertex[2];
	if (needST) parseST(iss, i);
	return vertex;
}

void PlyParser::parseST(std::istringstream& line, int i) {
	MyVector<float> vertexST(2);
	line >> vertexST[0] >> vertexST[1];
	uniqueST[i] = vertexST / stScale; // ST coordinates may need scaling
}

std::vector<int> PlyParser::parseIntElement(std::ifstream& file) {
	std::string line;
	std::getline(file, line);
	std::istringstream iss(line);
	unsigned int length;
	iss >> length;
	std::vector<int> elemList(length);
	for (unsigned int i = 0; i < length; i++) {
		iss >> elemList[i];
	}

	return elemList;
}

std::vector<float> PlyParser::parseFloatElement(std::ifstream& file) {
	std::string line;
	std::getline(file, line);
	std::istringstream iss(line);
	unsigned int length;
	iss >> length;
	std::vector<float> elemList(length);
	for (unsigned int i = 0; i < length; i++) {
		iss >> elemList[i];
	}

	return elemList;
}

std::vector<MyVector<float>> PlyParser::parseFaces(std::ifstream& file, int faceCount) {
	// Since not all vertices are used only once, the number of unique vertices (vertexCount)
	// is not equivalent to the actual number of vertices needed to render 
	// (also texture mapping is impossible with only unique vertices most of the time)
	size_t vertIdx, i, actualVertCount = faceCount * 3;
	std::vector<MyVector<float>> actualVertices(actualVertCount, MyVector<float>(3));
	vertTexs = std::vector<MyVector<float>>(actualVertCount, MyVector<float>(2));

	// Read the indices of unique vertices that make up each face
	vertIdx = 0;
	for (i = 0; i < faceCount; i++) {
		std::vector<int> indices = parseIntElement(file);
		if (stTextured) {
			vertTexs[vertIdx] = uniqueST[indices[0]];
			vertTexs[vertIdx + 1] = uniqueST[indices[1]];
			vertTexs[vertIdx + 2] = uniqueST[indices[2]];
		}
		actualVertices[vertIdx] = vertices[indices[0]];
		actualVertices[vertIdx + 1] = vertices[indices[1]];
		actualVertices[vertIdx + 2] = vertices[indices[2]];
		vertIdx += 3;
	}

	// Read the texture coordinates for each actual vertex
	if (uvTextured) {
		vertIdx = 0;
		for (i = 0; i < faceCount; i++) {
			std::vector<float> uv = parseFloatElement(file);
			vertTexs[vertIdx++] = { uv[0], uv[1] };
			vertTexs[vertIdx++] = { uv[2], uv[3] };
			vertTexs[vertIdx++] = { uv[4], uv[5] };
		}
	}

	return actualVertices;
}
