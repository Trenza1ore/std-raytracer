#pragma once

#define pBarSize 50

#include <iostream>
#include <string>
#include <chrono>

// A progress bar class for tracking progress and updating in real time
class ProgressBar {
public:
	ProgressBar(int iterCount, int barLength, std::string& name, int objNum) {
		progressBarLength = barLength;

		PUnit = iterCount / barLength;
		PPercent = 100.0 / barLength;

		pUnit = PUnit / 10;
		pPercent = 10.0 / barLength;

		if (pUnit < 1) {
			std::cout << "Number of iterations too low, progress bar will be disabled";
			std::cout << std::endl;
			disabled = true;
		}

		clearPrevious = std::string(barLength + 32, '\b') + '[';

		if (objNum > 0)
			std::cout << "Rendering <" << name << "> (" << objNum << " objects)\n";

		if (!disabled) {
			std::cout << "[Starting..." << std::string(progressBarLength - 11, ' ')
				<< "]  0% (00:00:00:00/??:??:??:??)" << std::flush;
			begin = std::chrono::steady_clock::now();
			checkpoint = begin;
		}
	}

	// Update progress bar
	void update() {
		if (disabled) return;

		progress++;
		bool isMajor = (progress % PUnit == 0);
		bool isMinor = (progress % pUnit == 0);

		int percentage;

		// Minor tick
		if (isMinor) {
			percentage = pPercent * ++pCounter;
			minorPercent++;
		}

		// Major tick overwrites minor tick
		if (isMajor) {
			// Just a sanity check
			if (PCounter < progressBarLength)
				PCounter++;
			percentage = PPercent * PCounter;
			minorPercent = 0;
		}

		// Either tick
		if (isMinor || isMajor) {
			lastCheckpoint = checkpoint;
			checkpoint = std::chrono::steady_clock::now();
			std::cout << clearPrevious << std::string(PCounter, progressBarSymbol);

			// Display additional progress digit for minor tick
			int remainingLength = progressBarLength - PCounter;
			if (isMinor && (remainingLength > 0)) {
				std::cout << std::to_string(std::min(9, minorPercent)).substr(0, 1);
				remainingLength--;
			}
			std::cout << std::string(remainingLength, ' ');

			int renderTime = std::chrono::duration_cast<std::chrono::seconds>(checkpoint - begin).count();
			float sinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(checkpoint - lastCheckpoint).count();
			int estimatedLength = (renderTime * 100.0 / percentage) + (sinceLastUpdate / 10.0 / pPercent);

			std::cout << ']';
			if (percentage < 100) {
				std::cout << ' ';
				std::string firstDigit = std::to_string(percentage / 10);
				if (firstDigit[0] == '0') {
					std::cout << ' ';
				}
				else {
					std::cout << firstDigit;
				}
				std::cout << percentage % 10 << "% (";
			}
			else {
				std::cout << " fin (";
			}
			printTime(renderTime);
			std::cout << '/';
			printTime(estimatedLength);
			std::cout << ')' << std::flush;
		}
	}

	void printTime(int seconds) {
		if (seconds <= 0) {
			std::cout << "??:??:??:??";
			return;
		}
		std::string day = std::to_string(seconds / 86400).substr(0, 2);
		seconds = seconds % 86400;
		std::string hr = std::to_string(seconds / 3600);
		seconds = seconds % 3600;
		std::string min = std::to_string(seconds / 60);
		std::string sec = std::to_string(seconds % 60);
		std::string sep1 = ":", sep2 = ":", sep3 = ":";
		if (day.length() < 2) std::cout << "0";
		if (hr.length() < 2) sep1 = sep1 + "0";
		if (min.length() < 2) sep2 = sep2 + "0";
		if (sec.length() < 2) sep3 = sep3 + "0";
		std::cout << day << sep1 << hr << sep2 << min << sep3 << sec;
	}

protected:
	char progressBarSymbol = '#';
	int PCounter = 0;
	int pCounter = 0;
	int progress = 0;
	int pUnit;
	int PUnit;
	int progressBarLength;
	float PPercent;
	float pPercent;
	int minorPercent = 0;
	bool disabled = false;
	std::string clearPrevious;
	std::chrono::steady_clock::time_point begin, checkpoint, lastCheckpoint;
};
