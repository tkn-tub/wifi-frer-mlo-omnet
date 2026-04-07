//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef UMAC_FRERUMAC_H_
#define UMAC_FRERUMAC_H_

#include "src/umac/UMac.h" // MLO

namespace inet {

class Ieee8021CbUMac : public UMac
{
    protected:
        cValueMap *forwardingTable = nullptr;

    protected:
        virtual void initialize(int stage) override;

        virtual void handleUpperPacket(Packet *packet) override;
        virtual void sendPacket(Packet *packet) override;

    private:
        virtual bool check8021rProtocol(Packet *packet);
        virtual void sendPacketToInterface(Packet *packet);
        virtual NetworkInterface *getDefaultInterface(Packet *packet);
};
}

#endif /* UMAC_FRERUMAC_H_ */
