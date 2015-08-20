//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
   \author Dane Coffey
   \file   PQLabsClient.H
   \brief  
*/


#ifndef VRG3D_PQLABSEVENTS_H
#define VRG3D_PQLABSEVENTS_H


#ifdef USE_PQLABS
  #include <G3D/G3D.h>
  #include <GLG3D/GLG3D.h>
  #include  <PQMTClient.h>
  #include <G3D/GMutex.h>
#endif


#include  "InputDevice.h"

namespace VRG3D {

class PQLabsEvents : public InputDevice
{
public:

  PQLabsEvents(int port, string ip);

#ifdef USE_PQLABS
  virtual ~PQLabsEvents();

  PQLabsEvents( const std::string   name,
                         G3D::Log     *log,
              const ConfigMapRef  map );

  void pollForInput( G3D::Array<VRG3D::EventRef>  &events );

  int init();

private:
  G3D::GMutex _eventLock;
  G3D::Array<VRG3D::EventRef>    _unpolledEvents;
 
  void AddUnpolledEvent(VRG3D::Event* value);
//////////////////////call back functions///////////////////////
	// OnReceivePointFrame: function to handle when recieve touch point frame
	//	the unmoving touch point won't be sent from server. The new touch point with its pointevent is TP_DOWN
	//	and the leaving touch point with its pointevent will be always sent from server;
	static void OnReceivePointFrame(int frame_id,int time_stamp,int moving_point_count,const PQ_SDK_MultiTouch::TouchPoint * moving_point_array, void * call_back_object);
	// OnReceivePointFrame: function to handle when recieve touch gesture
	static void OnReceiveGesture(const PQ_SDK_MultiTouch::TouchGesture & ges, void * call_back_object);
	// OnServerBreak: function to handle when server break(disconnect or network error)
	static void OnServerBreak(void * param, void * call_back_object);
	// OnReceiveError: function to handle when some errors occur on the process of receiving touch datas.
	static void OnReceiveError(int err_code,void * call_back_object);
	// OnGetServerResolution: function to get the resolution of the server system.attention: not the resolution of touch screen. 
	static void OnGetServerResolution(int x, int y, void * call_back_object);
	// OnGetDeviceInfo: function to get the information of the touch device.
	static void OnGetDeviceInfo(const PQ_SDK_MultiTouch::TouchDeviceInfo & device_info, void * call_back_object);
//////////////////////call back functions end ///////////////////////

	// functions to handle TouchGestures, attention the means of the params
	void InitFuncOnTG();
	// set the call back functions while reciving touch data;
	void SetFuncsOnReceiveProc();

	// OnTouchPoint: function to handle TouchPoint
	void OnTouchPoint(const PQ_SDK_MultiTouch::TouchPoint & tp);
	// OnTouchGesture: function to handle TouchGesture
	void OnTouchGesture(const PQ_SDK_MultiTouch::TouchGesture & tg);
	//

	//here use function pointer table to handle the different gesture type;
	typedef void (*PFuncOnTouchGesture)(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void DefaultOnTG(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object); // just show the gesture

	static void OnTG_TouchStart(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void OnTG_Down(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void OnTG_Move(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void OnTG_Up(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);

	//
	static void OnTG_SecondDown(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void OnTG_SecondUp(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);

	//
	static void OnTG_SplitStart(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void OnTG_SplitApart(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void OnTG_SplitClose(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
	static void OnTG_SplitEnd(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);

	// OnTG_TouchEnd: to clear what need to clear;
	static void OnTG_TouchEnd(const PQ_SDK_MultiTouch::TouchGesture & tg,void * call_object);
private:
	PFuncOnTouchGesture m_pf_on_tges[TG_TOUCH_END + 1];

////////////////////////////////////assistant functions /////////////////////////////////

////////////////////////////////////assistant functions /////////////////////////////////

#else                             // else ifndef USE_PQLABS
  virtual ~PQLabsEvents() {};

  PQLabsEvents( const std::string   name,
                         G3D::Log     *log,
              const ConfigMapRef  map )
  {
    unsupportedDevice( name, log, "USE_PQLABS" );
  }

#endif                   // USE_PQLABS
  
private:
  int							_port;
  string						_ip;
  int							_width;
  int							_height;
};

}                        // end namespace VRG3D

#endif                  // VRG3D_PQLABSEVENTS_H
