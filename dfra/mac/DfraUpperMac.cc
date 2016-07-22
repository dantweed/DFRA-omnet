//
// Copyright (C) 2015 Andras Varga
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
// Author: Andras Varga
//

#include <DfraMacUtils.h>
#include "MacUtils.h"
#include "DfraUpperMac.h"
#include "DfraMac.h"
#include "Ieee80211Mac.h"
#include "IRx.h"
#include "IContention.h"
#include "ITx.h"
#include "MacParameters.h"
#include "dfra/mac/FrameExchanges.h"
#include "DuplicateDetectors.h"
#include "IFragmentation.h"
#include "IRateSelection.h"
#include "IRateControl.h"
#include "IStatistics.h"
#include "inet/common/INETUtils.h"
#include "inet/common/queue/IPassiveQueue.h"
#include "inet/common/ModuleAccess.h"

#include "inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h"

namespace inet {
namespace ieee80211 {

#define MSG_CHANGE_SCHED 99
#define ST_FRAME_EXCHANGE 1

#define MAX_BO 11

Define_Module(DfraUpperMac);

DfraUpperMac::DfraUpperMac()
{
}

DfraUpperMac::~DfraUpperMac()
{
    delete frameExchange;
    delete duplicateDetection;
    delete fragmenter;
    delete reassembly;
    delete params;
    delete utils;
    delete [] contention;
    //might or might not need to deal with deleting the queue and/or elements ...
}

void DfraUpperMac::initialize()
{
    mac = check_and_cast<DfraMac *>(getModuleByPath(par("macModule"))); //TODO: ?
    rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
    tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
    contention = nullptr;
    collectContentionModules(getModuleByPath(par("firstContentionModule")), contention);

    maxQueueSize = par("maxQueueSize"); //14 per parameters currently
    transmissionQueue.setName("txQueue");
    transmissionQueue.setup(par("prioritizeMulticast") ? (CompareFunc)DfraMacUtils::cmpMgmtOverMulticastOverUnicast : (CompareFunc)DfraMacUtils::cmpMgmtOverData);

    rateSelection = check_and_cast<IRateSelection*>(getModuleByPath(par("rateSelectionModule")));
    rateControl = dynamic_cast<IRateControl*>(getModuleByPath(par("rateControlModule"))); // optional module
    rateSelection->setRateControl(rateControl);

    params = extractParameters(rateSelection->getSlowestMandatoryMode());
    utils = new DfraMacUtils(params, rateSelection);
    rx->setAddress(params->getAddress());

    statistics = check_and_cast<IStatistics*>(getModuleByPath(par("statisticsModule")));
    statistics->setMacUtils((MacUtils*)utils);
    statistics->setRateControl(rateControl);

    duplicateDetection = new NonQoSDuplicateDetector(); //TODO or LegacyDuplicateDetector();
    fragmenter = check_and_cast<IFragmenter *>(inet::utils::createOne(par("fragmenterClass")));
    reassembly = check_and_cast<IReassembly *>(inet::utils::createOne(par("reassemblyClass")));

    WATCH(maxQueueSize);
    WATCH(fragmentationThreshold);
}

inline double fallback(double a, double b) {return a!=-1 ? a : b;}
inline simtime_t fallback(simtime_t a, simtime_t b) {return a!=-1 ? a : b;}

IMacParameters *DfraUpperMac::extractParameters(const IIeee80211Mode *slowestMandatoryMode)
{
    const IIeee80211Mode *referenceMode = slowestMandatoryMode;  // or any other; slotTime etc must be the same for all modes we use

    MacParameters *params = new MacParameters();
    params->setAddress(mac->getAddress());
    params->setShortRetryLimit(fallback(par("shortRetryLimit"), 7));
    params->setLongRetryLimit(fallback(par("longRetryLimit"), 4));
    params->setRtsThreshold(par("rtsThreshold"));
    params->setPhyRxStartDelay(referenceMode->getPhyRxStartDelay());
    params->setUseFullAckTimeout(par("useFullAckTimeout"));
    params->setEdcaEnabled(false);
    params->setSlotTime(fallback(par("slotTime"), referenceMode->getSlotTime()));
    params->setSifsTime(fallback(par("sifsTime"), referenceMode->getSifsTime()));
    int aCwMin = referenceMode->getLegacyCwMin();
    int aCwMax = referenceMode->getLegacyCwMax();

    //DT: A/DIFS only used for gaurd interval wtih slotted scheduling
    /*params->setAifsTime(AC_LEGACY, fallback(par("difsTime"), referenceMode->getSifsTime() + DfraMacUtils::getAifsNumber(AC_LEGACY) * params->getSlotTime()));
    params->setEifsTime(AC_LEGACY, params->getSifsTime() + params->getAifsTime(AC_LEGACY) + slowestMandatoryMode->getDuration(LENGTH_ACK));
    params->setCwMin(AC_LEGACY, fallback(par("cwMin"), DfraMacUtils::getCwMin(AC_LEGACY, aCwMin)));
    params->setCwMax(AC_LEGACY, fallback(par("cwMax"), DfraMacUtils::getCwMax(AC_LEGACY, aCwMax, aCwMin)));
    params->setCwMulticast(AC_LEGACY, fallback(par("cwMulticast"), DfraMacUtils::getCwMin(AC_LEGACY, aCwMin)));*/

    //Changes for DFRA
    params->setAifsTime(AC_LEGACY, SimTime(50,SIMTIME_US));  //FIXME: should this be 25 us .... ?
    params->setEifsTime(AC_LEGACY, params->getAifsTime(AC_LEGACY));   //We're not concerned with EIFS at this point may want to add back in at a later time
    params->setCwMin(AC_LEGACY, 1); //Default to 1, will be dynamically set later
    //Redundant, I know, but I don't want to change the interface
    params->setCwMax(AC_LEGACY,params->getCwMin(AC_LEGACY));
    params->setCwMulticast(AC_LEGACY,params->getCwMin(AC_LEGACY));

    return params;
}

void DfraUpperMac::handleMessage(cMessage *msg)
{
    if (msg->getKind() == ST_FRAME_EXCHANGE) {//DRB scheduling control
        frameExchange->start();
        delete msg;
    }
    else if (msg->getContextPointer() != nullptr)
        ((MacPlugin *)msg->getContextPointer())->handleSelfMessage(msg);
    else
        ASSERT(false);
}

void DfraUpperMac::scheduleUpdate(cMessage *msg)
{
    if (msg->getKind() == MSG_CHANGE_SCHED) {
        mySchedule = (SchedulingInfo*)msg->getContextPointer();
        delete msg;
    }
    else
        ASSERT(false);
}

void DfraUpperMac:: upperFrameReceived(cPacket *msg)
{
    Ieee80211DataOrMgmtFrame *frame = check_and_cast<Ieee80211DataOrMgmtFrame *>(msg);
    Enter_Method("upperFrameReceived(\"%s\")", frame->getName());
        take(frame);

        EV_INFO << "Frame " << frame << " received from higher layer, receiver = " << frame->getReceiverAddress() << endl;

        if (maxQueueSize > 0 && transmissionQueue.length() >= maxQueueSize && dynamic_cast<Ieee80211DataFrame *>(frame)) {
            EV << "Dataframe " << frame << " received from higher layer but MAC queue is full, dropping\n";
            delete frame;
            return;
        }

        ASSERT(!frame->getReceiverAddress().isUnspecified());
        frame->setTransmitterAddress(params->getAddress());
        duplicateDetection->assignSequenceNumber(frame);

        if (frame->getByteLength() <= fragmentationThreshold)
            enqueue(frame);
        else {
            auto fragments = fragmenter->fragment(frame, fragmentationThreshold);
            for (Ieee80211DataOrMgmtFrame *fragment : fragments)
                enqueue(fragment);
        }
}

void DfraUpperMac::upperFrameReceived(Ieee80211DataOrMgmtFrame *frame)
{
    Enter_Method("upperFrameReceived(\"%s\")", frame->getName());
    take(frame);

    EV_INFO << "Frame " << frame << " received from higher layer, receiver = " << frame->getReceiverAddress() << endl;

    if (maxQueueSize > 0 && transmissionQueue.length() >= maxQueueSize && dynamic_cast<Ieee80211DataFrame *>(frame)) {
        EV << "Dataframe " << frame << " received from higher layer but MAC queue is full, dropping\n";
        delete frame;
        return;
    }

    ASSERT(!frame->getReceiverAddress().isUnspecified());
    frame->setTransmitterAddress(params->getAddress());
    duplicateDetection->assignSequenceNumber(frame);

    if (frame->getByteLength() <= fragmentationThreshold)
        enqueue(frame);
    else {
        auto fragments = fragmenter->fragment(frame, fragmentationThreshold);
        for (Ieee80211DataOrMgmtFrame *fragment : fragments)
            enqueue(fragment);
    }
}

void DfraUpperMac::enqueue(Ieee80211DataOrMgmtFrame *frame)
{
    //DT: Changed so that all frames are put in the queue, then removed later on frame exchange finished
    txElem *elem = new txElem(0,frame);
    transmissionQueue.insert(elem);
    if (!frameExchange)
        startSendDataFrameExchange(elem->frame, 0, AC_LEGACY);
}

//DT: need to work on this to have DFRA conformity on association reqs/responses
void DfraUpperMac::lowerFrameReceived(Ieee80211Frame *frame)
{
    Enter_Method("lowerFrameReceived(\"%s\")", frame->getName());
    delete frame->removeControlInfo();          //TODO
    take(frame);

    if (!utils->isForUs(frame)) {
        EV_INFO << "This frame is not for us" << std::endl;
        delete frame;
        if (frameExchange)
            frameExchange->corruptedOrNotForUsFrameReceived();
    }
    else {
        // offer frame to ongoing frame exchange
        IFrameExchange::FrameProcessingResult result = frameExchange ? frameExchange->lowerFrameReceived(frame) : IFrameExchange::IGNORED;
        bool processed = (result != IFrameExchange::IGNORED);
        if (processed) {
            // already processed, nothing more to do
            if (result == IFrameExchange::PROCESSED_DISCARD)
                delete frame;
        }
        else if (Ieee80211RTSFrame *rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(frame)) {
            sendCts(rtsFrame);
            delete rtsFrame;
        }
        else if (Ieee80211DataOrMgmtFrame *dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(frame)) {
            if (!utils->isBroadcastOrMulticast(frame))
                sendAck(dataOrMgmtFrame);
            if (duplicateDetection->isDuplicate(dataOrMgmtFrame)) {
                EV_INFO << "Duplicate frame " << frame->getName() << ", dropping\n";
                delete dataOrMgmtFrame;
            }
            else {
                if (!utils->isFragment(dataOrMgmtFrame))
                    mac->sendUp(dataOrMgmtFrame);
                else {
                    Ieee80211DataOrMgmtFrame *completeFrame = reassembly->addFragment(dataOrMgmtFrame);
                    if (completeFrame)
                        mac->sendUp(completeFrame);
                }
            }
        }
        else {
            EV_INFO << "Unexpected frame " << frame->getName() << ", dropping\n";
            delete frame;
        }
    }
}

void DfraUpperMac::corruptedFrameReceived()
{
    if (frameExchange)
        frameExchange->corruptedOrNotForUsFrameReceived();
}

void DfraUpperMac::channelAccessGranted(IContentionCallback *callback, int txIndex)
{
    Enter_Method("channelAccessGranted()");
    callback->channelAccessGranted(txIndex);
}

void DfraUpperMac::internalCollision(IContentionCallback *callback, int txIndex)
{
    Enter_Method("internalCollision()");
    if (callback)
        callback->internalCollision(txIndex); //DT - no collision controller ..
}

void DfraUpperMac::transmissionComplete(ITxCallback *callback)
{
    Enter_Method("transmissionComplete()");
    if (callback)
        callback->transmissionComplete();
}


//FIXME: Factor this function, this is ridiculously convoluted and poorly structured
void DfraUpperMac::startSendDataFrameExchange(Ieee80211DataOrMgmtFrame *frame, int txIndex, AccessCategory ac)
{
    ASSERT(!frameExchange);
    bool broadOrMulticast = utils->isBroadcastOrMulticast(frame);
    simtime_t nextTxTime;

    int currDRBnum = floor((simTime() - mySchedule->beaconReference)/mySchedule->drbLength);
    uint8 nextTxDRB;
    int backoff;
    BYTE drbSched = 0;

    //Check for missed beacon, if so so continue on with existing schedule
    //FIXME: better missed beacon behavior

    if (currDRBnum > mySchedule->numDRBs) {
        simtime_t beaconInterval = ((int)mySchedule->numDRBs)*mySchedule->drbLength;
        while (simTime() > (mySchedule->beaconReference + beaconInterval) ){
             mySchedule->beaconReference += beaconInterval;
        }
        currDRBnum = floor((simTime() - mySchedule->beaconReference)/mySchedule->drbLength);
    }

    nextTxDRB = currDRBnum;
    while (drbSched == 0 && nextTxDRB <  mySchedule->numDRBs) {
        if (nextTxDRB % 2 ==0)
            drbSched = (mySchedule->mysched[nextTxDRB/2] & 0xf0) >> 4;
        else
            drbSched = (mySchedule->mysched[(nextTxDRB-1)/2] & 0x0f);
        if (!drbSched) nextTxDRB++;
    }

    if (drbSched != 0 && nextTxDRB <  mySchedule->numDRBs){ //Otherwise, wait until next beacon

        BYTE frameType = ((mySchedule->frameTypes[mySchedule->aid -1] >> nextTxDRB) % 0x01);
        if (frameType == 0) {
            backoff = drbSched;
        }else{
            if (drbSched <= 0xB)
                backoff = rand() % (MAX_BO-drbSched) + drbSched ;
            else if (drbSched == 0xC)
                backoff = rand() % (int)ceil(MAX_BO/2)+1;
            else if (drbSched == 0xF)
                backoff = MAX_BO  - (rand() % (int)ceil(MAX_BO/2)) ;
            else
                ASSERT(false);
        }
        ASSERT(backoff < MAX_BO);
        nextTxTime = mySchedule->beaconReference + ((int)nextTxDRB)*mySchedule->drbLength;


        //We're ahead and sometime has gone wrong so fail dramatically
        if (simTime() > nextTxTime || (mySchedule->beaconReference + ((int)mySchedule->numDRBs)*mySchedule->drbLength) < nextTxTime)
            ASSERT(false);

        //Do I really want to use params?? Need to set ifs min to be something like 25us for guard interval, but
        ((MacParameters *)params)->setCwMin(AC_LEGACY, backoff);
        ((MacParameters *)params)->setCwMulticast(AC_LEGACY, backoff);

        //Set up actual frame exchance
        if (broadOrMulticast)
            utils->setFrameMode(frame, rateSelection->getModeForMulticastDataOrMgmtFrame(frame));
        else
            utils->setFrameMode(frame, rateSelection->getModeForUnicastDataOrMgmtFrame(frame));

        FrameExchangeContext context;
        context.ownerModule = this;
        context.params = params;
        context.utils = utils;
        context.contention = contention;
        context.tx = tx;
        context.rx = rx;
        context.statistics = statistics;

        bool useRtsCts = frame->getByteLength() > params->getRtsThreshold();

        if (broadOrMulticast) {
            frameExchange = new SendMulticastDataFrameExchange(&context, this, frame, txIndex, ac);
            frameExchange->start(); //FIXME: temporary fix for beacons so reference matches send time, need to correct this
        }
        else {
            if (useRtsCts)
                frameExchange = new SendDataWithRtsCtsFrameExchange(&context, this, frame, txIndex, ac); //DT: not currently used
            else
                frameExchange = new SendDataWithAckFrameExchange(&context, this, frame, txIndex, ac);

            scheduleAt(nextTxTime, new cMessage("startFrameExchange", ST_FRAME_EXCHANGE));
        }
    }
}

//DT: Edited to use queue to store for retransmission, now need to track retry attempts (may need to use different object in the queue
void DfraUpperMac::frameExchangeFinished(IFrameExchange *what, bool successful)
{//DT: TODO: rewrite this so it is more logical (!successful first, to eliminate duplicate lines of code for checking if tx queue is empty
    EV_INFO << "Frame exchange finished" << std::endl;

    txElem *elem = nullptr;
    if (successful) {
        elem = check_and_cast<txElem *>(transmissionQueue.pop());
        delete elem;
        elem = nullptr;
        if (!transmissionQueue.isEmpty())
            elem = check_and_cast<txElem *>(transmissionQueue.front());
    }
    else {
        elem = check_and_cast<txElem *>(transmissionQueue.front());
        if (++elem->retryNumber > params->getShortRetryLimit()) { //Drop after retry limit (not RTS/CTS supported yet)
            elem = check_and_cast<txElem *>(transmissionQueue.pop());
            delete elem;
            elem = nullptr;
            if (!transmissionQueue.isEmpty())
                elem = check_and_cast<txElem *>(transmissionQueue.front());
        } else {
            Ieee80211DataOrMgmtFrame *frame = new Ieee80211DataOrMgmtFrame(*elem->frame);
            delete (elem->frame);
            elem->frame = frame;
        }
    }
    delete frameExchange;
    frameExchange = nullptr;
    if (elem)
        startSendDataFrameExchange(elem->frame, 0, AC_LEGACY);
}

void DfraUpperMac::sendAck(Ieee80211DataOrMgmtFrame *frame)
{
    Ieee80211ACKFrame *ackFrame = utils->buildAckFrame(frame);
    tx->transmitFrame(ackFrame, params->getSifsTime(), nullptr);
}

void DfraUpperMac::sendCts(Ieee80211RTSFrame *frame)
{
    Ieee80211CTSFrame *ctsFrame = utils->buildCtsFrame(frame);
    tx->transmitFrame(ctsFrame, params->getSifsTime(), nullptr);
}

} // namespace ieee80211
} // namespace inet

