#include "version.hpp"

#define GIT_COMMIT_HASH 2ae7260
#define GIT_BRANCH master

#define BACKEND_TYPE cpu

#define XMR_STAK_NAME "xmr-stak"
#define XMR_STAK_VERSION "2.2.0"

#if defined(__APPLE__)
#define OS_TYPE "mac"
#elif defined(__FreeBSD__)
#define OS_TYPE "bsd"
#elif defined(__linux__)
#define OS_TYPE "lin"
#else
#define OS_TYPE "unk"
#endif

#define COIN_TYPE "monero"

#define XMRSTAK_PP_TOSTRING1(str) #str
#define XMRSTAK_PP_TOSTRING(str) XMRSTAK_PP_TOSTRING1(str)

#define VERSION_LONG  XMR_STAK_NAME "/" XMR_STAK_VERSION "/" XMRSTAK_PP_TOSTRING(GIT_COMMIT_HASH) "/" XMRSTAK_PP_TOSTRING(GIT_BRANCH) "/" OS_TYPE "/" XMRSTAK_PP_TOSTRING(BACKEND_TYPE) "/" COIN_TYPE "/"
#define VERSION_SHORT XMR_STAK_NAME " " XMR_STAK_VERSION " " XMRSTAK_PP_TOSTRING(GIT_COMMIT_HASH)
#define VERSION_HTML "v" XMR_STAK_VERSION "-" XMRSTAK_PP_TOSTRING(GIT_COMMIT_HASH)

const char ver_long[]  = VERSION_LONG;
const char ver_short[] = VERSION_SHORT;
const char ver_html[] = VERSION_HTML;
