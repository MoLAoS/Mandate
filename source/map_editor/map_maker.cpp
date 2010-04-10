// ==============================================================
//	This file is part of the Glest Advanced Engine
//
//	Copyright (C) 2010 - 
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "map_maker.h"

namespace MapEditor {

/// MAP RANDOMIZATION
/// Terrain randomization is based on the "Diamond-Square" algorithm.
/// Diamond-Square terrain generation works by taking the average of the heights
/// from four cells in a square or diamond pattern and adding a randomization value.
/// Every iteration includes one square stage and one diamond stage. After each iteration
/// there will be 2^n number of cells
/// Object/Resource Randomization is based of of having seed positions and growing a mass
/// of a given object or resource to a set population (TODO randomize population size?).
/// A start point is randomly decided or it is based of off player start location
/// and if the cell is clear a single point is added.
/// The next point to be added in the group is randomly picked from the eight adjacent
/// squares. This continues until the mass has been grown to the desired size.

/* Main TODO list
	* Optimize seeding for terrain
	* Seed some lakes or rivers (on user input?)
	* Properly calculate and properly randomize resource and object placement
		* What is done now I have found is OK for a quick random map game but
		  for anyone who wants the map to look good OK is nowhere near enough.
	* Add realistic weathering to terrain?
		* Rivers/Canyons throught mountains?
		* Earthquakes/Fault lines?
	* Create a dialog for user input on what are now const int values
	* Add some aesthetics
		* Hanged/Impaled
		* Statues
		* Mountains
		* Recalculate some terrain here to make it look better? (see realistic weathering)
*/
const int MAP_WIDTH = 64;

void MapMaker::randomize() {
	random.init(time(NULL));

	// Set height to a base level
	// randomizeHeights() was taken out because there is already a button
	// for it and much more realistic terrain should be randomized
	int baseHeight = random.randRange(8, 12);
	map->reset(MAP_WIDTH, MAP_WIDTH, baseHeight, 1);

	///@todo pass more parameters to these functions
	setStartLocations(baseHeight);
	diamondSquare(baseHeight);
	
	//generateResoures();

	// forest growth is sometimes getting stuck in an infinite loop 
	//growForests();

	//addEyeCandy();
}

void MapMaker::setStartLocations(float baseHeight, int numFactions, float minDistance) {
	// Randomizes player start locations and makes sure to seed the corresponding terrain values
	// TODO I havn't figured out any optimal seeding yet so the actual method of seeding
	// still needs to be finallized
	map->resetFactions(numFactions);

	for (int i = 0; i < numFactions; ++i) {
		int randX = random.randRange(1, 5);
		int randY = random.randRange(1, 5);
		int randXmod = random.randRange((map->w / 15), (2 * map->w / 15));
		int randYmod = random.randRange((map->h / 15), (2 * map->h / 15));

		map->startLocations[i].x = (map->w / 5) * randX - randXmod;
		map->startLocations[i].y = (map->h / 5) * randY - randYmod;
		// check to make sure we arn't too close to other players
		for (int j = 0; j < numFactions; j++) {
			if (i == j) continue;
			if (map->startLocations[i].dist(map->startLocations[j]) < minDistance) {
				i--;
				break;
			}
		}
		// Seed the height of the player location up
		for (int x = -1; x < 2; x++) {
			for (int y = -1; y < 2; y++) {
				// This formula is a little bit brutal to read:
				// Basically it takes the start locations and divides them by one eighth the map width
				// Rounds the eighth map width numbers and multiplies them by one eighth the mpa width
				// This has the effect of finding the closest point that is a one eighth cell
				// finding the one eighth (or 1/16) cells is important because it ensures the points are calculated
				// on an early iteration of the diamond-square algorithm and are thus good seed points
				int X1 = ((int)(((map->startLocations[i].x + x*(MAP_WIDTH / 8.f)) / (MAP_WIDTH / 8.f)) + 0.5f)) * (MAP_WIDTH / 8.f);
				int Y1 = ((int)(((map->startLocations[i].y + y*(MAP_WIDTH / 8.f)) / (MAP_WIDTH / 8.f)) + 0.5f)) * (MAP_WIDTH / 8.f);
				int X2 = ((int)(((map->startLocations[i].x + x*(MAP_WIDTH / 16.f)) / (MAP_WIDTH / 16.f)) + 0.5f)) * (MAP_WIDTH / 16.f);
				int Y2 = ((int)(((map->startLocations[i].y + y*(MAP_WIDTH / 16.f)) / (MAP_WIDTH / 16.f)) + 0.5f)) * (MAP_WIDTH / 16.f);
				if (map->inside (X1, Y1)) {
					map->cells[X1][Y1].height = baseHeight + 3;
				}
				if (map->inside (X2, Y2)) {
					map->cells[X2][Y2].height = baseHeight + 3;
				}
			}
		}
	}
}

void MapMaker::diamondSquare(float baseHeight, float delta, float roughness) {
	// Defines what level of the algorithm we are at right now
	// This will be a control variable in the for loops
	// Square stage will loop through 0 to map_width
	// while diamond stages will use -stepping to (map_width - stepping)
	// after each stage stepping will be cut in half for the next stage
	int stepping = MAP_WIDTH;
	float heightRand = delta;

	// the roughness constant should be bellow zero if the randomization is decreasing
	// with each itteration the more negative it is the less randomness there will be.
	const float ROUGHNESS_CONSTANT = roughness;
	const float DELTA_HEIGHT = delta;

	// Square stage
	// loop through the map taking the four corners of each square
	// average them and add a random value
	// AD		o		CE
	//
	//
	// o		BE		o
	//
	//
	// AF		o		CF

	// Diamond stage
	// loop through the map taking the four corner values of each diamond
	// average them and add a random value
	// o		BD		o
	//
	//
	// AE		BE		CE
	//
	//
	// o		BF		o

	// For simplicity's sake we will assume that the map is a square grid of size (2^n) + 1
	// If we try to index a cell outside this range we will assume the map wraps
	// Or maybe I'll think of something better...

	for (int t = 0; t < log2(MAP_WIDTH); t++) {

		for (int i = 0; i < MAP_WIDTH; i+=stepping) {
			for (int j = 0; j < MAP_WIDTH; j+=stepping) {
				int A = i;
				int B = i + stepping/2;
				int C = i + stepping;

				int D = j;
				int E = j + stepping/2;
				int F = j + stepping;

				//if (A < 0) A += MAP_WIDTH;
				if (C > MAP_WIDTH - 1) C -= MAP_WIDTH;

				//if (D < 0) D += MAP_WIDTH;
				if (F > MAP_WIDTH - 1) F -= MAP_WIDTH;

				if (map->cells[B][E].height == baseHeight) {
					map->cells[B][E].height =
						((map->cells[A][D].height +
						map->cells[C][E].height +
						map->cells[A][F].height +
						map->cells[C][F].height) * 0.25f) +
						random.randRange(-heightRand, heightRand);
				}
			}
		}

		stepping /= 2;
		// Stepping was divided by 2 here so for the diamond stage
		// We need to skip every other diamond
		bool skip = true;

		for (int i = -stepping; i <= MAP_WIDTH-stepping; i+=stepping) {
			for (int j = -stepping; j <= MAP_WIDTH-stepping; j +=stepping) {
				if (skip) {
					skip = !skip;
					continue;
				}
				int A = i;
				int B = i + stepping;
				int C = i + stepping*2;

				int D = j;
				int E = j + stepping;
				int F = j + stepping*2;

				if (A < 0) A += MAP_WIDTH;
				if (C > MAP_WIDTH - 1) C -= MAP_WIDTH;

				if (D < 0) D += MAP_WIDTH;
				if (F > MAP_WIDTH - 1) F -= MAP_WIDTH;

				if (map->inside(B, E) && map->cells[B][E].height == baseHeight) {
					map->cells[B][E].height =
						((map->cells[B][D].height +
						map->cells[A][E].height +
						map->cells[C][E].height +
						map->cells[B][F].height) * 0.25f) +
						random.randRange(-heightRand, heightRand);
				}
				skip = !skip;
			}
		}
		heightRand *= pow(2, ROUGHNESS_CONSTANT);
	}
}

void MapMaker::generateResoures(/* something to control 'resource allotment' */) {
	const int GOLD_PILE = 5; // size of each resource pile
	const int STONE_PILE = 3;
	const int RESOURCE_SEEDS = 1; // extra seeds for resource deposits
	const int RESOURCE_RADIUS = 5; // distance from the start location to place resource seeds

	for (int i = 0; i < map->maxFactions + RESOURCE_SEEDS; i++) {
		Vec2i goldSeed;
		int goldRand;

		do {
			goldRand = random.randRange(0, 7);
			if (goldRand > 3) goldRand++;

			goldSeed.x = ((i < map->maxFactions)? map->startLocations[i].x:30) + (((goldRand % 3) - 1) * RESOURCE_RADIUS);
			goldSeed.y = ((i < map->maxFactions)? map->startLocations[i].y:30) + (((goldRand / 3) - 1) * RESOURCE_RADIUS);
		} while (!map->inside (goldSeed.x, goldSeed.y));

		for (int j = 0; j < GOLD_PILE; j++) {
			map->cells[goldSeed.x][goldSeed.y].resource = 1;
			while (map->cells[goldSeed.x][goldSeed.y].resource == 1) {
				int next = random.randRange(0,8);
				if (map->inside( goldSeed.x + ((next % 3) - 1), goldSeed.y + ((next / 3) - 1) )) {
					goldSeed.x += ((next % 3) - 1);
					goldSeed.y += ((next / 3) - 1);
				}
			}
		}

		Vec2i stoneSeed;
		int stoneRand;
		do {
		stoneRand = random.randRange(0, 7);
		if (stoneRand > 3) stoneRand++;
		if (stoneRand == goldRand) {
			stoneRand += 5 + goldRand;
			while (stoneRand > 8) {
				stoneRand -= 8;
			}
		}

		stoneSeed.x = ((i < map->maxFactions)? map->startLocations[i].x:30) + (((stoneRand % 3) - 1) * RESOURCE_RADIUS);
		stoneSeed.y = ((i < map->maxFactions)? map->startLocations[i].y:30) + (((stoneRand / 3) - 1) * RESOURCE_RADIUS);
		} while (!map->inside (stoneSeed.x, stoneSeed.y));

		for (int j = 0; j < STONE_PILE; j++) {
			map->cells[stoneSeed.x][stoneSeed.y].resource = 2;
			while (map->cells[stoneSeed.x][stoneSeed.y].resource == 2) {
				int next = random.randRange(0,8);
				if (map->inside( stoneSeed.x + ((next % 3) - 1), stoneSeed.y + ((next / 3) - 1) )) {
					if (map->cells[stoneSeed.x + ((next % 3) - 1)][stoneSeed.y + ((next / 3) - 1)].resource == 0) {
						stoneSeed.x += ((next % 3) - 1);
						stoneSeed.y += ((next / 3) - 1);
					}
				}
			}
		} // End stone populaton
	} // End for: resource populaton
}

void MapMaker::growForests(/* control variables ?? */) {
	const int FOREST_SEEDS = 3; // number of extra seed points to use apart from forests near start locations
	const int FOREST_GROWTH = 50; // size of each forest
	const int FOREST_RADIUS = 8; // distance of forest from player start location Should be about RESOURCE_RADIUS + 3

	for (int i = 0; i < FOREST_SEEDS + map->maxFactions; i++) {
		Vec2i forestSeed;
		// Make sure we aren't seeding outside the map or on top of anything
		// and set a seed point
		do {
			int forestRand = random.randRange(0,7);
			if (forestRand > 3) forestRand++;

			forestSeed.x = ((i < map->maxFactions)? map->startLocations[i].x:30) + (((forestRand % 3) - 1) * FOREST_RADIUS);
			forestSeed.y = ((i < map->maxFactions)? map->startLocations[i].y:30) + (((forestRand / 3) - 1) * FOREST_RADIUS);
		} while (!map->inside (forestSeed.x, forestSeed.y) || (map->cells[forestSeed.x][forestSeed.y].resource != 0) );

		// grow the forests
		map->cells[forestSeed.x][forestSeed.y].object = 1;

		bool doNext = false;

		for (int j = 0; j < FOREST_GROWTH; j++) {
			// Loop until there is a spot that forest can be planted

			//INFINITE LOOP: this occasionally never ends...
			while (map->cells[forestSeed.x][forestSeed.y].object == 1 || !doNext) {
				int next = random.randRange(0,8);
				if (map->inside( forestSeed.x + ((next % 3) - 1), forestSeed.y + ((next / 3) - 1) )) {
					if ( map->cells[forestSeed.x + ((next % 3) - 1)][forestSeed.y + ((next / 3) - 1)].resource == 0 ) {
						// Check to make sure the forest isn't growing into a player's territory
						doNext = true;
						for (int k = 0; k < map->maxFactions; k++) {
							if (forestSeed.dist(map->startLocations[k]) < FOREST_RADIUS) {
								doNext = false;
								break;
							}
						}
						forestSeed.x += ((next % 3) - 1);
						forestSeed.y += ((next / 3) - 1);
					}
				}
			}

			map->cells[forestSeed.x][forestSeed.y].object = 1;
			doNext = false;
		} // End Single Forest Growth
	} // End Forest Seeding
}


} // end namespace MapEditor
