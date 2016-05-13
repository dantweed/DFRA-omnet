#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtFrames_m.h"
#include "DFRAMgmtFrames.h"
#include <cstdint>

namespace inet {
namespace ieee80211 {

Register_Class(DFRABeaconFrameBody);

DFRABeaconFrameBody::DFRABeaconFrameBody() : ::inet::ieee80211::Ieee80211BeaconFrameBody()
{
    
    this->setBodyLength(bodyLength = 8 + 2 + 2 + (2 + sizeof(SSID)) + (2 + supportedRates.numRates) + 32);
    this->sched = 0;
}

DFRABeaconFrameBody::~DFRABeaconFrameBody()
{
}

void DFRABeaconFrameBody::copy(const DFRABeaconFrameBody& other)
{
    this->SSID = other.SSID;
    this->supportedRates = other.supportedRates;
    this->beaconInterval = other.beaconInterval;
    this->channelNumber = other.channelNumber;
    this->handoverParameters = other.handoverParameters;
    this->sched = other.sched;
}

void DFRABeaconFrameBody::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::ieee80211::Ieee80211FrameBody::parsimPack(b);
    doParsimPacking(b,this->SSID);
    doParsimPacking(b,this->supportedRates);
    doParsimPacking(b,this->beaconInterval);
    doParsimPacking(b,this->channelNumber);
    doParsimPacking(b,this->handoverParameters);
    doParsimPacking(b,this->sched);

}

void DFRABeaconFrameBody::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::ieee80211::Ieee80211FrameBody::parsimUnpack(b);
    doParsimUnpacking(b,this->SSID);
    doParsimUnpacking(b,this->supportedRates);
    doParsimUnpacking(b,this->beaconInterval);
    doParsimUnpacking(b,this->channelNumber);
    doParsimUnpacking(b,this->handoverParameters);
    doParsimUnpacking(b,this->sched);
}

uint32_t DFRABeaconFrameBody::getSchedule()
{
    return this->sched;
}

void DFRABeaconFrameBody::setSchedule(uint32_t sched)
{
    this->sched = sched;
}

Register_Class(DFRABeaconFrame);

DFRABeaconFrame::DFRABeaconFrame(const char *name, int kind) : ::inet::ieee80211::Ieee80211BeaconFrame(name,kind)
{
    this->setType(ST_BEACON);
    this->setByteLength(28+this->body.getBodyLength());

}

DFRABeaconFrame::DFRABeaconFrame(const DFRABeaconFrame& other) : ::inet::ieee80211::Ieee80211BeaconFrame(other)
{
    copy(other);
}

void DFRABeaconFrame::copy(const DFRABeaconFrame& other)
{
    this->body = other.body;
}

DFRABeaconFrame::~DFRABeaconFrame()
{
}
DFRABeaconFrameBody& DFRABeaconFrame::getBody()
{
    return this->body;
}

void DFRABeaconFrame::setBody(const DFRABeaconFrameBody& body)
{
    this->body = body;
}

} //namespace ieee80211  
} //namespace inet
