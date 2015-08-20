//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author Daniel Keefe (dfk)
   \file   VRPNTrackerDevice.h
   \brief  A driver that connects to VRPN and creates CoordinateFrame Events
           based on tracker data from VRPN.
*/


#ifndef VRPNTRACKERDEVICE_H
#define VRPNTRACKERDEVICE_H


#ifdef USE_VRPN
  // Note: This include ordering is important, don't screw with it!
  #include <G3D/G3D.h>
  #include <GLG3D/GLG3D.h>
#endif


#include  "InputDevice.h"


class vrpn_Tracker_Remote;
class vrpn_Connection;

namespace VRG3D {

/**
    Calibration Procedure:

    1. Set TrackerUnitsToRoomUnitsScale to get everything in feet.

    2. Adjust DeviceToRoom by printing out the position of sensor 0
    until it reports the origin of RoomSpace in the correct place and
    +X, +Y, and +Z all point in the correct direction.  You can print
    out the value of sensor 0 by calling printSensor0(true).

    3. Use the TestTrackers function of the IS3D/programTemplate demo
    to draw the CoordinateFrame for each tracker.  Adjust each
    tracker's PropToTracker transformation until it aligns itself with
    the RoomSpace frame when the tracker is held at the origin of
    RoomSpace.

    Here's an easy way to think about it: Hold up each prop at the
    origin of RoomSpace in the orientation that you want to be
    considered "normal".  Then, look at the drawing of its frame.  For
    each of the vectors in its frame (X,Y,Z) that are shown, ask
    yourself, what is the RoomSpace vector that this arrow depicts?
    Then enter that value in the appropriate X,Y, or Z column in the
    PropToTracker frame.

    4. For some of the props you may want the origin for the prop to
    be somewhere on the prop other than where the tracker is
    positioned.  For these, measure the distance from the tracker to
    where you want the origin to be, call this vector V.  Then, put
    this vector into the translation part of the PropToTracker frame.
    Be careful, the translation is applied after the rotation
    specified in the frame, so for example, if the first column of the
    frame is (0,1,0,0), you should put the Y component of V in that
    column, so it would become (0,1,0,Y[1]).  If it were (0,-1,0,0),
    you would need to put (0,-1,0,-Y[1]).
*/
class VRPNTrackerDevice : public InputDevice
{
public:

  VRPNTrackerDevice(
        const std::string            &vrpnTrackerDeviceName,
        const G3D::Array<std::string>     &eventsToGenerate,
        const double                 &trackerUnitsToRoomUnitsScale,
        const G3D::CoordinateFrame        &deviceToRoom,
        const G3D::Array<G3D::CoordinateFrame> &propToTracker,
        const G3D::Array<G3D::CoordinateFrame> &finalOffset,
        const bool                   &waitForNewReportInPoll,
        const bool                   &convertLHtoRH = false,
        const bool                   &ignoreZeroes = false);

  virtual ~VRPNTrackerDevice();


#ifdef USE_VRPN
  VRPNTrackerDevice( const std::string   name,
                                G3D::Log     *log,
                     const ConfigMapRef  map );

  void processEvent(const G3D::CoordinateFrame &vrpnEvent, int sensorNum);

  std::string getEventName(int trackerNumber);

  void pollForInput(G3D::Array<EventRef> &events);

  void setPrintSensor0(bool b) { _printSensor0 = b; }

private:
  vrpn_Connection        *_vrpnConnection;
  vrpn_Tracker_Remote    *_vrpnDevice;
  G3D::Array<std::string>      _eventNames;
  double                  _trackerUnitsToRoomUnitsScale;
  G3D::CoordinateFrame         _deviceToRoom;
  G3D::Array<G3D::CoordinateFrame>  _propToTracker;
  G3D::Array<G3D::CoordinateFrame>  _finalOffset;
  bool                    _printSensor0;
  bool                    _waitForNewReport;
  bool                    _convertLHtoRH;
  bool                    _ignoreZeroes;
  bool                    _newReportFlag;
  G3D::Array<EventRef>         _pendingEvents;

#else
  virtual ~VRPNTrackerDevice() {};

  VRPNTrackerDevice( const std::string   name,
                                G3D::Log     *log,
                     const ConfigMapRef  map )
  {
    unsupportedDevice( name, log, "USE_VRPN" );
  }

#endif                       // USE_VRPN
};

}                            // end namespace VRG3D

#endif                       // VRPNTRACKERDEVICE_H
