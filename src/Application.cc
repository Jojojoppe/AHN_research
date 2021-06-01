#include "Application.h"
#include "StaticApplication.h"
#include "VeinsApplication.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

#include "veins/visualizer/roads/RoadsCanvasVisualizer.h"

#include "veins_inet/veins_inet.h"
#include "veins_inet/VeinsInetSampleMessage_m.h"

#include <memory>
#include <sstream>
#include <vector>

using namespace inet;
using namespace omnetpp;

Application::Application(inet::ApplicationBase * parent, std::function<void(std::unique_ptr<Packet>)> sendPacket, veins::TimerManager * timerManager, bool staticApplication){
    this->parent = parent;
    this->sendPacket = sendPacket;
    this->timerManager = timerManager;
    this->staticApplication = staticApplication;

    // get simulation parameters
    beaconPeriod = parent->getParentModule()->getAncestorPar("beaconPeriod");
    
    this->ID = parent->getId();
}

Application::~Application(){
}

void Application::startApplication(){
    // Get random value between 0 and 1;
    auto randwait_i = parent->intrand(1000);
    float randwait = randwait_i/1000.0;

    std::function<void()> callback = std::bind(&Application::beaconCallback, this);
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(simTime() + randwait*beaconPeriod));
}

void Application::stopApplication(){
}

void Application::processPacket(std::shared_ptr<inet::Packet> pk){
    auto contentChunk = pk->peekAtFront<VeinsInetSampleMessage>();
    std::string content = contentChunk->getRoadId();

    std::istringstream beaconContent(content);
    std::string tmp;

    beaconContent >> tmp; beaconContent >> tmp;
    int m_id = std::stoi(tmp);
    beaconContent >> tmp; beaconContent >> tmp;
    int m_x = std::stoi(tmp);
    beaconContent >> tmp; beaconContent >> tmp;
    int m_y = std::stoi(tmp);

    // Check if self message
    if(m_id == this->ID) return;

    EV_INFO << "m_id : " << m_id << std::endl;
    EV_INFO << "m_x : " << m_x << std::endl;
    EV_INFO << "m_y : " << m_y << std::endl;

    // Get position from mobility
    cModule * ppmodule = parent->getParentModule()->getSubmodule("mobility");
    IMobility *mmobility = check_and_cast<IMobility *>(ppmodule);
    Coord pos = mmobility->getCurrentPosition();
    // Draw line between self and txer
    auto * line = createLine(pos.x, pos.y, m_x, m_y, "purple", 4);
    showLine(line);
}

void Application::beaconCallback(){
    std::function<void()> callback = std::bind(&Application::beaconCallback, this);
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(simTime() + beaconPeriod));

    auto payload = makeShared<VeinsInetSampleMessage>();
    payload->removeTagIfPresent<CreationTimeTag>(b(0), b(-1));
    auto creationTimeTag = payload->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(simTime());
    payload->setChunkLength(B(500));

    // Set beacon content
    std::ostringstream beaconContent;
    beaconContent << "ID " << this->ID << " ";
    // Get position from mobility
    cModule * ppmodule = parent->getParentModule()->getSubmodule("mobility");
    IMobility *mmobility = check_and_cast<IMobility *>(ppmodule);
    Coord pos = mmobility->getCurrentPosition();
    beaconContent << "x " << pos.x << " ";
    beaconContent << "y " << pos.y << " ";
    // Set content
    payload->setRoadId(beaconContent.str().c_str());

    std::unique_ptr<Packet> packet = std::make_unique<Packet>(std::string("beacon").c_str());
    packet->insertAtBack(payload);
    sendPacket(std::move(packet));
}

Application * Application::getAppFromId(int id){
    cModule * mod = parent->getSimulation()->getModule(id);
    if(!mod) return nullptr;
    if(staticApplication){
        StaticApplication * app = check_and_cast<StaticApplication*>(mod);
        if(!app) return nullptr;
        return app->app;
    }else{
        VeinsApplication * app = check_and_cast<VeinsApplication*>(mod);
        if(!app) return nullptr;
        return app->app;
    }
}

cPolylineFigure * Application::createLine(double x1, double y1, double x2, double y2, const char * color, double width){
    std::vector<cFigure::Point> points = {cFigure::Point(x1, y1), cFigure::Point(x2, y2)};
    auto * line = new cPolylineFigure();
    line->setPoints(points);
    line->setLineWidth(width);
    line->setLineColor(cFigure::Color(color));
    return line;
}

void Application::showLine(cPolylineFigure * line){
    cCanvas * canvas = parent->getSimulation()->getSystemModule()->getCanvas();
    canvas->addFigure(line);
}

void Application::deleteLine(cPolylineFigure * line){
    cCanvas * canvas = parent->getSimulation()->getSystemModule()->getCanvas();
    canvas->removeFigure(line);
}