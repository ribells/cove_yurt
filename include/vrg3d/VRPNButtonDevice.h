//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author     Daniel Keefe (dfk)
   \maintainer Daniel Keefe (dfk)

   \created 2004-01-28
   \edited  2004-01-28

   \file  VRPNButtonDevice.h

   \brief A driver that connects to VRPN and creates Events
          based on button data from VRPN.
*/


#ifndef VRPNBUTTONDEVICE_H
#define VRPNBUTTONDEVICE_H

#ifdef USE_VRPN
  // Note: This include ordering is important, don't screw with it!
  #include <G3D/G3D.h>
  #include <GLG3D/GLG3D.h>
#endif

#include "InputDevice.h"


class vrpn_Button_Remote;

namespace VRG3D {


class VRPNButtonDevice : public InputDevice
{
public:
  VRPNButtonDevice(const std::string &vrpnButtonDeviceName,
                   const G3D::Array<std::string> &eventsToGenerate);



#ifdef USE_VRPN
  virtual ~VRPNButtonDevice();

  VRPNButtonDevice( const std::string   name,
                               G3D::Log     *log,
                    const ConfigMapRef  map );

  void pollForInput(G3D::Array<EventRef> &events);

  std::string getEventName(int buttonNumber);
  void sendEvent(int buttonNumber, bool down);

private:
  vrpn_Button_Remote  *_vrpnDevice;
  G3D::Array<std::string>   _eventNames;
  G3D::Array<EventRef>      _pendingEvents;


#else
  virtual ~VRPNButtonDevice() {};

  VRPNButtonDevice( const std::string   name,
                               G3D::Log     *log,
                    const ConfigMapRef  map )
  {
    unsupportedDevice( name, log, "USE_VRPN" );
  }

#endif
};

}                           // end namespace

#endif                      // USE_VRPN
