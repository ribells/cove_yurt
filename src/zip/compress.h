#pragma once

#include <string>
#include <vector>
using namespace std;

string getLastUnzipFile();
bool unzip_files(string strFile, string strPath, int iCount = -1);
bool zip_files(string strFile, vector<string> files, bool bIncludePaths);
bool zip_files(string strFile, vector<string> files, vector<string> filenames, bool bIncludePaths);

bool compressFiles(string strFile, vector<string> files);
void decompressFiles(string strFile, string strOutputFolder);
