#include <filesystem>
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "ImagePPM.h"
#include "MyMatrix.h"
#include "MyVector.h"
#include "MyCamera.h"

int main(int argc, char* argv[]) {
	const std::vector<std::string> renderType = {
		"scene default", "binary", "ray-trace (blinn-phong)", "path-trace", "preview" };
	std::string help = "Usage:\n";
	help += "RunRaytracer.exe [-r render_dir] [-s scene_dir] [-f frame_start] ";
	help += "[-m frame_max] [-t step] [-p|-d|-q] [scene_name]\n";
	help += "\t-r set directory to save render, defaults to ../TestSuite/\n";
	help += "\t-s set directory to load scene from, defaults to ../Resources/\n";
	help += "\t-f set the index of starting frame, defaults to 0\n";
	help += "\t-m set the maximum number of frames to render, defaults to +inf\n";
	help += "\t-t set the step for frames to render, defaults to 1\n";
	help += "\t-p Preview: overwrite render setting with preview option\n";
	help += "\t-d Decent:  overwrite render setting with ray-tracer option\n";
	help += "\t-q Quality: overwrite render setting with path-tracer option\n";
	help += "\tscene_name: scene to render (no file extension), defaults to scene_anim";

	std::string testSuiteDir = "../TestSuite/", resourcesDir = "../Resources/";
	std::vector<std::string> sceneName = { "scene_anim" };
	std::vector<MyCamera> cam(sceneName.size());
	std::vector<RenderMode> passes = { scene };
	int fStart = 0, fMax = INT32_MAX, fStep = 1;

	std::vector<std::string> args(argc);
	for (int i = 0; i < argc; i++) {
		args[i] = argv[i];
	}

	int state = 0; // ignore first argument (binary name)
	for (std::string& arg : args) {
		// Skip binary's name if passed in
		int len = arg.length();
		if (len >= 12 && arg.substr(len-12, 12) == "RunRaytracer")
			continue;
		// Parse flags
		if (arg[0] == '-') {
			switch (arg[1])
			{
			case 'r':
				state = 1;
				break;
			case 's':
				state = 2;
				break;
			case 'f':
				state = 3;
				break;
			case 'm':
				state = 4;
				break;
			case 't':
				state = 5;
				break;
			case 'p':
				state = 6;
				break;
			case 'd':
				state = 7;
				break;
			case 'q':
				state = 8;
				break;
			default:
				std::cout << "Unrecognized argument: " << arg << std::endl;
				std::cout << help << std::endl;
				return 0;
			}
		}
		// Parse arguments
		else {
			switch (state)
			{
			case 1:
				testSuiteDir = arg;
				break;
			case 2:
				resourcesDir = arg;
				break;
			case 3:
				fStart = std::stoi(arg);
				break;
			case 4:
				fMax = std::stoi(arg);
				break;
			case 5:
				fStep = std::stoi(arg);
				break;
			case 6:
				passes[0] = preview;
				state = 0;
				break;
			case 7:
				passes[0] = phong;
				state = 0;
				break;
			case 8:
				passes[0] = pathtracer;
				state = 0;
				break;
			}
			if (state == 0)
				sceneName[0] = arg;
			state = 0;
		}
	}

	std::chrono::steady_clock::time_point begin, end;
	std::string unit;
	float renderTime;

	for (int i = 0; i < sceneName.size(); i++)
		cam[i].readConfig(resourcesDir + sceneName[i]);

	for (int i = 0; i < sceneName.size(); i++) {
		for (RenderMode r : passes) {
			std::cout << "\nRender pass: " << renderType[r] << std::endl;
			int f = fStart, total = cam[i].frames, fCounter = 0;
			if (total <= fStart) {
				throw std::runtime_error("Frame start (" + std::to_string(fStart) +
					") exceeds total number of frames in scene: " + std::to_string(total));
			}
			std::string totalFrame = " / " + std::to_string(total) + " frames";
			// Calibrate camera position
			int prevFrame = f - 1;
			if (f != 0) cam[i].calibrateRays(prevFrame);
			for (f; f < total; f += fStep) {
				std::cout << "Frame: " << f << totalFrame << std::endl;
				begin = std::chrono::steady_clock::now();
				cam[i].renderFrame(testSuiteDir + sceneName[i], f, prevFrame, r);
				end = std::chrono::steady_clock::now();
				unit = "ms";
				renderTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
				if (renderTime > 1000.0f) {
					renderTime /= 1000.0f;
					unit = "s";
				}
				std::cout << "\nThat took " << renderTime << unit << " to render and post-process" << std::endl;
				if (fMax <= ++fCounter) {
					break;
				}
				prevFrame = f;
			}
		}
	}

	return 0;

	// Old testing codes for image I/O tests
	/*
	int width = 800, height = 600;
	const unsigned char red = 120, green = 60, blue = 90;

	// Blank.ppm: pure black
	PPMImage blank(width, height);
	blank.write(testSuiteDir + "0_Blank.ppm");

	// SomeRed.ppm: 300x300 red region
	PPMImage someRed(width, height);
	someRed.setRegion(500, 300, width, height, 0, red);
	someRed.write(testSuiteDir + "1_SomeRed.ppm");

	// SomeBlue.ppm: 400x400 blue region overlapping red
	PPMImage someBlue = someRed;
	someBlue.setRegion(400, 200, width, height, 2, blue);
	someBlue.write(testSuiteDir + "2_SomeBlue.ppm");

	// SomeGreen.ppm: 500x500 green region overlapping blue
	PPMImage someGreen = someBlue;
	someGreen.setRegion(300, 100, width, height, 1, green);
	someGreen.write(testSuiteDir + "3_SomeGreen.ppm");

	// Gray.ppm: filled with (120, 120, 120)
	PPMImage gray(width, height);
	gray.setAllPixels(std::vector<unsigned char>(width * height * 3, 120));
	gray.write(testSuiteDir + "Gray.ppm");

	// Final.ppm: adding Gray.ppm's pixel values with SomeGreen.ppm's
	MyMatrix<unsigned char> grayMatrix(height, width * 3, char{});  // Matrix with row-major order
	MyMatrix<unsigned char> greenMatrix(height, width * 3, char{});

	auto grayPixels = gray.getAllPixels();
	auto greenPixels = someGreen.getAllPixels();

	for (size_t row = 0; row < height; ++row) {
		for (size_t col = 0; col < width * 3; ++col) {
			grayMatrix[row][col] = grayPixels[row * width * 3 + col];
			greenMatrix[row][col] = greenPixels[row * width * 3 + col];
		}
	}

	// Use MyMatrix's addition operator
	MyMatrix<unsigned char> finalMatrix = grayMatrix + greenMatrix;

	// Convert the matrix back to a flat vector for PPMImage
	std::vector<unsigned char> finalPixels(width * height * 3);
	for (size_t row = 0; row < height; ++row) {
		for (size_t col = 0; col < width * 3; ++col) {
			finalPixels[row * width * 3 + col] = finalMatrix[row][col];
		}
	}

	PPMImage finalImage(width, height);
	finalImage.setAllPixels(finalPixels);
	finalImage.write(testSuiteDir + "4_Final.ppm");

	PPMImage triangleImage = finalImage;
	float back_vertices[6] = { 100, 200, 500, 50, 300, 400 };
	unsigned char colors[9] = { 255,0,0,0,255,0,0,0,255 };
	triangleImage.drawTriangle(back_vertices, colors);
	triangleImage.write(testSuiteDir + "5_Culling.ppm");

	PPMImage cullImage(width, height);
	cullImage.setAllPixels(triangleImage.getAllPixels());
	float vertices[6] = { 600, 350, 0, 0, 200, 400 };
	cullImage.drawTriangle(vertices, colors);
	cullImage.write(testSuiteDir + "6_Triangle.ppm");

	float not_back_vertices[6] = { 100, 200, 300, 400, 500, 50 };
	cullImage.drawTriangle(not_back_vertices, colors);
	cullImage.write(testSuiteDir + "7_SanityCheck.ppm");

	// Measure drawing time of 1000 triangle faces of 54963 pixels (5-15 sec)
	int i;
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	for (i = 0; i < 1000; i++) {
		cullImage.drawTriangle(back_vertices, colors);
		cullImage.drawTriangle(not_back_vertices, colors);
	}
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Drawing 1000 culled and unculled faces of 54963 pixels took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
		<< " [ms]" << std::endl;

	begin = std::chrono::steady_clock::now();
	for (i = 0; i < 1000; i++) {
		cullImage.drawTriangle(back_vertices, colors);
	}
	end = std::chrono::steady_clock::now();
	std::cout << "Drawing 1000 unculled faces of 54963 pixels took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
		<< " [ms]" << std::endl;

	begin = std::chrono::steady_clock::now();
	for (i = 0; i < 1000; i++) {
		cullImage.drawTriangle(not_back_vertices, colors);
	}
	end = std::chrono::steady_clock::now();
	std::cout << "Drawing 1000 unculled faces of 54963 pixels took "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
		<< " [ms]" << std::endl;
	cullImage.write(testSuiteDir + "8_ExtraSanityCheck.ppm");

	width = 1200; height = 800;
	PPMImage triangleTest(width, height);
	const std::string scene = "mirror_image";
	const std::string fileName = testSuiteDir + scene + ".json";
	std::cout << "cam create";
	MyCamera cam;
	cam.readConfig(fileName);
	std::cout << " success" << std::endl;
	std::cout << "read json";

	std::ifstream file(fileName);
	nlohmann::json sceneConfig;
	file >> sceneConfig;

	for (auto& object : sceneConfig["scene"]["shapes"]) {
		std::string objType = object["type"];
		std::cout << objType << std::endl;
		if (objType == "triangle") {
			std::vector<float> vec0 = object["v0"].get<std::vector<float>>();
			std::vector<float> vec1 = object["v1"].get<std::vector<float>>();
			std::vector<float> vec2 = object["v2"].get<std::vector<float>>();
			MyVector<float> v0 = { vec0.at(0), vec0.at(1), vec0.at(2), 1 };
			MyVector<float> v1 = { vec1.at(0), vec1.at(1), vec1.at(2), 1 };
			MyVector<float> v2 = { vec2.at(0), vec2.at(1), vec2.at(2), 1 };
			v0 = cam.world2screen(v0);
			v1 = cam.world2screen(v1);
			v2 = cam.world2screen(v2);
			std::cout << "Translated!" << std::endl;
			float triangleVertices[6] = { v0[0], v0[1], v1[0], v1[1], v2[0], v2[1] };
			triangleTest.drawTriangle(triangleVertices, colors);
		}
	}

	triangleTest.write(testSuiteDir + scene + ".ppm");
	*/
}

