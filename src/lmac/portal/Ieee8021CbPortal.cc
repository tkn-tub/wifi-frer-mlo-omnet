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

#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/EtherType_m.h"
#include "inet/linklayer/common/FcsMode_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "Ieee8021CbPortal.h"

#ifdef INET_WITH_ETHERNET
#include "inet/linklayer/ethernet/common/Ethernet.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"
#include "inet/physicallayer/wired/ethernet/EthernetPhyHeader_m.h"
#endif // ifdef INET_WITH_ETHERNET

#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/portal/Ieee80211Portal.h"
#include "inet/linklayer/ieee8022/Ieee8022Llc.h"
#include "inet/linklayer/ieee8022/Ieee8022LlcHeader_m.h"
#include "inet/linklayer/ieee8021r/Ieee8021rTagHeader_m.h"

#include "src/lmac/mac/Ieee80211beProtocol.h"

namespace inet {
namespace ieee80211 {

Define_Module(Ieee8021CbPortal);

void Ieee8021CbPortal::decapsulate(Packet *packet) {
    packet->trim();
    int typeOrLength = packet->getByteLength();
    const auto& llcHeader = packet->peekAtFront<Ieee8022LlcHeader>();
    if (llcHeader->getSsap() == 0xAA && llcHeader->getDsap() == 0xAA && llcHeader->getControl() == 0x03) {
        const auto& snapHeader = dynamicPtrCast<const Ieee8022LlcSnapHeader>(llcHeader);
        if (snapHeader == nullptr)
            throw cRuntimeError("LLC header indicates SNAP header, but SNAP header is missing");
        if (snapHeader->getOui() == 0) {
            // snap header with ethertype
            typeOrLength = snapHeader->getProtocolId();
            packet->eraseAtFront(snapHeader->getChunkLength());
        }
    }

    // MODIFIED: Remove R-TAG - first it should go to U-MAC.
    const auto& snapHeader = dynamicPtrCast<const Ieee8022LlcSnapHeader>(llcHeader);
    const Protocol *prot = ProtocolGroup::getEthertypeProtocolGroup()->getProtocol(snapHeader->getProtocolId());
    Ptr<const Ieee8021rTagEpdHeader> rTagHeader = nullptr;

    if (*prot == Protocol::ieee8021rTag) {
        rTagHeader = packet->peekAtFront<Ieee8021rTagEpdHeader>();
        packet->eraseAtFront(rTagHeader->getChunkLength());
    }

    const auto& ethernetHeader = makeShared<EthernetMacHeader>();
    ethernetHeader->setSrc(packet->getTag<MacAddressInd>()->getSrcAddress());
    ethernetHeader->setDest(packet->getTag<MacAddressInd>()->getDestAddress());
    ethernetHeader->setTypeOrLength(typeOrLength);
    packet->insertAtFront(ethernetHeader);
    const auto& ethernetFcs = makeShared<EthernetFcs>();
    ethernetFcs->setFcsMode(fcsMode);
    packet->insertAtBack(ethernetFcs);

    // MODIFIED: Re-insert rTagHeader
    if (rTagHeader != nullptr) {
        packet->insertAtFront(rTagHeader);
    }

    // MODIFIED: This protocol tag is required to pass the packet through U-MAC.
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(ieee80211be);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(ieee80211be);
    packet->addTagIfAbsent<MacProtocolInd>()->setProtocol(prot);
}

}
}

