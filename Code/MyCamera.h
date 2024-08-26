#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <vector>
#include <stdexcept>
#include <memory>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cfloat>
#include <future>
#include "MyMatrix.h"
#include "MyVector.h"
#include "Ray.h"
#include "SceneObject.h"
#include "PlyParser.h"
#include "ImagePPM.h"
#include "ProgressBar.h"
#include "ImageProcessing.h"

enum RenderMode {
	scene,
	binary,
	phong,
	pathtracer,
	preview
};

class MyCamera {
public:
	MyCamera();
	bool readConfig(std::string filename);
	ImagePPM renderFrame(const std::string& savePath, int frameNo, int lastframe, RenderMode r);
	MyVector<float> traceRay(const RenderMode r, std::vector<HitInfo> hits, const MyVector<float>& lastPos, int current, int prev, int objNum, int lightNum, int frameNo, int depth);

	std::vector<std::vector<std::shared_ptr<SceneObject>>> objects;
	std::vector<std::vector<std::shared_ptr<Light>>> lights;
	std::vector<MyVector<float>> camMovement;
	std::vector<int> objCount, lightCount;
	RenderMode defaultRenderer;
	std::vector<uint8_t> bgChar;
	MyVector<float> bgVec;
	MyVector<float> ambientIntensity;
	MyMatrix<float> TranMat;
	int frames;
	unsigned int cameraSamples = 1;
	unsigned int shadowSamples = 1;
	float apertureDiameter = -1;
	std::string name;
	std::string type;
	bool simulateAperture = false;
	int width, height;
	void calibrateRays(int f);

protected:
	float hfWidth, hfHeight;
	const MyVector<float> DEFAULT = { 0, 0, 0 };
	MyVector<float> position;
	MyVector<float> lookAt;
	MyVector<float> upVector;
	MyVector<float> rightDir;
	MyVector<float> upDir;
	std::vector<Ray> rays;
	std::vector<float> wRay;
	float fov;
	float f;
	float exposure;
	float gamma;
	float Lw;
	int nbounce;
	std::default_random_engine e;
	std::uniform_real_distribution<float> uniformRand = std::uniform_real_distribution<float>(-1.0, 1.0);
	std::normal_distribution<float> normalRand = std::normal_distribution<float>(0.0f, 0.75f);
	std::vector<BVHNode> BVHTree;

	// Helper method for reading JSON
	void parseJSON(const nlohmann::json& j);
	void loadScene(const nlohmann::json& j);

	// Helper method for updating rays
	void generateTransformationMatrix();
	void generateRays();
	void updateRays(int f, int lastPos);
};

