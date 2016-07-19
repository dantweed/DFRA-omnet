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

#ifndef DFRAMACUTILS_H
#define DFRAMACUTILS_H

#include "inet/common/INETDefs.h"
#include "inet/physicallayer/ieee80211/mode/IIeee80211Mode.h"
#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/ieee80211/mac/AccessCategory.h"
#include "DfraUpperMac.h"

using namespace inet::physicallayer;

namespace inet {
namespace ieee80211 {

class Ieee80211Frame;
class Ieee80211RTSFrame;
class Ieee80211CTSFrame;
class Ieee80211ACKFrame;
class Ieee80211DataOrMgmtFrame;
class IMacParameters;
class IRateSelection;


/**
 * Frame manipulation and other utilities for use by frame exchanges and UpperMac.
 */
class INET_API DfraMacUtils
{
    private:
        IMacParameters *params;
        IRateSelection *rateSelection;
    protected:
        struct txElem : cObject {
            int retryNumber;;
            Ieee80211DataOrMgmtFrame *frame;
            txElem(int retries, Ieee80211DataOrMgmtFrame *frame_) {
                retryNumber = retries;
                frame =  frame_;
            }
        };
    public:
        DfraMacUtils(IMacParameters *params, IRateSelection *rateSelection);
        virtual ~DfraMacUtils() {};

        virtual simtime_t getAckDuration() const;
        virtual simtime_t getCtsDuration() const;
        virtual simtime_t getAckEarlyTimeout() const;  // reception of ACK should begin within this timeout period
        virtual simtime_t getAckFullTimeout() const;  // ACK should be fully received within this timeout period
        virtual simtime_t getCtsEarlyTimeout() const;
        virtual simtime_t getCtsFullTimeout() const;

        virtual Ieee80211RTSFrame *buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame) const;
        virtual Ieee80211RTSFrame *buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame, const IIeee80211Mode *dataFrameMode) const;
        virtual Ieee80211RTSFrame *buildRtsFrame(const MACAddress& receiverAddress, simtime_t duration) const;
        virtual Ieee80211CTSFrame *buildCtsFrame(Ieee80211RTSFrame *rtsFrame) const;
        virtual Ieee80211ACKFrame *buildAckFrame(Ieee80211DataOrMgmtFrame *dataFrame) const;

        virtual Ieee80211Frame *setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const;
        virtual const IIeee80211Mode *getFrameMode(Ieee80211Frame *frame) const;

        virtual bool isForUs(Ieee80211Frame *frame) const;
        virtual bool isSentByUs(Ieee80211Frame *frame) const;
        virtual bool isBroadcastOrMulticast(Ieee80211Frame *frame) const;
        virtual bool isBroadcast(Ieee80211Frame *frame) const;
        virtual bool isFragment(Ieee80211DataOrMgmtFrame *frame) const;
        virtual bool isCts(Ieee80211Frame *frame) const;
        virtual bool isAck(Ieee80211Frame *frame) const;

        static simtime_t getTxopLimit(AccessCategory ac, const IIeee80211Mode *mode);
        static int getAifsNumber(AccessCategory ac);
        static int getCwMax(AccessCategory ac, int aCwMax, int aCwMin);
        static int getCwMin(AccessCategory ac, int aCwMin);

        static int cmpMgmtOverData(txElem *a, txElem *b);
        static int cmpMgmtOverMulticastOverUnicast(txElem *a, txElem *b);
};

} // namespace ieee80211
} // namespace inet

#endif