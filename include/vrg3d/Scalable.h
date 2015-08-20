\
#ifdef WIN32
#  include <windows.h>
#endif


#ifdef WIN32
#include <GL/gl.h>
#include "glext.h"
#else
//#include <GL/gl.h>
//#include <GL/glx.h>
#endif

#include <G3D/G3DAll.h>
#include <string>
#include <set>


void ScalableSetView(double, double, double);
void ScalableSetView0(double, double, double);

void ScalableInit(const char* ScalableMesh);
void ScalableClose();
void ScalablePreSwap(bool left);

Vector3 getTopLeft();
Vector3 getTopRight();
Vector3 getBotLeft();
Vector3 getBotRight();

/*struct GLExtensions
{
public:
  
  // Description:
  // Get the full list of extensions we have.
  void Initialize();
  
  // Description:
  // Is a particular extension supported.
  inline bool IsExtensionSupported(const char *ExtensionText)
  {
    return (m_glExtSet.find(ExtensionText) != m_glExtSet.end());
  }
  
  // Description:
  // Get the extension function, return NULL if not ssupported.
  //static void * GetFunction(const char *ExtensionText);
  
  // Description:
  // Once we grab during the initialize. They could be NULL.
  PFNGLGENFRAMEBUFFERSEXTPROC            glGenFramebuffersEXT;
  PFNGLDELETEFRAMEBUFFERSEXTPROC         glDeleteFramebuffersEXT;
  PFNGLBINDFRAMEBUFFEREXTPROC            glBindFramebufferEXT;
  PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC     glCheckFramebufferStatusEXT;
  PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC 
                                         glGetFramebufferAttachmentParameterivEXT;
  PFNGLGENERATEMIPMAPEXTPROC             glGenerateMipmapEXT;
  PFNGLFRAMEBUFFERTEXTURE2DEXTPROC       glFramebufferTexture2DEXT;
  PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC    glFramebufferRenderbufferEXT;
  PFNGLGENRENDERBUFFERSEXTPROC           glGenRenderbuffersEXT;
  PFNGLDELETERENDERBUFFERSEXTPROC        glDeleteRenderbuffersEXT;
  PFNGLBINDRENDERBUFFEREXTPROC           glBindRenderbufferEXT;
  PFNGLRENDERBUFFERSTORAGEEXTPROC        glRenderbufferStorageEXT;
  PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
  PFNGLISRENDERBUFFEREXTPROC             glIsRenderbufferEXT;
  PFNGLACTIVETEXTUREARBPROC              glActiveTexture;

protected:
  std::set<std::string> m_glExtSet;
  
  // Description:
  // Fill in the set of strings.
  static void GetOpenGLExtensions(std::set<std::string>& glExtSet);
};*/

