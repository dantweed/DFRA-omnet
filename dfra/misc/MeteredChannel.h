//
// Copyright (C) 2005 Andras Varga
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

#ifndef __METEREDCHANNEL_H
#define __METEREDCHANNEL_H

#include "inet/common/INETDefs.h"

namespace inet {

/**
 * A cDatarateChannel extended with throughput calculation. Values
 * get displayed on the link, using the connection's "t=" display
 * string tag.
 *
 * The display can be customized with the "format" attribute.
 * In the format string, the following characters will get expanded:
 *   - 'N': number of packets
 *   - 'V': volume (in bytes)
 *   - 'p': current packet/sec
 *   - 'b': current bandwidth
 *   - 'u': current channel utilization (%)
 *   - 'P': average packet/sec on [0,now)
 *   - 'B': average bandwidth on [0,now)
 *   - 'U': average channel utilization (%) on [0,now)
 * Other characters are copied verbatim.
 *
 * "Current" actually means the last measurement interval, which is
 * 10 packets or 0.1s, whichever comes first.
 *
 * PROBLEM: display only gets updated if there's traffic! (For example, a
 * high pk/sec value might stay displayed even when the link becomes idle!)
 */
class INET_API MeteredChannel : public cDatarateChannel
{
  protected:
    // configuration
    const char *fmt;    // display format
    unsigned int batchSize;    // number of packets in a batch
    simtime_t maxInterval;    // max length of measurement interval (measurement ends
                              // if either batchSize or maxInterval is reached, whichever
                              // is reached first)
    simtime_t startTime;
    // global statistics
    long numPackets;
    double numBits;    // double to avoid overflow

    // current measurement interval
    simtime_t intvlStartTime;
    simtime_t intvlLastPkTime;
    unsigned long intvlNumPackets;
    unsigned long intvlNumBits;

    // reading from last interval
    double currentBitPerSec;
    double currentPkPerSec;

    //New
    cOutVector bitpersecVector;

  protected:
    virtual void beginNewInterval(simtime_t now);
    virtual void updateDisplay();
    virtual void finish();
  public:
    /**
     * Constructor.
     */
    explicit MeteredChannel(const char *name = nullptr);

    /**
     * Destructor.
     */
    virtual ~MeteredChannel();

    /**
     * Add parameters and initialize the stat variables
     */
    virtual void initialize() override;

    /**
     * Adds statistics and live display to the channel.
     */
    virtual void processMessage(cMessage *msg, simtime_t t, result_t& result) override;
};

} // namespace inet

#endif // ifndef __METEREDCHANNEL_H

