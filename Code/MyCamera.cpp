#define _USE_MATH_DEFINES

#include <MyCamera.h>

MyCamera::MyCamera() : type(""), width(0), height(0), hfWidth(0), hfHeight(0),
fov(0), f(0), exposure(0), rays(), objCount(), bgVec(), gamma(2.2f), ambientIntensity(),
bgChar(3), frames(1), nbounce(0), defaultRenderer(binary), TranMat(3, 4, 0) {}

bool MyCamera::readConfig(std::string filename) {
	name = filename;
	std::cout << "\nScene \"" + name + "\" loaded" << std::endl;

	filename = filename + ".json";
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open scene description file: " + filename);
		return false;
	}
	nlohmann::json j;
	file >> j;
	parseJSON(j);
	return true;
}

void MyCamera::parseJSON(const nlohmann::json& j) {
	// Read properties from json
	type = j["camera"]["type"];
	if (type == "aperture") {
		simulateAperture = true;
		apertureDiameter = 0.25f;
		if (j["camera"].contains("diameter")) {
			apertureDiameter = j["camera"]["diameter"];
		}
		type += " (diameter: " + std::to_string(apertureDiameter) + ')';
	}
	width = j["camera"]["width"];
	height = j["camera"]["height"];
	exposure = j["camera"]["exposure"];
	if (j["camera"].contains("Lw"))
		Lw = j["camera"]["Lw"];
	else
		Lw = 0;

	// Fov needs to be converted into radian form
	fov = (double)j["camera"]["fov"] * M_PI / 180.0; // convert fov to radians form

	// Pre-calculate several frequently-used values
	hfWidth = width / 2.0f;
	hfHeight = height / 2.0f;

	// Calculate focal length from height and fov, width is more commonly used but
	// the example renders used height to calculate focal length
	f = height / (2 * std::tan(fov / 2.0f));

	// Read position/look at/up vectors from json and store in MyVector objects
	position = MyVector<float>(j["camera"]["position"].get<std::vector<float>>());
	lookAt = MyVector<float>(j["camera"]["lookAt"].get<std::vector<float>>());
	upVector = MyVector<float>(j["camera"]["upVector"].get<std::vector<float>>());

	// Read background color from json, ambient intensity defaults to background color unless specified
	ambientIntensity = MyVector<float>(j["scene"]["backgroundcolor"].get<std::vector<float>>());
	bgVec = ambientIntensity;
	//colorDataConvert(bgVec);

	// Create an unsigned char/int8 copy of background for faster PPM image writing
	for (int i = 0; i < 3; i++) {
		bgChar[i] = (uint8_t)intensity2RGB(bgVec[i]);
	}

	// Gamma correction value
	if (j["camera"].contains("gamma")) {
		gamma = j["camera"]["gamma"];
	}

	// Get the number of frames, shadow samples, anti-aliasing samples
	frames = 1;
	shadowSamples = 1;
	cameraSamples = 1;
	if (j.contains("frames")) frames = j["frames"];
	if (j.contains("shadow samples")) shadowSamples = j["shadow samples"];
	if (j.contains("camera samples")) cameraSamples = j["camera samples"];

	// Initialize int std::vectors
	objCount = std::vector<int>(frames);
	lightCount = std::vector<int>(frames);

	objects = std::vector<std::vector<std::shared_ptr<SceneObject>>>(frames);
	lights = std::vector<std::vector<std::shared_ptr<Light>>>(frames);
	if (frames > 1)
		camMovement = std::vector<MyVector<float>>(frames, MyVector<float>(3));
	else
		camMovement = { position };
	BVHTree = std::vector<BVHNode>(frames);

	// Get default render mode
	std::string mode = j["rendermode"];
	if (mode == "phong") {
		defaultRenderer = phong;
		nbounce = j["nbounces"];
	}
	else if (mode == "pathtracer") {
		defaultRenderer = pathtracer;
		nbounce = j["nbounces"];
	}
	else if (mode == "preview") {
		defaultRenderer = preview;
		nbounce = 1;
	}
	else {
		defaultRenderer = binary;
	}

	std::cout << "Shadow sample: " << shadowSamples << " | Camera sample: ";
	std::cout << cameraSamples << " | Light bounce: " << nbounce << std::endl;

	// Generate camera transformation matrix
	generateTransformationMatrix();

	loadScene(j);
}

void MyCamera::loadScene(const nlohmann::json& j) {
	auto& sceneData = j["scene"];

	const std::string modelFormat = "ply", camID = "cam";

	int objCounter = 0, lightCounter = 0, plyFaceCount = 0, dynamicObjects = 0;

	bool animated = j.contains("animation path"), hasPLY = false;
	std::vector<std::vector<MyVector<float>>> animPos, animRot;
	std::vector<std::string> animIDs;
	if (animated) {
		// Open animation json file
		std::string path = j["animation path"];
		std::ifstream file(path);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open animation description file: " + path);
		}
		nlohmann::json j;
		file >> j;

		// Update frame count if needed
		if (frames != j["frames"]) {
			frames = j["frames"];
			objCount = std::vector<int>(frames);
			lightCount = std::vector<int>(frames);
			objects = std::vector<std::vector<std::shared_ptr<SceneObject>>>(frames);
			lights = std::vector<std::vector<std::shared_ptr<Light>>>(frames);
			camMovement = std::vector<MyVector<float>>(frames, MyVector<float>(3));
			BVHTree = std::vector<BVHNode>(frames);
		}

		std::cout << frames << " frame(s) of scene data will be loaded" << std::endl;

		// Load position and rotation animation data
		dynamicObjects = j["animated objects"];
		animPos = std::vector<std::vector<MyVector<float>>>(dynamicObjects,
			std::vector<MyVector<float>>(frames, MyVector<float>(3)));
		animRot = std::vector<std::vector<MyVector<float>>>(dynamicObjects,
			std::vector<MyVector<float>>(frames, MyVector<float>(3)));
		animIDs = std::vector<std::string>(dynamicObjects);
		int animObjCounter = 0;
		for (auto& animData : j["animations"]) {
			std::string currentID = animData["id"];
			std::vector<MyVector<float>> objPos(frames, MyVector<float>(3)),
				objRot(frames, MyVector<float>(3));
			int f = 0;
			for (auto& transform : animData["transform"]) {
				MyVector<float> posTemp(transform["pos"].get<std::vector<float>>());
				MyVector<float> rotTemp(transform["rot"].get<std::vector<float>>());
				objPos[f] = posTemp;
				objRot[f] = rotTemp;
				f++;
				if (f > frames) {
					std::string errMsg;
					errMsg = "The number of animated frames declared (" + std::to_string(frames) +
						") is lower than actually presented in json for object: " + currentID;
					throw std::runtime_error(errMsg);
				}
			}
			if (currentID == camID) {
				camMovement = objPos;
				dynamicObjects -= 1;
			}
			else {
				animIDs[animObjCounter] = currentID;
				animPos[animObjCounter] = objPos;
				animRot[animObjCounter] = objRot;
				animObjCounter++;
			}
		}
	}
	else {
		frames = 1;
		objCount = std::vector<int>(frames);
		lightCount = std::vector<int>(frames);
		objects = std::vector<std::vector<std::shared_ptr<SceneObject>>>(frames);
		lights = std::vector<std::vector<std::shared_ptr<Light>>>(frames);
		camMovement = { position };
		BVHTree = std::vector<BVHNode>(frames);
	}

	// Load in all the shapes
	for (auto& object : sceneData["shapes"]) {
		std::string objType = object["type"];

		bool validObject = true, useTexture = false, useAlias = false;
		std::string alias = "";
		std::string objID = std::to_string(objCounter), loadedText = " loaded";
		if (object.contains("alias")) {
			alias = object["alias"];
			useAlias = true;
		}

		if (objType == "triangle") {
			MyVector<float> v0 = MyVector<float>(object["v0"].get<std::vector<float>>());
			MyVector<float> v1 = MyVector<float>(object["v1"].get<std::vector<float>>());
			MyVector<float> v2 = MyVector<float>(object["v2"].get<std::vector<float>>());
			if (object.contains("uv0")) {
				useTexture = true;
				MyVector<float> uv0 = MyVector<float>(object["uv0"].get<std::vector<float>>());
				MyVector<float> uv1 = MyVector<float>(object["uv1"].get<std::vector<float>>());
				MyVector<float> uv2 = MyVector<float>(object["uv2"].get<std::vector<float>>());
				std::shared_ptr ptr = std::make_shared<TriFace>(v0, v1, v2, uv0, uv1, uv2);
				for (int f = 0; f < frames; f++) {
					objects[f].push_back(ptr);
				}
			}
			else {
				std::shared_ptr ptr = std::make_shared<Triangle>(v0, v1, v2, false);
				for (int f = 0; f < frames; f++) {
					objects[f].push_back(ptr);
				}
			}
		}
		else if (objType == "uvsphere" || objType == "cubemap" || objType == "sphere" || objType == "sphericalmap") {
			MyVector<float> center(object["center"].get<std::vector<float>>());
			MyVector<float> euler = { 0, 0, 0 };
			if (object.contains("euler")) {
				MyVector<float> temp(object["euler"].get<std::vector<float>>());
				euler = temp;
			}
			float radius = object["radius"];
			bool isAnimated = false;
			int animIndex = -1;
			if (animated) {
				for (int i = 0; i < dynamicObjects; i++) {
					if (animIDs[i] == objID || animIDs[i] == alias) {
						animIndex = i;
						isAnimated = true;
						std::cout << "Animation data found for object: " << objID << std::endl;
						break;
					}
				}
			}

			useTexture = true;
			int textureType = 0;
			if (objType == "cubemap") {
				textureType = 1;
			}
			else if (objType == "sphericalmap") {
				textureType = 2;
			}

			if (isAnimated) {
				for (int f = 0; f < frames; f++) {
					MyVector<float>& pos = animPos[animIndex][f];
					if (useTexture) {
						MyVector<float>& rot = animRot[animIndex][f];
						switch (textureType) {
						case 0:
							objects[f].push_back(std::make_shared<UVSphere>(pos, radius, rot));
							break;
						case 1:
							objects[f].push_back(std::make_shared<CubeMap>(pos, radius, rot));
							break;
						case 2:
							objects[f].push_back(std::make_shared<SphericalMap>(pos, radius, rot));
							break;
						}
					}
					else {
						objects[f].push_back(std::make_shared<Sphere>(pos, radius));
					}
				}
			}
			else {
				if (useTexture) {
					MyVector<float>& rot = animRot[animIndex][f];
					if (textureType == 0) {
						std::shared_ptr<UVSphere> ptr = std::make_shared<UVSphere>(center, radius, euler);
						for (int f = 0; f < frames; f++) objects[f].push_back(ptr);
					}
					else if (textureType == 1) {
						std::shared_ptr<CubeMap> ptr = std::make_shared<CubeMap>(center, radius, euler);
						for (int f = 0; f < frames; f++) objects[f].push_back(ptr);
					}
					else {
						std::shared_ptr<SphericalMap> ptr = std::make_shared<SphericalMap>(center, radius, euler);
						for (int f = 0; f < frames; f++) objects[f].push_back(ptr);
					}
				}
				else {
					std::shared_ptr ptr = std::make_shared<Sphere>(center, radius);
					for (int f = 0; f < frames; f++) objects[f].push_back(ptr);
				}
			}
		}
		else if (objType == "cylinder" || objType == "uvcylinder") {
			float radius = object["radius"], height = object["height"];
			MyVector<float> center(object["center"].get<std::vector<float>>());
			MyVector<float> axis(object["axis"].get<std::vector<float>>());

			// Construct rotation matrix for UV mapping
			// rotate by z axis first, then x axis
			axis = axis.normalize();
			MyVector<float> euler = { atan2f(sqrt(axis[0] * axis[0] + axis[2] * axis[2]), axis[1]), 0, 0};
			MyMatrix<float> rotMat = rotationMatrix(euler); // z axis rotation matrix
			euler = { 0, 0, atan2(axis[0], axis[2]) };
			rotMat = rotMat * rotationMatrix(euler); // zx
			useTexture = objType == "uvcylinder";
			std::shared_ptr ptr = std::make_shared<Cylinder>(center, radius, height, axis, rotMat);
			for (int f = 0; f < frames; f++) {
				objects[f].push_back(ptr);
			}
		}
		else if (objType == modelFormat) {
			if (!hasPLY) {
				std::cout << "\n[!] Warning: ply model data is not well validated at the moment, ";
				std::cout << "if program freezes during loading, check the model\n" << std::endl;
				hasPLY = true;
			}
			std::string modelPath = object["path"];
			MyVector<float> offset(object["offset"].get<std::vector<float>>());
			MyVector<float> scale(object["scale"].get<std::vector<float>>());
			MyVector<float> euler = { 0, 0, 0 }, textureScale = { 1, 1 };
			if (object.contains("euler"))
				euler = MyVector<float>(object["euler"].get<std::vector<float>>());
			if (object.contains("texturescale"))
				textureScale = MyVector<float>(object["texturescale"].get<std::vector<float>>());
			PlyParser model(modelPath);
			model.stScale = textureScale;
			std::vector<MyVector<float>> faces = model.loadModel(scale, offset, euler);
			if (model.comment.length() > 0)
				std::cout << "PLY model comment: " << model.comment << std::endl;
			plyFaceCount = model.faceCount;
			if (plyFaceCount > 0) objCounter--;
			loadedText = loadedText + ": " + modelPath;
			objID = objID + '-' + std::to_string(objCounter + plyFaceCount);

			useTexture = false;
			if (object.contains("material")) {
				useTexture = object["material"].contains("texturepath") && (model.uvTextured || model.stTextured);
			}

			bool isAnimated = false;
			int animIndex = -1;
			if (animated) {
				for (int i = 0; i < dynamicObjects; i++) {
					if (animIDs[i] == objID || animIDs[i] == alias) {
						animIndex = i;
						isAnimated = true;
						std::cout << "Animation data found for object: " << objID << std::endl;
						break;
					}
				}
			}

			if (isAnimated) {
				for (int f = 0; f < frames; f++) {
					MyVector<float>& pos = animPos[animIndex][f];
					MyVector<float>& rot = animRot[animIndex][f];
					faces = model.loadModel(scale, pos, rot);
					if (useTexture) {
						for (int i = 0, vertIdx = 0; i < plyFaceCount; i++) {
							MyVector<float>
								& uv0 = model.vertTexs[vertIdx], & v0 = faces[vertIdx++],
								& uv1 = model.vertTexs[vertIdx], & v1 = faces[vertIdx++],
								& uv2 = model.vertTexs[vertIdx], & v2 = faces[vertIdx++];
							objects[f].push_back(std::make_shared<TriFace>(v0, v1, v2, uv0, uv1, uv2));
						}
					}
					else {
						for (int i = 0, vertIdx = 0; i < plyFaceCount; i++) {
							MyVector<float>
								& v0 = faces[vertIdx++],
								& v1 = faces[vertIdx++],
								& v2 = faces[vertIdx++];
							objects[f].push_back(std::make_shared<Triangle>(v0, v1, v2, false));
						}
					}
				}
			}
			else {
				if (useTexture) {
					for (int i = 0, vertIdx = 0; i < plyFaceCount; i++) {
						MyVector<float>
							& uv0 = model.vertTexs[vertIdx], & v0 = faces[vertIdx++],
							& uv1 = model.vertTexs[vertIdx], & v1 = faces[vertIdx++],
							& uv2 = model.vertTexs[vertIdx], & v2 = faces[vertIdx++];
						for (int f = 0; f < frames; f++) {
							objects[f].push_back(std::make_shared<TriFace>(v0, v1, v2, uv0, uv1, uv2));
						}
					}
				}
				else {
					for (int i = 0, vertIdx = 0; i < plyFaceCount; i++) {
						MyVector<float>
							& v0 = faces[vertIdx++],
							& v1 = faces[vertIdx++],
							& v2 = faces[vertIdx++];
						for (int f = 0; f < frames; f++) {
							objects[f].push_back(std::make_shared<Triangle>(v0, v1, v2, false));
						}
					}
				}
			}
			objCounter += plyFaceCount;
		}
		else {
			validObject = false;
		}

		if (validObject) {
			if (defaultRenderer != binary) {
				auto& mat = object["material"];
				float ks = mat["ks"], kd = mat["kd"], se = mat["specularexponent"],
					rl = mat["reflectivity"], rr = mat["refractiveindex"],
					ka = DEFAULT_KA, transmittance = 0, roughness = -1, metallic = 0;
				bool refl = mat["isreflective"], refr = mat["isrefractive"];
				if (!refl) rl = 0;
				MyVector<float> dc(mat["diffusecolor"].get<std::vector<float>>());
				MyVector<float> sc(mat["specularcolor"].get<std::vector<float>>());

				// Optional attributes
				if (mat.contains("ka"))
					ka = mat["ka"];
				if (mat.contains("roughness"))
					roughness = pow(mat["roughness"], 2.0f);
				if (mat.contains("metallic"))
					metallic = mat["metallic"];
				if (mat.contains("transmittance")) {
					transmittance = mat["transmittance"];
				}
				else if (refr) {
					transmittance = DEFAULT_TRANSMITTANCE;
				}

				bool texturedMaterial = false;
				ImagePPM img;
				if (useTexture && mat.contains("texturepath")) {
					std::string path = mat["texturepath"];
					if (!img.read(path)) {
						throw std::runtime_error("Failed to load PPM image <" + path + ">");
					}
					texturedMaterial = true;
				}
				else {
					if (mat.contains("texturepath"))
						std::cout << "[!] Object type <" << objType << "> does not support texture" << std::endl;
				}

				if (texturedMaterial) {
					std::shared_ptr<Material> material = std::make_shared<Material>(ka, ks, kd,
						se, img, sc, refl, rl, refr, rr, transmittance, roughness, metallic);
					for (int f = 0; f < frames; f++) {
						// For a 3D model, assign the same material to all faces
						if (objType == modelFormat) {
							for (int i = 0; i < plyFaceCount; i++) {
								int objIndex = objCounter - i;
								objects[f][objIndex]->setMaterial(material);
								objects[f][objIndex]->index = objIndex;
							}
						}
						else {
							objects[f][objCounter]->setMaterial(material);
							objects[f][objCounter]->index = objCounter;
						}
					}
				}
				else {
					std::shared_ptr<Material> material = std::make_shared<Material>(ka, ks, kd,
						se, dc, sc, refl, rl, refr, rr, transmittance, roughness, metallic);
					for (int f = 0; f < frames; f++) {
						// For a 3D model, assign the same material to all faces
						if (objType == modelFormat) {
							for (int i = 0; i < plyFaceCount; i++) {
								int objIndex = objCounter - i;
								objects[f][objIndex]->setMaterial(material);
								objects[f][objIndex]->index = objIndex;
							}
						}
						else {
							objects[f][objCounter]->setMaterial(material);
							objects[f][objCounter]->index = objCounter;
						}
					}
				}

				if (texturedMaterial) {
					std::string texPath = mat["texturepath"];
					loadedText = loadedText + " with texture: " + texPath;
				}
			}
			else {
				// A fake material is required
				std::shared_ptr<Material> material = std::make_shared<Material>();
				for (int f = 0; f < frames; f++) {
					// For a 3D model, assign the same material to all faces
					if (objType == modelFormat) {
						for (int i = 0; i < plyFaceCount; i++) {
							int objIndex = objCounter - i;
							objects[f][objIndex]->setMaterial(material);
							objects[f][objIndex]->index = objIndex;
						}
					}
					else {
						objects[f][objCounter]->setMaterial(material);
						objects[f][objCounter]->index = objCounter;
					}
				}
			}

			if (useAlias) {
				std::cout << "Object[" << objID << "] (" << alias << ") " << objType << loadedText << std::endl << std::endl;
			}
			else {
				std::cout << "Object[" << objID << "] " << objType << loadedText << std::endl << std::endl;
			}
		}

		if (validObject) {
			objCounter++;
		}
		else {
			std::cout << "[!] Unknown object type: " << objType << std::endl;
		}

	}
	std::cout << std::endl << objCounter << " objects loaded\n" << std::endl;

	std::cout << "Constructing BVH trees for every frame" << std::endl;
	if (objCounter > 1) {
		for (int f = 0; f < frames; f++) {
			BVHTree[f] = BVHNode(objects[f]);
		}
	}
	else if (objCounter == 1) {
		for (int f = 0; f < frames; f++) {
			std::vector<std::shared_ptr<SceneObject>> fake = { objects[f][0], objects[f][0] };
			BVHTree[f] = BVHNode(fake);
		}
	}
	std::cout << "BVH trees successfully constructed\n" << std::endl;

	// Load in all the light sources
	if (sceneData.contains("lightsources")) {
		for (auto& light : sceneData["lightsources"]) {
			std::string lightType = light["type"];
			std::string lightID = lightType + std::to_string(lightCounter);

			bool isAnimated = false;
			int animIndex = -1;
			if (animated) {
				for (int i = 0; i < dynamicObjects; i++) {
					if (animIDs[i] == lightID) {
						animIndex = i;
						isAnimated = true;
						std::cout << "Animation data found for light: " << lightID << std::endl;
						break;
					}
				}
			}

			if (lightType == "pointlight" || lightType == "arealight") {
				// Optional attenuation attributes
				float kc = KC, kl = KL, kq = KQ;
				if (light.contains("kc"))
					kc = light["kc"];
				if (light.contains("kl"))
					kc = light["kl"];
				if (light.contains("kq"))
					kc = light["kq"];

				MyVector<float> lInt = MyVector<float>(light["intensity"].get<std::vector<float>>());

				bool isArea = lightType[0] == 'a' && shadowSamples > 1;
				std::vector<float> size(3);
				// Defaults to using an X-Z plane for area light, but allows X-Y-Z 3D rectangle shape if needed
				if (isArea) {
					std::vector<float> temp = light["size"].get<std::vector<float>>();
					if (temp.size() < 3)
						size = { temp[0], -1, temp[1] };
					else
						size = { temp[0], temp[1], temp[2] };
				}

				if (isAnimated) {
					for (int f = 0; f < frames; f++) {
						MyVector<float>& pos = animPos[animIndex][f];
						std::shared_ptr ptr = std::make_shared<Light>(pos, lInt, kc, kl, kq, false);
						if (isArea) ptr = std::make_shared<AreaLight>(pos, lInt, kc, kl, kq, size[0], size[1], size[2]);
						lights[f].push_back(ptr);
					}
				}
				else {
					MyVector<float> lPos = MyVector<float>(light["position"].get<std::vector<float>>());
					std::shared_ptr ptr = std::make_shared<Light>(lPos, lInt, kc, kl, kq, false);
					if (isArea) ptr = std::make_shared<AreaLight>(lPos, lInt, kc, kl, kq, size[0], size[1], size[2]);
					for (int f = 0; f < frames; f++) {
						lights[f].push_back(ptr);
					}
				}
				lightCounter++;
			}
			else if (lightType == "ambient") {
				ambientIntensity = MyVector<float>(light["intensity"].get<std::vector<float>>());
			}

			std::cout << lightID << " loaded" << std::endl;
		}
	}
	std::cout << std::endl << lightCounter << " light sources loaded" << std::endl;
	for (int f = 0; f < frames; f++) {
		objCount[f] = objCounter;
		lightCount[f] = lightCounter;
	}

	generateRays();
}

// Method for rendering a frame, with render mode override option r (binary scenes shouldn't be overriden)
ImagePPM MyCamera::renderFrame(const std::string& savePath, int frameNo, int lastframe, RenderMode r) {
	std::vector<double> LwVals = { 0 };
	if (Lw != 0) LwVals.push_back(Lw);
	std::vector<ImagePPM> processedFrames(LwVals.size(), { width, height });
	ImagePPM currentFrameRaw(width, height);
	unsigned char bgColor[3] = { bgChar[0], bgChar[1], bgChar[2] };
	if (r == scene) 
		r = defaultRenderer;
	if (r != binary) 
		currentFrameRaw.setRegion(0, 0, width, height, bgColor);

	// Formatting strings for saving rendered images
	std::string fNum = "";
	std::string SNum = "";

	if (frames > 1) {
		fNum = std::to_string(frameNo);
		fNum = "_" + std::string(std::to_string(frames).length() - fNum.length(), '0') + fNum;
	}

	if (cameraSamples > 1) {
		SNum = '_' + std::to_string(cameraSamples);
	}

	int x, y, ri = 0, objNum = objCount[frameNo], lightNum = lightCount[frameNo];

	double invertGamma = 1 / (double)gamma;
	MyMatrix<float> R(width, height, bgVec[0] * exposure),
		G(width, height, bgVec[1] * exposure),
		B(width, height, bgVec[2] * exposure);

	// Update camera rays' origin
	updateRays(frameNo, lastframe);

	// Use std::async thread pool if needed
	const bool multithreading = cameraSamples > 1 && r != binary;
	std::vector<std::future<MyVector<float>>> tasks(cameraSamples);

	// Create a dynamic progress bar for rendering complex scenes
	ProgressBar progressBar(width * height * cameraSamples, pBarSize, name, objNum);
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			MyVector<float> finalColor = { 0, 0, 0 };
			float totalWeight = 0;
			int currentRI = ri;
			for (int s = 0; s < cameraSamples; s++, ri++) {
				bool hit = false;
				Ray& pixelRay = rays[ri];
				totalWeight += wRay[ri];

				progressBar.update();

				std::vector<HitInfo> hits(objNum);
				std::vector<int> objToCheck;
				BVHTree[frameNo].check(objToCheck, pixelRay);
				for (int objIdx : objToCheck) {
					hits[objIdx] = objects[frameNo][objIdx]->intersect(pixelRay, true);
					hit |= hits[objIdx].hit;
				}

				if (hit) {
					switch (r)
					{
					case binary:
						currentFrameRaw.setPixelValue(x, y, 0, 255);
						break;
					default:
						// Find index of nearest hit object
						int current = 0;
						float distMin = FLT_MAX;
						for (int objIdx = 0; objIdx < objNum; objIdx++) {
							if (hits[objIdx].hit && hits[objIdx].dist < distMin) {
								distMin = hits[objIdx].dist;
								current = objIdx;
							}
						}
						if (multithreading)
							tasks[s] = std::async(std::launch::async, &MyCamera::traceRay, this, r, hits, std::cref(position), current, -1, objNum, lightNum, frameNo, 0);
						else
							finalColor = traceRay(r, hits, position, current, -1, objNum, lightNum, frameNo, 0);
						break;
					}
				}
				else {
					if (multithreading)
						tasks[s] = std::async(std::launch::async, [&] { return bgVec; });
					else
						finalColor = bgVec;
				}
			}

			if (r != binary) {
				if (multithreading) {
					for (int s = 0; s < cameraSamples; s++) {
						finalColor += tasks[s].get() * wRay[s + currentRI];
					}
					finalColor /= totalWeight;
				}
				for (int c = 0; c < 3; c++) { currentFrameRaw.setPixelValue(x, y, c, std::clamp<float>(finalColor[c] * 255, 0, 255)); }
				finalColor *= exposure;
				R[x][y] = finalColor[0]; G[x][y] = finalColor[1]; B[x][y] = finalColor[2];
			}
		}
		//currentFrameRaw.write("../TestSuite/TEMP.ppm"); // DEBUG
	}

	switch (r) {
	case binary:
		currentFrameRaw.write(savePath + SNum + fNum + ".ppm");
		break;
	case preview:
		currentFrameRaw.write(savePath + "_Preview" + SNum + fNum + ".ppm");
		break;
	default:
		ReinhardToneMapping(processedFrames, R, G, B, LwVals, invertGamma);
		int variantsNum = processedFrames.size();
		processedFrames[0].write(savePath + SNum + fNum + ".ppm");
		if (variantsNum > 1) {
			for (int i = 1; i < variantsNum; i++) {
				processedFrames[i].write(savePath + "_" + SNum + "_" + std::to_string(i) + "_" + fNum + ".ppm");
			}
		}
		currentFrameRaw.write(savePath + "_RAW" + fNum + ".ppm");
		break;
	}
	return currentFrameRaw;
}

// Recursive function for tracing rays and returning a final color
MyVector<float> MyCamera::traceRay(const RenderMode r, std::vector<HitInfo> hits, const MyVector<float>& lastPos,
	int current, int prev, int objNum, int lightNum, int frameNo, int depth) {
	if (depth++ > nbounce) {
		return DEFAULT;
	}

	// Get material properties
	const Material& objMat = *objects[frameNo][current]->material.get();
	const bool useBRDF = objMat.useBRDF;
	MyVector<float> albedo;

	// Adjust albedo / diffuse color to use texture if applicable
	if (hits[current].useUV && objMat.useTexture) {
		double u = hits[current].hitU, v = hits[current].hitV;
		albedo = objMat.diffuseColorAt(u, v);
	}
	else {
		albedo = objMat.albedo;
	}

	// For preview mode, return albedo directly
	if (r == preview) {
		return albedo;
	}

	// Initialize constants for calculation
	const MyVector<float>& camPos = camMovement[frameNo];
	const MyVector<float> pos = hits[current].hitPos;
	const MyVector<float> incidentDir = (pos - lastPos).normalize(), N = hits[current].normal;
	const MyVector<float> viewDir = (camPos - pos).normalize();
	const float I_dot_N = incidentDir.dot(N), N_dot_V = std::max(N.dot(viewDir), EPSILON);
	const float alphaSq = objMat.alphaSq, F0 = objMat.F0; // For BRDF

	// Initialize combined color with ambient component
	MyVector<float> combined = albedo * ambientIntensity * objMat.ka;
	MyVector<float> diffuse = { 0, 0, 0 }, specular = { 0, 0, 0 };

	// Reflection and refraction does not need to respect energy conservation if BRDF not used
	float reflectWeight = objMat.reflectivity, refractWeight = objMat.transmittance,
		diffuseWeight, frensnelReflectance;

	// Whether to cast additional reflective/refractive/diffusive rays depend on material /render settings
	bool useReflect = objMat.isReflective, useRefract = objMat.isRefractive, useDiffuse = ((r == pathtracer) && useBRDF);

	// Iterate through all light sources
	for (int l = 0; l < lightNum; l++) {
		const bool isArea = lights[frameNo][l]->isArea;
		int sampleCount = 1;
		if (isArea) sampleCount = shadowSamples;
		//MyVector<float> intensity = lights[frameNo][l]->flatIntensity();
		const MyVector<float> intensity = lights[frameNo][l]->intensityAt(pos) / sampleCount;
		double lightDiffuse = 0, lightSpecular = 0;

		for (int s = 0; s < sampleCount; s++) {
			bool receiveLight = true;
			// Check for shadow
			const Ray shadowRay = Ray(pos, lights[frameNo][l]->getPos() - pos);
			float litFactor = 1;
			std::vector<int> objToCheck;
			BVHTree[frameNo].check(objToCheck, shadowRay);
			for (int objIdx : objToCheck) {
				if (objects[frameNo][objIdx]->intersect(shadowRay, false).hit) {
					if (useRefract) {
						litFactor *= objMat.transmittance;
					}
					else {
						receiveLight = false;
						break;
					}
				}
			}

			// Current implementation recalculates specular/diffuse components for each light sample to achieve better 
			// visual quality, if shared across all samples, the color can appear a bit off...
			if (receiveLight) {
				const MyVector<float> H = (viewDir + shadowRay.d).normalize();
				const double N_dot_H = std::max(N.dot(H), EPSILON), N_dot_L = std::max(N.dot(shadowRay.d), EPSILON);
				if (useBRDF) {
					const double term0 = std::max(N_dot_H * N_dot_H * (alphaSq - 1) + 1, 0.0001),
						term1 = 4 * N_dot_L * N_dot_V,
						term2 = N_dot_L + sqrt(alphaSq + (1 - alphaSq) * N_dot_L * N_dot_L),
						term3 = N_dot_V + sqrt(alphaSq + (1 - alphaSq) * N_dot_V * N_dot_V);
					const double D = alphaSq / (M_PI * term0 * term0);
					const double F = F0 + (1 - F0) * powf(1 - viewDir.dot(H), 5);
					const double G = term1 / (term2 * term3);
					lightSpecular += litFactor * F * (D * F * G) / std::max(term1, 0.0001);
					lightDiffuse += litFactor * (1 - F) * N_dot_L;
				}
				else {
					lightSpecular += litFactor * pow(N_dot_H, objMat.specularExponent) * objMat.ks;
					lightDiffuse += litFactor * N_dot_L * objMat.kd;
				}
			}
		}
		diffuse += intensity * lightDiffuse;
		specular += intensity * lightSpecular;
	}


	// Adjust weighting of each component with respect to energy conservation for BRDF materials
	const float term1 = 1 - objMat.transmittance, term2 = term1 * objMat.reflectivity;
	if (useBRDF) {
		// Use Schlick's approximation for frensnel reflectance
		frensnelReflectance = F0 + (1 - F0) * pow(1 - N_dot_V, 5);
		// Adjust reflection/refraction/diffusion(indirect) weights for BRDF 
		reflectWeight = term2 + objMat.transmittance * frensnelReflectance;
		refractWeight = objMat.transmittance * (1 - frensnelReflectance);
		diffuseWeight = 1 - (term1 - term2) - reflectWeight - refractWeight;
		diffuse *= 1 - objMat.metallic;
	}
	// Adjust local diffuse color weighting according to transmittance and reflectivity
	diffuse *= (term1 - term2);
	combined += specular * objMat.specularColor;
	combined += diffuse * albedo;

	std::future<MyVector<float>> reflectTask, refractTask, diffuseTask; // std::async's thread pool is used for multi-threading

	// Refraction ray
	if (useRefract) {
		// If current == prev, same object ID indicates that light is leaving, otherwise it is entering object
		float ratio = objMat.IOR, c = I_dot_N;
		if (current != prev) {
			ratio = 1 / objMat.IOR;
			c = -I_dot_N;
		}
		const float dirChange = 1 - ratio * ratio * (1 - c * c);
		useRefract = false;
		// Avoid total internal reflection
		if (dirChange >= 0) {
			const MyVector<float> refractDir = incidentDir * ratio + N * (ratio * c - sqrtf(dirChange));
			const Ray refractiveRay(pos, refractDir);

			int previous = current;
			current = -1;
			float distMin = FLT_MAX;
			std::vector<int> objToCheck;
			BVHTree[frameNo].check(objToCheck, refractiveRay);
			for (int objIdx : objToCheck) {
				hits[objIdx] = objects[frameNo][objIdx]->intersect(refractiveRay, true);
				float dist = hits[objIdx].dist;
				if (hits[objIdx].hit && dist < distMin) {
					distMin = dist;
					current = objIdx;
				}
			}

			if (current >= 0) {
				refractTask = std::async(std::launch::async, &MyCamera::traceRay, this, r, hits, std::cref(pos), current, previous, objNum, lightNum, frameNo, depth);
				useRefract = true;
			}
			else {
				combined += bgVec * refractWeight;
			}
		}
	}

	// Reflection ray
	if (useReflect) {
		MyVector<float> reflectDir = incidentDir - (N * I_dot_N * 2);
		Ray reflectiveRay(pos, reflectDir);

		int previous = current;
		current = -1;
		float distMin = FLT_MAX;
		std::vector<int> objToCheck;
		BVHTree[frameNo].check(objToCheck, reflectiveRay);
		for (int objIdx : objToCheck) {
			hits[objIdx] = objects[frameNo][objIdx]->intersect(reflectiveRay, true);
			float dist = hits[objIdx].dist;
			if (hits[objIdx].hit && dist < distMin && objIdx != previous) {
				distMin = dist;
				current = objIdx;
			}
		}

		if (current >= 0) {
			reflectTask = std::async(std::launch::async, &MyCamera::traceRay, this, r, hits, std::cref(pos), current, -1, objNum, lightNum, frameNo, depth);
		}
		else {
			combined += bgVec * reflectWeight;
			useReflect = false;
		}
	}

	// (Indirect) Diffusion ray, path-trace mode only
	if (useDiffuse) {
		/* Previously used naive diffusion model
		while (invalid) {
			diffuseDir = { uniformRand(e), uniformRand(e), uniformRand(e) };
			invalid = diffuseDir.length() <= EPSILON;
		}

		// Make sure that it's in the same hemisphere as normal vector
		if (diffuseDir.dot(N) < 0) diffuseDir *= -1; */

		// Lambertian model is used for diffusion: diffuse ray shoots from the incident point P
		// to a random position on a sphere with radius 1 centered at P + N (surface normal)
		bool invalid = true;
		MyVector<float> diffuseDir, randPoint;
		while (invalid) {
			// Ensure diffuse direction has a non-zero length
			randPoint = { uniformRand(e), uniformRand(e), uniformRand(e) };
			diffuseDir = N - randPoint.normalize();
			invalid = diffuseDir.length() <= EPSILON; 
		}

		Ray diffusiveRay(pos, diffuseDir);

		int previous = current;
		current = -1;
		float distMin = FLT_MAX;
		std::vector<int> objToCheck;
		BVHTree[frameNo].check(objToCheck, diffusiveRay);
		for (int objIdx : objToCheck) {
			hits[objIdx] = objects[frameNo][objIdx]->intersect(diffusiveRay, true);
			float dist = hits[objIdx].dist;
			if (hits[objIdx].hit && dist < distMin && objIdx != previous) {
				distMin = dist;
				current = objIdx;
			}
		}

		if (current >= 0) {
			// Limit diffusion depth as it is too expensive, treat it as if it had 2x the recursion depth
			diffuseTask = std::async(std::launch::async, &MyCamera::traceRay, this, r, hits, std::cref(pos), current, previous, objNum, lightNum, frameNo, depth * 2);
		}
		else {
			combined += bgVec * diffuseWeight;
			useDiffuse = false;
		}
	}

	// Add all components together for final color
	if (useReflect) combined += reflectTask.get() * reflectWeight;
	if (useRefract) combined += refractTask.get() * refractWeight;
	if (useDiffuse) combined += diffuseTask.get() * diffuseWeight;

	return combined;
}

// Generates camera transformation matrix (and aperture's direction vectors)
void MyCamera::generateTransformationMatrix() {
	// Construct rotation matrix
	MyVector<float> F = (lookAt - position).normalize();
	MyVector<float> R = F.cross(upVector).normalize();
	MyVector<float> U = R.cross(F);
	MyMatrix<float> RotMat(3, 3, 0);

	rightDir = R * apertureDiameter;
	upDir = U * apertureDiameter;

	U = U * -1; // invert up direction (y-axis) for left-handed coordinate system
	RotMat[0] = R.data;
	RotMat[1] = U.data;
	RotMat[2] = F.data;
	RotMat = RotMat.transpose();
	// Calculate translation vector
	MyVector<float> t = position.multiply(RotMat) * -1;

	// Construct translation matrix
	TranMat[0] = { R[0], U[0], F[0], t[0] };
	TranMat[1] = { R[1], U[1], F[1], t[1] };
	TranMat[2] = { R[2], U[2], F[2], t[2] };
}

// Generate camera rays and assign weightings to each
void MyCamera::generateRays() {
	rays = std::vector<Ray>(width * height * cameraSamples);
	wRay = std::vector<float>(width * height * cameraSamples);
	int x, y, s, i = 0;
	float maxOffsetDist = sqrt(2.0);
	std::cout << "Generating camera rays " << std::endl;
	ProgressBar progressBar(width * height * cameraSamples, pBarSize, name, 0);
	for (x = 0; x < width; x++) {
		float xPos = x + 0.5 - hfWidth;
		for (y = 0; y < height; y++) {
			float yPos = hfHeight - y - 0.5;
			for (s = 0; s < cameraSamples; s++) {
				float dx = std::clamp<float>(normalRand(e), -1, 1), dy = std::clamp<float>(normalRand(e), -1, 1);
				float weight = maxOffsetDist - sqrt(dx * dx + dy * dy);
				MyVector<float> pxPos = { xPos + dx, yPos + dy, -f, 1 };
				if (s == 0) {
					weight = 1;
					pxPos = { xPos, yPos, -f, 1 };
				}
				progressBar.update();
				wRay[i] = weight;
				Ray newRay;
				if (simulateAperture) {
					// Constrain the random position to be within a unit circle of radius 1
					float dx, dy, length = 2;
					while (length > 1 + EPSILON) {
						dx = uniformRand(e), dy = uniformRand(e);
						length = dx * dy;
					}
					MyVector<float> onAperture = position + upDir * uniformRand(e) + rightDir * uniformRand(e);
					MyVector<float> direction = onAperture - pxPos.multiply(TranMat);
					newRay = Ray(onAperture, direction);
				}
				else {
					MyVector<float> direction = position - pxPos.multiply(TranMat);
					newRay = Ray(position, direction);
				}
				rays[i++] = newRay; // i++ returns i before increment
			}
		}
	}
	std::cout << "\nSuccessfully generated " << i << " camera rays of type: " << type << std::endl;
}

// Update camera rays' origin to match camera position for each frame
void MyCamera::updateRays(int f, int lastPos)
{
	if (frames < 2) return; // No need for non-animations
	MyVector<float>& posLast = position, & posNow = camMovement[f];
	if (lastPos >= 0) {
		posLast = camMovement[lastPos];
	}
	MyVector<float> delta = posNow - posLast;
	for (Ray& ray : rays) {
		ray.o += delta;
	}
}

// Calibrate camera rays to its accurate position at a specific frame 
// (usually not needed unless render didn't start at frame 0)
void MyCamera::calibrateRays(int f)
{
	if (frames < 2 || f < 0) return;
	MyVector<float>& posCurrent = camMovement[f];
	MyVector<float> delta = posCurrent - position;
	for (Ray& ray : rays) {
		ray.o += delta;
	}
}
