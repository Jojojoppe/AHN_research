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

int Application::db_txcount = 0;

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
    timerManager->create(veins::TimerSpecification(callback).oneshotIn(0.0));

    std::ostringstream appname;
    appname << ID;
    parent->getParentModule()->setName(appname.str().c_str());
}

void Application::stopApplication(){
}

void Application::finish(){
    EV_INFO << "log_rx_rejects = " << log_rx_rejects << std::endl;
    EV_INFO << "log_tx_rejects = " << log_tx_rejects << std::endl;

    EV_INFO << "log_rx_success = " << log_rx_success << std::endl;
    EV_INFO << "log_tx_success = " << log_tx_success << std::endl;

    EV_INFO << "log_rx_data = " << log_rx_data << " B" << std::endl;
    EV_INFO << "log_tx_data = " << log_tx_data << " B" << std::endl;

    EV_INFO << "delays = [";
    for(auto & d : db_delays) EV_INFO << d.first << ":" << d.second << ",";
    EV_INFO << "]" << std::endl;
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
    //EV_INFO << content << std::endl;
    //EV_INFO << "m_id : " << m_id << std::endl;
    //EV_INFO << "m_x : " << m_x << std::endl;
    //EV_INFO << "m_y : " << m_y << std::endl;
    //EV_INFO << "m_type: " << m_type << std::endl;

    if(m_type=="RTS"){
        //EV_INFO << "m_rts_rx: " << m_rts_rx << std::endl;
        //EV_INFO << "m_rts_dur: " << m_rts_dur << std::endl;
    }else if(m_type=="CTS"){
        //EV_INFO << "m_cts_rx: " << m_cts_rx << std::endl;
        //EV_INFO << "m_cts_del: " << m_cts_del << std::endl;
    }
    // --------------------

    // Update neighbor list
    if(std::find(neighbors.begin(), neighbors.end(), m_id)==neighbors.end()){
        // Not yet in neighbor list, add to it
        neighbors.push_back(m_id);
        //EV_INFO << "New neighbor: [";
        //for(auto & ii : neighbors) EV_INFO << ii << ", ";
        //EV_INFO << "]" << std::endl;
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

    EV_INFO << "sending " << beaconContent.str() << std::endl;

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

void Application::setColor(const char * color){
    auto & dps = parent->getParentModule()->getDisplayString();
    dps.setTagArg("i", 1, color);
    dps.setTagArg("i", 2, 50);
}

// ---------------------
// mmWave data functions
// ---------------------

bool Application::startTransmissionRx(int txer){
    if(antennaBusy){
        EV_ERROR << "Already ongoing transmission! rx_reject!" << std::endl;
        EV_ERROR << "Antenna currently directed to " << antennaDirected << " and cannot be directed to " << txer << std::endl;
        log_rx_rejects++;
        return false;
    }
    antennaBusy = true;
    antennaDirected = txer;
    return true;
}

bool Application::endTransmissionRx(int txer){
    antennaBusy = false;
    log_rx_success++;
    log_rx_data += nrPackets*nrBytesPerPacket;
    return true;
}

void Application::startTransmissionTx(int rxer){
    if(antennaBusy){
        EV_ERROR << "Already ongoing transmission! tx_reject!" << std::endl;
        EV_ERROR << "Antenna currently directed to " << antennaDirected << " and cannot be directed to " << rxer << std::endl;
        EV_ERROR << "Current time " << simTime().dbl() << std::endl;
        log_tx_rejects++;
        return;
    }
    auto * app = getAppFromId(rxer);
    if(app->startTransmissionRx(ID)){
        EV_INFO << "Transmit mmWave data to " << rxer << " until " << simTime().dbl() + dataExchangeInterval << std::endl;

        antennaBusy = true;
        antennaDirected = rxer;

        Coord txPos = getPos();
        Coord rxPos = app->getPos();
        showLineTimed(rxPos.x, rxPos.y, txPos.x, txPos.y, "purple", 8, dataExchangeInterval);
    }
}

void Application::endTransmissionTx(int rxer){
    EV_INFO << "End mmWave transmission to " << rxer << std::endl;

    antennaBusy = false;
    log_tx_success++;
    log_tx_data += nrPackets*nrBytesPerPacket;
    db_delays[rxer] = simTime().dbl() - db_starttime;

    auto * app = getAppFromId(rxer);
    app->endTransmissionRx(ID);
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
    //if(nodeName=="52" && neighbors.size()>=4 && db_oneshotTransmission){
    // if((nodeName=="52" || nodeName=="114") && neighbors.size()>=4 && db_oneshotTransmission){
    if(neighbors.size()>=(int)parent->getParentModule()->getAncestorPar("txDstCount") && db_txcount<(int)parent->getParentModule()->getAncestorPar("txCount")){
        db_oneshotTransmission = false;
        db_starttime = simTime().dbl();
        for(auto & n : neighbors){
            db_delays[n] = +1.0/0.0;
        }
        db_destinations = neighbors;
        db_txcount++;

        setColor("red");

        EV_INFO << "Data generated for " << ID << std::endl;

        // Send RTS
        sendRts = true;
    }
}

// This function is ran when an RTS is received
void Application::mmw_recv_rts(int txer, double duration, std::vector<int> & rxers){
    // Put RTS info in unscheduled transmissions list
    mmw_unscheduled_rts[txer] = std::vector<std::pair<int, double>>();
    for(auto & rxer : rxers){
        mmw_unscheduled_rts[txer].push_back(std::make_pair(rxer, duration));

        // Check if I'm in the rxer list
        if(rxer==ID){
            // Notify beacon generator to generate CTS beacon
            sendCts = true;
        }
    }
}

// This function is ran when a CTS is received
void Application::mmw_recv_cts(int txer, std::vector<double> & delays, std::vector<int> & rxers){
    for(int i=0; i<delays.size(); i++){
        // Schedule transmission
        mmw_schedule(rxers[i], simTime().dbl()+delays[i], txer, dataExchangeInterval, true);
        mmw_schedule(txer, simTime().dbl()+delays[i], rxers[i], dataExchangeInterval, false);
    }
}

// This function is called when its sending an RTS
void Application::mmw_send_rts(){

    // Normally this function would check for LOS neighbours

    rts_rxNodes = db_destinations;
    rts_transmissionDuration = dataExchangeInterval;
}

// This function is called when its sending a CTS
void Application::mmw_send_cts(){
    cts_rxNodes.clear();
    cts_delays.clear();

    // Check unscheduled transmissions list (RTS) to create CTS beacon
    for(auto & ii : mmw_unscheduled_rts){
        int txer = ii.first;
        int ii_i = 0;
        for(auto & jj : ii.second){
            int rxer = jj.first;
            double duration = jj.second;

            if(rxer==ID){
                // Need to send CTS to txer

                double delay = mmw_findTimeSlot(txer, duration);
                // Add 0.00001 delay to compensate for sub 6GHz flight time
                delay += 0.00001;
                EV_INFO << "Feasible delay for CTS found to be " << delay << std::endl;

                // Add to CTS beacon
                cts_rxNodes.push_back(txer);
                cts_delays.push_back(delay);

                // Add to scheduled list
                mmw_schedule(rxer, simTime().dbl()+delay, txer, duration, false);
                mmw_schedule(txer, simTime().dbl()+delay, rxer, duration, true);

                // Remove from unscheduled list
                ii.second.erase(ii.second.begin()+ii_i);
            }

            ii_i++;
        }
    }
}

// Find first feasible spot for txer and rxer (ME), returns time between now and starttime
double Application::mmw_findTimeSlot(int txer, double duration){

    std::vector<double> changes;

    // Find changes in rxer
    for(auto & ii : mmw_scheduled[ID]){
        double i_starttime = ii.first;
        double i_duration = std::get<1>(ii.second);
        double i_endtime = i_starttime+i_duration;
        changes.push_back(i_endtime);
    }
    // Find changes in txer
    for(auto & ii : mmw_scheduled[txer]){
        double i_starttime = ii.first;
        double i_duration = std::get<1>(ii.second);
        double i_endtime = i_starttime+i_duration;
        changes.push_back(i_endtime);
    }

    // Loop over all changes and check if both are available
    for(auto & ii : changes){
        bool aOK = mmw_getStateAt(ii, duration, ID);
        bool bOK = mmw_getStateAt(ii, duration, txer);
        //EV_INFO << "change to free at: " << ii << " " << aOK << " " << bOK << std::endl;
        if(!aOK && !bOK){
            // Found feasible spot
            return ii-simTime().dbl();
        }
    }

    return 0.0;
}

// Get state of node for a duration from scheduled transmissions list
bool Application::mmw_getStateAt(double time, double duration, int node){
    // Check if node is known to be scheduled
    if(mmw_scheduled.count(node)<=0) return false;

    for(auto & ii : mmw_scheduled[node]){
        double i_starttime = ii.first;
        auto & i_info = ii.second;
        int i_othernode = std::get<0>(i_info);
        double i_duration = std::get<1>(i_info);
        bool i_direction = std::get<2>(i_info);

        // Check if starttime is in this scheduled transmission
        if(time>i_starttime && time<i_starttime+i_duration){
            // Start time collides with scheduled moment
            return true;
        }
        // Check if end time is in this scheduled transmission
        if(time+duration>i_starttime && time+duration<i_starttime+i_duration){
            // End time collides with scheduled moment
            return true;
        }
    }
    return false;
}

void Application::mmw_schedule(int node, double starttime, int othernode, double duration, bool direction){
    if(mmw_scheduled.count(node)<=0){
        // Not yet in scheduled list, create map
        mmw_scheduled[node] = std::map<double, std::tuple<int, double, bool>>();
    }
    mmw_scheduled[node][starttime] = std::make_tuple(othernode, duration, direction);
    // Schedule removal function to remove scheduled slot from list
    std::function<void()> callback = std::bind(&Application::mmw_unschedule, this, node, othernode, starttime, direction);
    timerManager->create(veins::TimerSpecification(callback).oneshotAt(starttime+duration));
    //EV_INFO << "transmission scheduled at "<< starttime << "-" << starttime+duration << " between " << node << " and " << othernode << std::endl;

    // Check if I need to send
    if(node==ID && direction){
        // Schedule transmitting function
        std::function<void()> callback = std::bind(&Application::startTransmissionTx, this, othernode);
        timerManager->create(veins::TimerSpecification(callback).oneshotAt(starttime));
    }
}

void Application::mmw_unschedule(int node, int othernode, double starttime, bool direction){
    //EV_INFO << "transmission between " << node << " and " << othernode << " ended" << std::endl;

    // Remove from scheduled map
    mmw_scheduled[node].erase(starttime);

    // Check if I was sending
    if(node==ID && direction){
        endTransmissionTx(othernode);
    }
}