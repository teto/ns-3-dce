// -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//         Gustavo Carneiro <gjc@inescporto.pt>

#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4 && m_ipv4->GetObject < Node > ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << m_ipv4->GetObject < Node > ()->GetId () << "] "; }

#include <iomanip>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ip-program-dce-routing.h"
#include "ns3/linux-stack-helper.h"
#include "../netlink/netlink-socket.h"


NS_LOG_COMPONENT_DEFINE ("IpProgramDceRouting");

using std::make_pair;

namespace ns3 {

static Time start = Seconds(0.1);

NS_OBJECT_ENSURE_REGISTERED (IpProgramDceRouting);

TypeId
IpProgramDceRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IpProgramDceRouting")
    .SetParent<Ipv4StaticRouting> ()
    .AddConstructor<IpProgramDceRouting> ()
  ;
  return tid;
}

IpProgramDceRouting::IpProgramDceRouting ()
{
  NS_LOG_FUNCTION (this);
}

IpProgramDceRouting::~IpProgramDceRouting ()
{
}

//void
//IpProgramDceRouting::NotifyInterfaceUp (uint32_t i)
//{
//  NS_LOG_FUNCTION (this << i);
//
//  Ipv4StaticRouting::NotifyInterfaceUp (i);
//
//  m_netlink->NotifyIfLinkMessage (m_ipv4->GetNetDevice (i)->GetIfIndex ());
//}
//
//void
//IpProgramDceRouting::NotifyInterfaceDown (uint32_t i)
//{
//  NS_LOG_FUNCTION (this << i);
//
//  Ipv4StaticRouting::NotifyInterfaceDown (i);
//
//  m_netlink->NotifyIfLinkMessage (m_ipv4->GetNetDevice (i)->GetIfIndex ());
//}
//
//void
//IpProgramDceRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
//{
//  NS_LOG_FUNCTION (this << interface << " " << address.GetLocal ());
//
//  Ipv4StaticRouting::NotifyAddAddress (interface, address);
//  // NS_LOG_DEBUG ("Not implemented yet");
//}
//void
//IpProgramDceRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
//{
//  NS_LOG_FUNCTION (this << interface << " " << address.GetLocal ());
//
//  Ipv4StaticRouting::NotifyRemoveAddress (interface, address);
//  // NS_LOG_DEBUG ("Not implemented yet");
//}

//#if 0
void
IpProgramDceRouting::AddNetworkRouteTo (Ipv4Address network,
                          Ipv4Mask networkMask,
                          Ipv4Address nextHop,
                          uint32_t interface,
                          uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << " " << networkMask << " " << nextHop << " " << interface << " " << metric);   //
    std::ostringstream cmd_oss;
    cmd_oss.str ("");
//    cmd_oss << "route add 10.2.0.0/16 via " << if2.GetAddress (1, 0) << " dev sim1";
//mask.GetPrefixLength
    // TODO/WARNING Careful device name may be wrong or depend on simu
    cmd_oss << "route add " << network << "/" << networkMask.GetPrefixLength()
        << " via " << nextHop
        << " dev " << "sim" << (int)interface

//    m_ipv4->GetNetDevice (i)->GetName()
    ;
    Ptr<Node> node = m_ipv4->GetObject<Node> ();
    NS_ASSERT(node);
    NS_LOG_DEBUG("Running ip command:\t" << cmd_oss.str());
//    LinuxStackHelper::RunIp ( node, cmd_oss.str ().c_str ());
    LinuxStackHelper::RunIp ( node, start, cmd_oss.str ().c_str ());
    // will be generated
//    Ipv4StaticRouting::AddNetworkRouteTo (network, networkMask, nextHop, interface, metric);
}

void
IpProgramDceRouting::AddNetworkRouteTo (Ipv4Address network,
                          Ipv4Mask networkMask,
                          uint32_t interface,
                          uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << " " << networkMask << " " << interface << " " << metric);   //
    std::ostringstream cmd_oss;
    cmd_oss.str ("");
    Ptr<Node> node = m_ipv4->GetObject<Node> ();
    NS_ASSERT(node);
//    cmd_oss << "route add 10.2.0.0/16 via " << if2.GetAddress (1, 0) << " dev sim1";
//mask.GetPrefixLength
    // TODO/WARNING Careful device name may be wrong or depend on simu
    cmd_oss << "route add " << network << "/" << networkMask.GetPrefixLength()
        << " dev " << "sim" << (int)interface;

    LinuxStackHelper::RunIp ( node, start, cmd_oss.str ().c_str ());
}

void
IpProgramDceRouting::SetDefaultRoute (Ipv4Address nextHop,
                        uint32_t interface,
                        uint32_t metric)
{
  NS_LOG_FUNCTION (this << nextHop << " " << interface << " " << metric);
//    NS_FATAL_ERROR("")
    std::ostringstream cmd_oss;
    cmd_oss.str ("");
    Ptr<Node> node = m_ipv4->GetObject<Node> ();
    NS_ASSERT(node);
    cmd_oss << "route add default via " << nextHop << " dev sim" << interface;
//      << " table " << (i+1)
    ;
    LinuxStackHelper::RunIp ( node, start, cmd_oss.str ().c_str ());

//    Ipv4StaticRouting::AddNetworkRouteTo (nextHop, interface, metric);
}
//#endif

void
IpProgramDceRouting::SetIpv4 (Ptr<Ipv4> ipv4)
{
  // do some other stuff
  m_ipv4 = ipv4;
//  m_netlink = CreateObject<NetlinkSocket> ();
//  m_netlink->SetNode (ipv4->GetObject<Node> ());
//  m_netlink->Bind (); // not really necessary to do this

  Ipv4StaticRouting::SetIpv4 (ipv4);
}

void
IpProgramDceRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  *stream->GetStream () << std::endl;
  *stream->GetStream () << "Time: " << Simulator::Now ().GetSeconds () << "s" << std::endl;
  Ipv4StaticRouting::PrintRoutingTable (stream);
}

} // namespace ns3
