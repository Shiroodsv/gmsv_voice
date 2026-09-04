#pragma once

class INetChannel;

class INetChannelHandler {
public:
    virtual ~INetChannelHandler() = default;
    virtual void ConnectionStart(INetChannel* channel) = 0;
    virtual void ConnectionClosing(const char* reason) = 0;
    virtual void ConnectionCrashed(const char* reason) = 0;
    virtual void PacketStart(int incomingSequence, int outgoingAcknowledged) = 0;
    virtual void PacketEnd() = 0;
    virtual void FileRequested(const char* fileName, unsigned int transferId) = 0;
    virtual void FileReceived(const char* fileName, unsigned int transferId) = 0;
    virtual void FileDenied(const char* fileName, unsigned int transferId) = 0;
    virtual void FileSent(const char* fileName, unsigned int transferId) = 0;
    virtual bool ShouldAcceptFile(const char* fileName, unsigned int transferId) = 0;
};

class IClient : public INetChannelHandler {
public:
    virtual void Connect(const char* name, int userId, INetChannel* channel, bool fake, int challenge) = 0;
    virtual void Inactivate() = 0;
    virtual void Reconnect() = 0;
    virtual void Disconnect(const char* reason, ...) = 0;
    virtual int GetPlayerSlot() const = 0;
    virtual int GetUserID() const = 0;
};
