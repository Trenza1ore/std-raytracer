#define _USE_MATH_DEFINES

#include "SceneObject.h"
#include <iostream>

Sphere::Sphere(MyVector<float>& center, float radius)
	: SceneObject(center), radius(radius), r2(radius* radius) {}

std::vector<float> Sphere::getBoundingBox() {
	return std::vector<float> {
		center[0] - radius,
			center[1] - radius,
			center[2] - radius,
			center[0] + radius,
			center[1] + radius,
			center[2] + radius
	};
}

HitInfo Sphere::intersect(const Ray& ray, bool useUV) {
	// Let P be an intersection point, O be origin of sphere:
	// ray:         R(t) = R0 + t*Rd
	// sphere:      ||P - O||^2 - r^2 = 0
	// substitute:  ||R0 + t*Rd - O||^2 - r^2 = 0
	// [(R0 - O) + t*Rd] • [(R0 - O) + t*Rd] - r^2 = 0
	// t^2(Rd•Rd) + 2t*Rd•(R0 - O) + (R0 - O)•(R0 - O) - r^2 = 0
	// a = Rd•Rd, b = 2*Rd•(R0 - O), c = (R0 - O)•(R0 - O) - r^2
	// b/2 is calculated instead to save computation steps
	MyVector<float> oc = ray.o - center;
	float a, bHf, c, discriminant;
	a = ray.d.dot(ray.d);
	bHf = oc.dot(ray.d);
	c = oc.dot(oc) - r2;
	discriminant = bHf * bHf - a * c; // actually discriminant / 4

	if (discriminant < 0) {
		return HitInfo(false);
	}

	// Find roots t1, t2 and use the smallest non-negative (ray origin is camera position) root
	// Since sphere is convex, the closer intersection point is guarenteed to be facing the ray
	// Exception: if camera is inside sphere, but I actually like this behavior so... keeping it
	float discriminantSqrt = sqrt(discriminant); // actually sqrt(discriminant) / 2
	float t1 = (-bHf - discriminantSqrt) / a, t2 = (-bHf + discriminantSqrt) / a, t = -1;
	MyVector<float> p1 = ray.at(t1), p2 = ray.at(t2), p;
	float dist1 = (p1 - ray.o).length(), dist2 = (p2 - ray.o).length(), dist;
	if (dist1 < dist2 && t1 > EPSILON) {
		t = t1;
		p = p1;
		dist = dist1;
	}
	else if (t2 > EPSILON) {
		t = t2;
		p = p2;
		dist = dist2;
	}

	if (t > 0) {
		MyVector<float> n = (p - center) / radius;
		if (useUV) {
			return hitWithUV(dist, p, n);
		}
		return HitInfo(true, dist, p, n);
	}

	return HitInfo(false);
}

HitInfo Sphere::hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal) {
	return HitInfo(true, dist, hitPos, normal);
}

SphericalMap::SphericalMap(MyVector<float>& center, float radius, MyVector<float> euler)
	: Sphere(center, radius), rotMat(rotationMatrix(euler* M_PI / 180.0f)) {}

HitInfo SphericalMap::hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal) {
	MyVector<float> rotatedNormal = normal.multiply(rotMat);
	double u = (rotatedNormal[0] + 1) / 2.0, v = (rotatedNormal[1] + 1) / 2.0;
	return HitInfo(true, dist, hitPos, normal, u, v);
}

HitInfo UVSphere::hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal) {
	MyVector<float> rotatedNormal = normal.multiply(rotMat);
	double u = 0.5 + atan2(rotatedNormal[2], rotatedNormal[0]) / (2.0 * M_PI),
		v = 0.5 - asin(rotatedNormal[1]) / M_PI;
	return HitInfo(true, dist, hitPos, normal, u, v);
}

// Calculate cubemap UV mapping as described in an online blog by Scali
// https://scalibq.wordpress.com/2013/06/23/cubemaps/
HitInfo CubeMap::hitWithUV(float dist, MyVector<float>& hitPos, MyVector<float>& normal) {
	// Scale rotated normal by the 1 / EPSILON to prevent NaN in texture coordinates
	MyVector<float> rotatedNormal = normal.multiply(rotMat) * EPS_INV;
	std::vector<float> magnitude = {
		abs(rotatedNormal[0]), abs(rotatedNormal[1]), abs(rotatedNormal[2])
	};

	// Find the "main" axis with greatest magnitude
	float a = magnitude[0], b, c, mainAxis = 0;
	for (int i = 0; i < 3; i++) {
		if (magnitude[i] > a) {
			a = magnitude[i];
			mainAxis = i;
		}
	}

	// Map UV according to the main axis and its sign
	if (mainAxis == 0) {
		b = magnitude[1]; c = magnitude[2];
		if (rotatedNormal[mainAxis] > 0.0f) c = -c;
	}
	else if (mainAxis == 1) {
		b = -magnitude[2]; c = magnitude[0];
		if (rotatedNormal[mainAxis] < 0.0f) b = -b;
	}
	else {
		b = magnitude[1]; c = -magnitude[0];
		if (rotatedNormal[mainAxis] < 0.0f) c = -c;
	}
	double u = ((c / (double)a) + 1) / 2, v = ((b / (double)a) + 1) / 2;

	return HitInfo(true, dist, hitPos, normal, u, v);
}

Triangle::Triangle() : v0(), v1(), v2(), edge1(), edge2(), n(), m() {}

Triangle::Triangle(MyVector<float>& v0, MyVector<float>& v1, MyVector<float>& v2, bool singleSided) :
	v0(v0), v1(v1), v2(v2), edge1(v1 - v0), edge2(v2 - v0), n(edge1.cross(edge2).normalize()), m(n * -1.0f) {
	center = (v0 + v1 + v2) / 3.0f;
}

std::vector<float> Triangle::getBoundingBox() {
	return std::vector<float> {
		std::min({ v0[0], v1[0], v2[0] }),
			std::min({ v0[1], v1[1], v2[1] }),
			std::min({ v0[2], v1[2], v2[2] }),
			std::max({ v0[0], v1[0], v2[0] }),
			std::max({ v0[1], v1[1], v2[1] }),
			std::max({ v0[2], v1[2], v2[2] })
	};
}

HitInfo Triangle::intersect(const Ray& ray, bool useUV) {
	MyVector<float> h, s, q;
	float a, f, u, v;
	h = ray.d.cross(edge2);
	a = edge1.dot(h);

	if (a > -EPSILON && a < EPSILON)
		return HitInfo(false);  // Ray is parallel to the triangle.

	f = 1.0 / a;
	s = ray.o - v0;
	u = f * s.dot(h);

	if (u < 0 || u > 1)
		return HitInfo(false);

	q = s.cross(edge1);
	v = f * ray.d.dot(q);

	if (v < 0 || u + v > 1)
		return HitInfo(false);

	// Check the direction of the normal. If it is in the opposite direction of the ray, invert it
	bool invertNormal = n.dot(ray.d) > 0;

	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * edge2.dot(q);
	if (t > EPSILON) {
		MyVector<float> p = ray.at(t);
		float dist = (p - ray.o).length();
		if (invertNormal)
			return hitWithUV(dist, p, m);
		else
			return hitWithUV(dist, p, n);
	}

	return HitInfo(false); // This means that there is a line intersection but not a ray intersection.
}


HitInfo Triangle::hitWithUV(float dist, MyVector<float>& hitPos, const MyVector<float>& normal)
{
	return HitInfo(true, dist, hitPos, normal);
}

TriFace::TriFace(MyVector<float>& v0, MyVector<float>& v1, MyVector<float>& v2,
	MyVector<float>& uv0, MyVector<float>& uv1, MyVector<float>& uv2)
	: Triangle(v0, v1, v2, false), uv0(uv0), uv1(uv1), uv2(uv2),
	nLen(edge1.cross(edge2).length()) {}

HitInfo TriFace::hitWithUV(float dist, MyVector<float>& hitPos, const MyVector<float>& normal)
{
	// Calculate texture coordinates
	const MyVector<float> p = hitPos - v0;
	const float b1 = edge2.cross(p).length() / nLen;
	const float b2 = edge1.cross(p).length() / nLen;
	const float b0 = 1.0f - b1 - b2;
	const float u = b0 * uv0[0] + b1 * uv1[0] + b2 * uv2[0];
	const float v = b0 * uv0[1] + b1 * uv1[1] + b2 * uv2[1];
	return HitInfo(true, dist, hitPos, normal, u, v);
}

Cylinder::Cylinder(MyVector<float>& center, float radius, float height, MyVector<float>& axis, MyMatrix<float> rotMat)
	: SceneObject(center), radius(radius), height(height), axis(axis.normalize()), r2(pow(radius, 2.0f)),
	axisInv((axis * -1.0f).normalize()), uBound(height + EPSILON), lBound(-height - EPSILON), rotMat(rotMat) {}

std::vector<float> Cylinder::getBoundingBox() {
	float maxDist = radius + height;
	return std::vector<float> {
		center[0] - maxDist,
			center[1] - maxDist,
			center[2] - maxDist,
			center[0] + maxDist,
			center[1] + maxDist,
			center[2] + maxDist
	};
}

HitInfo Cylinder::hitWithUV(float dist, MyVector<float>& hitPos, float c)
{
	return HitInfo();
}

HitInfo Cylinder::intersect(const Ray& ray, bool useUV) {
	MyVector<float> oc = ray.o - center;
	float oc_dot_axis = oc.dot(axis);
	MyVector<float> oc_projected = axis * oc_dot_axis;
	MyVector<float> oc_perp = oc - oc_projected;
	float dir_dot_axis = ray.d.dot(axis);
	MyVector<float> dir_projected = axis * dir_dot_axis;
	MyVector<float> dir_perp = ray.d - dir_projected;

	float a = dir_perp.dot(dir_perp);
	float b = 2 * oc_perp.dot(dir_perp);
	float c = oc_perp.dot(oc_perp) - radius * radius;
	float discriminant = b * b - 4 * a * c;

	if (discriminant < 0) {
		return HitInfo(false); // No real roots, no intersection
	}

	float sqrt_discriminant = std::sqrt(discriminant);
	float t1 = (-b - sqrt_discriminant) / (2 * a);
	float t2 = (-b + sqrt_discriminant) / (2 * a);

	// Check for intersection with top and bottom faces
	float tTop = (uBound - oc_dot_axis) / dir_dot_axis;
	float tBottom = (lBound - oc_dot_axis) / dir_dot_axis;

	float z1 = oc_dot_axis + t1 * dir_dot_axis;
	float z2 = oc_dot_axis + t2 * dir_dot_axis;

	// Initialize normal
	MyVector<float> normal;

	// Since cylinder is convex, the closer intersection point is guarenteed to be facing the ray
	// Exception: if camera is inside cylinder, but I actually like this behavior so... keeping it
	float z, t = FLT_MAX, r;
	MyVector<float> p, n;
	bool hitSide = false, hitFace = false;

	// Check if the side has been hit
	if (t1 < t && t1 > EPSILON && z1 >= lBound && z1 <= uBound) {
		t = t1;
		z = z1;
		hitSide = true;
	}

	if (t2 < t && t2 > EPSILON && z2 >= lBound && z2 <= uBound) {
		t = t2;
		z = z2;
		hitSide = true;
	}

	// Check if the top/bottom cap has been hit
	if (tTop < t && tTop > EPSILON) {
		MyVector<float> pTop = ray.at(tTop);
		MyVector<float> toCenter = pTop - (center + axis * height);
		float rLen = toCenter.length();
		if (rLen <= radius) {
			t = tTop;
			p = pTop;
			n = axis;
			hitFace = true;
			r = rLen;
		}
	}

	if (tBottom < t && tBottom > EPSILON) {
		MyVector<float> pBottom = ray.at(tBottom);
		MyVector<float> toCenter = pBottom - (center + axisInv * height);
		float rLen = toCenter.length();
		if (rLen <= radius) {
			t = tBottom;
			p = pBottom;
			n = axisInv;
			hitFace = true;
			r = rLen;
		}
	}

	// Since the caps were checked later, we are sure if hitFace == true then a cap is hit first
	if (hitFace) {
		if (material->useTexture) {
			// Calculate polar coordinates for texture mapping
			const MyVector<float> p_ = (p - center).multiply(rotMat);
			double theta = atan2(p_[2], p_[0]);
			if (theta < 0) theta += (2 * M_PI); // normalize theta
			const float u = theta / (2 * M_PI), v = r / radius;
			return HitInfo(true, (p - ray.o).length(), p, n, u, v);
		}
		else {
			return HitInfo(true, (p - ray.o).length(), p, n);
		}
	}
	else if (hitSide) {
		p = ray.at(t);
		n = (p - (center + axis * z)).normalize();
		if (material->useTexture) {
			// Calculate texture mapping
			const MyVector<float> p_ = (p - center).multiply(rotMat);
			double theta = atan2(p_[2], p_[0]);
			if (theta < 0) theta += (2 * M_PI); // normalize theta
			const float u = theta / (2 * M_PI), v = (z + height) / (2 * height);
			return HitInfo(true, (p - ray.o).length(), p, n, u, v);
		}
		else {
			return HitInfo(true, (p - ray.o).length(), p, n);
		}
	}

	return HitInfo(false);
}
