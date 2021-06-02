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
#include <algorithm>

using namespace inet;
using namespace omnetpp;

Application::Application(inet::ApplicationBase * parent, std::function<void(std::unique_ptr<Packet>)> sendPacket, veins::TimerManager * timerManager, bool staticApplication){
    this->parent = parent;
    this->sendPacket = sendPacket;
    this->timerManager = timerManager;
    this->staticApplication = staticApplication;

    // get simulation parameters
    beaconPeriod = parent->getParentModule()->getAncestorPar("beaconPeriod");
    dataExchangeInterval = parent->getParentModule()->getAncestorPar("dataExchangeInterval");
    nrPackets = parent->getParentModule()->getAncestorPar("nrPackets");
    nrBytesPerPacket = parent->getParentModule()->getAncestorPar("nrBytesPerPacket");
    
    this->ID = parent->getId();
}

Application::~Application(){
}

void Application::startApplication(){
    // Beacon generation
    // -----------------
    // Get random value between 0 and 1;
    auto randwait_i = parent->intrand(1000);
    float randwait = randwait_i/1000.0;
    // Start beacon sending
    std::function<void()> callback = std::bind(&Application::beaconCallback, this);
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(randwait*beaconPeriod));
    // -----------------

    // Schedule mmw MAC main loop
    callback = std::bind(&Application::mmw_loop, this);
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(parent->getParentModule()->getAncestorPar("mmwLoopTime")));

    std::ostringstream appname;
    appname << ID;
    parent->getParentModule()->setName(appname.str().c_str());
}

void Application::stopApplication(){
}

void Application::processPacket(std::shared_ptr<inet::Packet> pk){
    auto contentChunk = pk->peekAtFront<VeinsInetSampleMessage>();
    std::string content = contentChunk->getRoadId();

    std::istringstream beaconContent(content);
    std::string tmp;

    // Get beacon values
    // -----------------
    beaconContent >> tmp; beaconContent >> tmp;
    int m_id = std::stoi(tmp);
    beaconContent >> tmp; beaconContent >> tmp;
    int m_x = std::stoi(tmp);
    beaconContent >> tmp; beaconContent >> tmp;
    int m_y = std::stoi(tmp);
    beaconContent >> tmp; beaconContent >> tmp;
    std::string m_type = tmp;

    std::string m_rts_rx;
    double m_rts_dur;
    std::string m_cts_rx;
    std::string m_cts_del;

    if(m_type=="RTS"){
        beaconContent >> tmp; beaconContent >> tmp;
        m_rts_rx = tmp;
        beaconContent >> tmp; beaconContent >> tmp;
        m_rts_dur = std::stod(tmp);
    }else if(m_type=="CTS"){
        beaconContent >> tmp; beaconContent >> tmp;
        m_cts_rx = tmp;
        beaconContent >> tmp; beaconContent >> tmp;
        m_cts_del = tmp;
    }
    // -----------------

    // Check if self message
    if(m_id == this->ID) return;

    // Print beacon content
    // --------------------
    EV_INFO << content << std::endl;
    EV_INFO << "m_id : " << m_id << std::endl;
    EV_INFO << "m_x : " << m_x << std::endl;
    EV_INFO << "m_y : " << m_y << std::endl;
    EV_INFO << "m_type: " << m_type << std::endl;

    if(m_type=="RTS"){
        EV_INFO << "m_rts_rx: " << m_rts_rx << std::endl;
        EV_INFO << "m_rts_dur: " << m_rts_dur << std::endl;
    }else if(m_type=="CTS"){
        EV_INFO << "m_cts_rx: " << m_cts_rx << std::endl;
        EV_INFO << "m_cts_del: " << m_cts_del << std::endl;
    }
    // --------------------

    // Update neighbor list
    if(std::find(neighbors.begin(), neighbors.end(), m_id)==neighbors.end()){
        // Not yet in neighbor list, add to it
        neighbors.push_back(m_id);
        EV_INFO << "New neighbor: [";
        for(auto & ii : neighbors) EV_INFO << ii << ", ";
        EV_INFO << "]" << std::endl;
    }

    if(m_type=="RTS"){
        std::stringstream ss(m_rts_rx);
        std::vector<int> rxers;
        while(ss.good()){
            std::string substr;
            getline(ss, substr, ',');
            if(substr!=""){
                rxers.push_back(std::stoi(substr));
            }
        }
        mmw_recv_rts(m_id, m_rts_dur, rxers);
    }else if(m_type=="CTS"){
        std::stringstream ssrx(m_cts_rx);
        std::vector<int> rxers;
        while(ssrx.good()){
            std::string substr;
            getline(ssrx, substr, ',');
            if(substr!=""){
                rxers.push_back(std::stoi(substr));
            }
        }
        std::stringstream ssdel(m_cts_del);
        std::vector<double> delays;
        while(ssdel.good()){
            std::string substr;
            getline(ssdel, substr, ',');
            if(substr!=""){
                delays.push_back(std::stod(substr));
            }
        }
        mmw_recv_cts(m_id, delays, rxers);
    }

}

void Application::beaconCallback(){
    std::function<void()> callback = std::bind(&Application::beaconCallback, this);
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(beaconPeriod));

    auto payload = makeShared<VeinsInetSampleMessage>();
    payload->removeTagIfPresent<CreationTimeTag>(b(0), b(-1));
    auto creationTimeTag = payload->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(simTime());
    payload->setChunkLength(B(800));

    // Set beacon content
    // ------------------
    std::ostringstream beaconContent;
    // Add ID
    beaconContent << "ID " << this->ID << " ";
    // Add position
    Coord pos = getPos();
    beaconContent << "x " << pos.x << " ";
    beaconContent << "y " << pos.y << " ";

    // Check if there is RTS to transmit
    if(sendRts){
        mmw_send_rts();
        beaconContent << "tp RTS ";
        beaconContent << "rx ";
        for(auto & ii : rts_rxNodes){
            beaconContent << ii << ",";
        }
        beaconContent << " ";
        beaconContent << "dur " << rts_transmissionDuration << " ";

        sendRts = false;
        rts_rxNodes.clear();
    }
    // Check if there is CTS to transmit
    else if(sendCts){
        mmw_send_cts();
        beaconContent << "tp CTS ";
        beaconContent << "rx ";
        for(auto & ii : cts_rxNodes){
            beaconContent << ii << ",";
        }
        beaconContent << " del ";
        for(auto & ii : cts_delays){
            beaconContent << ii << ",";
        }
        beaconContent << " ";

        sendCts = false;
    }
    // If no special beacon
    else{
        beaconContent << "tp NONE ";
    }

    // ------------------
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

Coord Application::getPos(){
    cModule * ppmodule = parent->getParentModule()->getSubmodule("mobility");
    IMobility *mmobility = check_and_cast<IMobility *>(ppmodule);
    return mmobility->getCurrentPosition();
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

void Application::showLineTimed(double x1, double y1, double x2, double y2, const char * color, double width, double time){
    auto * line = createLine(x1, y1, x2, y2, color, width);
    showLine(line);

    std::function<void()> callback = std::bind(&Application::deleteLine, this, line);
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(time));
}

bool Application::startTransmissionRx(int txer, int packets, int bytesPerPacket, double time, Coord txPos){
    if(!mmw_cur_rxing){
        mmw_cur_tx = txer;
        mmw_cur_rxing = true;
        return true;
    }
    return false;
}

bool Application::endTransmissionRx(){
    mmw_cur_rxing = false;
    return true;
}

void Application::startTransmissionTx(){
    // Get first transmission from list and delete it
    int rxer = -1;
    double time = +1.0/0.0;
    int i = 0;
    int rxer_i = 0;
    for(auto & ii : mmw_outgoing_scheduled){
        if(ii.second<time){
            time = ii.second;
            rxer = ii.first;
            rxer_i = i;
        }
        i++;
    }
    mmw_outgoing_scheduled.erase(mmw_outgoing_scheduled.begin() + rxer_i);

    EV_INFO << "Transmit mmWave data to " << rxer << std::endl;

    auto * app = getAppFromId(rxer);
    bool successful = app->startTransmissionRx(ID, nrPackets, nrBytesPerPacket, dataExchangeInterval, getPos());
    if(successful){
        Coord txPos = getPos();
        Coord rxPos = app->getPos();
        showLineTimed(rxPos.x, rxPos.y, txPos.x, txPos.y, "purple", 4, dataExchangeInterval);

        // Schedule end of transmission
        mmw_cur_rx = rxer;
        mmw_cur_txing = true;

        std::function<void()> callback = std::bind(&Application::endTransmissionTx, this);
        timerManager->create(veins::TimerSpecification(callback).oneshotIn(dataExchangeInterval));
    }else{
        EV_ERROR << "Scheduled transmission failed: antenna's not directed to each other!" << std::endl;
    }
}

void Application::endTransmissionTx(){
    mmw_cur_txing = false;
    auto * app = getAppFromId(mmw_cur_rx);
    app->endTransmissionTx();
}

// --------------------
// mmWave MAC functions
// --------------------

// This function is ran each x amount of time
void Application::mmw_loop(){
    nodeName = parent->getParentModule()->getName();
    std::function<void()> callback = std::bind(&Application::mmw_loop, this);
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(parent->getParentModule()->getAncestorPar("mmwLoopTime")));

    // DEBUG
    // Let node0 start transmission once when 4 neighbors are there
    if(nodeName=="52" && neighbors.size()>=4 && db_oneshotTransmission){
        db_oneshotTransmission = false;
        EV_INFO << "send RTS" << std::endl;

        // Send RTS
        sendRts = true;
    }
}

// This function is ran when an RTS is received
void Application::mmw_recv_rts(int txer, double duration, std::vector<int> & rxers){
    // Check if I'm in RTS message
    if(std::find(rxers.begin(), rxers.end(), ID)!=rxers.end()){
        // NEED TO RECEIVE DATA
        sendCts = true;
        mmw_todo[txer] = duration;
    }

    // Set ongoing transmissions list (reset if transmitter starts again with RTS)
    mmw_ogt_dur[txer] = duration;
    mmw_ogt_rxers[txer] = std::vector<int>(rxers);
    mmw_ogt_startime[txer] = simTime().dbl();
}

// This function is ran when a CTS is received
void Application::mmw_recv_cts(int txer, std::vector<double> & delays, std::vector<int> & rxers){
    // Check if CTS is for me
    if(std::find(rxers.begin(), rxers.end(), ID)!=rxers.end()){
        // NEED TO TRANSMIT DATA
        for(int i=0; i<delays.size(); i++){
            double del = delays[i];
            del += 0.0001;  // Add to allow for scheduling

            EV_INFO << "CTS received from " << txer << " with delay " << del << std::endl;
            EV_INFO << "Schedule transmission in " << simTime() + del << " (now it is " << simTime() << " )" << std::endl;

            // Schedule transmission
            mmw_outgoing_scheduled.push_back(std::make_pair(txer, simTime().dbl() + del));
            std::function<void()> callback = std::bind(&Application::startTransmissionTx, this);
            timerManager->create(veins::TimerSpecification(callback).oneshotIn(del));
        }
    }else{
        // NEED TO USE DATA FOR SCHEDULING
    }
}

// This function is called when its sending an RTS
void Application::mmw_send_rts(){
    rts_rxNodes = neighbors;
    rts_transmissionDuration = 0.050;
}

// This function is called when its sending a CTS
void Application::mmw_send_cts(){
    cts_rxNodes.clear();
    cts_delays.clear();

    // Check which transmissions must be done
    for(auto & tr : mmw_todo){
        int rxer = tr.first;
        double duration = tr.second;

        cts_rxNodes.push_back(rxer);
        cts_delays.push_back(0.0);
    }

    mmw_todo.clear();
}