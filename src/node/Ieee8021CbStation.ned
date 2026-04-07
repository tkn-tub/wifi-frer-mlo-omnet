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
import mlo.node.MLOStation;

module Ieee8021CbStation extends MLOStation
{
    parameters:
        umac.typename = default("Ieee8021CbUMac");
        ieee8021r.typename = default("Ieee8021rLayer");
        mobility.typename = default("StationaryMobility");
}