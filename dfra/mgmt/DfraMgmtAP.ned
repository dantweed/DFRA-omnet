//
// Copyright (C) 2006 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


package dfra.dfra.mgmt;
import inet.linklayer.ieee80211.mgmt.IIeee80211Mgmt;
//
// Used in 802.11 infrastructure mode: handles management frames for
// an access point (AP). It relies on a connected ~Ieee80211Mac for actual
// reception and transmission of frames.
//
// When simulating an AP that has other (Ethernet or wireless) interfaces
// as well, this module can be connected to a ~IMACRelayUnit which performs
// bridging of Ethernet frames. This module will perform conversion from/to
// ~EtherFrame for ~IMACRelayUnit.
//
// This module never switches channels, that is, it will operate on the channel
// the physical layer is configured for (see channelNumber in ~Ieee80211Radio).
//
// This module checks if there is an other module connected above the management
// module (e.g. RelayUnit) and sends the frame there if the frame's destination
// is not on the associated station's list. Multicast frames are broadcasted on
// the wireless card and a copy sent also up to the relay unit so it can broadcast
// it on other interfaces.
//
// @author Andras Varga
//
simple DfraMgmtAP like IIeee80211Mgmt
{
    parameters:
        string ssid = default("SSID");
        double beaconInterval @unit("s") = default(100ms);
        int numAuthSteps = default(4); // use 2 for Open System auth, 4 for WEP
        string encapDecap = default("eth") @enum("true", "false", "eth");   // if "eth", frames sent up are converted to EthernetIIFrame
        //dataRate: numeric; XXX TBD
        string radioModule = default("^.radio");   // The path to the Radio module  //FIXME remove default value
        string macModule = default("DfraMac");               // The path to the MAC module
        @display("i=block/cogwheel");
        @signal[NF_L2_AP_ASSOCIATED](type="ieee80211::DfraMgmtAP::NotificationInfoSta");
        @signal[NF_L2_AP_DISASSOCIATED](type="ieee80211::DfraMgmtAP::NotificationInfoSta");
        
        @class(ieee80211::DfraMgmtAP);
        @class(ieee80211::DfraMgmtAP::NotificationInfoSta);
    gates:
        input upperLayerIn; // from ~IMACRelayUnit; can be left unconnected
        output upperLayerOut; // to ~IMACRelayUnit; can be left unconnected
        input macIn @labels(Ieee80211Frame); // from ~Ieee80211Mac
        output macOut @labels(Ieee80211Frame); // to ~Ieee80211Mac
        input agentIn @loose;   // unused dummy to statisfy the IIeee80211Mgmt interface
        output agentOut @loose; // unused dummy to statisfy the IIeee80211Mgmt interface
}

