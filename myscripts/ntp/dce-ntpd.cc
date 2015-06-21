#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
//#include "ns3/clock-perfect.h"

//#include "../model/ipv4-dce-routing.h"
#include "ns3/ipv4-dce-routing-helper.h"

#define ENABLE_NTPD 1
#define ENABLE_NTIMED

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceNtpd");


enum NtpServerType {
SERVER_NTPD,
SERVER_CHRONYD,
SERVER_PTPD,
SERVER_OPENNTPD,    //!
NTP_NTIMED
};


NtpServerType serverType = SERVER_NTPD;
NtpServerType clientType = NTP_NTIMED;



bool SetClientType(std::string type) {

    NS_LOG_FUNCTION(type);
    if(type== "ntpd") {
        serverType = SERVER_NTPD;
    }
    else if(type == "chronyd") {
        serverType = SERVER_CHRONYD;
    }
    else if(type == "ptpd") {
        serverType = SERVER_PTPD;
    }
    else if(type == "openntpd") {
        serverType = SERVER_OPENNTPD;
    }
    else if(type == "openntpd") {
        return false;
    }

    return true;
}


bool SetServerType(std::string type) {

    NS_LOG_FUNCTION(type);
    if(type== "ntpd") {
        serverType = SERVER_NTPD;
    }
    else if(type == "chronyd") {
        serverType = SERVER_CHRONYD;
    }
    else if(type == "ptpd") {
        serverType = SERVER_PTPD;
    }
    else if(type == "openntpd") {
        serverType = SERVER_OPENNTPD;
    }
    else {
        return false;
    }

    return true;
}

void SetupProgram(DceApplicationHelper& dce, NtpServerType type, bool server, bool useDebug = true)
{
  NS_LOG_INFO("Setup program of type " << type << " as server == " << server);

  uid_t root_uid = 0;
  dce.SetEuid(root_uid);
  dce.SetUid(root_uid);

  std::string suffix = (server) ? "server" : "client";

  switch(type) {

    case SERVER_CHRONYD:
        NS_LOG_INFO("Setup chronyd");
          dce.SetBinary ("chronyd");
          dce.ResetArguments ();
          dce.ResetEnvironment ();

          dce.AddArgument ("-f");
          dce.AddArgument ("chrony"+suffix+".conf");
          dce.AddArgument ("-4"); //accept only v4
          if(useDebug) {
            dce.AddArgument("-d"); // First to prevent fork
            dce.AddArgument("-d"); // 2nd to enable display message
          }
        break;

    case SERVER_OPENNTPD:
    case SERVER_PTPD:

    case SERVER_NTPD:
          NS_LOG_INFO("Setup ntpd");
          dce.SetBinary ("ntpd");
          dce.ResetArguments ();
          dce.ResetEnvironment ();
          dce.AddArgument ("-c");
          dce.AddArgument ("ntp"+suffix+".conf");
          dce.AddArgument ("-n");   // don't fork

          if(useDebug) {
            dce.AddArgument("-D"); // Alternatively -dddd
            dce.AddArgument("5");
          }
          break;

      case NTP_NTIMED:
        dce.SetBinary ("ntimed-client");

        // TODO install a defective clock on that node

        dce.ResetArguments ();
        dce.ResetEnvironment ();
        dce.AddArgument ("--poll-server"); // /!\ poll-server does not steer the clock
        // address of the NTP server
        dce.AddArgument ("10.1.1.2");
        dce.AddArgument ("-t"); //! export values to ntimed.traces
        dce.AddArgument ("ntimed.traces");


        break;
      default:
        break;
  };
}

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

  Time startupTime = Seconds(0.7);
  Time simDuration = Seconds(1000);

  CommandLine cmd;
  NS_LOG_INFO("Parsing");
  cmd.AddValue ("stack", "Name of IP stack: ns3/linux/freebsd.", stack);
  cmd.AddValue ("debug", "Debug. Default true (0)", useDebug);
  cmd.AddValue ("server", "Server to use", MakeCallback(SetServerType));
  cmd.AddValue ("client", "client to use", MakeCallback(SetServerType));
//  cmd.AddValue ("bw", "BandWidth. Default 1m.", bandWidth);
NS_LOG_INFO(argc << argv[1]);
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


  pointToPoint.EnablePcapAll ("ntp-" + stack, true);


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
//  Ptr<ClockPerfect> serverClock = CreateObject<ClockPerfect>();
//  serverClock->SetMaxRandomOffset(200);
//  serverClock->SetTime(200);
//  nServer->SetClock( serverClock );
//  NS_LOG_UNCOND("");
  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);


  ///////////////////////////////////////
  /// Client configuration
  ///////////////////////////////////////
  // Launch ntp client on node 0
  SetupProgram(dce, clientType, false);

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.7));


  ///////////////////////////////////////
  /// Server configuration
  ///////////////////////////////////////
  // Launch ntp server on node 1
  SetupProgram(dce, serverType, true);
  apps = dce.Install (nServer);



//  dceRoutingHelper.Create(nServer);



  apps.Start (Seconds (0.6)); // TODO use startup time

//  setPos (nodes.Get (0), 1, 10, 0);
//  setPos (nodes.Get (1), 50,10, 0);

  Simulator::Stop (simDuration);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
