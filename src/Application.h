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
#include <utility>

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
    // Ongoing transmissions
    std::unordered_map<int, std::vector<int>> mmw_ogt_rxers;
    std::unordered_map<int, double> mmw_ogt_dur;
    std::unordered_map<int, double> mmw_ogt_startime;
    std::unordered_map<int, std::vector<std::pair<int, double>>> mmw_ogt_scheduled;
    // My transmissions
    std::unordered_map<int, double> mmw_todo;
    std::vector<std::pair<int, double>> mmw_outgoing_scheduled;
    // Current transmission
    int mmw_cur_rx;
    bool mmw_cur_txing = false;
    int mmw_cur_tx;
    bool mmw_cur_rxing = false;
    // ----------

    // DEBUG
    bool db_oneshotTransmission = true;

public:
    Application(inet::ApplicationBase * parent, std::function<void(std::unique_ptr<inet::Packet>)> sendPacket, veins::TimerManager * timerManager, bool staticApplication);
    virtual ~Application();

    void startApplication();
    void stopApplication();

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

    void startTransmissionTx();
    void endTransmissionTx();

    // mmWave MAC functions
    void mmw_loop();
    void mmw_recv_rts(int txer, double duration, std::vector<int> & rxers);
    void mmw_recv_cts(int txer, std::vector<double> & delays, std::vector<int> & rxers);
    void mmw_send_rts();
    void mmw_send_cts();
};

#endif
