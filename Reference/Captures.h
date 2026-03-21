#pragma once
#include "Utils.h"
#include "BattleFireVulkan.h"
Texture* LoadHDRICubeMapFromFile(const char* inFilePath,int inCubeMapResolution,
	const char *inVSFilePath= "Res/SkyBox.vsb",const char*inFSFilePath= "Res/Texture2D2CubeMap.fsb");
Texture* CaptureDiffuseIrradiance(Texture* inSrcCubeMap,int inCubeMapResolution, 
	const char* inVSFilePath = "Res/SkyBox.vsb", const char* inFSFilePath = "Res/CaptureDiffuseIrradiance.fsb");
Texture* CapturePrefilteredColor(Texture* inSrcCubeMap, int inCubeMapResolution,
	const char* inVSFilePath = "Res/SkyBox.vsb", const char* inFSFilePath = "Res/CapturePrefilteredColor.fsb");
Texture* GenerateBRDF(int inResolution=512, const char* inVSFilePath = "Res/FSQ.vsb", const char* inFSFilePath = "Res/CaptureBRDF.fsb");