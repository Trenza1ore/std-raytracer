#pragma once

#define KC 1.0f
#define KL 0.22f
#define KQ 0.2f

#include <vector>
#include <random>
#include "MyVector.h"
#include "SceneObject.h"

class Light {
public:
	Light() : isArea(false), kc(0), kl(0), kq(0) {};
	Light(MyVector<float>& position, MyVector<float>& intensity, 
		float kc, float kl, float kq, bool isArea);
	MyVector<float> intensityAt(const MyVector<float>& p) const;
	MyVector<float> flatIntensity() const;
	virtual const MyVector<float>& getPos();

	const bool isArea;
	const MyVector<float> position;  // Position of the light
	const MyVector<float> intensity;        // Intensity of the light (RGB components)
	const float kc, kl, kq;          // Attenuation factors: constant, linear, quadratic
};

class AreaLight : public Light {
public:

	AreaLight(MyVector<float>& position, MyVector<float>& intensity, 
		float kc, float kl, float kq, float sizeX, float sizeY, float sizeZ);
	const MyVector<float>& getPos() override;

	std::vector<MyVector<float>> randomPositions;
	unsigned int i = 0;
};