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

#ifndef __DFRAUPPERMAC_H
#define __DFRAUPPERMAC_H

#include "IUpperMac.h"
#include "IFrameExchange.h"
#include "AccessCategory.h"
#include "inet/physicallayer/ieee80211/mode/IIeee80211Mode.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

using namespace inet::physicallayer;

namespace inet {
namespace ieee80211 {

class IRx;
class IContentionCallback;
class ITxCallback;
class Ieee80211Mac;
class Ieee80211RTSFrame;
class IMacQoSClassifier;
class IMacParameters;
class DfraMacUtils;
class ITx;
class IContention;
class IDuplicateDetector;
class IFragmenter;
class IReassembly;
class IRateSelection;
class IRateControl;
class IStatistics;

class DfraMac;
class DfraContention;


class INET_API DfraUpperMac : public cSimpleModule, public IUpperMac, protected IFrameExchange::IFinishedCallback
{
    public:
        typedef std::list<Ieee80211DataOrMgmtFrame*> Ieee80211DataOrMgmtFrameList;

    protected:
        IMacParameters *params = nullptr;
        DfraMacUtils *utils = nullptr;
        DfraMac *mac = nullptr;
        IRx *rx = nullptr;
        ITx *tx = nullptr;
        IContention **contention = nullptr;

        int maxQueueSize;
        int fragmentationThreshold = 2346;

        cQueue transmissionQueue;
        IFrameExchange *frameExchange = nullptr;
        IDuplicateDetector *duplicateDetection = nullptr;
        IFragmenter *fragmenter = nullptr;
        IReassembly *reassembly = nullptr;
        IRateSelection *rateSelection = nullptr;
        IRateControl *rateControl = nullptr;
        IStatistics *statistics = nullptr;

        //For scheduling info //DT
        using BYTE = uint8;
        struct SchedulingInfo{
                int aid;
                BYTE frameTypes;
                BYTE mysched;
                simtime_t beaconReference;
                simtime_t drbLength;
                int numDRBs;
                SchedulingInfo(){}
                ~SchedulingInfo(){}
        };
        SchedulingInfo *mySchedule;

        int currDRBnum = 0;

        struct txElem : cObject {
            int retryNumber;;
            Ieee80211DataOrMgmtFrame *frame = nullptr;
            txElem(int retries, Ieee80211DataOrMgmtFrame *frame_) {
                retryNumber = retries;
                frame =  frame_;
            }
            ~txElem() {if (frame) delete frame; }
        };
    protected:
        void initialize() override;
        virtual IMacParameters *extractParameters(const IIeee80211Mode *slowestMandatoryMode);
        void handleMessage(cMessage *msg) override;

        virtual void enqueue(Ieee80211DataOrMgmtFrame *frame);
        virtual void startSendDataFrameExchange(Ieee80211DataOrMgmtFrame *frame, int txIndex, AccessCategory ac);
        virtual void frameExchangeFinished(IFrameExchange *what, bool successful) override;

        void sendAck(Ieee80211DataOrMgmtFrame *frame);
        void sendCts(Ieee80211RTSFrame *frame);

    public:
        DfraUpperMac();
        virtual ~DfraUpperMac();
        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void upperFrameReceived(cPacket *msg);
        virtual void lowerFrameReceived(Ieee80211Frame *frame) override;
        virtual void corruptedFrameReceived() override;
        virtual void channelAccessGranted(IContentionCallback *callback, int txIndex) override;
        virtual void internalCollision(IContentionCallback *callback, int txIndex) override;
        virtual void transmissionComplete(ITxCallback *callback) override;
        void scheduleUpdate(cMessage *msg);
};

} // namespace ieee80211
} // namespace inet

#endif

