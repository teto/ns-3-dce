#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
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


/**
TODO Enable mptcp
Can't find route
Creates 2 nodes (client/server) and nRtrs routers

in this configuration, server is ns3 and client is kernel
afficher une topologie
**/
int main (int argc, char *argv[])
{
  uint32_t nRtrs = 2;
  const Time simMaxDuration = Seconds(200);
  CommandLine cmd;
  cmd.AddValue ("nRtrs", "Number of routers. Default 2", nRtrs);
  cmd.Parse (argc, argv);

  NodeContainer nodes, routers;
  nodes.Create (2);
  Ptr<Node> clientNode = nodes.Get(0);
  Ptr<Node> serverNode = nodes.Get(1);
  routers.Create (nRtrs);

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType",
                                      StringValue ("UcontextFiberManager"));

  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));
  LinuxStackHelper linuxStack;
  linuxStack.Install (clientNode);
  linuxStack.Install (routers);



  InternetStackHelper nsStack;
//  Ipv4DceRoutingHelper ipv4DceRoutingHelper;
//  nsStack.SetRoutingHelper (ipv4DceRoutingHelper);
  nsStack.Install (serverNode);

  dceManager.Install (nodes);
  dceManager.Install (routers);

  PointToPointHelper pointToPoint;
  NetDeviceContainer devices1, devices2;
  Ipv4AddressHelper address1, address2;
  std::ostringstream cmd_oss;
  address1.SetBase ("10.1.0.0", "255.255.255.0");
  address2.SetBase ("10.2.0.0", "255.255.255.0");

//  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<IpProgramDceRouting> serverRouting = Create<IpProgramDceRouting>();
  Ptr<Ipv4> ipv4Server = serverNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG(ipv4Server , "node does not have ipv4");
  serverRouting->SetIpv4(ipv4Server);
//  Ptr<Ipv4StaticRouting> serverRouting = ipv4RoutingHelper.GetStaticRouting (ipv4Server);
//  NS_ASSERT_MSG(serverRouting, "wrong static routing");

  // configure routers
  for (uint32_t i = 0; i < nRtrs; i++)
    {
      // Left link (from client to routers)
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
      pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
      devices1 = pointToPoint.Install (clientNode, routers.Get (i));
      // Assign ip addresses
      Ipv4InterfaceContainer if1 = address1.Assign (devices1);
      address1.NewNetwork ();
      // setup ip routes
      cmd_oss.str ("");
      cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << (i+1);
      LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.1." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
      LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add default via " << if1.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
      LinuxStackHelper::RunIp (clientNode, Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.1.0.0/16 via " << if1.GetAddress (1, 0) << " dev sim0";
      LinuxStackHelper::RunIp (routers.Get (i), Seconds (0.2), cmd_oss.str ().c_str ());

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
      cmd_oss << "10.2." << i << "0";
//      cmd_oss << "route add 10.2." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
//      LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
//        Simulator::Schedule(Seconds (0.2), IpProgramDceRouting::AddNetworkRouteTo, serverRouting,
        serverRouting->AddNetworkRouteTo(
                        Ipv4Address(cmd_oss.str().c_str()),
                        Ipv4Mask("/24"),
                        i
                        );


//      cmd_oss.str ("");
//      cmd_oss << "route add default via " << if2.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
//      LinuxStackHelper::RunIp (serverNode, Seconds (0.1), cmd_oss.str ().c_str ());
//        Simulator::Schedule(Seconds (0.2), Ipv4StaticRouting::SetDefaultRoute, serverRouting,
        serverRouting->SetDefaultRoute(
                    if2.GetAddress (1, 0),
                    i
                        );
//      cmd_oss.str ("");
//      cmd_oss << "route add 10.2.0.0/16 via " << if2.GetAddress (1, 0) << " dev sim1";
//      LinuxStackHelper::RunIp (routers.Get (i), Seconds (0.2), cmd_oss.str ().c_str ());
    // TODO this should be delayed/scheduled to let the kernel
//      Simulator::Schedule(Seconds (0.2), Ipv4StaticRouting::AddNetworkRouteTo, serverRouting,
      serverRouting->AddNetworkRouteTo(
                    Ipv4Address("10.2.0.0"), // network
                    Ipv4Mask("/16"),         // mask
                    if2.GetAddress (1, 0),    // nextHop
                    1,                        // interface
                    0                       // metric (optional)
                    );
//  virtual void AddNetworkRouteTo (Ipv4Address network,
//                          Ipv4Mask networkMask,
//                          Ipv4Address nextHop,
//                          uint32_t interface,
//                          uint32_t metric = 0);

      setPos (routers.Get (i), 50, i * 20, 0);
    }


  // default route
  LinuxStackHelper::RunIp (clientNode, Seconds (0.1), "route add default via 10.1.0.2 dev sim0");
  LinuxStackHelper::RunIp (clientNode, Seconds (0.1), "rule show");

  // TODO change that with ns3 command
//  LinuxStackHelper::RunIp (serverNode, Seconds (0.1), "route add default via 10.2.0.2 dev sim0");

//  clientNode->GetObject<Ipv4>

  // nextHop/interface/metric
  serverRouting->SetDefaultRoute (Ipv4Address ("10.2.0.2"), 0, 1);
  // Schedule Up/Down (XXX: didn't work...)
  // if we do that then server should be DCE
//  LinuxStackHelper::RunIp (serverNode, Seconds (1.0), "link set dev sim0 multipath off");
//  LinuxStackHelper::RunIp (serverNode, Seconds (15.0), "link set dev sim0 multipath on");
//  LinuxStackHelper::RunIp (serverNode, Seconds (30.0), "link set dev sim0 multipath off");

  // debug
  linuxStack.SysctlSet (nodes, ".net.mptcp.mptcp_debug", "1");


  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.2.0.1");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("100");

  apps = dce.Install (clientNode);
  apps.Start (Seconds (5.0));
  apps.Stop (simMaxDuration);

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");
  apps = dce.Install (serverNode);

  pointToPoint.EnablePcapAll ("mixed-mptcp", false);

  apps.Start (Seconds (4));

  setPos (clientNode, 0, 20 * (nRtrs - 1) / 2, 0);
  setPos (serverNode, 100, 20 * (nRtrs - 1) / 2, 0);

  Simulator::Stop (simMaxDuration);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
