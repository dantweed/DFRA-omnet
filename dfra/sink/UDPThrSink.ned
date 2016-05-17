package dfra.dfra.sink;

import inet.applications.contract.IUDPApp;

//
// Consumes and prints packets received from the ~UDP module.
//
simple UDPThrSink like IUDPApp
{
    parameters:
        string interfaceTableModule;   // The path to the InterfaceTable module
        double startTime @unit(s) = default(0s); // application start time
        double stopTime @unit(s) = default(-1s);  // time of finishing receiving
        int localPort; // local port
        bool receiveBroadcast = default(false); // if true, makes the socket receive broadcast packets
        string multicastGroup = default(""); // if non-empty, makes the socket receive multicast packets sent to that group
        string multicastSources = default(""); // if non-empty, than the socket receives multicast packets from those sources only

        @display("i=block/sink");
        @signal[rcvdPk](type=cPacket);
        @statistic[rcvdPk](title="packets received"; source=rcvdPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[endToEndDelay](title="end-to-end delay"; source="messageAge(rcvdPk)"; unit=s; record=histogram,vector; interpolationmode=none);
    gates:
        input udpIn @labels(UDPControlInfo/up);
        output udpOut @labels(UDPControlInfo/down);
}

