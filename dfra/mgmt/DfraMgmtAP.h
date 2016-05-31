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

#ifndef __DFRAMGMTAP_H
#define __DFRAMGMTAP_H

#include <map>
#include <vector>

#include "inet/common/INETDefs.h"

#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAPBase.h"

namespace inet {

namespace ieee80211 {

//namespace dfra {

/**
 * Used in 802.11 infrastructure mode: handles management frames for
 * an access point (AP). See corresponding NED file for a detailed description.
 *
 * @author Andras Varga
 */
class INET_API DfraMgmtAP : public Ieee80211MgmtAPBase, protected cListener
{
  public:
    /** State of a STA */
    enum STAStatus { NOT_AUTHENTICATED, AUTHENTICATED, ASSOCIATED };

    /** Describes a STA */
    struct STAInfo
    {
        MACAddress address;
        STAStatus status;
        int AID;
        int authSeqExpected;    // when NOT_AUTHENTICATED: transaction sequence number of next expected auth frame
        //int consecFailedTrans;  //XXX
        //double expiry;          //XXX association should expire after a while if STA is silent?
    };

    //
    // Stores DFRA scheduling info
    //
    using BYTE =  uint8;
    struct Sched {
        int numStations = 0;
        BYTE frameTypes = 0;
        BYTE *staSchedules = nullptr;
    };


    class NotificationInfoSta : public cObject
    {
        MACAddress apAddress;
        MACAddress staAddress;

      public:
        void setApAddress(const MACAddress& a) { apAddress = a; }
        void setStaAddress(const MACAddress& a) { staAddress = a; }
        const MACAddress& getApAddress() const { return apAddress; }
        const MACAddress& getStaAddress() const { return staAddress; }
    };

    struct MAC_compare
    {
        bool operator()(const MACAddress& u1, const MACAddress& u2) const { return u1.compareTo(u2) < 0; }
    };
    typedef std::map<MACAddress, STAInfo, MAC_compare> STAList;

  protected:
    // configuration
    std::string ssid;
    int channelNumber = -1;
    simtime_t beaconInterval;
    int numAuthSteps = 0;
    Ieee80211SupportedRatesElement supportedRates;

    //ADDED: AID control
    int nextAID;
    std::multiset<int> recycledAIDs;
    const int MAXAID = 2007;

    //ADDED: Schedule info
    Sched *schedule;


    // state
    STAList staList;    ///< list of STAs
    cMessage *beaconTimer = nullptr;

  public:
    DfraMgmtAP() {}
    virtual ~DfraMgmtAP();

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int) override;

    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleTimer(cMessage *msg) override;

    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleUpperMessage(cPacket *msg) override;

    /** Implements abstract Ieee80211MgmtBase method -- throws an error (no commands supported) */
    virtual void handleCommand(int msgkind, cObject *ctrl) override;

    /** Called by the signal handler whenever a change occurs we're interested in */
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, long value DETAILS_ARG) override;

    /** Utility function: return sender STA's entry from our STA list, or nullptr if not in there */
    virtual STAInfo *lookupSenderSTA(Ieee80211ManagementFrame *frame);

    /** Utility function: set fields in the given frame and send it out to the address */
    virtual void sendManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress& destAddr);

    /** Utility function: creates and sends a beacon frame */
    virtual void sendBeacon();

    /** @name Processing of different frame types */
    //@{
    virtual void handleDataFrame(Ieee80211DataFrame *frame) override;
    virtual void handleAuthenticationFrame(Ieee80211AuthenticationFrame *frame) override;
    virtual void handleDeauthenticationFrame(Ieee80211DeauthenticationFrame *frame) override;
    virtual void handleAssociationRequestFrame(Ieee80211AssociationRequestFrame *frame) override;
    virtual void handleAssociationResponseFrame(Ieee80211AssociationResponseFrame *frame) override;
    virtual void handleReassociationRequestFrame(Ieee80211ReassociationRequestFrame *frame) override;
    virtual void handleReassociationResponseFrame(Ieee80211ReassociationResponseFrame *frame) override;
    virtual void handleDisassociationFrame(Ieee80211DisassociationFrame *frame) override;
    virtual void handleBeaconFrame(Ieee80211BeaconFrame *frame) override;
    virtual void handleProbeRequestFrame(Ieee80211ProbeRequestFrame *frame) override;
    virtual void handleProbeResponseFrame(Ieee80211ProbeResponseFrame *frame) override;
    //@}

    void sendAssocNotification(const MACAddress& addr);

    void sendDisAssocNotification(const MACAddress& addr);

    /** lifecycle support */
    //@{

  protected:
    virtual void start() override;
    virtual void stop() override;
    //@}
  private:
    int getLowestUnusedAID(); //Added
};

//} // namespace dfra

} // namespace ieee80211

} // namespace inet

#endif // ifndef __DFRAMGMTAP_H

