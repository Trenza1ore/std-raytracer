#include "Lights.h"
#include <cmath>

Light::Light(MyVector<float>& position, MyVector<float>& intensity, float kc, float kl, float kq, bool isArea)
    : position(position), kc(kc), kl(kl), kq(kq), intensity(intensity), isArea(isArea) {}

MyVector<float> Light::intensityAt(const MyVector<float>& p) const {
    // Compute the distance between the light source and the point
    float distance = (position - p).length();

    // Compute the attenuation
    float attenuation = 1.0f / (kc + kl * distance + kq * distance * distance);

    // Scale the intensity by the attenuation
    return intensity * attenuation;
}

MyVector<float> Light::flatIntensity() const {
    return intensity;
}

const MyVector<float>& Light::getPos()
{
    return position;
}

AreaLight::AreaLight(MyVector<float>& position, MyVector<float>& intensity,
    float kc, float kl, float kq, float sizeX, float sizeY, float sizeZ)
    : Light(position, intensity, kc, kl, kq, true)
{ 
    // X axis is always used, y and z are optional
    float xMin = position[0] - sizeX, zMin = position[2], yMin = position[1];
    if (sizeY > EPSILON) yMin -= sizeY; else sizeY = 0;
    if (sizeZ > EPSILON) zMin -= sizeZ; else sizeZ = 0;
    float xMax = xMin + sizeX * 2, yMax = yMin + sizeY * 2, zMax = zMin + sizeZ * 2;
    randomPositions = std::vector<MyVector<float>>(96000, MyVector<float>({ 0, 0, 0 }));
    std::default_random_engine e;
    std::uniform_real_distribution<float> 
        uniformX = std::uniform_real_distribution<float>(xMin, xMax),
        uniformY = std::uniform_real_distribution<float>(yMin, yMax),
        uniformZ = std::uniform_real_distribution<float>(zMin, zMax);

    // Sample 96000 random points within the area light to loop around
    for (int i = 0; i < 96000; i++) {
        randomPositions[i] = { uniformX(e), uniformY(e), uniformZ(e) };
    }
}

const MyVector<float>& AreaLight::getPos()
{
    if (i > 95999) i = 0;
    return randomPositions[i++];
}