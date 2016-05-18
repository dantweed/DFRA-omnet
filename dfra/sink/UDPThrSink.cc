#include "inet/networklayer/common/L3AddressResolver.h"
#include "UDPThrSink.h"

#include "inet/common/ModuleAccess.h"
#include "inet/transportlayer/contract/udp/UDPControlInfo_m.h"

namespace inet {

Define_Module(UDPThrSink);

simsignal_t UDPThrSink::rcvdPkSignal = registerSignal("rcvdPk");

UDPThrSink::~UDPThrSink()
{
    cancelAndDelete(selfMsg);
}

void UDPThrSink::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numReceived = 0;
        WATCH(numReceived);

        localPort = par("localPort");
        startTime = par("startTime").doubleValue();
        stopTime = par("stopTime").doubleValue();
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("UDPSinkTimer");
    }
}

void UDPThrSink::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
                processStart();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else if (msg->getKind() == UDP_I_DATA) {
        // process incoming packet
        processPacket(PK(msg));
    }
    else if (msg->getKind() == UDP_I_ERROR) {
        EV_WARN << "Ignoring UDP error report\n";
        delete msg;
    }
    else {
        throw cRuntimeError("Unrecognized message (%s)%s", msg->getClassName(), msg->getName());
    }

    if (hasGUI()) {
        char buf[32];
        sprintf(buf, "rcvd: %d pks", numReceived);
        getDisplayString().setTagArg("t", 0, buf);
    }
}

void UDPThrSink::finish()
{
    ApplicationBase::finish();
    EV_INFO << "SINK STATS\n";
    EV_INFO << getFullPath() << ": received " << numReceived << " packets\n";
    EV_INFO << getFullPath() << ": received " << bits << " bits\n";
    EV_INFO << getFullPath() << ": Total Throughput:  " << bits/(simTime() - startTime) << " bps\n";
    EV_INFO << getFullPath() << ": Per station throughput:\n ";
    for (std::map<L3Address,uint64_t>::iterator it = stats.begin(); it  != stats.end(); it++) {
        EV_INFO << it->first << " => " << it->second/(simTime() - startTime) << "bps\t\n";
    }
}

void UDPThrSink::setSocketOptions()
{
    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);

    // join multicastGroup
    const char *groupAddr = par("multicastGroup");
    multicastGroup = L3AddressResolver().resolve(groupAddr);
    if (!multicastGroup.isUnspecified()) {
        if (!multicastGroup.isMulticast())
            throw cRuntimeError("Wrong multicastGroup setting: not a multicast address: %s", groupAddr);
        socket.joinMulticastGroup(multicastGroup);
    }
}

void UDPThrSink::processStart()
{
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort);
    setSocketOptions();

    if (stopTime >= SIMTIME_ZERO) {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void UDPThrSink::processStop()
{
    if (!multicastGroup.isUnspecified())
        socket.leaveMulticastGroup(multicastGroup); // FIXME should be done by socket.close()
    socket.close();
}

void UDPThrSink::processPacket(cPacket *pk)
{
    //DT
    //EV_INFO << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk) << endl;
    UDPDataIndication *ctrl = check_and_cast<UDPDataIndication *>(pk->getControlInfo());
    uint64_t currBits = pk->getBitLength();
    bits += currBits;
    auto ret = stats.insert( std::pair<L3Address,uint64_t>( ctrl->getSrcAddr(),currBits) );

    if ( ! ret.second ) {
        (stats.find(ctrl->getSrcAddr()))->second += currBits;
    }

    emit(rcvdPkSignal, pk);
    delete pk;

    numReceived++;
}

bool UDPThrSink::handleNodeStart(IDoneCallback *doneCallback)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
    return true;
}

bool UDPThrSink::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (selfMsg)
        cancelEvent(selfMsg);
    //TODO if(socket.isOpened()) socket.close();
    return true;
}

void UDPThrSink::handleNodeCrash()
{
    if (selfMsg)
        cancelEvent(selfMsg);
}

} // namespace inet

