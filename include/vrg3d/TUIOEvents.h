//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author Daniel Keefe (dfk)
   \file   TUIOClient.H
   \brief  
*/


#ifndef VRG3D_TUIOEVENTS_H
#define VRG3D_TUIOEVENTS_H


#ifdef USE_TUIO
  #include <G3D/G3D.h>
  #include <GLG3D/GLG3D.h>
  #include  <TuioClient.h>
#endif


#include  "InputDevice.h"


#ifdef USE_TUIO
  using namespace TUIO;
#endif


#define TUIO_PORT  3333

namespace VRG3D {

class TUIOEvents : public InputDevice
{
public:

  TUIOEvents(int     port = TUIO_PORT,
             double  xScaleFactor = 1.0,
             double  yScaleFactor=1.0 );


#ifdef USE_TUIO
  virtual ~TUIOEvents();

  TUIOEvents( const std::string   name,
                         G3D::Log     *log,
              const ConfigMapRef  map );

  void pollForInput( G3D::Array<VRG3D::EventRef>  &events );


private:
  TuioClient *_tuioClient;
  G3D::Set<int>    _cursorsDown;
  double      _xScale;
  double      _yScale;

#else                             // else ifndef USE_TUIO
  virtual ~TUIOEvents() {};

  TUIOEvents( const std::string   name,
                         G3D::Log     *log,
              const ConfigMapRef  map )
  {
    unsupportedDevice( name, log, "USE_TUIO" );
  }

#endif                   // USE_TUIO
};

}                        // end namespace VRG3D

#endif                  // VRG3D_TUIOEVENTS_H
