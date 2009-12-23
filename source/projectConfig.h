// for VC project
#ifndef _PROJECTCONFIG_H_
#define _PROJECTCONFIG_H_

#include <string>

//#cmakedefine USE_SDL
//#cmakedefine USE_POSIX_SOCKETS
//#cmakedefine X11_AVAILABLE
//
//#cmakedefine HAVE_GLOB_H
//#cmakedefine HAVE_SYS_IOCTL_H
//#cmakedefine HAVE_SYS_FILIO_H
//#cmakedefine HAVE_SYS_TIME_H
//#cmakedefine HAVE_BYTESWAP_H


// trailed slash is needed
// relative path for config_dir is relative to data_dir, ./ stores in data_dir
#define DEFAULT_CONFIG_DIR "./"
#define DEFAULT_DATA_DIR   "./"

// defined in main/main.h; FIXME: there's probably a better way
extern std::string configDir;
extern const std::string dataDir;


#endif