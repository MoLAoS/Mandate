// checks that really physfs 2.0 or later is available
// as the find module in cmake can't check for a version

#include <physfs.h>


int main(int argc, char **argv){
	PHYSFS_init(argv[0]);
	if(PHYSFS_isInit()){
		PHYSFS_mount("./", 0, 1);

		PHYSFS_deinit();
	}

	return 0;
}
