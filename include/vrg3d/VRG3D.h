//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
 * \author Daniel Keefe (dfk)
 *
 * \file  VRG3D.h
 *
 */


#ifndef VRG3D_H
#define VRG3D_H

// Include all of G3D
#include <G3D/G3D.h>
#include <GLG3D/GLG3D.h>

// Include all of the VRG3D library here
#include "ConfigMap.h"
#include "DisplayTile.h"
#include "Event.h"
#include "EventNet.h"
#include "G3DOperators.h"
#include "InputDevice.h"
#include "MouseToTracker.h"
#include "ProjectionVRCamera.h"
#include "SynchedSystem.h"
#include "VRApp.h"

#ifdef USE_VRPN
   #include "VRPNAnalogDevice.h"
   #include "VRPNButtonDevice.h"
   #include "VRPNTrackerDevice.h"
#endif

using namespace VRG3D;

#include <iostream>
using namespace std;

#endif
