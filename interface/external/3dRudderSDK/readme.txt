
Instructions for adding the 3dRudder library (3dRudderSDK) to Interface
Nicolas PERRET 17/04/2018

Clone or download the 3dRudder SDK from https://github.com/3drudder/3dRudderSDK 


1. Copy the 3dRudderSDK folders from the 3dRudderSDK  directory (Lib, Include) into the interface/externals/3dRudderSDK folder.
   This readme.txt should be there as well.
   
   The files needed in the folders are:
   
   If so our CMake find module expects you to set the ENV variable 'HIFI_LIB_DIR' to a directory containing a subfolder '3dRudderSDK' that contains the 2 folders mentioned above (Include, Lib).
    
2. Clear your build directory, run cmake and build, and you should be all set.
