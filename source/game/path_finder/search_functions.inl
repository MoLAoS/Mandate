// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
//
// search_functions.inl
//
// INLINE FILE, do not place any silly namespaces or what-not in here, this
// is a part of search_engine.h
//

/** Goal function for 'normal' search */
class PosGoal {
private:
	Vec2i target; /**< search target */

public:
	PosGoal(const Vec2i &target) : target(target) {}
	
	/** The goal function  @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if pos is target, else false	*/
	bool operator()(const Vec2i &pos, const float costSoFar) const { 
		return pos == target; 
	}
};

/** Goal function for 'get within x of' searches */
class RangeGoal {
private:
	Vec2i target; /**< search target	   */
	float range; /**< range to get within */

public:
	RangeGoal(const Vec2i &target, float range) : target(target), range(range) {}
	
	/** The goal function @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if pos is within range of target, else false	*/
	bool operator()(const Vec2i &pos, const float costSoFar) const { 
		return pos.dist(target) <= range; 
	}
};

/** Goal function using influence map */
template<typename type> class InfluenceGoal {
private:
	const TypeMap<type> *iMap; /**< InfluenceMap to use */	
	type threshold; /**< influence 'threshold' of goal */

public:
	InfluenceGoal(const TypeMap<type> *iMap, type threshold) : iMap(iMap), threshold(threshold) {}
	
	/** The goal function  @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if influence at pos on iMap is greater than threashold, else false */
	bool operator()( const Vec2i &pos, const float costSoFar) const { 
		return iMap->getInfluence(pos) > threshold; 
	}
};

/** Goal function for free cell search */
class FreeCellGoal {
private:
	Field field; /**< field to find a free cell in */

public:
	FreeCellGoal(Field field) : field(field) {}
	
	/** The goal function @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if pos is free, else false */
	bool operator()(const Vec2i &pos, const float costSoFar) const { 
		return theMap.isFreeCell(pos, field); 
	}
};

/** Goal function to find a free position that a unit of 'size' can occupy. */
class FreePosGoal {
private:
	AnnotatedMap *aMap;	 /**< Annotated Map to use		*/
	Field field;	/**< field to find position in	   */
	int size;  /**< size of unit to find position for */

public:
	FreePosGoal(AnnotatedMap *aMap, Field field, int size) : aMap(aMap), field(field), size(size) {}

	/** The goal function  @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos
	  * @return true if a unit of size can occupy pos in field (according to aMap), else false */
	bool operator()(const Vec2i &pos, const float costSoFar) const {
		return aMap->canOccupy(pos, size, field);
	}
};

/** The 'No Goal' function. Just returns false. Use with care! Use Cost function to control termination
  * by exhausting the open list. */
class NoGoal {
public:
	NoGoal(){}
	/** The goal function  @param pos the position to ignore
	  * @param costSoFar the cost of the shortest path to pos
	  * @return false */
	bool operator()(const Vec2i &pos, const float costSoFar) const { 
		return false; 
	}
};

/** Helper goal, used to build distance maps. */
class DistanceBuilderGoal {
private:
	float cutOff; /**< a 'cutoff' distance, search ends after this is reached. */
	TypeMap<float> *iMap; /**< inluence map to write distance data into.	  */

public:
	DistanceBuilderGoal(float cutOff, TypeMap<float> *iMap) : cutOff(cutOff), iMap(iMap) {}
	
	/** The goal function, writes ( cutOff - costSoFar ) into the influence map.
	  * @param pos position to test @param costSoFar the cost of the shortest path to pos
	  * @return true if costSoFar exceeeds cutOff, else false. */
	bool operator()(const Vec2i &pos, const float costSoFar) const {
		if (costSoFar > cutOff) {
			return true;
		}
		if (costSoFar < iMap->getInfluence(pos)) {
			iMap->setInfluence(pos, cutOff - costSoFar);
		}
		return false;
	}
};

/** Helper goal, used to build influence maps. */
class InfluenceBuilderGoal {
private:
	float cutOff;
	TypeMap<float> *iMap;

public:
	InfluenceBuilderGoal(float cutOff, TypeMap<float> *iMap) : cutOff(cutOff), iMap(iMap) {}
	/** The goal function, WIP @param pos position to test
	  * @param costSoFar the cost of the shortest path to pos */
	bool operator()(const Vec2i &pos, const float costSoFar) const {
		if (costSoFar > cutOff) {
			return true;
		}
		if (cutOff - costSoFar > iMap->getInfluence(pos)) {
			iMap->setInfluence(pos, cutOff - costSoFar);
		}
		return false;
	}
};

/** A uniform cost function */
class UniformCost {
private:
	float cost;		/**< The uniform cost to return */

public:
	UniformCost(float cost) : cost(cost) {}

	/** The cost function @param p1 position 1 @param p2 position 2 ('adjacent' p1)
	  * @return cost */
	float operator()(const Vec2i &p1, const Vec2i &p2) const { return cost; }
};

/** distance cost, no obstacle checks */
class DistanceCost {
public:
	DistanceCost(){}
	/** The cost function @param p1 position 1 @param p2 position 2 ('adjacent' p1)
	  * @return 1.0 if p1 and p2 are 'in line', else SQRT2 */
	float operator()(const Vec2i &p1, const Vec2i &p2) const {
		assert ( p1.dist(p2) < 1.5 );
		if ( p1.x != p2.x && p1.y != p2.y ) {
			return SQRT2;
		}
		return 1.0f;
	}
};

/** The movement cost function */
class MoveCost {
private:
	const int size;				  /**< size of agent	  */
	const Field field;			 /**< field to search in */
	const AnnotatedMap *aMap;	/**< map to search on   */

public:
	MoveCost(const Unit *unit, const AnnotatedMap *aMap) 
			: size(unit->getSize()), field(unit->getCurrField()), aMap(aMap) {}
	MoveCost(const Field field, const int size, const AnnotatedMap *aMap )
			: size(size), field(field), aMap(aMap) {}

	/** The cost function @param p1 position 1 @param p2 position 2 ('adjacent' p1)
	  * @return cost of move, possibly infinite */
	float operator()(const Vec2i &p1, const Vec2i &p2) const {
		assert(p1.dist(p2) < 1.5 && p1 != p2);
		assert(theMap.isInside(p2));
		if (!aMap->canOccupy(p2, size, field)) {
			return numeric_limits<float>::infinity();
		}
		if (p1.x != p2.x && p1.y != p2.y) {
			Vec2i d1, d2;
			getDiags(p1, p2, size, d1, d2);
			assert(theMap.isInside(d1) && theMap.isInside(d2));
			if (!aMap->canOccupy(d1, 1, field) || !aMap->canOccupy(d2, 1, field) ) {
				return numeric_limits<float>::infinity();
			}
			return SQRT2;
		}
		return 1.0f;
		// todo... height
	}
};

/** Diaginal Distance Heuristic */
class DiagonalDistance {
public:
	DiagonalDistance(const Vec2i &target) : target(target) {}
	/** search target */
	Vec2i target;	
	/** The heuristic function. @param pos the position to calculate the heuristic for
	  * @return an estimate of the cost to target */
	float operator()(const Vec2i &pos) const {
		float dx = (float)abs(pos.x - target.x), 
			  dy = (float)abs(pos.y - target.y);
		float diag = dx < dy ? dx : dy;
		float straight = dx + dy - 2 * diag;
		return 1.4 * diag + straight;
	}
};

/** Diagonal Distance Overestimating Heuristic */
class OverEstimate {
private:
	Vec2i target; /**< search target */

public:
	OverEstimate(const Vec2i &target) : target(target) {}
	
	/** The heuristic function. @param pos the position to calculate the heuristic for
	  * @return an (over) estimate of the cost to target */
	float operator()(const Vec2i &pos) const {
		float dx = (float)abs(pos.x - target.x), 
			  dy = (float)abs(pos.y - target.y);
		float diag = dx < dy ? dx : dy;
		float estimate = 1.4 * diag + (dx + dy - 2 * diag);
		estimate *= 1.25;
		return estimate;
	}
};

/** Diagonal Distance underestimating Heuristic */
class UnderEstimate {
private:
	Vec2i target; /**< search target */

public:
	UnderEstimate(const Vec2i &target) : target(target) {}
	
	/** The heuristic function. @param pos the position to calculate the heuristic for
	  * @return an (over) estimate of the cost to target */
	float operator()(const Vec2i &pos) const {
		float dx = (float)abs(pos.x - target.x), 
			  dy = (float)abs(pos.y - target.y);
		float diag = dx < dy ? dx : dy;
		float estimate = 1.4 * diag + (dx + dy - 2 * diag);
		estimate *= 0.75;
		return estimate;
	}
};

/** The Zero Heuristic, for doing Dijkstra searches */
class ZeroHeuristic {
public:
	ZeroHeuristic(){}
	/** The 'no heuristic' function. @param pos the position to ignore @return 0.f */
	float operator()(const Vec2i &pos) const { return 0.0f; }
};
