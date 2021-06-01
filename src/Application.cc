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

#include "veins_inet/veins_inet.h"
#include "veins_inet/VeinsInetSampleMessage_m.h"

#include <memory>
#include <sstream>

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

    beaconContent >> tmp;
    beaconContent >> tmp;
    int m_id = std::stoi(tmp);

    // Check if self message
    if(m_id == this->ID) return;

    EV_INFO << "m_id : " << m_id << std::endl;

    cModule * mod = parent->getSimulation()->getModule(m_id);
    // Check if static or veins application
    Application * sender;
    if(staticApplication){
        StaticApplication * app = check_and_cast<StaticApplication*>(mod);
        sender = app->app;
    }else{
        VeinsApplication * app = check_and_cast<VeinsApplication*>(mod);
        sender = app->app;
    }

    sender->test("Test123");
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
    payload->setRoadId(beaconContent.str().c_str());

    std::unique_ptr<Packet> packet = std::make_unique<Packet>(std::string("beacon").c_str());
    packet->insertAtBack(payload);
    sendPacket(std::move(packet));
}

void Application::test(const char * msg){
    EV_INFO << parent->getId() << " received a msg: " << msg << std::endl;
}