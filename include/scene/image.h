#ifndef IMAGE_H
#define IMAGE_H

#include <string>
using namespace std;

unsigned char * loadImage(string filename, int &width, int &height);
void saveImage(string filename, const unsigned char * image, int width, int height, int quality = 100); 

#endif