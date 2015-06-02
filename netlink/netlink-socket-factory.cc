/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 Liu Jian
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Liu Jian <liujatp@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "netlink-socket-factory.h"
#include "netlink-socket.h"
#include "ns3/node.h"
#include "ns3/log.h"

#include "ns3/trace-helper.h"
#include <sstream>

//#include "ns3/pcap-file-wrapper.h"
//#include "netlink/netlink-socket.h"
NS_LOG_COMPONENT_DEFINE ("DceNetlinkSocketFactory");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NetlinkSocketFactory);

TypeId
NetlinkSocketFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetlinkSocketFactory")
    .AddConstructor<NetlinkSocketFactory> ()
    .SetParent<SocketFactory> ()
  ;
  return tid;
}

NetlinkSocketFactory::NetlinkSocketFactory ()
{
}

Ptr<Socket> NetlinkSocketFactory::CreateSocket (void)
{
  NS_LOG_FUNCTION_NOARGS();
  Ptr<Node> node = GetObject<Node> ();
  Ptr<NetlinkSocket> socket = CreateObject<NetlinkSocket> ();
  socket->SetNode (node);

            static int id = 0;

              PcapHelper pcapHelper;

//              std::to_string (id); // only valid in C++11
              std::ostringstream filename;
              filename << "netlink" << node->GetId() << "-" << id++ << ".pcap";
    NS_LOG_UNCOND("Creating socket [" << filename.str() << "]");
              Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename.str(), std::ios::out,PcapHelper::DLT_NETLINK);
              // for now we test only one socket
              pcapHelper.HookDefaultSink<NetlinkSocket> ( socket, "PromiscSniffer", file);

  return socket;
}
} // namespace ns3
