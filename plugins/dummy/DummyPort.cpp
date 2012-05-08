/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * DummyPort.cpp
 * The Dummy Port for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <iostream>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/dummy/DummyPort.h"

namespace ola {
namespace plugin {
namespace dummy {


DummyPort::DummyPort(DummyDevice *parent, unsigned int id)
  : BasicOutputPort(parent, id, true) {
    for (unsigned int i = 0; i < DummyPort::kNumberOfResponders; i++) {
      UID uid(OPEN_LIGHTING_ESTA_CODE, DummyPort::kStartAddress + i);
      m_responders[uid] = new DummyResponder(uid);
    }
}


/*
 * Write operation
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 */
bool DummyPort::WriteDMX(const DmxBuffer &buffer,
                         uint8_t priority) {
  (void) priority;
  m_buffer = buffer;
  stringstream str;
  std::string data = buffer.Get();

  str << "Dummy port: got " << buffer.Size() << " bytes: ";
  for (unsigned int i = 0;
       i < m_responders.begin()->second->Footprint() && i < data.size(); i++)
    str << "0x" << std::hex << 0 + (uint8_t) data.at(i) << " ";
  OLA_INFO << str.str();
  return true;
}


/*
 * This returns a single device
 */
void DummyPort::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  RunDiscovery(callback);
}


/*
 * This returns a single device
 */
void DummyPort::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  RunDiscovery(callback);
}


/*
 * Handle an RDM Request
 */
void DummyPort::SendRDMRequest(const ola::rdm::RDMRequest *request,
                               ola::rdm::RDMCallback *callback) {
  UID dest = request->DestinationUID();
  if (dest.IsBroadcast()) {
    for (ResponderMap::iterator i = m_responders.begin();
        i != m_responders.end(); i++) {
      i->second->SendRDMRequest(request, callback);
    }
  } else {
      ResponderMap::iterator i = m_responders.find(dest);
      if (i != m_responders.end()) {
        i->second->SendRDMRequest(request, callback);
      } else {
          std::vector<std::string> packets;
          callback->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
          delete request;
      }
  }
}


void DummyPort::RunDiscovery(RDMDiscoveryCallback *callback) {
  ola::rdm::UIDSet uid_set;
  for (ResponderMap::iterator i = m_responders.begin();
    i != m_responders.end(); i++) {
    uid_set.AddUID(i->second->UID());
  }
  callback->Run(uid_set);
}


DummyPort::~DummyPort() {
  for (ResponderMap::iterator i = m_responders.begin();
      i != m_responders.end(); i++) {
    delete i->second;
  }
}
}  // dummy
}  // plugin
}  // ola
