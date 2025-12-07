#pragma once

#define EPSILON 1e-4f
#define EPS_INV 1e+4f

#include <memory>
#include <cfloat>
#include <utility>
#include "Ray.h"
#include "MyMatrix.h"
#include "MyVector.h"
#include "Material.h"
#include "HitInfo.h"
#include "Lights.h"
#include "PlyParser.h"

class SceneObject {
public:
    // unique ptr of classes that inherit from abstract class can have issues with destructor
    virtual ~SceneObject() = default;

    SceneObject() {};
    SceneObject(MyVector<float>& center) : center(center) {};

    virtual HitInfo intersect(const Ray& ray, bool useUV) = 0;
    virtual std::vector<float> getBoundingBox() = 0;

    void setMaterial(std::shared_ptr<Material>& material) {
        this->material = material;
    }

    MyVector<float> center = { 0, 0, 0 };
    int index = -1;
    std::shared_ptr<Material> material;
};

class Triangle : public SceneObject {
public:
    Triangle();
    Triangle(MyVector<float>& v0, MyVector<float>& v1, MyVector<float>& v2, bool singleSided);
    virtual HitInfo hitWithUV(float dist, MyVector<float>& hitPos, const MyVector<float>& normal);
    HitInfo intersect(const Ray& ray, bool useUV) override;
    std::vector<float> getBoundingBox() override;

    const MyVector<float> v0, v1, v2, edge1, edge2, n, m;
};

class TriFace : public Triangle {
public:
    TriFace(MyVector<float>& v0, MyVector<float>& v1, MyVector<float>& v2, MyVector<float>& uv0, MyVector<float>& uv1, MyVector<float>& uv2);
    HitInfo hitWithUV(float dist, MyVector<float>& hitPos, const MyVector<float>& normal) override;

protected:
    const float nLen;
    const MyVector<float> uv0, uv1, uv2;
};

class Sphere : public SceneObject {
public:
    Sphere(MyVector<float>& center, float radius);
    virtual HitInfo hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal);
    HitInfo intersect(const Ray& ray, bool useUV) override;
    std::vector<float> getBoundingBox() override;

    const float radius;
    const float r2;
};

class SphericalMap : public Sphere {
public:
    SphericalMap(MyVector<float>& center, float radius, MyVector<float> euler);
    virtual HitInfo hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal) override;

    const MyMatrix<float> rotMat;
};

class UVSphere : public SphericalMap {
public:
    using SphericalMap::SphericalMap; // reuse constructors
    HitInfo hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal) override;
};

class CubeMap : public SphericalMap {
public:
    using SphericalMap::SphericalMap; // reuse constructors
    HitInfo hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal) override;
};

class Cylinder : public SceneObject {
public:
    Cylinder(MyVector<float>& center, float radius, float height, MyVector<float>& axis, MyMatrix<float> rotMat);
    HitInfo hitWithUV(float dist, MyVector<float>& hitPos, float z);
    HitInfo intersect(const Ray& ray, bool useUV) override;
    std::vector<float> getBoundingBox() override;

protected:
    const MyMatrix<float> rotMat;
    const float radius, height, r2, uBound, lBound;
    const MyVector<float> axis, axisInv;
};

class BVHLeaf {
public:
    int objIndex = -1;

    BVHLeaf() {};
    BVHLeaf(std::shared_ptr<SceneObject> &obj) : objIndex(obj->index) {};

    virtual void check(std::vector<int> &collideList, const Ray &ray) {
        if (objIndex > -1)
            collideList.push_back(objIndex);
    }
};

class BVHNode : public BVHLeaf {
public:
    std::shared_ptr<BVHLeaf> child1, child2;
    MyVector<float> 
        min = { FLT_MAX, FLT_MAX, FLT_MAX },
        max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    BVHNode() {};
    BVHNode(std::vector<std::shared_ptr<SceneObject>>& objects) {
        int objCount = objects.size();

        for (std::shared_ptr<SceneObject>& obj : objects) {
            std::vector<float> minmax = obj->getBoundingBox();
            for (int axis = 0; axis < 3; axis++) {
                min[axis] = std::min(min[axis], minmax[axis] - EPSILON);
                max[axis] = std::max(max[axis], minmax[axis + 3] + EPSILON);
            }
        }

        // Find the longest axis to split at
        float maxLength = 0, splitVal;
        int longestAxis = -1;
        for (int axis = 0; axis < 3; axis++) {
            float length = max[axis] - min[axis];
            if (length > maxLength) {
                maxLength = length;
                longestAxis = axis;
                splitVal = min[axis] + (length / 2);
            }
        }

        if (longestAxis < 0) {
            std::string errMsg;
            errMsg = "When constructing BVH tree, encountered bounding volume of non-positive volume";
            throw std::runtime_error(errMsg);
        }

        std::vector<float> objAxisVal(objects.size());
        std::vector<int> group1, group2;
        int size1 = 0, size2 = 0;
        for (int i = 0; i < objCount; i++) {
            float axisVal = objects[i]->center[longestAxis];
            objAxisVal[i] = axisVal;
            if (axisVal > splitVal) {
                group2.push_back(i);
                size2++;
            }
            else {
                group1.push_back(i);
                size1++;
            }
        }

        // Balance the two groups
        int loopCounter = 0;
        while (abs(size1 - size2) > 1) {
            if (size1 > size2) {
                float maxVal = -FLT_MAX;
                int toPop;
                for (int i = 0; i < size1; i++) {
                    int objIdx = group1[i];
                    float axisLowerBound = objects[objIdx]->getBoundingBox()[longestAxis];
                    if (axisLowerBound >= maxVal) {
                        maxVal = axisLowerBound;
                        toPop = i;
                    }
                }
                group2.push_back(std::move(group1[toPop]));
                group1.erase(group1.begin() + toPop);
                size2++;
                size1--;
            }
            else {
                float minVal = FLT_MAX;
                int toPop;
                for (int i = 0; i < size2; i++) {
                    int objIdx = group2[i];
                    float axisUpperBound = objects[objIdx]->getBoundingBox()[longestAxis + 3];
                    if (axisUpperBound <= minVal) {
                        minVal = axisUpperBound;
                        toPop = i;
                    }
                }
                group1.push_back(std::move(group2[toPop]));
                group2.erase(group2.begin() + toPop);
                size1++;
                size2--;
            }
            if (++loopCounter >= objCount)
                throw std::runtime_error("Endless loop detected while balancing BVH nodes");
        }

        std::vector<std::shared_ptr<SceneObject>> left(size1), right(size2);
        for (int i = 0; i < size1; i++)
            left[i] = objects[group1[i]];
        for (int i = 0; i < size2; i++)
            right[i] = objects[group2[i]];

        if (size1 > 1) {
            child1 = std::make_shared<BVHNode>(left);
        }
        else {
            child1 = std::make_shared<BVHLeaf>(left[0]);
        }

        if (size2 > 1) {
            child2 = std::make_shared<BVHNode>(right);
        }
        else {
            child2 = std::make_shared<BVHLeaf>(right[0]);
        }
    };

    void check(std::vector<int>& collideList, const Ray& ray) override {
        // Iteratively narrow down the possible range of t by computing the
        // possible range of t for x/y/z axis respectively, the lower bound
        // will always starts at 0 (EPSILON) while the upper bound starts as 
        // the upper bound with x-axis and is lowered with every axis check, 
        // if upper bound ends up lower than lower bound, no real solution for t
        float t1, t2, tMin = EPSILON, tMax = FLT_MAX;
        for (int axis = 0; axis < 3; axis++) {
            // if ray's direction has magnitude in the axis direction, lowers 
            // upper bound and raises lower bound for t
            if (abs(ray.d[axis]) > EPSILON) {
                t1 = (min[axis] - ray.o[axis]) / ray.d[axis];
                t2 = (max[axis] - ray.o[axis]) / ray.d[axis];
                tMin = std::max(tMin, std::min(t1, t2));
                tMax = std::min(tMax, std::max(t1, t2));
            }
            // ray can still intersect if its origin is inside bounding volume
            else if (ray.o[axis] < min[axis] || ray.o[axis] > max[axis]) {
                return;
            }
            // if the lower bound is higher than upper bound then no intersection
            if (tMax < tMin)
                return;
        }
        child1->check(collideList, ray);
        child2->check(collideList, ray);
    }
};