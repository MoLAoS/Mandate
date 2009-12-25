// dummy for VC project
#ifndef _PROJECTCONFIG_H_
#define _PROJECTCONFIG_H_

#include <string>


// trailed slash is needed
// relative path for config_dir is relative to data_dir, ./ stores in data_dir
#define DEFAULT_CONFIG_DIR "./"
#define DEFAULT_DATA_DIR   "./"

// defined in main/main.h; FIXME: there's probably a better way
extern std::string configDir;
extern const std::string dataDir;


#endif
