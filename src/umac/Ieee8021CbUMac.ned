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

package frer_mlo.umac;

import inet.linklayer.contract.ILinkLayer;

import mlo.umac.UMac;
import mlo.umac.IUMac;

module Ieee8021CbUMac extends UMac like IUMac
{
    parameters:
        @class(inet::Ieee8021CbUMac);
        mode = "Ieee8021CbUMac";
		object forwardingTable @mutable = default({});
}
