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

#include "MeteredChannel.h"

namespace inet {

Register_Class(MeteredChannel);

MeteredChannel::MeteredChannel(const char *name) : cDatarateChannel(name)
{
    fmt = nullptr;
    batchSize = 10;    // packets
    maxInterval = 0.1;    // seconds

    numPackets = 0;
    numBits = 0;

    intvlStartTime = intvlLastPkTime = 0;
    intvlNumPackets = intvlNumBits = 0;

    currentBitPerSec = currentPkPerSec = 0;
}

MeteredChannel::~MeteredChannel()
{
}

//MeteredChannel& MeteredChannel::operator=(const MeteredChannel& ch)
//{
//    if (this==&ch) return *this;
//    cDatarateChannel::operator=(ch);
//    numPackets = ch.numPackets;
//    numBits = ch.numBits;
//    fmt = ch.fmt;
//    return *this;
//}

void MeteredChannel::initialize()
{
    cDatarateChannel::initialize();
    fmt = par("thruputDisplayFormat");
    bitpersecVector.setName("thruput (bit/sec)");
    startTime = -1;
}

void MeteredChannel::processMessage(cMessage *msg, simtime_t t, result_t& result)
{
    if (startTime < 0) startTime = simTime();  //Record start time of first packet

    cDatarateChannel::processMessage(msg, t, result);

    cPacket *pkt = dynamic_cast<cPacket *>(msg);
    // TODO handle disabled state (show with different style?/color? or print "disabled"?)
    if (!pkt || !fmt || *fmt == 0 || result.discard)
        return;

    // count packets and bits
    numPackets++;
    numBits += pkt->getBitLength();

    // packet should be counted to new interval
    if (intvlNumPackets >= batchSize || t - intvlStartTime >= maxInterval)
        beginNewInterval(t);

    intvlNumPackets++;
    intvlNumBits += pkt->getBitLength();
    intvlLastPkTime = t;

    // update display string
    updateDisplay();
}

void MeteredChannel::beginNewInterval(simtime_t now)
{
    simtime_t duration = now - intvlStartTime;

    // record measurements
    currentBitPerSec = intvlNumBits / duration;
    currentPkPerSec = intvlNumPackets / duration;

    // restart counters
    intvlStartTime = now;
    intvlNumPackets = intvlNumBits = 0;
    bitpersecVector.recordWithTimestamp(intvlStartTime, currentBitPerSec);
}

void MeteredChannel::updateDisplay()
{
    // produce label, based on format string
    char buf[200];
    char *p = buf;
    simtime_t tt = getTransmissionFinishTime();
    if (tt == 0)
        tt = simTime();
    double bps = (tt == 0) ? 0 : numBits / tt;
    double bytes;
    for (const char *fp = fmt; *fp && buf + 200 - p > 20; fp++) {
        switch (*fp) {
            case 'N':    // number of packets
                p += sprintf(p, "%ld", numPackets);
                break;

            case 'V':    // volume (in bytes)
                bytes = floor(numBits / 8);
                if (bytes < 1024)
                    p += sprintf(p, "%gB", bytes);
                else if (bytes < 1024 * 1024)
                    p += sprintf(p, "%.3gKiB", bytes / 1024);
                else
                    p += sprintf(p, "%.3gMiB", bytes / 1024 / 1024);
                break;

            case 'p':    // current packet/sec
                p += sprintf(p, "%.3gpps", currentPkPerSec);
                break;

            case 'b':    // current bandwidth
                if (currentBitPerSec < 1000000)
                    p += sprintf(p, "%.3gk", currentBitPerSec / 1000);
                else
                    p += sprintf(p, "%.3gM", currentBitPerSec / 1000000);
                break;

            case 'u':    // current channel utilization (%)
                if (getDatarate() == 0)
                    p += sprintf(p, "n/a");
                else
                    p += sprintf(p, "%.3g%%", currentBitPerSec / getDatarate() * 100.0);
                break;

            case 'P':    // average packet/sec on [0,now)
                p += sprintf(p, "%.3gpps", tt == 0 ? 0 : numPackets / tt);
                break;

            case 'B':    // average bandwidth on [0,now)
                if (bps < 1000000)
                    p += sprintf(p, "%.3gk", bps / 1000);
                else
                    p += sprintf(p, "%.3gM", bps / 1000000);
                break;

            case 'U':    // average channel utilization (%) on [0,now)
                if (getDatarate() == 0)
                    p += sprintf(p, "n/a");
                else
                    p += sprintf(p, "%.3g%%", bps / getDatarate() * 100.0);
                break;

            default:
                *p++ = *fp;
                break;
        }
    }
    *p = '\0';

    // display label
    getSourceGate()->getDisplayString().setTagArg("t", 0, buf);
}

void MeteredChannel::finish()
{
    simtime_t duration = simTime() - startTime;

    recordScalar("duration", duration);
    recordScalar("total packets", numPackets);
    recordScalar("total bits", numBits);

    recordScalar("avg throughput (bit/s)", numBits / duration.dbl());
    recordScalar("avg packets/s", numPackets / duration.dbl());


}

} // namespace inet

