#include "StaticApplication.h"

#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

using namespace inet;

Define_Module(StaticApplication);

StaticApplication::StaticApplication(){
}

int StaticApplication::numInitStages() const{
    return inet::NUM_INIT_STAGES;
}

void StaticApplication::initialize(int stage){
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
    }
}

void StaticApplication::handleStartOperation(LifecycleOperation* operation){
    L3AddressResolver().tryResolve("224.0.0.1", destAddress);
    ASSERT(!destAddress.isUnspecified());

    socket.setOutputGate(gate("socketOut"));
    socket.bind(L3Address(), portNumber);

    const char* interface = par("interface");
    ASSERT(interface[0]);
    IInterfaceTable* ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
#if INET_VERSION >= 0x0402
    NetworkInterface* ie = ift->findInterfaceByName(interface);
#else
    InterfaceEntry* ie = ift->getInterfaceByName(interface);
#endif
    ASSERT(ie);
    socket.setMulticastOutputInterface(ie->getInterfaceId());

    MulticastGroupList mgl = ift->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);

    socket.setCallback(this);

    bool ok = startApplication();
    ASSERT(ok);
}

bool StaticApplication::startApplication(){
    // Create application
    app = new Application(this, std::bind(&StaticApplication::sendPacket, this, std::placeholders::_1), &timerManager, true);
    // Start application
    app->startApplication();
    return true;
}

bool StaticApplication::stopApplication(){
    app->stopApplication();
    return true;
}

void StaticApplication::handleStopOperation(LifecycleOperation* operation){
    bool ok = stopApplication();
    ASSERT(ok);

    socket.close();
}

void StaticApplication::handleCrashOperation(LifecycleOperation* operation){
    socket.destroy();
}

void StaticApplication::finish(){
    ApplicationBase::finish();
}

StaticApplication::~StaticApplication(){
    delete app;
}

void StaticApplication::refreshDisplay() const{
    ApplicationBase::refreshDisplay();

    char buf[100];
    sprintf(buf, "okay");
    getDisplayString().setTagArg("t", 0, buf);
}

void StaticApplication::handleMessageWhenUp(cMessage* msg){
    if (timerManager.handleMessage(msg)) return;

    if (msg->isSelfMessage()) {
        throw cRuntimeError("This module does not use custom self messages");
        return;
    }

    socket.processMessage(msg);
}

void StaticApplication::socketDataArrived(UdpSocket* socket, Packet* packet){
    auto pk = std::shared_ptr<inet::Packet>(packet);

    // ignore local echoes
    auto srcAddr = pk->getTag<L3AddressInd>()->getSrcAddress();
    if (srcAddr == Ipv4Address::LOOPBACK_ADDRESS) {
        EV_DEBUG << "Ignored local echo: " << pk.get() << endl;
        return;
    }

    // statistics
    emit(packetReceivedSignal, pk.get());

    // process incoming packet
    processPacket(pk);
}

void StaticApplication::socketErrorArrived(UdpSocket* socket, Indication* indication){
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void StaticApplication::socketClosed(UdpSocket* socket){
    if (operationalState == State::STOPPING_OPERATION) {
        startActiveOperationExtraTimeOrFinish(-1);
    }
}

void StaticApplication::timestampPayload(inet::Ptr<inet::Chunk> payload){
    payload->removeTagIfPresent<CreationTimeTag>(b(0), b(-1));
    auto creationTimeTag = payload->addTag<CreationTimeTag>();
    creationTimeTag->setCreationTime(simTime());
}

void StaticApplication::sendPacket(std::unique_ptr<inet::Packet> pk){
    emit(packetSentSignal, pk.get());
    socket.sendTo(pk.release(), destAddress, portNumber);
}

std::unique_ptr<inet::Packet> StaticApplication::createPacket(std::string name){
    return std::unique_ptr<Packet>(new Packet(name.c_str()));
}

void StaticApplication::processPacket(std::shared_ptr<inet::Packet> pk){
    app->processPacket(pk);
}