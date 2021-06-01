#ifndef __WIRELESS_STATICAPPLICATION_H_
#define __WIRELESS_STATICAPPLICATION_H_

#include "Application.h"

#include "veins_inet/veins_inet.h"
#include "veins/modules/utility/TimerManager.h"

#include "inet/common/INETDefs.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include <vector>

class StaticApplication : public inet::ApplicationBase, public inet::UdpSocket::ICallback{
    friend class Application;
    
    inet::L3Address destAddress;
    const int portNumber = 9001;
    inet::UdpSocket socket;

    veins::TimerManager timerManager{this};

    Application * app;

protected:
    virtual int numInitStages() const override;
    virtual void initialize(int stage) override;
    virtual void handleStartOperation(inet::LifecycleOperation* doneCallback) override;
    virtual bool startApplication();
    virtual bool stopApplication();
    virtual void handleStopOperation(inet::LifecycleOperation* doneCallback) override;
    virtual void handleCrashOperation(inet::LifecycleOperation* doneCallback) override;
    virtual void finish() override;

    virtual void refreshDisplay() const override;
    virtual void handleMessageWhenUp(inet::cMessage* msg) override;

    virtual void socketDataArrived(inet::UdpSocket* socket, inet::Packet* packet) override;
    virtual void socketErrorArrived(inet::UdpSocket* socket, inet::Indication* indication) override;
    virtual void socketClosed(inet::UdpSocket* socket) override;

    virtual std::unique_ptr<inet::Packet> createPacket(std::string name);
    virtual void processPacket(std::shared_ptr<inet::Packet> pk);
    virtual void timestampPayload(inet::Ptr<inet::Chunk> payload);
    virtual void sendPacket(std::unique_ptr<inet::Packet> pk);

public:
    StaticApplication();
    virtual ~StaticApplication();
};

#endif
