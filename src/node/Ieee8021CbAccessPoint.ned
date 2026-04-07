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

package frer_mlo.node;

import inet.linklayer.ieee8021r.IIeee8021rLayer;
import mlo.node.MLOAccessPoint;

module Ieee8021CbAccessPoint extends MLOAccessPoint
{
    parameters:
        lmac[*].llc.typename = default("Ieee8021CbPortal");
        bridging.typename = default("Ieee8021CbRelay");
        ieee8021r.typename = default("Ieee8021rLayer");
        umac.typename = default("Ieee8021CbUMac");

    submodules:
        ieee8021r: <default("")> like IIeee8021rLayer if typename != "" {
            @display("p=889,282");
        }
    connections allowunconnected:
        bl.out++ --> ieee8021r.upperLayerIn if exists(ieee8021r);
        ieee8021r.upperLayerOut --> bl.in++ if exists(ieee8021r);
        ieee8021r.lowerLayerOut --> nl.in++ if exists(ieee8021r);
        nl.out++ --> ieee8021r.lowerLayerIn if exists(ieee8021r);
}

