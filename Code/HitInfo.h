#pragma once

#include "MyVector.h"

struct HitInfo {
	bool hit;
	float dist;
	MyVector<float> hitPos;
	MyVector<float> normal;
	bool useUV;
	double hitU;
	double hitV;

	HitInfo() : hit(false), dist(0.0f), hitPos(), normal(), hitU(0), hitV(0), useUV(false) {}
	HitInfo(bool hit) : hit(hit), dist(0.0f), hitPos(), normal(), hitU(0), hitV(0), useUV(false) {}
	HitInfo(bool hit, float dist, const MyVector<float>& hitPos, const MyVector<float>& normal)
		: hit(hit), dist(dist), hitPos(hitPos), normal(normal), hitU(0), hitV(0), useUV(false) {}
	HitInfo(bool hit, float dist, const MyVector<float>& hitPos, const MyVector<float>& normal, float u, float v)
		: hit(hit), dist(dist), hitPos(hitPos), normal(normal), hitU(u), hitV(v), useUV(true) {}
};