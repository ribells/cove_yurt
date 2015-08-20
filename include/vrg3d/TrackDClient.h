//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author Daniel Keefe (dfk)
   \file   TrackDClient.h
   \brief
*/


#ifndef TRACKDCLIENT_H
#define TRACKDCLIENT_H

#ifdef USE_TRACKD
  #include <G3D/G3D.h>
  #include <GLG3D/GLG3D.h>
#endif

#include  "InputDevice.h"

namespace VRG3D {

class TrackDClient : public InputDevice
{
public:

  TrackDClient(
        int                          trackerShMemKey,
        int                          wandShMemKey,
        const G3D::Array<std::string>     &trackerEventsToGenerate,
        const double                 &trackerUnitsToRoomUnitsScale,
        const G3D::CoordinateFrame        &deviceToRoom,
        const G3D::Array<G3D::CoordinateFrame> &propToTracker,
        const G3D::Array<G3D::CoordinateFrame> &finalOffset,
        const G3D::Array<std::string>     &buttonEventsToGenerate,
        const G3D::Array<std::string>     &valuatorEventsToGenerate
        );


#ifdef USE_TRACKD
  virtual ~TrackDClient();

  TrackDClient( std::string   name, G3D::Log *log, ConfigMapRef  map );

  std::string getTrackerName(int trackerNumber);
  std::string getButtonName(int buttonNumber);
  std::string getValuatorName(int valuatorNumber);

  void pollForInput(G3D::Array<EventRef> &events);


private:
  G3D::Array<std::string>      _tEventNames;
  double                  _trackerUnitsToRoomUnitsScale;
  G3D::CoordinateFrame         _deviceToRoom;
  G3D::Array<G3D::CoordinateFrame>  _propToTracker;
  G3D::Array<G3D::CoordinateFrame>  _finalOffset;

  G3D::Array<std::string>      _bEventNames;
  G3D::Array<int>              _buttonStates;

  G3D::Array<std::string>      _vEventNames;
  G3D::Array<double>           _valuatorStates;

  void *_trackerMemory;
  void *_wandMemory;
  int   _numSensors;
  int   _numButtons;
  int   _numValuators;


#else                         // else ifndef USE_TRACKD
  virtual ~TrackDClient() {};

  TrackDClient( std::string   name,
                G3D::Log          *log,
                ConfigMapRef  map = NULL )
  {
    unsupportedDevice( name, log, "USE_TRACKD" );
  }

#endif
};

}                            // end namespace VRG3D

#endif                       // USE_TRACKD
