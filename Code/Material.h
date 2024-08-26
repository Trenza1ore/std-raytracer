#pragma once

#define DEFAULT_KA 0.2f
#define DEFAULT_DIFFUSE_FACTOR 0.2f
#define DEFAULT_TRANSMITTANCE 1.0f

#include "SceneObject.h"
#include "ImagePPM.h"
#include "MyVector.h"

// Uniform material class for Blinn-Phong and BRDF materials alike (due to inheritance's performance degrade)
class Material {
public:
	const float ka;                         // Ambient coefficient (Phong)
	const float ks;                         // Specular coefficient (Phong)
	const float kd;                         // Diffuse coefficient (Phong)
	const float specularExponent;           // Specular exponent (Phong)
	const MyVector<float> albedo;           // Diffuse color (Phong) / Albedo (BRDF)
	const MyVector<float> specularColor;    // Specular color (Both)
	const bool isReflective;                // Whether material reflects light (Both)
	const float reflectivity;               // Weighting between diffuse color and reflection color (Phong) / Amount of light reflected (BRDF)
	const bool isRefractive;                // Whether material refracts light (Both)
	const float IOR;                        // Refraction index (Both)
	const float transmittance;              // How much light passes through (Both)
	const bool useTexture;                  // Whether to use texture maps (Both)
	const bool useBRDF;                     // Whether to use BRDF (Both)
	std::vector<float> texture;             // Texture map (Both)
	const int texWidth, texHeight;          // Texture map width / height (Both)
	const float F0;                         // Fresnel term at 0 degree (BRDF)
	const float alphaSq;                    // Alpha = Roughness^2, Alpha^2 = Roughness^4, capped at 0.0001 (BRDF)
	const float metallic;					// Metallic (BRDF)
	const bool isMetal;						// Whether to use metallic property (BRDF)

	Material(float ka, float ks, float kd, float specularExponent, MyVector<float>& albedo,
		MyVector<float>& specularColor, bool isReflective, float reflectivity, bool isRefractive,
		float IOR, float transmittance, float roughness, float metallic)
		: useTexture(false), useBRDF(roughness > -EPSILON), albedo(albedo), texWidth(0), texHeight(0),
		ka(ka), ks(ks), kd(kd), specularExponent(specularExponent), specularColor(specularColor), texture(),
		isReflective(isReflective), isRefractive(isRefractive), transmittance(transmittance), reflectivity(reflectivity),
		IOR(IOR), alphaSq(std::max(pow(roughness, 2.0), 0.0001)), metallic(metallic), isMetal(metallic > EPSILON), 
		F0(pow((IOR - 1) / (IOR + 1), 2.0) * (1 - metallic) + metallic) {};

	Material(float ka, float ks, float kd, float specularExponent, ImagePPM& textureImage,
		MyVector<float>& specularColor, bool isReflective, float reflectivity, bool isRefractive,
		float IOR, float transmittance, float roughness, float metallic)
		: useTexture(true), useBRDF(roughness > -EPSILON), texWidth(textureImage.width), texHeight(textureImage.height),
		ka(ka), ks(ks), kd(kd), specularExponent(specularExponent),
		specularColor(specularColor), isReflective(isReflective), isRefractive(isRefractive), transmittance(transmittance),
		reflectivity(reflectivity), albedo(1), metallic(metallic), isMetal(metallic > EPSILON),
		IOR(IOR), alphaSq(std::max(pow(roughness, 2.0), 0.0001)), F0(pow((IOR - 1) / (IOR + 1), 2.0) * (1 - metallic) + metallic)
	{
		// Set up 2D array (std::vector) to store texture
		this->texture = std::vector<float>(texWidth * texHeight * 3);

		// Store texture values
		int x, y, c, i = 0;
		for (x = 0; x < texWidth; x++) {
			for (y = 0; y < texHeight; y++) {
				texture[i++] = textureImage.getPixelValue(x, y, c = 0) / 255.0;
				texture[i++] = textureImage.getPixelValue(x, y, ++c) / 255.0;
				texture[i++] = textureImage.getPixelValue(x, y, ++c) / 255.0;
			}
		}
	}

	Material() : useTexture(false), useBRDF(false), texWidth(0), texHeight(0), ka(0), ks(0), kd(0),
		specularExponent(0), specularColor(1), albedo(1), isReflective(false), isRefractive(false),
		transmittance(1), reflectivity(0), IOR(0), alphaSq(0), texture(), F0(0), metallic(0), isMetal(false){};

	// Todo: texture coordinate
	const MyVector<float> diffuseColorAt(double u, double v) const {
		if (!useTexture) return albedo;
		int x = std::clamp<int>(texWidth * u, 0, texWidth - 1),
			y = std::clamp<int>(texHeight * v, 0, texHeight - 1),
			i = (x * texHeight + y) * 3;
		return MyVector<float>{ texture[i++], texture[i++], texture[i] };
	}
};