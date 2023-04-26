#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <windows.h>

void ReadDirectory(const std::string& directory, const std::string& pattern, std::map<std::string, std::string>& filelist, bool recursive)
{
	std::string searchpattern(directory + "/" + pattern);
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile(searchpattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			std::string filename(data.cFileName);
			filename.erase(filename.find_last_of("."), std::string::npos);
			filelist[filename] = directory + "/" + data.cFileName;
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
	if (recursive) {
		std::string dirpattern = directory + "/*";
		if ((hFind = FindFirstFile(dirpattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
			do {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					char subdir[MAX_PATH];
					strcpy(subdir, directory.c_str());
					strcat(subdir, "/");
					strcat(subdir, data.cFileName);
					if ((strcmp(data.cFileName, ".") != 0) && (strcmp(data.cFileName, "..") != 0)) {
						ReadDirectory(subdir, pattern, filelist, recursive);
					}
				}
			} while (FindNextFile(hFind, &data) != 0);
			FindClose(hFind);
		}
	}
}

const std::string assetpath = "../../../data/";

const std::string GetAssetPath(){
	return assetpath;
}

const std::string GetFontPath() {
	return assetpath + "fonts/";
}

const std::string GetModelPath() {
	return assetpath + "models/";
}

const std::string GetShaderPath() {
	return assetpath + "shaders/";
}

const std::string GetEnvironmentPath() {
	return assetpath + "environments/";
}

const std::string GetTexturePath() {
	return assetpath + "textures/";
}