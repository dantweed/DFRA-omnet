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

#include "dfra/mgmt/DfraMgmtSTA.h"

#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/common/NotifierConsts.h"
#include "inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "inet/common/INETUtils.h"

namespace inet {

namespace ieee80211 {

using namespace physicallayer;

//TBD supportedRates!
//TBD use command msg kinds?
//TBD implement bitrate switching (Radio already supports it)
//TBD where to put LCC header (SNAP)..?
//TBD mac should be able to signal when msg got transmitted

Define_Module(DfraMgmtSTA);

// message kind values for timers
#define MK_AUTH_TIMEOUT           1
#define MK_ASSOC_TIMEOUT          2
#define MK_SCAN_SENDPROBE         3
#define MK_SCAN_MINCHANNELTIME    4
#define MK_SCAN_MAXCHANNELTIME    5
#define MK_BEACON_TIMEOUT         6

#define MSG_CHANGE_SCHED          99


#define MAX_BEACONS_MISSED        1.5  // beacon lost timeout, in beacon intervals (doesn't need to be integer)

std::ostream& operator<<(std::ostream& os, const DfraMgmtSTA::ScanningInfo& scanning)
{
    os << "activeScan=" << scanning.activeScan
       << " probeDelay=" << scanning.probeDelay
       << " curChan=";
    if (scanning.channelList.empty())
        os << "<none>";
    else
        os << scanning.channelList[scanning.currentChannelIndex];
    os << " minChanTime=" << scanning.minChannelTime
       << " maxChanTime=" << scanning.maxChannelTime;
    os << " chanList={";
    for (int i = 0; i < (int)scanning.channelList.size(); i++)
        os << (i == 0 ? "" : " ") << scanning.channelList[i];
    os << "}";

    return os;
}

std::ostream& operator<<(std::ostream& os, const DfraMgmtSTA::APInfo& ap)
{
    os << "AP addr=" << ap.address
       << " chan=" << ap.channel
       << " ssid=" << ap.ssid
        //TBD supportedRates
       << " beaconIntvl=" << ap.beaconInterval
       << " rxPower=" << ap.rxPower
       << " authSeqExpected=" << ap.authSeqExpected
       << " isAuthenticated=" << ap.isAuthenticated;
    return os;
}

std::ostream& operator<<(std::ostream& os, const DfraMgmtSTA::AssociatedAPInfo& assocAP)
{
    os << "AP addr=" << assocAP.address
       << " chan=" << assocAP.channel
       << " ssid=" << assocAP.ssid
       << " beaconIntvl=" << assocAP.beaconInterval
       << " receiveSeq=" << assocAP.receiveSequence
       << " rxPower=" << assocAP.rxPower;
    return os;
}

DfraMgmtSTA::~DfraMgmtSTA()
{
    delete mySchedule;
}

void DfraMgmtSTA::initialize(int stage)
{
    Ieee80211MgmtBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        isScanning = false;
        isAssociated = false;
        assocTimeoutMsg = nullptr;
        myIface = nullptr;
        numChannels = par("numChannels");

        host = getContainingNode(this);
        host->subscribe(NF_LINK_FULL_PROMISCUOUS, this);

        WATCH(isScanning);
        WATCH(isAssociated);

        WATCH(scanning);
        WATCH(assocAP);
        WATCH_LIST(apList);
    }
    else if (stage == INITSTAGE_LINK_LAYER_2) {
        IInterfaceTable *ift = findModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        if (ift) {
            myIface = ift->getInterfaceByName(utils::stripnonalnum(findModuleUnderContainingNode(this)->getFullName()).c_str());
        }
    }
}

void DfraMgmtSTA::handleTimer(cMessage *msg)
{
    if (msg->getKind() == MK_AUTH_TIMEOUT) {
        // authentication timed out
        APInfo *ap = (APInfo *)msg->getContextPointer();
        EV << "Authentication timed out, AP address = " << ap->address << "\n";

        // send back failure report to agent
        sendAuthenticationConfirm(ap, PRC_TIMEOUT);
    }
    else if (msg->getKind() == MK_ASSOC_TIMEOUT) {
        // association timed out
        APInfo *ap = (APInfo *)msg->getContextPointer();
        EV << "Association timed out, AP address = " << ap->address << "\n";

        // send back failure report to agent
        sendAssociationConfirm(ap, PRC_TIMEOUT);
    }
    else if (msg->getKind() == MK_SCAN_MAXCHANNELTIME) {
        // go to next channel during scanning
        bool done = scanNextChannel();
        if (done)
            sendScanConfirm(); // send back response to agents' "scan" command
        delete msg;
    }
    else if (msg->getKind() == MK_SCAN_SENDPROBE) {
        // Active Scan: send a probe request, then wait for minChannelTime (11.1.3.2.2)
        delete msg;
        sendProbeRequest();
        cMessage *timerMsg = new cMessage("minChannelTime", MK_SCAN_MINCHANNELTIME);
        scheduleAt(simTime() + scanning.minChannelTime, timerMsg);    //XXX actually, we should start waiting after ProbeReq actually got transmitted
    }
    else if (msg->getKind() == MK_SCAN_MINCHANNELTIME) {
        // Active Scan: after minChannelTime, possibly listen for the remaining time until maxChannelTime
        delete msg;
        if (scanning.busyChannelDetected) {
            EV << "Busy channel detected during minChannelTime, continuing listening until maxChannelTime elapses\n";
            cMessage *timerMsg = new cMessage("maxChannelTime", MK_SCAN_MAXCHANNELTIME);
            scheduleAt(simTime() + scanning.maxChannelTime - scanning.minChannelTime, timerMsg);
        }
        else {
            EV << "Channel was empty during minChannelTime, going to next channel\n";
            bool done = scanNextChannel();
            if (done)
                sendScanConfirm(); // send back response to agents' "scan" command
        }
    }
    else if (msg->getKind() == MK_BEACON_TIMEOUT) {
        // missed a few consecutive beacons
        beaconLost();
    }
    else {
        throw cRuntimeError("internal error: unrecognized timer '%s'", msg->getName());
    }
}

void DfraMgmtSTA::handleUpperMessage(cPacket *msg)
{
    if (!isAssociated || assocAP.address.isUnspecified()) {
        EV << "STA is not associated with an access point, discarding packet" << msg << "\n";
        delete msg;
        return;
    }

    Ieee80211DataFrame *frame = encapsulate(msg);
    sendDown(frame);
}

void DfraMgmtSTA::handleCommand(int msgkind, cObject *ctrl)
{
    if (dynamic_cast<Ieee80211Prim_ScanRequest *>(ctrl))
        processScanCommand((Ieee80211Prim_ScanRequest *)ctrl);
    else if (dynamic_cast<Ieee80211Prim_AuthenticateRequest *>(ctrl))
        processAuthenticateCommand((Ieee80211Prim_AuthenticateRequest *)ctrl);
    else if (dynamic_cast<Ieee80211Prim_DeauthenticateRequest *>(ctrl))
        processDeauthenticateCommand((Ieee80211Prim_DeauthenticateRequest *)ctrl);
    else if (dynamic_cast<Ieee80211Prim_AssociateRequest *>(ctrl))
        processAssociateCommand((Ieee80211Prim_AssociateRequest *)ctrl);
    else if (dynamic_cast<Ieee80211Prim_ReassociateRequest *>(ctrl))
        processReassociateCommand((Ieee80211Prim_ReassociateRequest *)ctrl);
    else if (dynamic_cast<Ieee80211Prim_DisassociateRequest *>(ctrl))
        processDisassociateCommand((Ieee80211Prim_DisassociateRequest *)ctrl);
    else if (ctrl)
        throw cRuntimeError("handleCommand(): unrecognized control info class `%s'", ctrl->getClassName());
    else
        throw cRuntimeError("handleCommand(): control info is nullptr");
    delete ctrl;
}

Ieee80211DataFrame *DfraMgmtSTA::encapsulate(cPacket *msg)
{
    Ieee80211DataFrameWithSNAP *frame = new Ieee80211DataFrameWithSNAP(msg->getName());

    // frame goes to the AP
    frame->setToDS(true);

    // receiver is the AP
    frame->setReceiverAddress(assocAP.address);
    frame->setAID(assocAP.aid);  //DT: adding AID to all frames
    // destination address is in address3
    Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl *>(msg->removeControlInfo());
    frame->setAddress3(ctrl->getDest());
    frame->setEtherType(ctrl->getEtherType());
    int up = ctrl->getUserPriority();
    if (up >= 0) {
        // make it a QoS frame, and set TID
        frame->setType(ST_DATA_WITH_QOS);
        frame->addBitLength(QOSCONTROL_BITS);
        frame->setTid(up);
    }
    delete ctrl;

    frame->encapsulate(msg);
    return frame;
}

cPacket *DfraMgmtSTA::decapsulate(Ieee80211DataFrame *frame)
{
    cPacket *payload = frame->decapsulate();

    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setSrc(frame->getAddress3());
    ctrl->setDest(frame->getReceiverAddress());
    int tid = frame->getTid();
    if (tid < 8)
        ctrl->setUserPriority(tid); // TID values 0..7 are UP
    Ieee80211DataFrameWithSNAP *frameWithSNAP = dynamic_cast<Ieee80211DataFrameWithSNAP *>(frame);
    if (frameWithSNAP)
        ctrl->setEtherType(frameWithSNAP->getEtherType());
    payload->setControlInfo(ctrl);

    delete frame;
    return payload;
}

DfraMgmtSTA::APInfo *DfraMgmtSTA::lookupAP(const MACAddress& address)
{
    for (auto & elem : apList)
        if (elem.address == address)
            return &(elem);

    return nullptr;
}

void DfraMgmtSTA::clearAPList()
{
    for (auto & elem : apList)
        if (elem.authTimeoutMsg)
            delete cancelEvent(elem.authTimeoutMsg);

    apList.clear();
}

void DfraMgmtSTA::changeChannel(int channelNum)
{
    EV << "Tuning to channel #" << channelNum << "\n";

    Ieee80211ConfigureRadioCommand *configureCommand = new Ieee80211ConfigureRadioCommand();
    configureCommand->setChannelNumber(channelNum);
    cMessage *msg = new cMessage("changeChannel", RADIO_C_CONFIGURE);
    msg->setControlInfo(configureCommand);
    send(msg, "macOut");
}

void DfraMgmtSTA::beaconLost()
{
    EV << "Missed a few consecutive beacons -- AP is considered lost\n";
    emit(NF_L2_BEACON_LOST, myIface);
}

void DfraMgmtSTA::sendManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress& address)
{
    // frame goes to the specified AP
    frame->setToDS(true);
    frame->setReceiverAddress(address);
    //XXX set sequenceNumber?

    sendDown(frame);
}

void DfraMgmtSTA::startAuthentication(APInfo *ap, simtime_t timeout)
{
    if (ap->authTimeoutMsg)
        throw cRuntimeError("startAuthentication: authentication currently in progress with AP address=", ap->address.str().c_str());
    if (ap->isAuthenticated)
        throw cRuntimeError("startAuthentication: already authenticated with AP address=", ap->address.str().c_str());

    changeChannel(ap->channel);

    EV << "Sending initial Authentication frame with seqNum=1\n";

    // create and send first authentication frame
    Ieee80211AuthenticationFrame *frame = new Ieee80211AuthenticationFrame("Auth");
    frame->getBody().setSequenceNumber(1);
    //XXX frame length could be increased to account for challenge text length etc.
    sendManagementFrame(frame, ap->address);

    ap->authSeqExpected = 2;

    // schedule timeout
    ASSERT(ap->authTimeoutMsg == nullptr);
    ap->authTimeoutMsg = new cMessage("authTimeout", MK_AUTH_TIMEOUT);
    ap->authTimeoutMsg->setContextPointer(ap);
    scheduleAt(simTime() + timeout, ap->authTimeoutMsg);
}

void DfraMgmtSTA::startAssociation(APInfo *ap, simtime_t timeout)
{
    if (isAssociated || assocTimeoutMsg)
        throw cRuntimeError("startAssociation: already associated or association currently in progress");
    if (!ap->isAuthenticated)
        throw cRuntimeError("startAssociation: not yet authenticated with AP address=", ap->address.str().c_str());

    // switch to that channel
    changeChannel(ap->channel);

    // create and send association request
    Ieee80211AssociationRequestFrame *frame = new Ieee80211AssociationRequestFrame("Assoc");

    //XXX set the following too?
    // string SSID
    // Ieee80211SupportedRatesElement supportedRates;

    sendManagementFrame(frame, ap->address);

    // schedule timeout
    ASSERT(assocTimeoutMsg == nullptr);
    assocTimeoutMsg = new cMessage("assocTimeout", MK_ASSOC_TIMEOUT);
    assocTimeoutMsg->setContextPointer(ap);
    scheduleAt(simTime() + timeout, assocTimeoutMsg);
}

void DfraMgmtSTA::receiveSignal(cComponent *source, simsignal_t signalID, long value DETAILS_ARG)
{
    Enter_Method_Silent();
    // Note that we are only subscribed during scanning!
    if (signalID == IRadio::receptionStateChangedSignal) {
        IRadio::ReceptionState newReceptionState = (IRadio::ReceptionState)value;
        if (newReceptionState != IRadio::RECEPTION_STATE_UNDEFINED && newReceptionState != IRadio::RECEPTION_STATE_IDLE) {
            EV << "busy radio channel detected during scanning\n";
            scanning.busyChannelDetected = true;
        }
    }
}

void DfraMgmtSTA::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj DETAILS_ARG)
{
    Enter_Method_Silent();
    printNotificationBanner(signalID, obj);

    // Note that we are only subscribed during scanning!
    if (signalID == NF_LINK_FULL_PROMISCUOUS) {
        Ieee80211DataOrMgmtFrame *frame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(obj);
        if (!frame || frame->getControlInfo() == nullptr)
            return;
        if (frame->getType() != ST_BEACON)
            return;
        Ieee80211ReceptionIndication *ctl = dynamic_cast<Ieee80211ReceptionIndication *>(frame->getControlInfo());
        if (ctl == nullptr)
            return;
        Ieee80211BeaconFrame *beacon = (check_and_cast<Ieee80211BeaconFrame *>(frame));
        APInfo *ap = lookupAP(beacon->getTransmitterAddress());
        if (ap)
            ap->rxPower = ctl->getRecPow();
    }
}

void DfraMgmtSTA::processScanCommand(Ieee80211Prim_ScanRequest *ctrl)
{
    EV << "Received Scan Request from agent, clearing AP list and starting scanning...\n";

    if (isScanning)
        throw cRuntimeError("processScanCommand: scanning already in progress");
    if (isAssociated) {
        disassociate();
    }
    else if (assocTimeoutMsg) {
        EV << "Cancelling ongoing association process\n";
        delete cancelEvent(assocTimeoutMsg);
        assocTimeoutMsg = nullptr;
    }

    // clear existing AP list (and cancel any pending authentications) -- we want to start with a clean page
    clearAPList();

    // fill in scanning state
    ASSERT(ctrl->getBSSType() == BSSTYPE_INFRASTRUCTURE);
    scanning.bssid = ctrl->getBSSID().isUnspecified() ? MACAddress::BROADCAST_ADDRESS : ctrl->getBSSID();
    scanning.ssid = ctrl->getSSID();
    scanning.activeScan = ctrl->getActiveScan();
    scanning.probeDelay = ctrl->getProbeDelay();
    scanning.channelList.clear();
    scanning.minChannelTime = ctrl->getMinChannelTime();
    scanning.maxChannelTime = ctrl->getMaxChannelTime();
    ASSERT(scanning.minChannelTime <= scanning.maxChannelTime);

    // channel list to scan (default: all channels)
    for (int i = 0; i < (int)ctrl->getChannelListArraySize(); i++)
        scanning.channelList.push_back(ctrl->getChannelList(i));
    if (scanning.channelList.empty())
        for (int i = 0; i < numChannels; i++)
            scanning.channelList.push_back(i);


    // start scanning
    if (scanning.activeScan)
        host->subscribe(IRadio::receptionStateChangedSignal, this);
    scanning.currentChannelIndex = -1;    // so we'll start with index==0
    isScanning = true;
    scanNextChannel();
}

bool DfraMgmtSTA::scanNextChannel()
{
    // if we're already at the last channel, we're through
    if (scanning.currentChannelIndex == (int)scanning.channelList.size() - 1) {
        EV << "Finished scanning last channel\n";
        if (scanning.activeScan)
            host->unsubscribe(IRadio::receptionStateChangedSignal, this);
        isScanning = false;
        return true;    // we're done
    }

    // tune to next channel
    int newChannel = scanning.channelList[++scanning.currentChannelIndex];
    changeChannel(newChannel);
    scanning.busyChannelDetected = false;

    if (scanning.activeScan) {
        // Active Scan: first wait probeDelay, then send a probe. Listening
        // for minChannelTime or maxChannelTime takes place after that. (11.1.3.2)
        scheduleAt(simTime() + scanning.probeDelay, new cMessage("sendProbe", MK_SCAN_SENDPROBE));
    }
    else {
        // Passive Scan: spend maxChannelTime on the channel (11.1.3.1)
        cMessage *timerMsg = new cMessage("maxChannelTime", MK_SCAN_MAXCHANNELTIME);
        scheduleAt(simTime() + scanning.maxChannelTime, timerMsg);
    }

    return false;
}

void DfraMgmtSTA::sendProbeRequest()
{
    EV << "Sending Probe Request, BSSID=" << scanning.bssid << ", SSID=\"" << scanning.ssid << "\"\n";
    Ieee80211ProbeRequestFrame *frame = new Ieee80211ProbeRequestFrame("ProbeReq");
    frame->getBody().setSSID(scanning.ssid.c_str());
    sendManagementFrame(frame, scanning.bssid);
}

void DfraMgmtSTA::sendScanConfirm()
{
    EV << "Scanning complete, found " << apList.size() << " APs, sending confirmation to agent\n";

    // copy apList contents into a ScanConfirm primitive and send it back
    int n = apList.size();
    Ieee80211Prim_ScanConfirm *confirm = new Ieee80211Prim_ScanConfirm();
    confirm->setBssListArraySize(n);
    auto it = apList.begin();
    //XXX filter for req'd bssid and ssid
    for (int i = 0; i < n; i++, it++) {
        APInfo *ap = &(*it);
        Ieee80211Prim_BSSDescription& bss = confirm->getBssList(i);
        bss.setChannelNumber(ap->channel);
        bss.setBSSID(ap->address);
        bss.setSSID(ap->ssid.c_str());
        bss.setSupportedRates(ap->supportedRates);
        bss.setBeaconInterval(ap->beaconInterval);
        bss.setRxPower(ap->rxPower);
    }
    sendConfirm(confirm, PRC_SUCCESS);
}

void DfraMgmtSTA::processAuthenticateCommand(Ieee80211Prim_AuthenticateRequest *ctrl)
{
    const MACAddress& address = ctrl->getAddress();
    APInfo *ap = lookupAP(address);
    if (!ap)
        throw cRuntimeError("processAuthenticateCommand: AP not known: address = %s", address.str().c_str());
    startAuthentication(ap, ctrl->getTimeout());
}

void DfraMgmtSTA::processDeauthenticateCommand(Ieee80211Prim_DeauthenticateRequest *ctrl)
{
    const MACAddress& address = ctrl->getAddress();
    APInfo *ap = lookupAP(address);
    if (!ap)
        throw cRuntimeError("processDeauthenticateCommand: AP not known: address = %s", address.str().c_str());

    if (isAssociated && assocAP.address == address)
        disassociate();

    if (ap->isAuthenticated)
        ap->isAuthenticated = false;

    // cancel possible pending authentication timer
    if (ap->authTimeoutMsg) {
        delete cancelEvent(ap->authTimeoutMsg);
        ap->authTimeoutMsg = nullptr;
    }

    // create and send deauthentication request
    Ieee80211DeauthenticationFrame *frame = new Ieee80211DeauthenticationFrame("Deauth");
    frame->getBody().setReasonCode(ctrl->getReasonCode());
    sendManagementFrame(frame, address);
}

void DfraMgmtSTA::processAssociateCommand(Ieee80211Prim_AssociateRequest *ctrl)
{
    const MACAddress& address = ctrl->getAddress();
    APInfo *ap = lookupAP(address);
    if (!ap)
        throw cRuntimeError("processAssociateCommand: AP not known: address = %s", address.str().c_str());
    startAssociation(ap, ctrl->getTimeout());
}

void DfraMgmtSTA::processReassociateCommand(Ieee80211Prim_ReassociateRequest *ctrl)
{
    // treat the same way as association
    //XXX refine
    processAssociateCommand(ctrl);
}

void DfraMgmtSTA::processDisassociateCommand(Ieee80211Prim_DisassociateRequest *ctrl)
{
    const MACAddress& address = ctrl->getAddress();

    if (isAssociated && address == assocAP.address) {
        disassociate();
    }
    else if (assocTimeoutMsg) {
        // pending association
        delete cancelEvent(assocTimeoutMsg);
        assocTimeoutMsg = nullptr;
    }

    // create and send disassociation request
    Ieee80211DisassociationFrame *frame = new Ieee80211DisassociationFrame("Disass");
    frame->getBody().setReasonCode(ctrl->getReasonCode());
    sendManagementFrame(frame, address);
}

void DfraMgmtSTA::disassociate()
{
    EV << "Disassociating from AP address=" << assocAP.address << "\n";
    ASSERT(isAssociated);
    isAssociated = false;
    delete cancelEvent(assocAP.beaconTimeoutMsg);
    assocAP.beaconTimeoutMsg = nullptr;
    assocAP = AssociatedAPInfo();    // clear it
}

void DfraMgmtSTA::sendAuthenticationConfirm(APInfo *ap, int resultCode)
{
    Ieee80211Prim_AuthenticateConfirm *confirm = new Ieee80211Prim_AuthenticateConfirm();
    confirm->setAddress(ap->address);
    sendConfirm(confirm, resultCode);
}

void DfraMgmtSTA::sendAssociationConfirm(APInfo *ap, int resultCode)
{
    sendConfirm(new Ieee80211Prim_AssociateConfirm(), resultCode);
}

void DfraMgmtSTA::sendConfirm(Ieee80211PrimConfirm *confirm, int resultCode)
{
    confirm->setResultCode(resultCode);
    cMessage *msg = new cMessage(confirm->getClassName());
    msg->setControlInfo(confirm);
    send(msg, "agentOut");
}

int DfraMgmtSTA::statusCodeToPrimResultCode(int statusCode)
{
    return statusCode == SC_SUCCESSFUL ? PRC_SUCCESS : PRC_REFUSED;
}

void DfraMgmtSTA::handleDataFrame(Ieee80211DataFrame *frame)
{
    // Only send the Data frame up to the higher layer if the STA is associated with an AP,
    // else delete the frame
    if (isAssociated)
        sendUp(decapsulate(frame));
    else {
        EV << "Rejecting data frame as STA is not associated with an AP" << endl;
        delete frame;
    }
}

void DfraMgmtSTA::handleAuthenticationFrame(Ieee80211AuthenticationFrame *frame)
{
    MACAddress address = frame->getTransmitterAddress();
    int frameAuthSeq = frame->getBody().getSequenceNumber();
    EV << "Received Authentication frame from address=" << address << ", seqNum=" << frameAuthSeq << "\n";

    APInfo *ap = lookupAP(address);
    if (!ap) {
        EV << "AP not known, discarding authentication frame\n";
        delete frame;
        return;
    }

    // what if already authenticated with AP
    if (ap->isAuthenticated) {
        EV << "AP already authenticated, ignoring frame\n";
        delete frame;
        return;
    }

    // is authentication is in progress with this AP?
    if (!ap->authTimeoutMsg) {
        EV << "No authentication in progress with AP, ignoring frame\n";
        delete frame;
        return;
    }

    // check authentication sequence number is OK
    if (frameAuthSeq != ap->authSeqExpected) {
        // wrong sequence number: send error and return
        EV << "Wrong sequence number, " << ap->authSeqExpected << " expected\n";
        Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame("Auth-ERROR");
        resp->getBody().setStatusCode(SC_AUTH_OUT_OF_SEQ);
        sendManagementFrame(resp, frame->getTransmitterAddress());
        delete frame;

        // cancel timeout, send error to agent
        delete cancelEvent(ap->authTimeoutMsg);
        ap->authTimeoutMsg = nullptr;
        sendAuthenticationConfirm(ap, PRC_REFUSED);    //XXX or what resultCode?
        return;
    }

    // check if more exchanges are needed for auth to be complete
    int statusCode = frame->getBody().getStatusCode();

    if (statusCode == SC_SUCCESSFUL && !frame->getBody().getIsLast()) {
        EV << "More steps required, sending another Authentication frame\n";

        // more steps required, send another Authentication frame
        Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame("Auth");
        resp->getBody().setSequenceNumber(frameAuthSeq + 1);
        resp->getBody().setStatusCode(SC_SUCCESSFUL);
        // XXX frame length could be increased to account for challenge text length etc.
        sendManagementFrame(resp, address);
        ap->authSeqExpected += 2;
    }
    else {
/*
        if (statusCode == SC_SUCCESSFUL)
            EV << "Authentication successful\n";
        else
            EV << "Authentication failed\n";
*/

        // authentication completed
        ap->isAuthenticated = (statusCode == SC_SUCCESSFUL);
        delete cancelEvent(ap->authTimeoutMsg);
        ap->authTimeoutMsg = nullptr;
        sendAuthenticationConfirm(ap, statusCodeToPrimResultCode(statusCode));
    }

    delete frame;
}

void DfraMgmtSTA::handleDeauthenticationFrame(Ieee80211DeauthenticationFrame *frame)
{
    EV << "Received Deauthentication frame\n";
    const MACAddress& address = frame->getAddress3();    // source address
    APInfo *ap = lookupAP(address);
    if (!ap || !ap->isAuthenticated) {
        EV << "Unknown AP, or not authenticated with that AP -- ignoring frame\n";
        delete frame;
        return;
    }
    if (ap->authTimeoutMsg) {
        delete cancelEvent(ap->authTimeoutMsg);
        ap->authTimeoutMsg = nullptr;
        EV << "Cancelling pending authentication\n";
        delete frame;
        return;
    }

    EV << "Setting isAuthenticated flag for that AP to false\n";
    ap->isAuthenticated = false;
    delete frame;
}

void DfraMgmtSTA::handleAssociationRequestFrame(Ieee80211AssociationRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void DfraMgmtSTA::handleAssociationResponseFrame(Ieee80211AssociationResponseFrame *frame)
{
    EV << "Received Association Response frame\n";

    if (!assocTimeoutMsg) {
        EV << "No association in progress, ignoring frame\n";
        delete frame;
        return;
    }

    // extract frame contents
    MACAddress address = frame->getTransmitterAddress();
    int statusCode = frame->getBody().getStatusCode();
    short aid = frame->getAID();

    //XXX Ieee80211SupportedRatesElement supportedRates;
    delete frame;

    // look up AP data structure
    APInfo *ap = lookupAP(address);
    if (!ap)
        throw cRuntimeError("handleAssociationResponseFrame: AP not known: address=%s", address.str().c_str());

    if (isAssociated) {
        EV << "Breaking existing association with AP address=" << assocAP.address << "\n";
        isAssociated = false;
        delete cancelEvent(assocAP.beaconTimeoutMsg);
        assocAP.beaconTimeoutMsg = nullptr;
        assocAP = AssociatedAPInfo();
    }

    delete cancelEvent(assocTimeoutMsg);
    assocTimeoutMsg = nullptr;

    if (statusCode != SC_SUCCESSFUL) {
        EV << "Association failed with AP address=" << ap->address << "\n";
    }
    else {
        EV << "Association successful, AP address=" << ap->address << " AID=" << aid <<"\n";

        // change our state to "associated"
        isAssociated = true;
        (APInfo&)assocAP = (*ap);
        assocAP.aid = aid;
        emit(NF_L2_ASSOCIATED, myIface);

        getContainingNode(this)->bubble("Associated with AP");
        assocAP.beaconTimeoutMsg = new cMessage("beaconTimeout", MK_BEACON_TIMEOUT);
        scheduleAt(simTime() + MAX_BEACONS_MISSED * assocAP.beaconInterval, assocAP.beaconTimeoutMsg);
    }

    // report back to agent
    sendAssociationConfirm(ap, statusCodeToPrimResultCode(statusCode));
}

void DfraMgmtSTA::handleReassociationRequestFrame(Ieee80211ReassociationRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void DfraMgmtSTA::handleReassociationResponseFrame(Ieee80211ReassociationResponseFrame *frame)
{
    EV << "Received Reassociation Response frame\n";
    //TBD handle with the same code as Association Response?
}

void DfraMgmtSTA::handleDisassociationFrame(Ieee80211DisassociationFrame *frame)
{
    EV << "Received Disassociation frame\n";
    const MACAddress& address = frame->getAddress3();    // source address

    if (assocTimeoutMsg) {
        // pending association
        delete cancelEvent(assocTimeoutMsg);
        assocTimeoutMsg = nullptr;
    }
    if (!isAssociated || address != assocAP.address) {
        EV << "Not associated with that AP -- ignoring frame\n";
        delete frame;
        return;
    }

    EV << "Setting isAssociated flag to false\n";
    isAssociated = false;
    delete cancelEvent(assocAP.beaconTimeoutMsg);
    assocAP.beaconTimeoutMsg = nullptr;
}

void DfraMgmtSTA::handleBeaconFrame(Ieee80211BeaconFrame *frame)
{//FIXME: Re-organize and test

    Ieee80211BeaconFrameBody& body = frame->getBody();
    storeAPInfo(frame->getTransmitterAddress(), body);

    //Extract scheduling info
    Sched *schedule = ((Sched*)frame->getAddedFields());

    // if it is our associate AP, restart beacon timeout
    if ( (isAssociated && frame->getTransmitterAddress() == assocAP.address)  || !isAssociated) {
        EV << "Beacon is from associated AP, restarting beacon timeout timer\n";

        if (mySchedule) delete mySchedule;
        int numDRBs = (int)schedule->numDRBs;
        mySchedule = new SchedulingInfo(schedule->numDRBs);
        mySchedule->beaconReference = schedule->beaconReference;
        mySchedule->drbLength = body.getBeaconInterval()/numDRBs;  //if associated or not,  beacon interval can change

        memcpy(mySchedule->frameTypes, schedule->frameTypes, ceil(numDRBs/8));

        if (isAssociated) {
            ASSERT(assocAP.beaconTimeoutMsg != nullptr);
            cancelEvent(assocAP.beaconTimeoutMsg);
            scheduleAt(simTime() + MAX_BEACONS_MISSED * assocAP.beaconInterval, assocAP.beaconTimeoutMsg);
            mySchedule->aid = assocAP.aid; //-1 if not associated, by default
            memcpy(mySchedule->mysched, &schedule->staSchedules[(numDRBs/2)*(assocAP.aid-1)], (numDRBs/2));
        } else {//Set to RA, for correct DRBS, giving priority to Demand assigned stations
            if (!schedule->staSchedules) {//indicates AP has no associated stations, so all drbs are RA
                for (int j = 0; j < numDRBs/2; j++){ //over each 4bit drb schedule, two nibbles at a time
                    //bytewise schedule setting, from left to right using aid as BI multiplier, alternating which DRB
                    mySchedule->mysched[j] = (BYTE)0x22;
                }
            } else  {//Limited to 1st and 4th DRB, should be RA by
                mySchedule->mysched[0] = mySchedule->mysched[4] = (BYTE)0x22;
            }
        }

        //APInfo *ap = lookupAP(frame->getTransmitterAddress());
        //ASSERT(ap!=nullptr);
        if (isAssociated) EV << "Assoc: Rec'd beacon " << mySchedule->beaconReference << "\n";
        else EV <<"Not Assoc: Rec'd beacon "<< mySchedule->beaconReference << "\n";

        //Send schedule to  MAC layer
        cMessage *msg = new cMessage("changeSched", MSG_CHANGE_SCHED);
        msg->setSchedulingPriority(0);
        msg->setContextPointer(mySchedule);
        send(msg, "macOut");
        if (assocAP.aid > 1)
            int a = 1;
    }  else { //Associated and beacon is not from our AP
        //Do nothing ... but I'm leaving this here as may want to do something later
    }

    delete frame;

}

void DfraMgmtSTA::handleProbeRequestFrame(Ieee80211ProbeRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void DfraMgmtSTA::handleProbeResponseFrame(Ieee80211ProbeResponseFrame *frame)
{
    EV << "Received Probe Response frame\n";
    storeAPInfo(frame->getTransmitterAddress(), frame->getBody());
    delete frame;
}

void DfraMgmtSTA::storeAPInfo(const MACAddress& address, const Ieee80211BeaconFrameBody& body)
{
    APInfo *ap = lookupAP(address);
    if (ap) {
        EV << "AP address=" << address << ", SSID=" << body.getSSID() << " already in our AP list, refreshing the info\n";
    }
    else {
        EV << "Inserting AP address=" << address << ", SSID=" << body.getSSID() << " into our AP list\n";
        apList.push_back(APInfo());
        ap = &apList.back();
    }

    ap->channel = body.getChannelNumber();
    ap->address = address;
    ap->ssid = body.getSSID();
    ap->supportedRates = body.getSupportedRates();
    ap->beaconInterval = body.getBeaconInterval();

    //XXX where to get this from?
    //ap->rxPower = ...
}
void DfraMgmtSTA::finish() { if (mySchedule) delete mySchedule;}
} // namespace ieee80211

} // namespace inet

