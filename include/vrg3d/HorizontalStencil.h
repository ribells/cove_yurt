//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
 * \author John Huffman at Brown Univ. CCV
 *
 * \file  VerticalStencil.h
 * \brief Draws a vertical striped pattern in the stencil buffer for use
   with stereo rendering to a SAMSUNG TV display.

   Usage:

   vertical_stencil(w,h);

   glStencilFunc(GL_NOTEQUAL, 1, 1);
   // Draw scene for left eye

   glStencilFunc(GL_EQUAL, 1, 1);
   // Draw scene for right eye

 */

#ifndef __HORIZONTAL_STENCIL
#define __HORIZONTAL_STENCIL

#ifdef __cplusplus
extern "C" {
#endif

  void horizontal_stencil(int gliWindowWidth, int gliWindowHeight);

#ifdef __cplusplus
}
#endif

#endif

