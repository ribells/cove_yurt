#pragma once

 //!!! todo - note that I ripped the movie.cpp completely out of the Yurt-based code project

class CMovie
{
private:
	void *	pMovie;
	int		iMovieTex; //stored here for convenience - do not create or delete here!

public:
	CMovie();
	~CMovie(void);

	int StartNewMovie(const char * pchFileName, int iWidth, int iHeight, int iSpeed, int iQuality);
	int	AppendFrame(unsigned char *imageBuffer);

	bool OpenMovie(const char * pchFileName, int iWidth, int iHeight);
	unsigned char* GetMovieFrame(double dTime);
	void getMovieSize(int &iWidth, int &iHeight) const;
	int getMovieLength() const;
	double getMovieDuration() const;
	int getMovieTex() const { return iMovieTex; }
	void setMovieTex(int tx_id) { iMovieTex = tx_id; }
	bool isValid() const;
};
