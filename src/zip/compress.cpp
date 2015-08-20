 /* =============================================================================
	File: compress.cpp

  =============================================================================== */

#include <errno.h>

#ifdef WIN32
#include <windows.h>
#include <process.h>
#include <direct.h>
#define mkdir(x, y) _mkdir((x))
#else
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "utility.h"
#include "zip/zip.h"
#include "zip/unzip.h"
#include "compress.h"

#define WRITEBUFFERSIZE (8192)

//------[ function prototypes ]------//

void	progress_update(string pText, double val);
void	download_progress_update(string pText, double val);

string	g_LastUnzipFile;

/* =============================================================================
 =============================================================================== */
string getLastUnzipFile()
{
	return g_LastUnzipFile;
}

/* =============================================================================
 =============================================================================== */
int do_extract_currentfile(unzFile uf, const int *popt_extract_without_path, int *popt_overwrite, const char *password)
{
	char			filename_inzip[256];
	char			*filename_withoutpath;
	char			*p;
	int				err = UNZ_OK;
	FILE			*fout = NULL;
	void			*buf;
	uInt			size_buf;

	unz_file_info	file_info;
	err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);

	if (err != UNZ_OK)
	{
		printf("error %d with zipfile in unzGetCurrentFileInfo\n", err);
		return err;
	}

	size_buf = WRITEBUFFERSIZE;
	buf = (void *) malloc(size_buf);
	if (buf == NULL)
	{
		printf("Error allocating memory\n");
		return UNZ_INTERNALERROR;
	}

	p = filename_withoutpath = filename_inzip;
	while((*p) != '\0')
	{
		if (((*p) == '/') || ((*p) == '\\')) 
			filename_withoutpath = p + 1;
		p++;
	}

	if ((*filename_withoutpath) == '\0')
	{
		// if ((*popt_extract_without_path)==0)
		//		mkdir(filename_inzip, 0755);
	}
	else
	{
		const char	*write_filename;

		if ((*popt_extract_without_path) == 0)
			write_filename = filename_inzip;
		else
			write_filename = filename_withoutpath;

		err = unzOpenCurrentFilePassword(uf, password);
		if (err != UNZ_OK)
		{
			printf("error %d with zipfile in unzOpenCurrentFilePassword\n", err);
		}

		fout = fopen(write_filename, "wb");

		//  some zipfile don't contain directory alone before file
		if ((fout == NULL) && ((*popt_extract_without_path) == 0) && (filename_withoutpath != (char *) filename_inzip))
		{
			char	c = *(filename_withoutpath - 1);
			*(filename_withoutpath - 1) = '\0';
			makedir(write_filename);
			*(filename_withoutpath - 1) = c;
			fout = fopen(write_filename, "wb");
		}

		if (fout == NULL)
		{
			printf("error opening %s\n", write_filename);
		}
		else
		{

			/*
			 * printf(" extracting: %s\n",write_filename);
			 */
			do
			{
				err = unzReadCurrentFile(uf, buf, size_buf);
				if (err < 0)
				{
					printf("error %d with zipfile in unzReadCurrentFile\n", err);
					break;
				}

				if (err > 0)
				{
					if (fwrite(buf, err, 1, fout) != 1)
					{
						printf("error in writing extracted file\n");
						err = UNZ_ERRNO;
						break;
					}
					else
						g_LastUnzipFile = write_filename;
				}
			} while(err > 0);
			if (fout) 
				fclose(fout);
		}

		if (err == UNZ_OK)
		{
			err = unzCloseCurrentFile(uf);
			if (err != UNZ_OK)
			{
				printf("error %d with zipfile in unzCloseCurrentFile\n", err);
			}
		}
		else
			unzCloseCurrentFile(uf);	//  don't lose the error
	}

	free(buf);
	return err;
}

/* =============================================================================
 =============================================================================== */
bool do_extract(unzFile uf, int opt_extract_nopath, int opt_overwrite, const char *password, int iCount)
{
	unz_global_info gi;
	int				err;

	err = unzGetGlobalInfo(uf, &gi);
	if (err != UNZ_OK)
	{
		cout << ("error with zipfile in unzGetGlobalInfo");
		return false;
	}

	if (iCount == -1 || iCount > gi.number_entry) 
		iCount = gi.number_entry;

	for (int i = 0; i < iCount; i++)
	{
		if (do_extract_currentfile(uf, &opt_extract_nopath, &opt_overwrite, password) != UNZ_OK) 
			break;

		if ((i + 1) < iCount)
		{
			err = unzGoToNextFile(uf);
			if (err != UNZ_OK)
			{
				cout << ("error with zipfile in unzGoToNextFile");
				return false;
			}
		}
	}

	return true;
}

/* =============================================================================
 =============================================================================== */
bool unzip_files(string strFile, string strPath, int iCount)
{
	if (strFile == "" || strPath == "") 
		return false;
	if (!fileexists(strFile))
		return false;

	int		opt_extract_nopath = 0;
	int		opt_overwrite = 1;

	unzFile uf = unzOpen(strFile.c_str());

	if (uf == NULL)
	{
		strFile = remExt(strFile) + ".zip";
		uf = unzOpen(strFile.c_str());
	}

	if (uf == NULL)
	{
		cout << "Unable to open " + strFile + "\n";
		return false;
	}

	char	pchAppPath[1024] = "";
	getcwd(pchAppPath, 1024);
	if (!makedir(strPath) || !changedir(strPath))
	{
		cout << "Error changing path to " + strPath + "\n";
		return false;
	}

	bool	bRet = do_extract(uf, opt_extract_nopath, opt_overwrite, NULL, iCount);

	changedir(pchAppPath);

	unzClose(uf);

	return bRet;
}

/* =============================================================================
 =============================================================================== */
bool zip_files(string strFile, vector<string> files, vector<string> filenames, bool bIncludePaths)
{
	if (strFile == "" || files.size() == 0) 
		return false;

	int		opt_append = 0;
	int		opt_compress_level = 9;

	int		size_buf = WRITEBUFFERSIZE;
	void	*buf = (void *) malloc(size_buf);
	if (buf == NULL)
	{
		cout << ("Error allocating memory");
		return false;
	}

	zipFile zf = zipOpen(strFile.c_str(), opt_append);

	if (zf == NULL)
	{
		cout << ("error opening " + strFile);
		free(buf);
		return false;
	}

	string	strPath = getFilePath(strFile);
	for (int i = 0; i < files.size(); i++)
	{
		int		iPos = files[i].find(strPath);
		string	filenameinzip;
		if (files.size() == filenames.size())
			filenameinzip = filenames[i];
		else if (iPos != string::npos && bIncludePaths)
			filenameinzip = files[i].substr(strPath.size());
		else
			filenameinzip = getFileName(files[i]);

		FILE	*fin;
		int		size_read = 0;
		int		err = ZIP_OK;

		fin = fopen(files[i].c_str(), "rb");
		if (fin == NULL)
		{
			err = ZIP_ERRNO;
			cout << ("error opening file for reading " + files[i]);
		}

		if (err == ZIP_OK)
		{
			err = zipOpenNewFileInZip
				(
					zf,
					filenameinzip.c_str(),
					NULL,
					NULL,
					0,
					NULL,
					0,
					NULL,
					Z_DEFLATED,
					opt_compress_level
				);
		}

		if (err == ZIP_OK)
		{
			do
			{
				err = ZIP_OK;
				size_read = (int) fread(buf, 1, size_buf, fin);
				if ((size_read < size_buf) && (feof(fin) == 0))
				{
					cout << ("error reading file " + files[i]);
					err = ZIP_ERRNO;
				}

				if (size_read > 0)
				{
					err = zipWriteInFileInZip(zf, buf, size_read);
					if (err < 0)
					{
						cout << ("error writing file in zipfile " + files[i]);
					}
				}
			} while((err == ZIP_OK) && (size_read > 0));
		}

		if (fin) 
			fclose(fin);

		if (err < 0)
			err = ZIP_ERRNO;
		else if (zipCloseFileInZip(zf) != ZIP_OK)
			cout << ("error closing file in zipfile " + files[i]);
	}

	if (zipClose(zf, NULL) != ZIP_OK) 
		cout << ("error closing " + strFile);

	free(buf);
	return true;
}

/* =============================================================================
 =============================================================================== */
bool zip_files(string strFile, vector<string> files, bool bIncludePaths)
{
	vector<string>	filenames;
	return zip_files(strFile, files, filenames, bIncludePaths);
}

/* =============================================================================
 =============================================================================== */
bool compressFiles(string strFile, vector<string> files)
{
	string	strCmd;
	//progress_update("compressing...", -1);

	bool	bRet = zip_files(strFile, files, true);

	if (!bRet) 
		cout << "Error compressing file!" + strFile + "\n";

	return bRet;
}

/* =============================================================================
 =============================================================================== */
void decompressFiles(string strFile, string strOutputFolder)
{
	string	strCmd;
	//progress_update("decompressing...", -1);
	//download_progress_update("decompressing...", -1);

	if (!unzip_files(strFile, g_Env.m_LocalCachePath + "/" + strOutputFolder))
		cout << "Error decompressing file " + strFile + "\n";

	//progress_update("loading...", -1);
	//download_progress_update("loading...", -1);
}
