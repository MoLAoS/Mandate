#include "random.h"

#include <cassert>
#include <time.h>

namespace Shared{ namespace Util{

// =====================================================
//	class Random
// =====================================================

const int Random::m= 714025;
const int Random::a= 1366;
const int Random::b= 150889;

Random::Random(){
	lastNumber= 0;
}

void Random::init(int seed){
	time_t curSeconds = 0;
	time(&curSeconds);
	lastNumber= (seed ^ curSeconds ^ clock()) % m;
}


}}//end namespace
