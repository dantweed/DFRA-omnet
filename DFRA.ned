package dfra;


//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import dfra.dfra.node.DfraHost;
import dfra.dfra.node.DfraAP;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import dfra.dfra.mac.DfraMac;

import inet.linklayer.ieee80211.mac.Ieee80211Mac;


network DFRA
{
    parameters:
        int numHosts;
        **.mgmt.numChannels = 2;
    submodules:
        host[numHosts]: DfraHost {
            @display("r=,,#707070");            
            wlan[*].mgmtType = "DfraMgmtSTA"; //"Ieee80211MgmtSTA";//
            wlan[*].macType = "DfraMac";//"Ieee80211Mac";//"DfraMac";                      
        }
        ap: DfraAP {
            @display("p=213,174;r=,,#707070");
            wlan[*].mgmtType = "DfraMgmtAP";//"DfraMgmtAP"; //"Ieee80211MgmtAP";//"DfraMgmtAP"; //
            wlan[*].macType = "DfraMac";//"Ieee80211Mac";//"DfraMac";
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=61,46");
        }
        configurator: IPv4NetworkConfigurator {
            config = xml("<config><interface hosts='*' address='145.236.x.x' netmask='255.255.0.0'/></config>");
            @display("p=140,50");
        }
}
