package dfra;

import inet.node.ethernet.Eth100G;
import inet.node.inet.StandardHost;
import dfra.dfra.node.DfraStandardHost;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import dfra.dfra.node.DfraHost;
import dfra.dfra.node.DfraAP;

import dfra.dfra.mac.DfraMac;

network DFRA
{
    parameters:
        int numHosts;
        **.mgmt.numChannels = 2;
    submodules:
        host[numHosts]: DfraHost {
            @display("r=,,#707070");
            wlan[*].mgmtType = "DfraMgmtSTA";
            wlan[*].macType = "DfraMac";
           //wlan[*].activeScan = false;                             
        }
        ap: DfraAP {
            @display("p=213,174;r=,,#707070");
            wlan[*].mgmtType = "DfraMgmtAP";
            wlan[*].macType = "DfraMac";
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=61,46");
        }
        configurator: IPv4NetworkConfigurator {
            config = xml("<config><interface hosts='*' address='145.236.x.x' netmask='255.255.0.0'/></config>");
            @display("p=140,50");
        }
        sink: DfraStandardHost {
            @display("p=124,175");
        }
    connections:
        sink.ethg++ <--> Eth100G <--> ap.ethg++;
}
