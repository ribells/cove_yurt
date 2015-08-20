//  Copyright Regents of the University of Minnesota and Brown University, 2010.  All rights are reserved.

/**
 * \author Daniel Keefe (dfk)
 *
 * \file  EventNet.h
 * \brief
 *
 */

#ifndef EVENTNET_H
#define EVENTNET_H

#include "Event.h"

// ID's for NetMessages
#define EVENTNETMSG_TYPE       1101
#define EVENTBUFFERNETMSG_TYPE 1102

namespace VRG3D {

/// A message that contains a single event
class EventNetMsg
{
public:
  EventNetMsg(EventRef e) { event = e; }
  EventNetMsg() {}

  virtual ~EventNetMsg() {}

  G3D::uint32 type() const { return EVENTNETMSG_TYPE; }

  void serialize(G3D::BinaryOutput &b) const {
    alwaysAssertM(event.notNull(), "Null event in serialize method.");
    event->serialize(b);
  }

  void deserialize(G3D::BinaryInput &b) {
    event = new Event("temp-name");
    event->deserialize(b);
  }
  EventRef event;
};


/// A message that contains an array of Events
class EventBufferNetMsg
{
public:
  EventBufferNetMsg(const G3D::Array<EventRef> &msgEventBuffer) {
    eventBuffer = msgEventBuffer;
  }
  EventBufferNetMsg() {};

  virtual ~EventBufferNetMsg() {};

  G3D::uint32 type() const { return EVENTBUFFERNETMSG_TYPE; }

  void serialize(G3D::BinaryOutput &b) const {
    b.writeInt32(eventBuffer.size());
    for (int i=0;i<eventBuffer.size();i++) {
      eventBuffer[i]->serialize(b);
    }
  }

  void deserialize(G3D::BinaryInput &b) {
    G3D::int32 numEvents = b.readInt32();
    for (int i=0;i<numEvents;i++) {
      EventRef e = new Event("temp-name");
      e->deserialize(b);
      eventBuffer.append(e);
    }
  }

  G3D::Array<EventRef> eventBuffer;
};


} // end namespace

#endif

