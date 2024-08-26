#pragma once

#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include "SceneObject.h"
#include "MyVector.h"

class PlyParser {
public:
    PlyParser(const std::string& filename);
    std::vector<MyVector<float>> loadModel(MyVector<float>& scale, MyVector<float>& offset, MyVector<float> euler);
    int vertexCount, faceCount;
    bool uvTextured = false;
    bool stTextured = false;
    std::string comment;
    MyVector<float> stScale = { 1, 1 };
    std::vector<MyVector<float>> vertTexs;
private:
    std::string filePath;
    std::vector<MyVector<float>> vertices;
    std::vector<MyVector<float>> uniqueST;
    MyVector<double> parseVertex(std::ifstream& file, int i);
    void parseST(std::istringstream& line, int i);
    std::vector<int> parseIntElement(std::ifstream& file);
    std::vector<float> parseFloatElement(std::ifstream& file);
    std::vector<MyVector<float>> parseFaces(std::ifstream& file, int faceCount);
    bool needST = false;
};