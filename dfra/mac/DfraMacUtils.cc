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
#include "IMacParameters.h"
#include "IRateSelection.h"
#include "inet/linklayer/ieee80211/oldmac/Ieee80211Consts.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211DSSSMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211HRDSSSMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211HTMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211OFDMMode.h"

namespace inet {
namespace ieee80211 {

DfraMacUtils::DfraMacUtils(IMacParameters *params, IRateSelection *rateSelection) : params(params), rateSelection(rateSelection)
{
}

simtime_t DfraMacUtils::getAckDuration() const
{
    return rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK);
}

simtime_t DfraMacUtils::getCtsDuration() const
{
    return rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS);
}

simtime_t DfraMacUtils::getAckEarlyTimeout() const
{
    // Note: This excludes ACK duration. If there's no RXStart indication within this interval, retransmission should begin immediately
    return params->getSifsTime() + params->getSlotTime() + params->getPhyRxStartDelay();
}

simtime_t DfraMacUtils::getAckFullTimeout() const
{
    return params->getSifsTime() + params->getSlotTime() + getAckDuration();
}

simtime_t DfraMacUtils::getCtsEarlyTimeout() const
{
    return params->getSifsTime() + params->getSlotTime() + params->getPhyRxStartDelay(); // see getAckEarlyTimeout()
}

simtime_t DfraMacUtils::getCtsFullTimeout() const
{
    return params->getSifsTime() + params->getSlotTime() + getCtsDuration();
}

Ieee80211RTSFrame *DfraMacUtils::buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame) const
{
    return buildRtsFrame(dataFrame, getFrameMode(dataFrame));
}

Ieee80211RTSFrame *DfraMacUtils::buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame, const IIeee80211Mode *dataFrameMode) const
{
    // protect CTS + Data + ACK
    simtime_t duration =
            3 * params->getSifsTime()
            + rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS)
            + dataFrameMode->getDuration(dataFrame->getBitLength())
            + rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK);
    return buildRtsFrame(dataFrame->getReceiverAddress(), duration);
}

Ieee80211RTSFrame *DfraMacUtils::buildRtsFrame(const MACAddress& receiverAddress, simtime_t duration) const
{
    Ieee80211RTSFrame *rtsFrame = new Ieee80211RTSFrame("RTS");
    rtsFrame->setTransmitterAddress(params->getAddress());
    rtsFrame->setReceiverAddress(receiverAddress);
    rtsFrame->setDuration(duration);
    setFrameMode(rtsFrame, rateSelection->getModeForControlFrame(rtsFrame));
    return rtsFrame;
}

Ieee80211CTSFrame *DfraMacUtils::buildCtsFrame(Ieee80211RTSFrame *rtsFrame) const
{
    Ieee80211CTSFrame *frame = new Ieee80211CTSFrame("CTS");
    frame->setReceiverAddress(rtsFrame->getTransmitterAddress());
    frame->setDuration(rtsFrame->getDuration() - params->getSifsTime() - rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS));
    setFrameMode(rtsFrame, rateSelection->getModeForControlFrame(rtsFrame));
    return frame;
}

Ieee80211ACKFrame *DfraMacUtils::buildAckFrame(Ieee80211DataOrMgmtFrame *dataFrame) const
{
    Ieee80211ACKFrame *ackFrame = new Ieee80211ACKFrame("ACK");
    ackFrame->setReceiverAddress(dataFrame->getTransmitterAddress());

    if (!dataFrame->getMoreFragments())
        ackFrame->setDuration(0);
    else
        ackFrame->setDuration(dataFrame->getDuration() - params->getSifsTime() - rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK));
    setFrameMode(ackFrame, rateSelection->getModeForControlFrame(ackFrame));
    return ackFrame;
}

Ieee80211Frame *DfraMacUtils::setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const
{
    //DT FIXME no idea why the new frame sometimes has control info, need to investigate later
    // for now just deleting it when this issue arises..
    if(frame->getControlInfo() != nullptr)
         delete frame->removeControlInfo();
    Ieee80211TransmissionRequest *ctrl = new Ieee80211TransmissionRequest();
    ctrl->setMode(mode);
    frame->setControlInfo(ctrl);
    return frame;
}

const IIeee80211Mode *DfraMacUtils::getFrameMode(Ieee80211Frame *frame) const
{
    Ieee80211TransmissionRequest *ctrl = check_and_cast<Ieee80211TransmissionRequest*>(frame->getControlInfo());
    return ctrl->getMode();
}

bool DfraMacUtils::isForUs(Ieee80211Frame *frame) const
{
    return frame->getReceiverAddress() == params->getAddress() || (frame->getReceiverAddress().isMulticast() && !isSentByUs(frame));
}

bool DfraMacUtils::isSentByUs(Ieee80211Frame *frame) const
{
    if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(frame))
        return dataOrMgmtFrame->getAddress3() == params->getAddress();
    else
        return false;
}


bool DfraMacUtils::isBroadcastOrMulticast(Ieee80211Frame *frame) const
{
    return frame && frame->getReceiverAddress().isMulticast();  // also true for broadcast frames
}

bool DfraMacUtils::isBroadcast(Ieee80211Frame *frame) const
{
    return frame && frame->getReceiverAddress().isBroadcast();
}

bool DfraMacUtils::isFragment(Ieee80211DataOrMgmtFrame *frame) const
{
    return frame->getFragmentNumber() != 0 || frame->getMoreFragments() == true;
}

bool DfraMacUtils::isCts(Ieee80211Frame *frame) const
{
    return dynamic_cast<Ieee80211CTSFrame *>(frame);
}

bool DfraMacUtils::isAck(Ieee80211Frame *frame) const
{
    return dynamic_cast<Ieee80211ACKFrame *>(frame);
}

simtime_t DfraMacUtils::getTxopLimit(AccessCategory ac, const IIeee80211Mode *mode)
{
    switch (ac)
    {
        case AC_BK: return 0;
        case AC_BE: return 0;
        case AC_VI:
            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(6.016).get();
            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(3.008).get();
            else return 0;
        case AC_VO:
            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(3.264).get();
            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(1.504).get();
            else return 0;
        case AC_LEGACY: return 0;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return 0;
}

int DfraMacUtils::getAifsNumber(AccessCategory ac)
{
    switch (ac)
    {
        case AC_BK: return 7;
        case AC_BE: return 3;
        case AC_VI: return 2;
        case AC_VO: return 2;
        case AC_LEGACY: return 2;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return -1;
}

int DfraMacUtils::getCwMax(AccessCategory ac, int aCwMax, int aCwMin)
{
    switch (ac)
    {
        case AC_BK: return aCwMax;
        case AC_BE: return aCwMax;
        case AC_VI: return aCwMin;
        case AC_VO: return (aCwMin + 1) / 2 - 1;
        case AC_LEGACY: return aCwMax;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return -1;
}

int DfraMacUtils::getCwMin(AccessCategory ac, int aCwMin)
{
    switch (ac)
    {
        case AC_BK: return aCwMin;
        case AC_BE: return aCwMin;
        case AC_VI: return (aCwMin + 1) / 2 - 1;
        case AC_VO: return (aCwMin + 1) / 4 - 1;
        case AC_LEGACY: return aCwMin;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return -1;
}


int DfraMacUtils::cmpMgmtOverData(txElem *a, txElem *b)
{
    Ieee80211DataOrMgmtFrame *aFrame = a->frame;
    Ieee80211DataOrMgmtFrame *bFrame = b->frame;
    int aPri = dynamic_cast<Ieee80211ManagementFrame*>(aFrame) ? 1 : 0;  //TODO there should really exist a high-performance isMgmtFrame() function!
    int bPri = dynamic_cast<Ieee80211ManagementFrame*>(bFrame) ? 1 : 0;
    return bPri - aPri;
}

int DfraMacUtils::cmpMgmtOverMulticastOverUnicast(txElem *a, txElem *b)
{
    Ieee80211DataOrMgmtFrame *aFrame = a->frame;
    Ieee80211DataOrMgmtFrame *bFrame = b->frame;
    int aPri = dynamic_cast<Ieee80211ManagementFrame*>(aFrame) ? 2 : aFrame->getReceiverAddress().isMulticast() ? 1 : 0;
    int bPri = dynamic_cast<Ieee80211ManagementFrame*>(bFrame) ? 2 : bFrame->getReceiverAddress().isMulticast() ? 1 : 0;
    return bPri - aPri;
}


} // namespace ieee80211
} // namespace inet
