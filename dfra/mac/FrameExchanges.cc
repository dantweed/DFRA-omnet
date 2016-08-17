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

#include "dfra/mac/FrameExchanges.h"
#include "inet/common/INETUtils.h"
#include "inet/common/FSMA.h"
#include "inet/common/NotifierConsts.h"
#include "IContention.h"
#include "ITx.h"
#include "IRx.h"
#include "IMacParameters.h"
#include "IStatistics.h"
#include <DfraMacUtils.h>
#include "Ieee80211Frame_m.h"

using namespace inet::utils;

namespace inet {
namespace ieee80211 {

//------------------------------

SendDataWithAckFrameExchange::SendDataWithAckFrameExchange(FrameExchangeContext *context, IFinishedCallback *callback, Ieee80211DataOrMgmtFrame *dataFrame, int txIndex, AccessCategory accessCategory) :
    StepBasedFrameExchange(context, callback, txIndex, accessCategory), dataFrame(dataFrame)
{
    dataFrame->setDuration(params->getSifsTime() + utils->getAckDuration());
}

SendDataWithAckFrameExchange::~SendDataWithAckFrameExchange()
{
   //delete dataFrame; //DT moved deletions to other layer
}

std::string SendDataWithAckFrameExchange::info() const
{
    std::string ret = StepBasedFrameExchange::info();
    if (dataFrame) {
        ret += ", frame=";
        ret += dataFrame->getName();
    }
    return ret;
}

void SendDataWithAckFrameExchange::doStep(int step)
{

    switch (step) {
        case 0: startContentionIfNeeded(retryCount);
            break;
        case 1: transmitFrame(dupPacketAndControlInfo(dataFrame));
            break;
        case 2: {
            if (params->getUseFullAckTimeout())
                expectFullReplyWithin(utils->getAckFullTimeout());
            else
                expectReplyRxStartWithin(utils->getAckEarlyTimeout());
            break;
        }
        case 3: statistics->frameTransmissionSuccessful(dataFrame, retryCount); releaseChannel(); succeed(); break;
        default: ASSERT(false);
    }
}

IFrameExchange::FrameProcessingResult SendDataWithAckFrameExchange::processReply(int step, Ieee80211Frame *frame)
{
    switch (step) {
        case 2: if (utils->isAck(frame)) return PROCESSED_DISCARD; else return IGNORED;
        default: ASSERT(false); return IGNORED;
    }
}

void SendDataWithAckFrameExchange::processTimeout(int step)
{
    switch (step) {
        case 2: retry(); break;
        default: ASSERT(false);
    }
}

void SendDataWithAckFrameExchange::processInternalCollision(int step)
{
    switch (step) {
        case 0: retry(); break;
        default: ASSERT(false);
    }
}

void SendDataWithAckFrameExchange::retry()//DT retry means fail!
{
    releaseChannel();
    statistics->frameTransmissionUnsuccessfulGivingUp(dataFrame, retryCount);
    fail();
    ownerModule->emit(NF_LINK_BREAK, dataFrame);
}

//------------------------------

SendDataWithRtsCtsFrameExchange::SendDataWithRtsCtsFrameExchange(FrameExchangeContext *context, IFinishedCallback *callback, Ieee80211DataOrMgmtFrame *dataFrame, int txIndex, AccessCategory accessCategory) :
    StepBasedFrameExchange(context, callback, txIndex, accessCategory), dataFrame(dataFrame)
{
    dataFrame->setDuration(params->getSifsTime() + utils->getAckDuration());
}

SendDataWithRtsCtsFrameExchange::~SendDataWithRtsCtsFrameExchange()
{
    //delete dataFrame; //DT moved deletions to other layer
}

std::string SendDataWithRtsCtsFrameExchange::info() const
{
    std::string ret = StepBasedFrameExchange::info();
    if (dataFrame) {
        ret += ", frame=";
        ret += dataFrame->getName();
    }
    return ret;
}

void SendDataWithRtsCtsFrameExchange::doStep(int step)
{
    switch (step) {
        case 0: startContentionIfNeeded(shortRetryCount); break;
        case 1: transmitFrame(utils->buildRtsFrame(dataFrame)); break;
        case 2: expectReplyRxStartWithin(utils->getCtsEarlyTimeout()); break;
        case 3: transmitFrame(dupPacketAndControlInfo(dataFrame), params->getSifsTime()); break;
        case 4: expectReplyRxStartWithin(utils->getAckEarlyTimeout()); break;
        case 5: statistics->frameTransmissionSuccessful(dataFrame, longRetryCount); releaseChannel(); succeed(); break;
        default: ASSERT(false);
    }
}

IFrameExchange::FrameProcessingResult SendDataWithRtsCtsFrameExchange::processReply(int step, Ieee80211Frame *frame)
{
    switch (step) {
        case 2: if (utils->isCts(frame)) return PROCESSED_DISCARD; else return IGNORED;
        case 4: if (utils->isAck(frame)) return PROCESSED_DISCARD; else return IGNORED;
        default: ASSERT(false); return IGNORED;
    }
}

void SendDataWithRtsCtsFrameExchange::processTimeout(int step)
{
    switch (step) {
        case 2: retryRtsCts(); break;
        case 4: retryData(); break;
        default: ASSERT(false);
    }
}

void SendDataWithRtsCtsFrameExchange::processInternalCollision(int step)
{
    switch (step) {
        case 0: retryRtsCts(); break;
        default: ASSERT(false);
    }
}

void SendDataWithRtsCtsFrameExchange::retryRtsCts()
{
    releaseChannel();
    statistics->frameTransmissionGivenUp(dataFrame);
    fail();
}

void SendDataWithRtsCtsFrameExchange::retryData()
{
    releaseChannel();
    statistics->frameTransmissionUnsuccessfulGivingUp(dataFrame, longRetryCount);
    fail();
}

//------------------------------

SendMulticastDataFrameExchange::SendMulticastDataFrameExchange(FrameExchangeContext *context, IFinishedCallback *callback, Ieee80211DataOrMgmtFrame *dataFrame, int txIndex, AccessCategory accessCategory) :
    FrameExchange(context, callback), dataFrame(dataFrame), txIndex(txIndex), accessCategory(accessCategory)
{
    ASSERT(utils->isBroadcastOrMulticast(dataFrame));
    dataFrame->setDuration(0);
}

SendMulticastDataFrameExchange::~SendMulticastDataFrameExchange()
{
    //delete dataFrame; //DT moved deletions to other layer
}

std::string SendMulticastDataFrameExchange::info() const
{
    return dataFrame ? std::string("frame=") + dataFrame->getName() : "";
}

void SendMulticastDataFrameExchange::handleSelfMessage(cMessage *msg)
{
    ASSERT(false);
}

void SendMulticastDataFrameExchange::start()
{
    startContention();
}

void SendMulticastDataFrameExchange::startContention()
{
    AccessCategory ac = accessCategory;  // abbreviate
    contention[txIndex]->startContention(params->getAifsTime(ac), params->getEifsTime(ac), params->getCwMulticast(ac), params->getCwMulticast(ac), params->getSlotTime(), 0, this);
}

void SendMulticastDataFrameExchange::internalCollision(int txIndex)
{
    reportFailure();
}

void SendMulticastDataFrameExchange::channelAccessGranted(int txIndex)
{
    tx->transmitFrame(dupPacketAndControlInfo(dataFrame), SIMTIME_ZERO, this);
}

void SendMulticastDataFrameExchange::transmissionComplete()
{
    releaseChannel(txIndex);
    reportSuccess();
}


} // namespace ieee80211
} // namespace inet

