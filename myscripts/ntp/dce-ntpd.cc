#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/clock-perfect.h"

//#include "../model/ipv4-dce-routing.h"
#include "ns3/ipv4-dce-routing-helper.h"

#define ENABLE_NTPD 1
#define ENABLE_NTIMED

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceNtpd");
// ===========================================================================
//
//     node 0 (client)       node 1 (server)
//   +----------------+    +----------------+
//   |                |    |                |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
// 2 nodes : NTP client (node 0) en NTP server (node 1)
// Client may run NTIMED
//
// Server may run NTPD (tested) or openntpd or chronyd (untested yet)
//Before launching this program, I advise you to create symlinks in dce/build/bin towards the
//different binaires
// ===========================================================================
int main (int argc, char *argv[])
{
  std::string stack = "ns3";
  bool useDebug = true;
  std::string bandWidth = "1m";
  Time simDuration = Seconds(10);


  std::string time_server_binary = "ntpd";
//  std::string time_server_binary = "chronyd";

  CommandLine cmd;
  cmd.AddValue ("stack", "Name of IP stack: ns3/linux/freebsd.", stack);
  cmd.AddValue ("debug", "Debug. Default true (0)", useDebug);
  cmd.AddValue ("bw", "BandWidth. Default 1m.", bandWidth);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  Ptr<Node> nClient = nodes.Get (0);
  Ptr<Node> nServer = nodes.Get (1);



  /// Addition from :YcmDiags
  //

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices, devices2;
  devices = pointToPoint.Install (nodes);
//  devices2 = pointToPoint.Install (nodes);

  DceManagerHelper dceManager;
  Ipv4DceRoutingHelper ipv4RoutingHelper;

  dceManager.SetTaskManagerAttribute ("FiberManagerType", StringValue ("UcontextFiberManager"));

  if (stack == "ns3")
    {
      InternetStackHelper stack;

      stack.SetRoutingHelper (ipv4RoutingHelper);
//      stack.Install (nodes);

      stack.Install (nodes);
      dceManager.Install (nodes);
    }
  else if (stack == "linux")
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      dceManager.Install (nodes);
      LinuxStackHelper stack;
      stack.Install (nodes);
#else
      NS_LOG_ERROR ("Linux kernel stack for DCE is not available. build with dce-linux module.");
      // silently exit
      return 0;
#endif
    }
  else if (stack == "freebsd")
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::FreeBSDSocketFdFactory", "Library", StringValue ("libfreebsd.so"));
      dceManager.Install (nodes);
      FreeBSDStackHelper stack;
      stack.Install (nodes);
#else
      NS_LOG_ERROR ("FreeBSD kernel stack for DCE is not available. build with dce-freebsd module.");
      // silently exit
      return 0;
#endif
    }

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);
// For now let's stay monohomed
//  address.SetBase ("10.1.2.0", "255.255.255.252");
//  interfaces = address.Assign (devices2);

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
#ifdef KERNEL_STACK
  if (stack == "linux")
    {
      LinuxStackHelper::PopulateRoutingTables ();
    }
#endif

  /**
   Here we configure clocks:
  - TODO a defective one (either with lower or faster frequency) on the client
  - a nearly perfect one on the server (abs time + a random small offset). We needed
  to add a random offset

  install a defective clock on that node
  */
  // Pour l'instnat le client on s'en fout
//  nClient->SetClock()
  //!
  Ptr<ClockPerfect> serverClock = CreateObject<ClockPerfect>();
//  serverClock->SetMaxRandomOffset(200);
//  serverClock->SetTime(200);
//  nServer->SetClock( serverClock );
//  NS_LOG_UNCOND("");
  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch ntp client on node 0
#ifdef ENABLE_NTIMED
  dce.SetBinary ("ntimed-client");

  // TODO install a defective clock on that node

  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("--poll-server");
  // address of the NTP server
  dce.AddArgument ("10.1.1.2");
//  dce.AddArgument ("-i");
//  dce.AddArgument ("1");
//  dce.AddArgument ("--time");
//  dce.AddArgument ("10");
//  if (useDebug)
//    {
//      dce.AddArgument ("-u");
//      dce.AddArgument ("-b");
//      dce.AddArgument (bandWidth);
//    }

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.7));
  apps.Stop (Seconds (20));
#endif

  // Launch ntp server on node 1
  #ifdef ENABLE_NTPD
  dce.SetBinary ("ntpd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("/tmp/ntp.conf");
  #else
  dce.SetBinary ("chronyd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-f");
  dce.AddArgument ("/tmp/chrony.conf");


  #endif

  uid_t root_uid = 0;
  dce.SetEuid(root_uid);
  dce.SetUid(root_uid);

  if(useDebug) {
    // -dddd to increase log level
    dce.AddArgument("-dddd");
  }
//  dce.AddArgument ("/home/teto/dce/myscripts/ntp.conf");

  // will block the server, don't uncomment
//  dce.AddArgument ("-n"); // -n => don't fork
//  dce.AddArgument ("1");
//  if (useDebug)
//    {
//      dce.AddArgument ("-u");
//    }

  apps = dce.Install (nServer);



//  dceRoutingHelper.Create(nServer);

  pointToPoint.EnablePcapAll ("ntp-" + stack, false);

  apps.Start (Seconds (0.6));

//  setPos (nodes.Get (0), 1, 10, 0);
//  setPos (nodes.Get (1), 50,10, 0);

  Simulator::Stop (simDuration);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
