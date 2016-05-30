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

#include "dfra/mgmt/DfraMgmtAP.h"

#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"

#ifdef WITH_ETHERNET
#include "inet/linklayer/ethernet/EtherFrame.h"
#endif // ifdef WITH_ETHERNET

#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/NotifierConsts.h"

namespace inet {

namespace ieee80211 {

//namespace dfra {


using namespace physicallayer;

Define_Module(DfraMgmtAP);
Register_Class(DfraMgmtAP::NotificationInfoSta);

static std::ostream& operator<<(std::ostream& os, const DfraMgmtAP::STAInfo& sta)
{
    os << "state:" << sta.status;
    return os;
}

DfraMgmtAP::~DfraMgmtAP()
{
    cancelAndDelete(beaconTimer);
}

void DfraMgmtAP::initialize(int stage)
{
    Ieee80211MgmtAPBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // read params and init vars
        ssid = par("ssid").stringValue();
        beaconInterval = par("beaconInterval");
        numAuthSteps = par("numAuthSteps");
        if (numAuthSteps != 2 && numAuthSteps != 4)
            throw cRuntimeError("parameter 'numAuthSteps' (number of frames exchanged during authentication) must be 2 or 4, not %d", numAuthSteps);
        channelNumber = -1;    // value will arrive from physical layer in receiveChangeNotification()
        nextAID = 1;
        WATCH(ssid);
        WATCH(channelNumber);
        WATCH(beaconInterval);
        WATCH(numAuthSteps);
        WATCH_MAP(staList);

        //TBD fill in supportedRates

        // subscribe for notifications
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(Ieee80211Radio::radioChannelChangedSignal, this);

        // start beacon timer (randomize startup time)
        beaconTimer = new cMessage("beaconTimer");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        if (isOperational)
            scheduleAt(simTime() + uniform(0, beaconInterval), beaconTimer);
    }
}

void DfraMgmtAP::handleTimer(cMessage *msg)
{
    if (msg == beaconTimer) {
        sendBeacon();
        scheduleAt(simTime() + beaconInterval, beaconTimer);
    }
    else {
        throw cRuntimeError("internal error: unrecognized timer '%s'", msg->getName());
    }
}

void DfraMgmtAP::handleUpperMessage(cPacket *msg)
{
    Ieee80211DataFrame *frame = encapsulate(msg);
    MACAddress macAddr = frame->getReceiverAddress();
    if (!macAddr.isMulticast()) {
        auto it = staList.find(macAddr);
        if (it == staList.end() || it->second.status != ASSOCIATED) {
            EV << "STA with MAC address " << macAddr << " not associated with this AP, dropping frame\n";
            delete frame;    // XXX count drops?
            return;
        }
    }

    sendDown(frame);
}

void DfraMgmtAP::handleCommand(int msgkind, cObject *ctrl)
{
    throw cRuntimeError("handleCommand(): no commands supported");
}

void DfraMgmtAP::receiveSignal(cComponent *source, simsignal_t signalID, long value DETAILS_ARG)
{
    Enter_Method_Silent();
    if (signalID == Ieee80211Radio::radioChannelChangedSignal) {
        EV << "updating channel number\n";
        channelNumber = value;
    }
}

DfraMgmtAP::STAInfo *DfraMgmtAP::lookupSenderSTA(Ieee80211ManagementFrame *frame)
{
    auto it = staList.find(frame->getTransmitterAddress());
    return it == staList.end() ? nullptr : &(it->second);
}

void DfraMgmtAP::sendManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress& destAddr)
{
    frame->setFromDS(true);
    frame->setReceiverAddress(destAddr);
    frame->setAddress3(myAddress);
    sendDown(frame);
}

void DfraMgmtAP::sendBeacon()
{
    if (!schedule)
        schedule = new BYTE[2];  // eventually will use this to build schedule, and add functions to rebuild byte array (and expand/shrink as needed)

    schedule[0] = 0xff;
    schedule[1] = 0x11;


    EV << "Sending beacon\n";
    Ieee80211BeaconFrame *frame = new Ieee80211BeaconFrame("Beacon");
    Ieee80211BeaconFrameBody& body = frame->getBody();
    body.setSSID(ssid.c_str());
    body.setSupportedRates(supportedRates);
    body.setBeaconInterval(beaconInterval);
    body.setChannelNumber(channelNumber);
    body.setBodyLength(8 + 2 + 2 + (2 + ssid.length()) + (2 + supportedRates.numRates));
    frame->setAddedFields((void*)schedule);
    frame->setByteLength(28 + body.getBodyLength());
    frame->setReceiverAddress(MACAddress::BROADCAST_ADDRESS);
    frame->setFromDS(true);

    sendDown(frame);
}

void DfraMgmtAP::handleDataFrame(Ieee80211DataFrame *frame)
{
    // check toDS bit
    if (!frame->getToDS()) {
        // looks like this is not for us - discard
        EV << "Frame is not for us (toDS=false) -- discarding\n";
        delete frame;
        return;
    }

    // handle broadcast/multicast frames
    if (frame->getAddress3().isMulticast()) {
        EV << "Handling multicast frame\n";

        if (isConnectedToHL)
            sendToUpperLayer(frame->dup());

        distributeReceivedDataFrame(frame);
        return;
    }

    // look up destination address in our STA list
    auto it = staList.find(frame->getAddress3());
    if (it == staList.end()) {
        // not our STA -- pass up frame to relayUnit for LAN bridging if we have one
        if (isConnectedToHL) {
            sendToUpperLayer(frame);
        }
        else {
            EV << "Frame's destination address is not in our STA list -- dropping frame\n";
            delete frame;
        }
    }
    else {
        // dest address is our STA, but is it already associated?
        if (it->second.status == ASSOCIATED)
            distributeReceivedDataFrame(frame); // send it out to the destination STA
        else {
            EV << "Frame's destination STA is not in associated state -- dropping frame\n";
            delete frame;
        }
    }
}


//Find lowest AID which can be assigned, if already at max # of associations, indicate with -1
int DfraMgmtAP::getLowestUnusedAID()
{
    int retVal;

    if (recycledAIDs.empty())
        retVal =  nextAID++;
    else {
        std::multiset<int>::iterator it = recycledAIDs.begin();
        retVal = *it;
        recycledAIDs.erase(it);
    }
    if (retVal > MAXAID) retVal = -1;

    return retVal;
}

void DfraMgmtAP::handleAuthenticationFrame(Ieee80211AuthenticationFrame *frame)
{
    int frameAuthSeq = frame->getBody().getSequenceNumber();
    EV << "Processing Authentication frame, seqNum=" << frameAuthSeq << "\n";

    // create STA entry if needed
    STAInfo *sta = lookupSenderSTA(frame);
    if (!sta) {
        MACAddress staAddress = frame->getTransmitterAddress();
        sta = &staList[staAddress];    // this implicitly creates a new entry
        sta->address = staAddress;
        sta->status = NOT_AUTHENTICATED;
        sta->authSeqExpected = 1;
        sta->AID = getLowestUnusedAID();
    }

    // reset authentication status, when starting a new auth sequence
    // The statements below are added because the L2 handover time was greater than before when
    // a STA wants to re-connect to an AP with which it was associated before. When the STA wants to
    // associate again with the previous AP, then since the AP is already having an entry of the STA
    // because of old association, and thus it is expecting an authentication frame number 3 but it
    // receives authentication frame number 1 from STA, which will cause the AP to return an Auth-Error
    // making the MN STA to start the handover process all over again.
    if (frameAuthSeq == 1) {
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
        sta->status = NOT_AUTHENTICATED;
        sta->authSeqExpected = 1;
    }

    // check authentication sequence number is OK
    //ADDED: Check if AID is available
    if (frameAuthSeq != sta->authSeqExpected || sta->AID == -1) {
        // wrong sequence number: send error and return
        EV << "Wrong sequence number, " << sta->authSeqExpected << " expected\n";
        Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame("Auth-ERROR");
        resp->getBody().setStatusCode(SC_AUTH_OUT_OF_SEQ); //TODO: correctly deal with no AID available
        sendManagementFrame(resp, frame->getTransmitterAddress());
        delete frame;
        sta->authSeqExpected = 1;    // go back to start square
        return;
    }

    // station is authenticated if it made it through the required number of steps
    bool isLast = (frameAuthSeq + 1 == numAuthSteps);

    // send OK response (we don't model the cryptography part, just assume
    // successful authentication every time)
    EV << "Sending Authentication frame, seqNum=" << (frameAuthSeq + 1) << "\n";
    Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame(isLast ? "Auth-OK" : "Auth");
    resp->getBody().setSequenceNumber(frameAuthSeq + 1);
    resp->setAID(sta->AID);
    resp->getBody().setStatusCode(SC_SUCCESSFUL);
    resp->getBody().setIsLast(isLast);
    // XXX frame length could be increased to account for challenge text length etc.
    sendManagementFrame(resp, frame->getTransmitterAddress());

    delete frame;

    // update status
    if (isLast) {
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
        sta->status = AUTHENTICATED;    // XXX only when ACK of this frame arrives
        EV << "STA authenticated\n";
    }
    else {
        sta->authSeqExpected += 2;
        EV << "Expecting Authentication frame " << sta->authSeqExpected << "\n";
    }
}

void DfraMgmtAP::handleDeauthenticationFrame(Ieee80211DeauthenticationFrame *frame)
{
    EV << "Processing Deauthentication frame\n";

    STAInfo *sta = lookupSenderSTA(frame);
    delete frame;

    if (sta) {
        //XXX mark STA as not authenticated;!!!!alternatively, it could also be removed from staList!!!
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
//        sta->status = NOT_AUTHENTICATED;
//        sta->authSeqExpected = 1;
        recycledAIDs.insert(sta->AID);

        //delete station from list
        staList.erase(staList.find(sta->address));
    }
}

void DfraMgmtAP::handleAssociationRequestFrame(Ieee80211AssociationRequestFrame *frame)
{
    EV << "Processing AssociationRequest frame\n";

    // "11.3.2 AP association procedures"
    STAInfo *sta = lookupSenderSTA(frame);
    if (!sta || sta->status == NOT_AUTHENTICATED) {
        // STA not authenticated: send error and return
        Ieee80211DeauthenticationFrame *resp = new Ieee80211DeauthenticationFrame("Deauth");
        resp->getBody().setReasonCode(RC_NONAUTH_ASS_REQUEST);
        sendManagementFrame(resp, frame->getTransmitterAddress());
        delete frame;
        return;
    }

    delete frame;
    EV << "Processing AssociationRequest frame for station AID " << + sta->AID << "\n";
    // mark STA as associated
    if (sta->status != ASSOCIATED)
        sendAssocNotification(sta->address);
    sta->status = ASSOCIATED;    // XXX this should only take place when MAC receives the ACK for the response

    // send OK response
    Ieee80211AssociationResponseFrame *resp = new Ieee80211AssociationResponseFrame("AssocResp-OK");
    Ieee80211AssociationResponseFrameBody& body = resp->getBody();
    body.setStatusCode(SC_SUCCESSFUL);
    resp->setAID(sta->AID);
    body.setAid(sta->AID);    //XXX Added functions to create and manage AIDs
    body.setSupportedRates(supportedRates);
    sendManagementFrame(resp, sta->address);
}

void DfraMgmtAP::handleAssociationResponseFrame(Ieee80211AssociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void DfraMgmtAP::handleReassociationRequestFrame(Ieee80211ReassociationRequestFrame *frame)
{
    EV << "Processing ReassociationRequest frame\n";

    // "11.3.4 AP reassociation procedures" -- almost the same as AssociationRequest processing
    STAInfo *sta = lookupSenderSTA(frame);
    if (!sta || sta->status == NOT_AUTHENTICATED) {
        // STA not authenticated: send error and return
        Ieee80211DeauthenticationFrame *resp = new Ieee80211DeauthenticationFrame("Deauth");
        resp->getBody().setReasonCode(RC_NONAUTH_ASS_REQUEST);
        sendManagementFrame(resp, frame->getTransmitterAddress());
        delete frame;
        return;
    }

    delete frame;

    // mark STA as associated
    sta->status = ASSOCIATED;    // XXX this should only take place when MAC receives the ACK for the response

    // send OK response
    Ieee80211ReassociationResponseFrame *resp = new Ieee80211ReassociationResponseFrame("ReassocResp-OK");
    Ieee80211ReassociationResponseFrameBody& body = resp->getBody();
    body.setStatusCode(SC_SUCCESSFUL);
    body.setAid(sta->AID);    //XXX Added functions to manage AIDS
    resp->setAID(sta->AID);
    body.setSupportedRates(supportedRates);
    sendManagementFrame(resp, sta->address);
}

void DfraMgmtAP::handleReassociationResponseFrame(Ieee80211ReassociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void DfraMgmtAP::handleDisassociationFrame(Ieee80211DisassociationFrame *frame)
{
    STAInfo *sta = lookupSenderSTA(frame);
    delete frame;

    if (sta) {
        if (sta->status == ASSOCIATED)
            sendDisAssocNotification(sta->address);
        sta->status = AUTHENTICATED;
    }
}

void DfraMgmtAP::handleBeaconFrame(Ieee80211BeaconFrame *frame)
{
    dropManagementFrame(frame);
}

void DfraMgmtAP::handleProbeRequestFrame(Ieee80211ProbeRequestFrame *frame)
{
    EV << "Processing ProbeRequest frame\n";

    if (strcmp(frame->getBody().getSSID(), "") != 0 && strcmp(frame->getBody().getSSID(), ssid.c_str()) != 0) {
        EV << "SSID `" << frame->getBody().getSSID() << "' does not match, ignoring frame\n";
        dropManagementFrame(frame);
        return;
    }

    MACAddress staAddress = frame->getTransmitterAddress();
    delete frame;

    EV << "Sending ProbeResponse frame\n";
    Ieee80211ProbeResponseFrame *resp = new Ieee80211ProbeResponseFrame("ProbeResp");
    Ieee80211ProbeResponseFrameBody& body = resp->getBody();
    body.setSSID(ssid.c_str());
    body.setSupportedRates(supportedRates);
    body.setBeaconInterval(beaconInterval);
    body.setChannelNumber(channelNumber);
    sendManagementFrame(resp, staAddress);
}

void DfraMgmtAP::handleProbeResponseFrame(Ieee80211ProbeResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void DfraMgmtAP::sendAssocNotification(const MACAddress& addr)
{
    NotificationInfoSta notif;
    notif.setApAddress(myAddress);
    notif.setStaAddress(addr);
    emit(NF_L2_AP_ASSOCIATED, &notif);
}

void DfraMgmtAP::sendDisAssocNotification(const MACAddress& addr)
{
    NotificationInfoSta notif;
    notif.setApAddress(myAddress);
    notif.setStaAddress(addr);
    emit(NF_L2_AP_DISASSOCIATED, &notif);
}

void DfraMgmtAP::start()
{
    Ieee80211MgmtAPBase::start();
    scheduleAt(simTime() + uniform(0, beaconInterval), beaconTimer);
}

void DfraMgmtAP::stop()
{
    cancelEvent(beaconTimer);
    staList.clear();
    recycledAIDs.clear();
    Ieee80211MgmtAPBase::stop();
    delete schedule;
}

//} // namespace dfra

} // namespace ieee80211

} // namespace inet

