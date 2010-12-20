// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include <iostream>
#include <string>

#include "pixmap.h"
#include "math_util.h"

using namespace std;

namespace BlendNorm {

using namespace Shared::Graphics;
using namespace Shared::Math;
using namespace Shared::Platform;

/** @param filenames an array of three c-strings identifying the files to use for red, green and blue channels
  * @param redMap reference to a Pixmap2D pointer to create and load the red image
  * @param greenMap reference to a Pixmap2D pointer to load the green image into
  * @param blueMap reference to a Pixmap2D pointer to load the blue image
  */
void loadPixmaps(char **filenames, Pixmap2D *&redMap, Pixmap2D *&greenMap, Pixmap2D *&blueMap) {
	try {
		redMap = new Pixmap2D();
		redMap->load(filenames[0]);
		greenMap = new Pixmap2D();
		greenMap->load(filenames[1]);
		blueMap = new Pixmap2D();
		blueMap->load(filenames[2]);
	} catch (runtime_error &e) {
		cout << "Error loading pixmaps.\n\t" << e.what() << "\n";
		exit(0);
	}
}

/** Check sanity of the input pixmaps */
void checkPixmaps(Pixmap2D *&redMap, Pixmap2D *&greenMap, Pixmap2D *&blueMap) {
	int width = redMap->getW();
	int height = redMap->getH();
	if (width < 32 || height < 32) {
		cout << "pixmaps too small: pixmaps must be at least 32x32 pixels.\n";
		exit(0);
	}
	if (width != height) {
		cout << "pixmaps not square: pixmaps must have the same width and height.\n";
		exit(0);
	}
	if (!isPowerOfTwo(width)) {
		cout << "bad pixmap dimensions: pixmap dimensions must be a power of two.\n";
		exit(0);
	}
	if (greenMap->getW() != width || greenMap->getH() != height
	|| blueMap->getW() != width || blueMap->getH() != height) {
		cout << "bad pixmaps dimensions: all pixmaps must be the same size!.\n";
		exit(0);
	}
}

int makeBlendmap(char **filenames) {
	Pixmap2D *redMap, *greenMap, *blueMap, *output;

	// load and sanity check input pixmaps
	loadPixmaps(filenames, redMap, greenMap, blueMap);
	checkPixmaps(redMap, greenMap, blueMap);

	// create output pixmap
	int width = redMap->getW();
	int height = redMap->getH();
	output = new Pixmap2D();
	output->init(width, height, 4);

	// scratch vars
	uint8 val;
	float red, green, blue, alpha, rgbSum;
	Vec4f outPixel;

	for (int j=0; j < height; ++j) {
		for (int i=0; i < width; ++i) {

			// for each pixel

			// get red and alpha intensity from redMap
			redMap->getComponent(i, j, 0, val);
			red = val / 255.f;
			redMap->getComponent(i, j, 3, val);
			alpha = val / 255.f;

			// get green intensity from greenMap, and store alpha if larger than existing
			greenMap->getComponent(i, j, 1, val);
			green = val / 255.f;
			greenMap->getComponent(i, j, 3, val);
			if (val / 255.f > alpha) {
				alpha = val / 255.f;
			}

			// get blue intensity from blueMap, and store alpha if larger than existing
			blueMap->getComponent(i, j, 2, val);
			blue = val / 255.f;
			blueMap->getComponent(i, j, 3, val);
			if (val / 255.f > alpha) {
				alpha = val / 255.f;
			}

			// sum rgb
			rgbSum = red + green + blue;
			if (rgbSum > 0.f) {
				// normalise rgb and set a to 1 - alpha
				// rgb: divide by rgbSum (gives us each channel as a ratio of the total rgb)
				//    : multiply by alpha, so that the output r + g + b + a == 1.0
				outPixel.r = red / rgbSum * alpha;
				outPixel.g = green / rgbSum * alpha;
				outPixel.b = blue / rgbSum * alpha;
				outPixel.a = 1.f - alpha;
			} else {
				// no colour, set a == 1.0
				outPixel = Vec4f(0.f, 0.f, 0.f, 1.f);
			}

			output->setPixel(i, j, outPixel);
		}
	}
	try {
		output->save(filenames[3]);
	} catch (runtime_error &e) {
		cout << "Error saving output: " << e.what() << "\n";
		return 0;
	}
	cout << "blendmap \"" << filenames[3] << "\" successfully generated.\n\n";
	return 0;
}

} // end namespace BlendNorm

int main(int argc, char **argv) {
	string myName = argv[0];

	string::size_type n = myName.find_last_of('\\');
	if (n == string::npos) {
		n = myName.find_last_of('/');
	}
	if (n != string::npos) {
		myName = myName.substr(n + 1);
	}
	n = myName.find_last_of('.');
	if (n != string::npos) {
		myName = myName.substr(0, n);
	}

	if (argc != 5) {
		cout << "usage:\n" << myName << " infile infile infile outfile\n"
			<< " the three input files specified should contain the red, green & blue channels.\n"
			<< " ie,\n" << myName << " red.png green.png blue.png blendmap.tga\n\n"
			<< "DO NOT use PNG for the output, PNG save does not handle 4 channels.\n\n";
		exit(0);
	}
	return BlendNorm::makeBlendmap(&argv[1]);
}
