//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author     Daniel Keefe (dfk)
   \maintainer Daniel Keefe (dfk)

   \created 2004-01-28
   \edited  2004-01-28

   \file  VRPNAnalogDevice.h

   \brief A driver that connects to VRPN and creates Events
          based on analog data from VRPN.
*/

#ifndef VRPNANALOGDEVICE_H
#define VRPNANALOGDEVICE_H

#ifdef USE_VRPN
  // Note: This include ordering is important, don't screw with it!
  #include <G3D/G3D.h>
  #include <GLG3D/GLG3D.h>
#endif

#include  "InputDevice.h"


class vrpn_Analog_Remote;

namespace VRG3D {


class VRPNAnalogDevice : public InputDevice
{
public:
  VRPNAnalogDevice( const std::string &vrpnAnalogDeviceName,
                    const G3D::Array<std::string> &eventsToGenerate );


  virtual ~VRPNAnalogDevice();


#ifdef USE_VRPN
  VRPNAnalogDevice( const std::string   name,
                               G3D::Log     *log,
                    const ConfigMapRef  map );

  void        pollForInput(G3D::Array<EventRef> &events);
  void        sendEventIfChanged(int channelNumber, double data);
  std::string getEventName(int channelNumber);
  int         numChannels() { return _eventNames.size(); }

private:
  vrpn_Analog_Remote  *_vrpnDevice;
  G3D::Array<std::string>   _eventNames;
  G3D::Array<double>        _channelValues;
  G3D::Array<EventRef>      _pendingEvents;

#else
  virtual ~VRPNAnalogDevice() {};

  VRPNAnalogDevice( const std::string   name,
                               G3D::Log     *log,
                    const ConfigMapRef  map )
  {
    unsupportedDevice( name, log, "USE_VRPN" );
  }

#endif
};

}                    // end namespace

#endif               // USE_VRPN
