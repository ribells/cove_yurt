//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author Daniel Keefe (dfk)
   \file   ISenseDirect.h
   \brief  This is an untested driver for connecting directly to an
           Intersense tracking device using the .dll that can be
           downloaded from intersense.
*/



#ifndef ISENSEDIRECT_H
#define ISENSEDIRECT_H

#ifdef USE_ISENSE
  #include <G3D/G3D.h>
  #include <GLG3D/GLG3D.h>
  #include <isense.h>
#endif

#include "InputDevice.h"


namespace VRG3D {


class ISenseDirect : public InputDevice
{
public:
  ISenseDirect(
        const G3D::Array<std::string>     &trackerEventNames,
        const double                 &trackerUnitsToRoomUnitsScale,
        const G3D::CoordinateFrame        &deviceToRoom,
        const G3D::Array<G3D::CoordinateFrame> &propToTracker,
        const G3D::Array<G3D::CoordinateFrame> &finalOffset,
        const G3D::Array< G3D::Array<std::string> > &buttonEventNames
        );


#ifdef USE_ISENSE
  virtual ~ISenseDirect();

  ISenseDirect( std::string   name,
                G3D::Log          *log,
                ConfigMapRef  map );

  void pollForInput(G3D::Array<EventRef> &events);

  std::string getTrackerName(int trackerNumber);
  std::string getButtonName(int stationNumber, int buttonNumber);

private:
  G3D::Array<std::string>          _tEventNames;
  G3D::Array< G3D::Array<std::string> > _bEventNames;
  double                  _trackerUnitsToRoomUnitsScale;
  G3D::CoordinateFrame         _deviceToRoom;
  G3D::Array<G3D::CoordinateFrame>  _propToTracker;
  G3D::Array<G3D::CoordinateFrame>  _finalOffset;

  Bool                    _btnStatus[ISD_MAX_STATIONS][ISD_MAX_BUTTONS];
  ISD_TRACKER_HANDLE      _handle;
  ISD_STATION_INFO_TYPE   _stationInfo[ISD_MAX_STATIONS];
  int                     _maxStations;


#else        // else ifndef USE_ISENSE
  virtual ~ISenseDirect() {};

  ISenseDirect( std::string   name,
                G3D::Log          *log,
                ConfigMapRef  map )
  {
    unsupportedDevice( name, log, "USE_ISENSE" );
  }

#endif      // USE_ISENSE
};

}           // end namespace VRG3D

#endif      // ISENSEDIRECT_H
