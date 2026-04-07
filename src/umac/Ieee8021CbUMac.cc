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

#include "Ieee8021CbUMac.h"

#include <omnetpp.h>
#include <chrono>
#include <fstream>
#include <windows.h>
#include <iomanip>

#include "inet/linklayer/ieee8021r/Ieee8021rTagHeader_m.h"
#include "inet/linklayer/ethernet/common/EthernetMacHeader_m.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/protocolelement/redundancy/StreamTag_m.h"
#include "inet/common/SequenceNumberTag_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/ProtocolUtils.h"

#include "src/common/HighPrecisionLogger.h"

namespace inet {

using namespace std;

Register_Class(Ieee8021CbUMac);

void Ieee8021CbUMac::initialize(int stage) {
    UMac::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
        forwardingTable = check_and_cast<cValueMap *>(par("forwardingTable").objectValue());

}

void Ieee8021CbUMac::handleUpperPacket(Packet *packet) {
    //std::cout << this->getParentModule()->getName() << "." << this->getName() << "." <<  __func__ << "() logs - " << "Sent  " << packet->getName() << endl;

    //std::string packetName = packet->getName();
    //HighPrecisionLogger::logEvent(getFullPath().c_str(),"verification [2] U-MAC receives application packet:", packetName);

    // Remove dispatch requirement otherwise the MessageDispatchers bounces the packets back.
    packet->removeTagIfPresent<DispatchProtocolReq>();
    updateChannelAccessParameters();
    sendPacket(packet);
}

bool Ieee8021CbUMac::check8021rProtocol(Packet *packet) {
    auto packetProtocolTag = packet->findTag<PacketProtocolTag>();

    if (packetProtocolTag == nullptr)
        return false;

    auto packetProtocol = packetProtocolTag->getProtocol();
    return *packetProtocol == Protocol::ieee8021rTag;
}

NetworkInterface *Ieee8021CbUMac::getDefaultInterface(Packet *packet) {
    NetworkInterface *defaultInterface = nullptr;
    if (isStation) {
        defaultInterface = ift->findInterfaceByName(defaultInterfaceName);
    }
    else {
        int targetIftId = packet->getTag<InterfaceInd>()->getInterfaceId();
        defaultInterface = ift->getInterfaceById(targetIftId);
    }

    if (defaultInterface == nullptr)
        throw cRuntimeError("Cannot find default network interface");
    return defaultInterface;
}

void Ieee8021CbUMac::sendPacketToInterface(Packet *packet) {
    auto streamReq = packet->findTag<StreamReq>();

    if (!isStation && streamReq != nullptr) { // AccessPoint needs special handling of EthernetMacHeader and rTagHeader
        auto rTagHeader = packet->peekAtFront<Ieee8021rTagEpdHeader>();
        packet->eraseAtFront(rTagHeader->getChunkLength());
        auto ethernetMacHeader = packet->peekAtFront<EthernetMacHeader>();
        packet->eraseAtFront(ethernetMacHeader->getChunkLength());
        packet->insertAtFront(rTagHeader);
        packet->insertAtFront(ethernetMacHeader);
    }

    NetworkInterface *targetIft = getDefaultInterface(packet);

    // Matches stream name with the sending interface.
    if (streamReq != nullptr) {
        auto streamName = streamReq->getStreamName();
        if (forwardingTable->containsKey(streamName)) {
            auto targetIftName = forwardingTable->get(streamReq->getStreamName()).stringValue();
            targetIft = ift->findInterfaceByName(targetIftName);
            if (targetIft == nullptr)
                throw cRuntimeError("Cannot find network interface '%s'", targetIftName);
        }
    }

    if(targetIft->getState() == NetworkInterface::State::UP) {
        packet->removeTagIfPresent<InterfaceReq>();
        packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(targetIft->getInterfaceId());
        string packetName = packet->getName();
        recordScalar((std::string("#sent " + packetName + " on ift[") + std::to_string(targetIft->getInterfaceId()) + std::string("] at: ")).c_str(), simTime());
        //HighPrecisionLogger::logEvent(getFullPath().c_str(),"verification [X] U-MAC forwards to L-MAC:", packetName);
        send(packet, lowerLayerOutGateId);
    }
    else {
        // TODO: find better way to handle down interface
        delete packet;
    }

}

void Ieee8021CbUMac::sendPacket(Packet *packet) {

    //std::cout << this->getParentModule()->getName() << "." << this->getName() << "." <<  __func__ << std::endl;

    if (!check8021rProtocol(packet)) { // not handled by 802.1r modules yet --> send to 802.1r module (outgoing)
        // Reuse existing indicators of 802.1r module (incoming)
        auto streamInd = packet->findTag<StreamInd>();
        auto sequenceNumberInd = packet->findTag<SequenceNumberInd>();
        bool existingStream = streamInd != nullptr && sequenceNumberInd != nullptr;
        // sender: always send to rTagModules, maybe Packet belongs to stream
        // accessPoints: only send packets of existing stream to rTagModules
        if (isStation || existingStream) {
            if (existingStream) {
                packet->addTagIfAbsent<StreamReq>()->setStreamName(streamInd->getStreamName());
                packet->removeTag<StreamInd>();

                packet->addTagIfAbsent<SequenceNumberReq>()->setSequenceNumber(sequenceNumberInd->getSequenceNumber());
                packet->removeTag<SequenceNumberInd>();
            }
            ensureEncapsulationProtocolReq(packet, &Protocol::ieee8021rTag);
            setDispatchProtocol(packet);
            // This first circulates the packets through StreamIdentifier, instead of pushing them to uplink.

            //std::string packetName = packet->getName();
            //HighPrecisionLogger::logEvent(getFullPath().c_str(),"verification [3] U-MAC circulates to FRER:", packetName);

            send(packet, lowerLayerOutGateId);
        }
        else {
            sendPacketToInterface(packet);
        }
    }
    else { // already handled by 802.1r module --> find wanted interface
        auto streamReq = packet->findTag<StreamReq>();
        if (streamReq == nullptr) {
            // no stream identified, but the 802.1r module will always insert a header, which we will need to remove
            // this is important to handle non-FRER frames.
            auto rTagHeader = packet->peekAtFront<Ieee8021rTagEpdHeader>();
            packet->eraseAtFront(rTagHeader->getChunkLength());
            packet->removeTag<PacketProtocolTag>();

            // insert previous protocol indicated by rTagHeader
            auto typeOrLength = rTagHeader->getTypeOrLength();
            const Protocol *protocol;
            if (isIeee8023Length(typeOrLength))
                protocol = &Protocol::ieee8022llc;
            else
                protocol = ProtocolGroup::getEthertypeProtocolGroup()->getProtocol(typeOrLength);
            packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(protocol);
        }
        sendPacketToInterface(packet);
    }
}
}
