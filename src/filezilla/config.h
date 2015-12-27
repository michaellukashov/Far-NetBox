#pragma once

/* Check if the Platform SDK is installed. Especially in VC6 some components like
 * the theme api and debugging tools api aren't installed by default, The Platform SDK
 * includes the required headers.
 * Unfortunately there is no safe way to tell whether the platform SDK is installed.
 * One way to guess it is over the INVALID_SET_FILE_POINTER define, it at least is not 
 * defined in the header files of VC6, but it is defined in the Platform SDK.
 */
#ifndef INVALID_SET_FILE_POINTER
#error Please download and install the latest MS Platform SDK from http://www.microsoft.com/msdownload/platformsdk/sdkupdate/
#endif
