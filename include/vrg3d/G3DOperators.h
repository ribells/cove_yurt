//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.



#ifndef G3DOPERATORS_H
#define G3DOPERATORS_H

#include <G3D/G3D.h>
#include <GLG3D/GLG3D.h>

namespace VRG3D {

G3D::Array<std::string> splitStringIntoArray(const std::string &in);

/**
    This function changes occurances of /cygdrive/c/folder1/folder2/etc..
    to c:/folder1/folder2/etc.. when running in Windows.  Paths coming
    from program arguments or environment variables will tend to take
    the /cygdrive/<drive-letter>/ form when running under cygwin, but
    file commands compiled with the MS Visual C++ compiler can't deal
    with paths of this form.
*/
std::string decygifyPath(const std::string &in);


/**
   Given an input string, replaces all occurances of $(NAME) with the
   decygified (see above) value of the environment variable NAME.  If
   NAME is not defined, then a blank string is returned.
*/
std::string replaceEnvVars(const std::string &in);


void growAABox(G3D::AABox &box, const G3D::Vector3 &point);
void growAABox(G3D::AABox &box, const G3D::AABox &box2);

  G3D::Color3 Color3FromUints(G3D::uint8 r, G3D::uint8 g, G3D::uint8 b);

unsigned int hashCode(const double d);

// Vector2
std::ostream & operator<< ( std::ostream &os, const G3D::Vector2 &vec2);
std::istream & operator>> ( std::istream &is, G3D::Vector2 &vec2);
// Vector3
//std::ostream & operator<< ( std::ostream &os, const Vector3 &vec3);
std::istream & operator>> ( std::istream &is, G3D::Vector3 &vec3);
// Matrix3
std::ostream & operator<< ( std::ostream &os, const G3D::Matrix3 &m);
std::istream & operator>> ( std::istream &is, G3D::Matrix3 &m);
// Matrix4
std::ostream & operator<< ( std::ostream &os, const G3D::Matrix4 &m);
std::istream & operator>> ( std::istream &is, G3D::Matrix4 &m);
// CoordinateFrame
// << is already defined somewhere in G3D..
//std::ostream & operator<< ( std::ostream &os, const CoordinateFrame &m);
std::istream & operator>> ( std::istream &is, G3D::CoordinateFrame &m);
// Color3
std::ostream & operator<< ( std::ostream &os, const G3D::Color3 &c);
std::istream & operator>> ( std::istream &is, G3D::Color3 &c);
// Color4
std::ostream & operator<< ( std::ostream &os, const G3D::Color4 &c);
std::istream & operator>> ( std::istream &is, G3D::Color4 &c);

// etc.. add on as necessary..


std::string matrix4ToString(G3D::Matrix4 m);
std::string coordinateFrameToString(G3D::CoordinateFrame cf);

std::string  intToString(int i);
int          stringToInt(const std::string &in);

std::string  realToString(double r);
double       stringToReal(const std::string &in);



} // end namespace

#endif
