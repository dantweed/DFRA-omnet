package dfra.dfra.node;

import inet.common.lifecycle.NodeStatus;
import inet.linklayer.contract.IMACAddressTable;
import inet.linklayer.contract.IMACRelayUnit;
import inet.linklayer.contract.IWiredNic;
import inet.linklayer.contract.IWirelessNic;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;

import inet.common.queue.Sink;


module DfraAP
{
    parameters:
        @networkNode();
        @labels(node,ethernet-node,wireless-node);
        @display("i=device/accesspoint");
        string mobilityType = default("StationaryMobility"); // how the node moves around. see: inet.mobility
        string relayUnitType = default(firstAvailable("Ieee8021dRelay","MACRelayUnit")); // type of the IMACRelayUnit (MACRelayUnit, Ieee8021dRelay, etc)
        string macTableType = default("MACAddressTable"); // type of the ~IMACAddressTable
        int numRadios = default(1);               // the number of radios in the access point
        bool hasStatus = default(false);
        wlan[*].mgmtType = default("DfraMgmtAP");
        eth[*].encapType = "EtherEncapDummy";
        *.interfaceTableModule = default(absPath(".interfaceTable"));
    gates:
        input radioIn[numRadios] @directIn;
        inout ethg[] @labels(EtherFrame-conn);
    submodules:
        status: NodeStatus if hasStatus {
            @display("p=51,43");
        }

        interfaceTable: InterfaceTable {
            @display("p=51,121;is=s");
        }
        mobility: <default("StationaryMobility")> like IMobility {
            parameters:
                @display("p=208,43");
        }
        macTable: <macTableType> like IMACAddressTable {
            @display("p=73,19;is=s");
        }
        relayUnit: <relayUnitType> like IMACRelayUnit if sizeof(ethg)+numRadios>1 && relayUnitType != "" {
            parameters:
                @display("p=151,121");
        }
        wlan[numRadios]: <default("DfraNic")> like IWirelessNic {
            parameters:
                @display("p=89,225,row;q=queue");
        }
        eth[sizeof(ethg)]: <default("EthernetInterface")> like IWiredNic {
            parameters:
                mac.promiscuous = true;
                @display("p=208,225,row;q=txQueue");
        }        
    connections allowunconnected:
        // connections to network outside
        // wireless interfaces MUST be connected first (i.e. ports 0..numRadios-1)
        // because broadcasts must be handled differently for wireless IFs by the relayUnit
        for i=0..numRadios-1 {
            radioIn[i] --> wlan[i].radioIn;
            wlan[i].upperLayerOut --> relayUnit.ifIn++ if sizeof(ethg)+numRadios>1;
            wlan[i].upperLayerIn <-- relayUnit.ifOut++ if sizeof(ethg)+numRadios>1;
        }
        // ethernet must be connected only AFTER wireless ports
        for i=0..sizeof(ethg)-1 {
            eth[i].phys <--> ethg[i];
            eth[i].upperLayerIn <-- relayUnit.ifOut++ if sizeof(ethg)+numRadios>1;
            eth[i].upperLayerOut --> relayUnit.ifIn++ if sizeof(ethg)+numRadios>1;
        }
}
