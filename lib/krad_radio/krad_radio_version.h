#define VERSION_NUMBER 13

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define KRAD_VERSION VERSION_NUMBER
#define VERSION_STRING STR(VERSION_NUMBER)
#define VERSION VERSION_STRING
#define APPVERSION "Krad Radio Version " VERSION_STRING
#define KRAD_VERSION_STRING APPVERSION
#define KRAD_GIT_VERSION GIT_VERSION
