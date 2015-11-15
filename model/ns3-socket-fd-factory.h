#ifndef NS3_SOCKET_FD_FACTORY_H
#define NS3_SOCKET_FD_FACTORY_H

#include "socket-fd-factory.h"
#include <ns3/socket.h>

namespace ns3 {

class SocketFactory;

class Ns3SocketFdFactory : public SocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  Ns3SocketFdFactory ();
  void NotifyNewAggregate (void);

  virtual UnixFd * CreateSocket (int domain, int type, int protocol);

private:
  Callback<void, Ptr<Socket> > m_onTcpConnect;
  Callback<void, Ptr<Socket>, const Address& > m_onSocketCreation;
  Ptr<SocketFactory> m_netlink;
};

} // namespace ns3

#endif /* NS3_SOCKET_FD_FACTORY_H */
