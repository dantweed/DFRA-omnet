#ifndef __DFRAMGMTFRAMES_H
#define __DFRAMGMTFRAMES_H

#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtFrames_m.h"
#include <cstdint>


namespace inet {
namespace ieee80211 {

class INET_API DFRABeaconFrameBody : public ::inet::ieee80211::Ieee80211BeaconFrameBody
{
  private:
    void copy(const DFRABeaconFrameBody& other);

  protected:
    uint32_t sched;
  public:
    DFRABeaconFrameBody();
    DFRABeaconFrameBody(const DFRABeaconFrameBody& other);
    virtual ~DFRABeaconFrameBody();
    virtual void parsimPack(omnetpp::cCommBuffer *b) const;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b);
    virtual uint32_t getSchedule();
    virtual void setSchedule(uint32_t sched);

};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const DFRABeaconFrameBody& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, DFRABeaconFrameBody& obj) {obj.parsimUnpack(b);}

class INET_API DFRABeaconFrame : public ::inet::ieee80211::Ieee80211BeaconFrame
{
  protected:
    DFRABeaconFrameBody body;

  private:
    void copy(const DFRABeaconFrame& other);

  public:
    DFRABeaconFrame(const char *name=nullptr, int kind=0);
    DFRABeaconFrame(const DFRABeaconFrame& other);
    virtual ~DFRABeaconFrame();

    // field getter/setter methods
    virtual DFRABeaconFrameBody& getBody();
    virtual const DFRABeaconFrameBody& getBody() const {return const_cast<DFRABeaconFrame*>(this)->getBody();}
    virtual void setBody(const DFRABeaconFrameBody& body);
};


} //namespace ieee80211

} //namespace inet
#endif
