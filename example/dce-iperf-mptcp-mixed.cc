#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/log.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ip-program-dce-routing.h"
#include "ns3/constant-position-mobility-model.h"
#include <algorithm>

/**
unedefine XP  if you want to compile this test with ns3 master
**/
#define XP
#ifdef XP
#include "ns3/mptcp-scheduler.h"
#include "ns3/tcp-trace-helper.h"
#endif

/*

*/
#define IPERF3

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceIperfMpTcpMixed");


/* global since it is needed
 * in the path manager
 */
NodeContainer routers;
Ptr<Node> clientNode;
Ptr<Node> serverNode;

/* global config */
std::string scheduler = "default";
std::string congestionAlg = "lia";
std::string windowSize = "120KB";

// for good simulations put a longer duration here
const std::string iperfDuration =  "10";

/**
TODO write a path manager in case it is an ns3 client

**/

#ifdef XP
void
onSubflowEstablishement(Ptr<MpTcpSubflow> subflow)
{
    //!
    NS_LOG_UNCOND("Subflow connected");
    NS_ASSERT(subflow);
    static int counter = 0;
    std::ostringstream oss;
    oss << "client/subflow" << counter++;
    subflow->SetupTracing(oss.str());
}

void
onSubflowCreation(Ptr<MpTcpSubflow> subflow)
{
    //!
    NS_LOG_UNCOND("Subflow created");
    NS_ASSERT(subflow);
    static int counter = 0;
    std::ostringstream oss;
    oss << "server/subflow" << counter++;
    subflow->SetupTracing(oss.str());
}
#endif // XP


void
onServerCreation(Ptr<Socket> sock, const Address & from)
{
    NS_LOG_UNCOND("EUREKA  Server created !!");
    #ifdef XP
    //! start tracing
  TcpTraceHelper helper;

  
  Ptr<MpTcpSocketBase> server = DynamicCast<MpTcpSocketBase>(sock);

  /* */
  NS_ASSERT_MSG(server, "The passed socket should be the MPTCP meta socket");

 // TODO enable tracing straightaway
//  helper.SetupSocketTracing(m_metaClient, "client/");
  server->SetupTracing("server/meta");
//MpTcpSocketBase::SetSubflowAcceptCallback(
////  Callback<void, Ptr<MpTcpSubflow> > connectionRequest,
//  Callback<bool, Ptr<MpTcpSocketBase>, const Address &, const Address & > joinRequest,
//  Callback<void, Ptr<MpTcpSubflow> > connectionCreated
//)
    server->SetSubflowAcceptCallback(
        MakeNullCallback<bool, Ptr<MpTcpSocketBase>, const Address &, const Address & >(),
        MakeCallback(&onSubflowCreation)
    );
  // Connect to created subflow
//  server->SetSubflowConnectCallback(
    #endif // XP
}

void
onClientConnect(Ptr<Socket> socket)
{
  NS_LOG_UNCOND("EUREKA Client connected !!");
    #ifdef XP
  TcpTraceHelper helper;

  
  Ptr<MpTcpSocketBase> m_metaClient = DynamicCast<MpTcpSocketBase>(socket);

  /* */
  NS_ASSERT_MSG(m_metaClient, "The passed socket should be the MPTCP meta socket");

 // TODO enable tracing straightaway
//  helper.SetupSocketTracing(m_metaClient, "client/");
  m_metaClient->SetupTracing("client/meta");
  // TODO it should trigger this even for master !
//  m_metaClient->GetSubflow(0)->SetupSocketTracing("client/subflow0")
  m_metaClient->SetSubflowConnectCallback(
//                    MakeBoundCallback(&onSubflowEstablishement, "client/"),
                    MakeCallback(&onSubflowEstablishement),
                    MakeNullCallback<void, Ptr<MpTcpSubflow> >()
                                          );
  //! Enable tracing on new Subflow as always

  // only if fully established can you create new subflows
  if (!m_metaClient->FullyEstablished())
  {
    NS_LOG_UNCOND("Meta not fully established yet !!");
    return;
  }



  NS_LOG_LOGIC("Meta fully established, Creating subflows");
  //! Create additionnal subflows
  //! i starts at 1 because master is already using that path
  for (int i = 1; i < routers.GetN(); ++i)
  {
        //! 'i+1' because 0 is localhost
        Ipv4Address serverAddr = serverNode->GetObject<Ipv4>()->GetAddress(i+1, 0).GetLocal();
        Ipv4Address sourceAddr = clientNode->GetObject<Ipv4>()->GetAddress(i+1, 0).GetLocal();

        //! TODO, we should be able to not specify a port but it seems buggy so for now, let's set a port
      //  InetSocketAddress local( sourceAddr);
        InetSocketAddress local(sourceAddr, 0);
        InetSocketAddress remote(serverAddr, 5001);

        NS_LOG_LOGIC("ConnectNewSubflow from " << local << " to " << remote);
        m_metaClient->ConnectNewSubflow(local, remote);
  }
  #endif
}



void setPos (Ptr<Node> n, int x, int y, int z)
{
  Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
  n->AggregateObject (loc);
  Vector locVec2 (x, y, z);
  loc->SetPosition (locVec2);
}


enum StackType {
STACK_FREEBSD = 0,
STACK_NS,
STACK_LINUX
};

static const char* StackTypesStr[] = {
"FreeBSD",
"ns3",
"Linux"
};


bool setupStackType(enum StackType *val, std::string newVal)
{
    // TODO get the typeids child of
    NS_LOG_UNCOND("newVal=" << newVal);
    if(newVal == "ns") {
        *val = STACK_NS;
    }
    else if(newVal == "linux") {
        *val = STACK_LINUX;
    }
    else {
        return false;
    }

    NS_LOG_UNCOND("Stack type after arg processing=" << StackTypesStr[(int)*val]);
    return true;
}

Ptr<Ipv4StaticRouting>
GetRouting(Ptr<Node> node, enum StackType type)
{
  Ptr<Ipv4StaticRouting> routing;
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  NS_ASSERT_MSG(ipv4 , "node does not have ipv4");

  if(type == STACK_LINUX) {

    routing = Create<IpProgramDceRouting>();
    routing->SetIpv4(ipv4);
  }
  else if(type == STACK_NS) {

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
  //  Ptr<IpProgramDceRouting> serverRouting = Create<IpProgramDceRouting>();
  //  Ptr<Ipv4StaticRouting> serverRouting = Create<IpProgramDceRouting>();


//    serverRouting->SetIpv4(ipv4Server);
    routing = ipv4RoutingHelper.GetStaticRouting (ipv4);


  }
  else {
    NS_FATAL_ERROR("Unsupported stack (at the moment)");
  }
  NS_ASSERT(routing);
  return routing;
}

//Ipv4DceRouting
// ipv4 static routing
void
PrintRouterTable(Ptr<Ipv4DceRouting> routing, Ptr<OutputStreamWrapper> stream)
{
  routing->PrintRoutingTable(stream);
}
//NS_LOG_COMPONENT_DEFINE ("DceMpTcpHybrid");

void
setupNsNodes(NodeContainer nodes)
{
    #ifdef XP
//    TypeId sched, algTypeId;
    std::string sched_name;
    std::string alg_name;

    // Setup scheduler
    if(scheduler == "roundrobin") {
        sched_name= "ns3::MpTcpSchedulerRoundRobin";
    }
    else if(scheduler == "default") {
        sched_name="ns3::MpTcpSchedulerFastestRTT";
    }
    else {
        // TODO replace this check by one of the following one
        NS_FATAL_ERROR("invalid scheduler");
    }

    // TODO che
//    TypeId sched(scheduler)
//    sched.IsChildOf(MpTcpScheduler::GetTypeId());
//    NS_ASSERT_MSG( );

    // Setuup congestion control
    // Fix this !
    if(congestionAlg == "lia") {
        alg_name= "ns3::MpTcpSchedulerRoundRobin";
    }
//    else if(scheduler == "olia") {
//        alg_name="ns3::MpTcpSchedulerFastestRTT";
//    }
    else {
        // TODO replace this check by one of the following one
        NS_FATAL_ERROR("invalid congestion algorithm");
    }


    TypeId schedulerTypeId = TypeId::LookupByName(sched_name);
    TypeId algTypeId       = TypeId::LookupByName(alg_name);

    Config::SetDefault ("ns3::MpTcpSocketBase::Scheduler", TypeIdValue(schedulerTypeId));
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(algTypeId));
    #endif
}

void
setupLinuxNodes(LinuxStackHelper& linuxStack, NodeContainer nodes)
{

  linuxStack.SysctlSet ( nodes, ".net.mptcp.mptcp_scheduler", scheduler);
  linuxStack.SysctlSet ( nodes, ".net.ipv4.tcp_congestion_control", congestionAlg);
  // Buffer size allocated by iperf
//  linuxStack.SysctlSet ( nodes, "net.ipv4.tcp_rmem", "4096        87380   6291456");
//  linuxStack.SysctlSet ( nodes, "net.ipv4.tcp_mem", "4096        87380   6291456");
//  net.ipv4.tcp_mem = 187320       249763  374640
}

/**
TODO Enable mptcp
Can't find route
Creates 2 nodes (client/server) and nRtrs routers

in this configuration, server is ns3 and client is kernel
afficher une topologie

iperf2 or 3 test


TOPOLOGY:


Client (10.1.X.1, in files-0)  ------------- (10.1.X.2) nbRtrs Routers (10.2.X.2) -------- ((10.2.X.1) Server (in files-1)


SO_SNDBUF
SO_RCVBUF
**/
int main (int argc, char *argv[])
{
  uint32_t nRtrs = 1;   //! For now set only one router
  const uint32_t nMaxRtrs = 3;  //! max nb of routers
  const Time simMaxDuration = Seconds(200);


  CommandLine cmd;

  enum StackType client_stack = STACK_NS;
  enum StackType server_stack = STACK_NS;
  enum StackType router_stack = STACK_LINUX;

  /* times in milliseconds forward/backward */
  int forwardOwd[] = { 30,30,30 };
  int backwardOwd[] = { 30,30,30 };

  Ptr<Ipv4StaticRouting> serverRouting;
  Ptr<Ipv4StaticRouting> clientRouting;

  cmd.AddValue ("nRtrs", "Number of routers. Default 2", nRtrs);
  cmd.AddValue ("client_stack", "Clientstack. Default ", MakeBoundCallback(&setupStackType, &client_stack) );
  cmd.AddValue ("server_stack", "ServerStack. Default ", MakeBoundCallback(&setupStackType, &server_stack));
  cmd.AddValue ("scheduler", "bufferSize. Default ", scheduler);
  cmd.AddValue ("congestion", "congestion control. Default ", congestionAlg);
  cmd.AddValue ("window", "iperf --window parameter", windowSize);

  std::ostringstream oss;
  for ( uint32_t i = 0; i < nMaxRtrs; ++i) {
    oss.str("");
    oss << "forwardDelay" << i;
    //MakeBoundCallback(&setupStackType, &server_stack)
    cmd.AddValue ( oss.str(), "Forward delay ", forwardOwd[i]);
    oss.str("");
    oss << "backwardDelay" << i;
    cmd.AddValue ( oss.str(), "Backward delay ", backwardOwd[i]);
  }
  // TODO being able to configure Scheduler and congestion control
  /******************************************
  *** WARNING: For some reason, changing resolution with SetResolution 
   breaks tests
   ***/
//  Time::SetResolution (Time::MS);

//  Config::SetDefault ("ns3::MpTcpSocketBase::Scheduler", BooleanValue(true));
  #ifdef XP
  Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue(true));
  Config::SetDefault ("ns3::TcpSocketBase::NullISN", BooleanValue(false));
  #endif

  /**
  WARNING
   lowered segment size else I had ip fragmentation with wireshark bugs (have to make the dissector more robust)
  is that taken into account by DCE+linux ?
  **/
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  // choose congestion control
  // Setup node
  /**
   In other words, the more statistically rigorous way 
   to configure multiple independent replications is to use a fixed seed and to advance the run number. 
   https://www.nsnam.org/docs/manual/html/random-variables.html
  **/
  RngSeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
//  RngSeedManager::SetRun (7);   // Changes run number from default of 1 to 7
  NS_LOG_INFO("Seed=" << RngSeedManager::GetSeed());
  NS_LOG_INFO("Run=" << RngSeedManager::GetRun());
  /** 
  TODO for good simulations, you need to average the results of different runs with different seeds.
  For reproducibility, put the seed into a file.
  for instance use SetSeed()
  **/
//  ns3::RngSeedManager::SetSeed();
/**
if(clientStack == "ns3") etc...
**/
  cmd.Parse (argc, argv);

  // we limit to 3 routers
  nRtrs = std::min( (uint32_t)3, nRtrs);
  NodeContainer nodes;
  nodes.Create (2);
  clientNode = nodes.Get(0);
  serverNode = nodes.Get(1);
  routers.Create (nRtrs);

  /* TODO depending on command line arguments, load put nodes into these containers */
  NodeContainer linuxStackNodes, nsStackNodes;
//  linuxStackNodes.Add(routers);
  if(client_stack == STACK_LINUX) {
    linuxStackNodes.Add(clientNode);
  }
  else {
    nsStackNodes.Add(clientNode);
  }

  if(server_stack == STACK_LINUX) {
    linuxStackNodes.Add(serverNode);
  }
  else {
    nsStackNodes.Add(serverNode);
  }

  if(router_stack == STACK_LINUX) {
    linuxStackNodes.Add(routers);
  }
  else {
    nsStackNodes.Add(routers);
  }

  NS_LOG_UNCOND("Nb of routers => " << nRtrs);
  NS_LOG_UNCOND("Client stack => " << StackTypesStr[(int)client_stack]);
  NS_LOG_UNCOND("Server stack => " << StackTypesStr[(int)server_stack]);
  NS_LOG_UNCOND("Router stack => " << StackTypesStr[(int)router_stack]);
  NS_LOG_UNCOND("forwardOwd[0] => " << forwardOwd[0]);
  NS_LOG_UNCOND("backwardOwd[0] => " << backwardOwd[0]);

//  NS_ASSERT(serverRouting);

//  NodeContainer nsStacks;

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType",
                                      StringValue ("UcontextFiberManager"));

  /** Install the linux stack **/
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));
  LinuxStackHelper linuxStack;
  // TODO don't install it there
//  linuxStack.Install (clientNode);
  linuxStack.Install (linuxStackNodes);



  dceManager.Install (linuxStackNodes);
  setupLinuxNodes(linuxStack,linuxStackNodes);

  /** install in nsStacks **/
  dceManager.SetNetworkStack ("ns3::Ns3SocketFdFactory");
  
  dceManager.SetNetworkStackAttribute("OnTcpConnect", CallbackValue(MakeCallback(&onClientConnect)));
  // TODO do the same for server on ConnectionCreated !
  dceManager.SetNetworkStackAttribute("OnSocketCreation", CallbackValue(MakeCallback(&onServerCreation)));
  
  
  InternetStackHelper nsStack;
//  Ipv4DceRoutingHelper ipv4DceRoutingHelper;
//  nsStack.SetRoutingHelper (ipv4DceRoutingHelper);
  nsStack.Install (nsStackNodes);
  dceManager.Install (nsStackNodes);
  setupNsNodes(nsStackNodes);

  // TODO la il installe

  PointToPointHelper pointToPoint;
  NetDeviceContainer devices1, devices2;
  Ipv4AddressHelper address1, address2;
  std::ostringstream cmd_oss;
  address1.SetBase ("10.1.0.0", "255.255.255.0");
  address2.SetBase ("10.2.0.0", "255.255.255.0");

//  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  /** TODO
  Looking at convergence between DCE and ns3 to allow for seamless transitions
  Depending on the stack type, we setup a different static routing type
  **/
  serverRouting = GetRouting(serverNode, server_stack);
  clientRouting = GetRouting(clientNode, client_stack);
  Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>("rttables", std::ios::out);

  // configure routers
  for (uint32_t i = 0; i < nRtrs; i++)
    {
      NS_LOG_UNCOND("Setup for router #" << i);
      Ptr<Ipv4StaticRouting> routerRouting;
      Ptr<Node> routerNode = routers.Get (i);

      // Left link (from client to routers)
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));

      pointToPoint.SetChannelAttribute ("Delay", TimeValue( MilliSeconds(forwardOwd[i])) );
      #ifdef XP
      pointToPoint.SetChannelAttribute ("AlternateDelay", TimeValue( MilliSeconds(backwardOwd[i])));
      #endif
      devices1 = pointToPoint.Install (clientNode, routerNode);


       TimeValue t;
        devices1.Get(0)->GetChannel()->GetAttribute("Delay", t);
        std::cout << "BEBE=" << t.Get().As(Time::MS) << std::endl;
        #ifdef XP
        devices1.Get(0)->GetChannel()->GetAttribute("AlternateDelay", t);
        std::cout << t.Get().As(Time::MS) << std::endl;
        #endif


      // Assign ip addresses
      Ipv4InterfaceContainer if1 = address1.Assign (devices1);
      address1.NewNetwork ();
      /* setup ip routes between client and router */
      int clientInterface = if1.Get(0).second;  // instead of i
      int routerIf = if1.Get(1).second;  // instead of i

      switch(client_stack) {
      case STACK_LINUX:
          cmd_oss.str ("");
          cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << (i+1);
          LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
          cmd_oss.str ("");
          cmd_oss << "route add 10.1." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
          LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
          cmd_oss.str ("");
          cmd_oss << "route add default via " << if1.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
          LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
          break;

      case STACK_NS:
          cmd_oss.str ("");
          cmd_oss << "10.1." << i << ".0";
          clientRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()), Ipv4Mask("/24"), clientInterface);
          cmd_oss.str ("");
//      cmd_oss << "route add default via " << if1.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
//      LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
          clientRouting->SetDefaultRoute(if1.GetAddress (1, 0), clientInterface);
          break;

        default:
            NS_FATAL_ERROR("Unsupported stack");
      };

      /* setup routers routing router always runs linux*/
      cmd_oss.str ("");
      cmd_oss << "route add 10.1."<< i <<".0/24 via " << if1.GetAddress (1, 0) << " dev sim0";
      LinuxStackHelper::RunIp (routerNode, Seconds (0.2), cmd_oss.str ().c_str ());
//      Ptr<Ipv4> ipv4Router = serverNode->GetObject<Ipv4> ();
//      routerRouting = GetRouting(routerNode, router_stack);


//      cmd_oss.str ("");
//      cmd_oss << "10.1." << i << ".0";
//      routerRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()), Ipv4Mask("/24"), routerIf);


      // Right link (from server to routers)
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
      pointToPoint.SetChannelAttribute ("Delay", TimeValue( MilliSeconds(1)) );
      #ifdef XP
      pointToPoint.SetChannelAttribute ("AlternateDelay", TimeValue( MilliSeconds(1)));
      #endif
      devices2 = pointToPoint.Install (serverNode, routerNode);

      devices2.Get(0)->GetChannel()->GetAttribute("Delay", t);
      std::cout << "BIBI=" << t.Get().As(Time::MS) << std::endl;
      #ifdef XP
      devices2.Get(0)->GetChannel()->GetAttribute("AlternateDelay", t);
      std::cout << t.Get().As(Time::MS) << std::endl;
      #endif

      // Assign ip addresses
      Ipv4InterfaceContainer if2 = address2.Assign (devices2);
      address2.NewNetwork ();


      int serverIf = if2.Get(0).second;
      routerIf = if2.Get(1).second;
      

      /* setup ip routes between router and server */
      switch(server_stack)
      {
        case STACK_LINUX:
          cmd_oss.str ("");
          cmd_oss << "rule add from " << if2.GetAddress (0, 0) << " table " << (i+1);
          LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
          cmd_oss.str ("");
          cmd_oss << "route add 10.2." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
          LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
          cmd_oss.str ("");
          cmd_oss << "route add default via " << if2.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
          LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
          break;

        case STACK_NS:
    //      cmd_oss.str ("");
    //      cmd_oss << "route add default via " << if2.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
    //      LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
            cmd_oss.str ("");
            cmd_oss << "10.2." << i << ".0";
            serverRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()), Ipv4Mask("/24"), serverIf);
            serverRouting->SetDefaultRoute(if2.GetAddress (1, 0), serverIf);
            break;
        default:
            NS_FATAL_ERROR("Unsupported stack");
      };


    /*  */
    cmd_oss.str ("");
    cmd_oss << "route add 10.2."<< i <<".0/24 via " << if2.GetAddress (1, 0) << " dev sim1";
    LinuxStackHelper::RunIp (routerNode, Seconds (0.2), cmd_oss.str ().c_str ());



     #if 0
      cmd_oss.str ("");
      cmd_oss << "10.2." << i << ".0";
      serverRouting->AddNetworkRouteTo(
                    Ipv4Address(cmd_oss.str().c_str()), // network
                    Ipv4Mask("/24"),         // mask
                    if2.GetAddress (1, 0),    // nextHop
                    1,                        // interface
                    0                       // metric (optional)
                    );
      #endif
//      routerRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()),Ipv4Mask("/24"), routerIf);


      /*
      That won't work like this, need to use ipv4 dce routing
      */

      LinuxStackHelper::RunIp ( routerNode, Seconds (3), "route");
      linuxStack.SysctlSet (routerNode, ".net.ipv4.conf.default.forwarding", "1");
      setPos (routers.Get (i), 50, i * 20, 0);
    }

  #if 0
  // gives layer 2 information
  for( uint32_t n =0; n < clientNode->GetNDevices(); n++){
    Ptr<NetDevice> dev = clientNode->GetDevice(n);
    NS_LOG_UNCOND( " dev #" << n << "=" << dev << " GetIfIndex=" << dev->GetIfIndex()
            << "\naddress=" << Ipv4Address::IsMatchingType(dev->GetAddress())
            << "\n" << dev->GetInstanceTypeId().GetName()
            );
  }
  #endif
  
  if(server_stack == STACK_LINUX) {
    LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), "route add default via 10.2.0.2 dev sim0");
  }
  if(client_stack == STACK_LINUX) {
    LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "route add default via 10.1.0.2 dev sim0");
    LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "rule show");
  }
//  std::ostringstream stream;
//  *stream->GetStream() << "Client " << std::endl;
//  clientRouting->PrintRoutingTable(stream);
//  *stream->GetStream() << "Router " << std::endl;
//  serverRouting->PrintRoutingTable(stream);

//  for( uint32_t n =0; n < ipv4client->GetNInterfaces(); n++){
//    for( uint32_t a=0; a < ipv4client->GetNAddresses(n); a++){
//        NS_LOG_UNCOND( "Client addr " << n <<"/" << a << "=" << ipv4client->GetAddress(n,a));
//    }
//  }
//
//  for( uint32_t n =0; n < ipv4Server->GetNInterfaces(); n++){
//    for( uint32_t a=0; a < ipv4Server->GetNAddresses(n); a++){
//        NS_LOG_UNCOND( "Server  addr " << n <<"/" << a << "=" << ipv4Server->GetAddress(n,a));
//    }
//  }

  // default route
//  LinuxStackHelper::RunIp (clientNode, Seconds (0.1), "route add default via 10.1.0.2 dev sim0");
//  LinuxStackHelper::RunIp (clientNode, Seconds (0.1), "rule show");

  // TODO change that with ns3 command
//  LinuxStackHelper::RunIp (serverNode, Seconds (0.1), "route add default via 10.2.0.2 dev sim0");

//  clientNode->GetObject<Ipv4>

  // nextHop/interface/metric
//  serverRouting->SetDefaultRoute (Ipv4Address ("10.2.0.2"), 0, 1);

  // Schedule Up/Down (XXX: didn't work...)
  // if we do that then server should be DCE
//  LinuxStackHelper::RunIp (serverNode, Seconds (1.0), "link set dev sim0 multipath off");
//  LinuxStackHelper::RunIp (serverNode, Seconds (15.0), "link set dev sim0 multipath on");
//  LinuxStackHelper::RunIp (serverNode, Seconds (30.0), "link set dev sim0 multipath off");

  // debug

  linuxStack.SysctlSet (linuxStackNodes, ".net.mptcp.mptcp_debug", "1");
/* The four values in printk denote: 
  console_loglevel, default_message_loglevel, minimum_console_loglevel and default_console_loglevel respectively.
  */
  linuxStack.SysctlSet ( linuxStackNodes, ".kernel.printk", "0 4 8 1");
  // for the router
//  net.ipv4.conf.all.forwarding

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);
  #ifdef IPERF3
  // Launch iperf client on node 0
  dce.SetBinary ("iperf3");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.2.0.1");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("100");
  dce.AddArgument ("-V");   // verbose
  dce.AddArgument ("-J");   // Export to Json
  dce.AddArgument ("--logfile=client.res");  // into this file
//  dce.AddArgument ("-P");   // number of streams to run in parallel
//  dce.AddArgument ("1");

  apps = dce.Install ( clientNode );
  apps.Start (Seconds (5.0));
  apps.Stop (simMaxDuration);

  // Launch iperf server on node 1
  dce.SetBinary ("iperf3");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-V");   // verbose
  dce.AddArgument ("-J");   // verbose
  dce.AddArgument ("--logfile=server.res");  // into this file
  dce.AddArgument ("-s");   // server
  apps = dce.Install ( serverNode );
  #else
  /* By default iperf2 listens on port 5001

  */
  oss.str("");
  oss << "--window=" << windowSize;

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.2.0.1");
  dce.AddArgument ("--format=m"); // m  stands for Mbps / M for MBps
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument (iperfDuration);
  dce.AddArgument ("--bind=10.1.0.1");  // TODO get address programmatacilly from clientNode
  dce.AddArgument ("--reportstyle=C");  // To export as CSV
  dce.AddArgument ( oss.str());   // size of Rcv or send buffer
  // TODO use --format to choose output

  apps = dce.Install ( clientNode );
  


  apps.Start (Seconds (5.0));
  apps.Stop (simMaxDuration);

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("--bind=10.2.0.1");  // TODO get address programmatacilly from clientNode
  dce.AddArgument ("--parallel=1");
//  dce.AddArgument ("-P");
//  dce.AddArgument ("1");
  dce.AddArgument ( oss.str());   // size of Rcv or send buffer
  apps = dce.Install ( serverNode );
  #endif
  /* use the same name as dce-iperf-mptcp to ease postprocessing */
  pointToPoint.EnablePcapAll ("iperf-mptcp", false);

  apps.Start (Seconds (4));

  setPos (clientNode, 0, 20 * (nRtrs - 1) / 2, 0);
  setPos (serverNode, 100, 20 * (nRtrs - 1) / 2, 0);

  Simulator::Stop (simMaxDuration);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
