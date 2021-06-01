#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "veins_inet/veins_inet.h"
#include "veins/modules/utility/TimerManager.h"

#include "inet/common/INETDefs.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include <omnetpp.h>
using namespace omnetpp;

#include <functional>
#include <memory>

class Application{
private:    
    inet::ApplicationBase * parent;
    std::function<void(std::unique_ptr<inet::Packet>)> sendPacket;
    veins::TimerManager * timerManager;

    // Simulation parameters
    double beaconPeriod;

    // Important constants
    int ID;
    bool staticApplication;

public:
    Application(inet::ApplicationBase * parent, std::function<void(std::unique_ptr<inet::Packet>)> sendPacket, veins::TimerManager * timerManager, bool staticApplication);
    virtual ~Application();

    void startApplication();
    void stopApplication();

    void processPacket(std::shared_ptr<inet::Packet> pk);

    void beaconCallback();

private:
    Application * getAppFromId(int id);

    cPolylineFigure * createLine(double x1, double y1, double x2, double y2, const char * color, double width);
    void showLine(cPolylineFigure * line);
    void deleteLine(cPolylineFigure * line);
};

#endif
