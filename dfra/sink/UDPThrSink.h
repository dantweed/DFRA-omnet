#ifndef __UDPTHRSINK_H
#define __UDPTHRSINK_H

#include "inet/common/INETDefs.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include <map>


namespace inet {

/**
 * Consumes and prints packets received from the UDP module. See NED for more info.
 */
class INET_API UDPThrSink : public ApplicationBase
{
  protected:
    enum SelfMsgKinds { START = 1, STOP };

    UDPSocket socket;
    int localPort = -1;
    L3Address multicastGroup;
    simtime_t startTime;
    simtime_t stopTime;
    cMessage *selfMsg = nullptr;

    int numReceived = 0;
    std::map<L3Address,uint64_t> stats;
    static simsignal_t rcvdPkSignal;

  public:
    UDPThrSink() {}
    virtual ~UDPThrSink();

  protected:
    virtual void processPacket(cPacket *msg);
    virtual void setSocketOptions();

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;

    virtual void processStart();
    virtual void processStop();

    virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
    virtual bool handleNodeShutdown(IDoneCallback *doneCallback) override;
    virtual void handleNodeCrash() override;
};

} // namespace inet

#endif // ifndef __UDPTHRSINK_H

