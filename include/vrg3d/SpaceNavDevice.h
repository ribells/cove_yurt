//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author Daniel Keefe (dfk)
   \file   SpaceNavDevice.h
   \brief  Driver for the SpaceNavigator device made by 3DConnexion.
           This also works for other similar devices, like the SpacePilot,
           although it only supports 2 buttons and 6DOF input from the
           hockey puck thingy.

           OSX Setup: The current implementation only works on OSX and
           requires you to install 3DConnexion's SDK framework, which
           is packaged with their latest OSX driver.  Then, add
           "3DconnexionClient" to the FRAMEWORKS variable in your
           Makefile to link with the SDK.
*/


#ifndef SPACENAVDEV_H
#define SPACENAVDEV_H

#ifdef USE_SPACENAV
  #include  <G3D/G3D.h>
  #include  <GLG3D/GLG3D.h>
#endif

#include  "InputDevice.h"


namespace VRG3D {

class SpaceNavDevice : public InputDevice
{
public:

  //SpaceNavDevice();


#ifdef USE_SPACENAV
  SpaceNavDevice( const std::string   name,
                             G3D::Log     *log,
                  const ConfigMapRef  map = NULL )
  {
    log->println( "Creating new SpaceNavDevice" );
    setup();
  }

  void setup();

  virtual ~SpaceNavDevice();

  void pollForInput(G3D::Array<EventRef> &events);

private:
  bool connected;

#else                     // else ifndef USE_SPACENAV
  SpaceNavDevice( const std::string   name,
                             G3D::Log     *log,
                  const ConfigMapRef  map = NULL )
  {
    unsupportedDevice( name, log, "USE_SPACENAV" );
  }

  virtual ~SpaceNavDevice() {};

#endif             // USE_SPACENAV
};

}                  // end namespace VRG3D

#endif             // SPACENAVDEV_H
