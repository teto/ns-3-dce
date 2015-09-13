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

using namespace ns3;
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

bool setupStackType(enum StackType& val, std::string newVal)
{
    // TODO get the typeids child of
    if(newVal == "ns") {
        val = STACK_NS;
    }
    else if(newVal == "linux") {
        val = STACK_LINUX;
    }
    else {
        return false;
    }
    return true;
}


//NS_LOG_COMPONENT_DEFINE ("DceMpTcpHybrid");

/**
TODO Enable mptcp
Can't find route
Creates 2 nodes (client/server) and nRtrs routers

in this configuration, server is ns3 and client is kernel
afficher une topologie
**/
int main (int argc, char *argv[])
{
  uint32_t nRtrs = 1;   //! For now set only one router

  const Time simMaxDuration = Seconds(200);
  CommandLine cmd;
//  std::string clientStack = "";
//  std::string serverStack = "";
  /* by default we use linux stacks */
  //STACK_NS
  enum StackType client_stack = STACK_NS;
  enum StackType server_stack = STACK_NS;
  enum StackType router_stack = STACK_LINUX;
  Ptr<Ipv4StaticRouting> serverRouting;
  Ptr<Ipv4StaticRouting> clientRouting;

  cmd.AddValue ("nRtrs", "Number of routers. Default 2", nRtrs);
  /* TODO choose from an enum */
  cmd.AddValue ("clientStack", "Clientstack. Default ", MakeBoundCallback(&setupStackType, client_stack) );
//  cmd.AddValue ("clientStack", "Clientstack. Default ", clientStack);
  cmd.AddValue ("serverStack", "ServerStack. Default ", MakeBoundCallback(&setupStackType, server_stack));
//  cmd.AddValue ("routerStack", "Number of routers. Default ", nRtrs);

  Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue(true));
/**
if(clientStack == "ns3") etc...
**/
  cmd.Parse (argc, argv);

  NodeContainer nodes, routers;
  nodes.Create (2);
  Ptr<Node> clientNode = nodes.Get(0);
  Ptr<Node> serverNode = nodes.Get(1);
  routers.Create (nRtrs);

  /* TODO depending on command line arguments, load put nodes into these containers */
  NodeContainer linuxStackNodes, nsStackNodes;
  linuxStackNodes.Add(routers);
  if(client_stack == STACK_LINUX) {
    linuxStackNodes.Add(clientNode);
    clientRouting = Create<IpProgramDceRouting>();
  }
  else {
    nsStackNodes.Add(clientNode);
  }

  if(server_stack == STACK_LINUX) {
    linuxStackNodes.Add(serverNode);
    serverRouting = Create<IpProgramDceRouting>();

  }
  else {
    nsStackNodes.Add(serverNode);
    serverRouting = Create<Ipv4StaticRouting>();
  }
  NS_LOG_UNCOND("Nb of routers => " << nRtrs);
  NS_LOG_UNCOND("Client stack => " << StackTypesStr[(int)client_stack]);
  NS_LOG_UNCOND("Server stack => " << StackTypesStr[(int)server_stack]);

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

  /** install in nsStacks **/
  dceManager.SetNetworkStack ("ns3::Ns3SocketFdFactory");


  InternetStackHelper nsStack;
//  Ipv4DceRoutingHelper ipv4DceRoutingHelper;
//  nsStack.SetRoutingHelper (ipv4DceRoutingHelper);
  nsStack.Install (nsStackNodes);
  dceManager.Install (nsStackNodes);


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

//  Ptr<IpProgramDceRouting> serverRouting = Create<IpProgramDceRouting>();
//  Ptr<Ipv4StaticRouting> serverRouting = Create<IpProgramDceRouting>();
  Ptr<Ipv4> ipv4Server = serverNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG(ipv4Server , "node does not have ipv4");
  serverRouting->SetIpv4(ipv4Server);

  Ptr<Ipv4> ipv4client = clientNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG(ipv4client , "node does not have ipv4");
//  clientRouting->SetIpv4(ipv4client);
//    ns3::Ipv4StaticRoutingHelper::GetStaticRouting
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    clientRouting = ipv4RoutingHelper.GetStaticRouting (ipv4client);
//  NS_ASSERT_MSG(serverRouting, "wrong static routing");
//  if(client_stack == STACK_NS) {
//     = clientNode->GetObject<Ipv4StaticRouting>();
    NS_ASSERT(clientRouting);
//  }

  // configure routers
  for (uint32_t i = 0; i < nRtrs; i++)
    {
      NS_LOG_UNCOND("Setup for router #" << i);
      Ptr<Ipv4StaticRouting> routerRouting;

      // Left link (from client to routers)
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
      pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
      devices1 = pointToPoint.Install (clientNode, routers.Get (i));
      // Assign ip addresses
      Ipv4InterfaceContainer if1 = address1.Assign (devices1);
      address1.NewNetwork ();
      // setup ip routes
//      cmd_oss.str ("");
//      cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << (i+1);
//      LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
//      cmd_oss.str ("");
//      cmd_oss << "route add 10.1." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
//      LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
      int clientInterface = if1.Get(0).second;  // instead of i
      cmd_oss.str ("");
      cmd_oss << "10.1." << i << ".0";
      clientRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()), Ipv4Mask("/24"), clientInterface);

      cmd_oss.str ("");
//      cmd_oss << "route add default via " << if1.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
//      LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
      clientRouting->SetDefaultRoute(if1.GetAddress (1, 0), clientInterface);

      /* setup routers routing */

      if(router_stack == STACK_LINUX) {
          linuxStackNodes.Add(serverNode);
          routerRouting = Create<IpProgramDceRouting>();
//          cmd_oss.str ("");
//          cmd_oss << "route add 10.1." << i << ".0/24 via " << if1.GetAddress (1, 0) << " dev sim0";
//          LinuxStackHelper::RunIp (routers.Get (i), Seconds (0.2), cmd_oss.str ().c_str ());
        }
        else {

          routerRouting = Create<Ipv4StaticRouting>();

        }
      Ptr<Ipv4> ipv4Router = serverNode->GetObject<Ipv4> ();
      NS_ASSERT_MSG(ipv4Router, "router node does not have ipv4");
      routerRouting->SetIpv4(ipv4Router);

      cmd_oss.str ("");
      cmd_oss << "10.1." << i << ".0";
      // 0 c localhost ptet ?
      routerRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()), Ipv4Mask("/24"), 0);


      // Right link (from server to routers)
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
      pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ns"));
      devices2 = pointToPoint.Install (serverNode, routers.Get (i));
      // Assign ip addresses
      Ipv4InterfaceContainer if2 = address2.Assign (devices2);
      address2.NewNetwork ();
      // setup ip routes
//      cmd_oss.str ("");
//      cmd_oss << "rule add from " << if2.GetAddress (0, 0) << " table " << (i+1);
//      LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "10.2." << i << ".0";

    //      cmd_oss << "route add 10.2." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
    //      LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
      int serverIf = if2.Get(0).second;
      serverRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()), Ipv4Mask("/24"), serverIf);


    //      cmd_oss.str ("");
    //      cmd_oss << "route add default via " << if2.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
    //      LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
      serverRouting->SetDefaultRoute(if2.GetAddress (1, 0), serverIf);

    //      cmd_oss.str ("");
    //      cmd_oss << "route add 10.2.0.0/16 via " << if2.GetAddress (1, 0) << " dev sim1";
    //      LinuxStackHelper::RunIp (routers.Get (i), Seconds (0.2), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "10.2." << i << ".0";
      serverRouting->AddNetworkRouteTo(
                    Ipv4Address(cmd_oss.str().c_str()), // network
                    Ipv4Mask("/24"),         // mask
                    if2.GetAddress (1, 0),    // nextHop
                    1,                        // interface
                    0                       // metric (optional)
                    );

      routerRouting->AddNetworkRouteTo(Ipv4Address(cmd_oss.str().c_str()),Ipv4Mask("/24"), 1);

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

  for( uint32_t n =0; n < ipv4client->GetNInterfaces(); n++){
    for( uint32_t a=0; a < ipv4client->GetNAddresses(n); a++){
        NS_LOG_UNCOND( "Got addr " << n <<"/" << a << "=" << ipv4client->GetAddress(n,a));
    }
  }


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

//  linuxStack.SysctlSet (linuxStackNodes, ".net.mptcp.mptcp_debug", "1");
//  linuxStack.SysctlSet (linuxStackNodes, ".kernel.printk", "3 4 1 3");

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
  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.2.0.1");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("5");
  dce.AddArgument ("--reportstyle=C");  // To export as CSV

  apps = dce.Install ( clientNode );
  apps.Start (Seconds (5.0));
  apps.Stop (simMaxDuration);

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");
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
