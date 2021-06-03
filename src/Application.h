#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "veins_inet/veins_inet.h"
#include "veins/modules/utility/TimerManager.h"

#include "inet/common/INETDefs.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/common/geometry/common/Coord.h"

#include <omnetpp.h>
using namespace omnetpp;

#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <utility>
#include <tuple>

class Application{
private:    
    inet::ApplicationBase * parent;
    std::function<void(std::unique_ptr<inet::Packet>)> sendPacket;
    veins::TimerManager * timerManager;

    std::string nodeName;

    // Simulation parameters
    double beaconPeriod;
    double dataExchangeInterval;
    int nrPackets;
    int nrBytesPerPacket;

    // Important constants
    int ID;
    bool staticApplication;

    // Vehicle state
    std::vector<int> neighbors;

    // TX state
    bool sendRts = false;
    std::vector<int> rts_rxNodes;
    double rts_transmissionDuration;

    // Rx state
    bool sendCts = false;
    std::vector<int> cts_rxNodes;
    std::vector<double> cts_delays;

    // Scheduling
    // ----------

    // Scheduled transmissions
    // rxer/txer, {starttime, <rxer/txer, duration, direction(true for outgoing)>}
    std::unordered_map<int, std::map<double, std::tuple<int, double, bool>>> mmw_scheduled;

    // Non scheduled transmissions
    // txer, [<rxer, duration>]
    std::unordered_map<int, std::vector<std::pair<int, double>>> mmw_unscheduled_rts;
    // rxer, [<txer, duration>]
    std::unordered_map<int, std::vector<std::pair<int, double>>> mmw_unscheduled_cts;

    // DEBUG
    bool db_oneshotTransmission = true;

public:
    // LOGGING
    // -------
    int log_rx_rejects = 0;
    int log_tx_rejects = 0;
    int log_rx_success = 0;
    int log_tx_success = 0;

public:
    Application(inet::ApplicationBase * parent, std::function<void(std::unique_ptr<inet::Packet>)> sendPacket, veins::TimerManager * timerManager, bool staticApplication);
    virtual ~Application();

    void startApplication();
    void stopApplication();
    void finish();

    void processPacket(std::shared_ptr<inet::Packet> pk);

    void beaconCallback();

    // Returns false if no transmission can be done
    bool startTransmissionRx(int txer, int packets, int bytesPerPacket, double time, inet::Coord txPos);
    bool endTransmissionRx();

private:
    Application * getAppFromId(int id);
    inet::Coord getPos();

    cPolylineFigure * createLine(double x1, double y1, double x2, double y2, const char * color, double width);
    void showLine(cPolylineFigure * line);
    void deleteLine(cPolylineFigure * line);
    void showLineTimed(double x1, double y1, double x2, double y2, const char * color, double width, double time);

    void startTransmissionTx(int rxer);
    void endTransmissionTx(int rxer);

    // mmWave MAC functions
    void mmw_loop();
    void mmw_recv_rts(int txer, double duration, std::vector<int> & rxers);
    void mmw_recv_cts(int txer, std::vector<double> & delays, std::vector<int> & rxers);
    void mmw_send_rts();
    void mmw_send_cts();

    bool mmw_getStateAt(double time, double duration, int node);
    void mmw_schedule(int node, double starttime, int othernode, double duration, bool direction);
    void mmw_unschedule(int node, int othernode);
};

#endif
