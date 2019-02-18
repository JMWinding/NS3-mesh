/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * This is an example script for AODV manet routing protocol. 
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include <iostream>
#include <cmath>
#include <vector>

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

class AodvExample 
{
public:
  AodvExample ();
  /**
   * \brief Configure script parameters
   * \param argc is the command line argument count
   * \param argv is the command line arguments
   * \return true on successful configuration
  */
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /**
   * Report results
   * \param os the output stream
   */
  void Report (std::ostream & os);

private:

  // parameters
  /// Number of nodes
  uint32_t apNum;
  uint32_t clNum;

  /// Distance between nodes, meters
  double apStep;
  double clStep;

  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;

  // network
  /// nodes used in the example
  NodeContainer apNodes;
  std::vector<NodeContainer> clNodes;

  /// devices used in the example
  NetDeviceContainer meshDevices;
  std::vector<NetDeviceContainer> apDevices;
  std::vector<NetDeviceContainer> clDevices;

  /// interfaces used in the example
  Ipv4InterfaceContainer meshInterfaces;
  std::vector<Ipv4InterfaceContainer> apInterfaces;
  std::vector<Ipv4InterfaceContainer> clInterfaces;

  /// application
  bool udp;
  ApplicationContainer serverApp;
  std::vector<ApplicationContainer> clientApp;

private:
  /// Create the nodes
  void CreateNodes ();
  void CreateApNodes ();
  void CreateClNodes ();

  /// Create the devices
  void CreateDevices ();
  void CreateMeshDevices ();
  void CreateWifiDevices ();

  /// Create the network
  void InstallInternetStack ();
  void InstallMeshInternetStack ();
  void InstallWifiInternetStack ();

  /// Create the simulation applications
  void InstallApplications ();
};

int main (int argc, char **argv)
{
  AodvExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  // test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodvExample::AodvExample () :
  apNum (10),
  clNum (1),
  apStep (100),
  clStep (10),
  totalTime (100),
  pcap (false),
  printRoutes (false),
  udp (true)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("apNum", "Number of AP nodes.", apNum);
  cmd.AddValue ("clNum", "Number of CL nodes for each Service Set.", clNum);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("apStep", "AP grid step, m", apStep);
  cmd.AddValue ("clStep", "CL grid step, m", clStep);

  cmd.Parse (argc, argv);
  return true;
}

void
AodvExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
AodvExample::Report (std::ostream &)
{ 
}

void
AodvExample::CreateNodes ()
{
  CreateApNodes ();
  CreateClNodes ();
}

void
AodvExample::CreateApNodes ()
{
  std::cout << "Creating " << (unsigned)apNum << " APs " << apStep << " m apart.\n";
  apNodes.Create (apNum);
  // Name nodes
  for (uint32_t i = 0; i < apNum; ++i)
    {
      std::ostringstream os;
      os << "ap-" << i;
      Names::Add (os.str (), apNodes.Get (i));
    }
  // Create static grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (apStep),
                                 "DeltaY", DoubleValue (0),
                                 "GridWidth", UintegerValue (apNum),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (apNodes);
}

void
AodvExample::CreateClNodes ()
{
  std::cout << "Creating" << (unsigned)clNum << " CLs " << clStep << " m apart for each AP.\n";
  clNodes.reserve (apNum);
  for (uint32_t i = 0; i < apNum; ++i)
    {
      clNodes.at (i).Create (clNum);
      for (uint32_t j = 0; j < clNum; ++j)
        {
          std::ostringstream os;
          os << "cl-" << i << "-" << j;
          Names::Add (os.str(), clNodes.at (i).Get (j));
        }
      MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (0.0),
                                     "MinY", DoubleValue (clStep),
                                     "DeltaX", DoubleValue (apStep),
                                     "DeltaY", DoubleValue (0),
                                     "GridWidth", UintegerValue (clNum),
                                     "LayoutType", StringValue ("RowFirst"));
      mobility.Install (clNodes.at (i));
    }
}

void
AodvExample::CreateDevices ()
{
  CreateMeshDevices ();
  CreateWifiDevices ();
}

void
AodvExample::CreateMeshDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  //80211n_2_4GHZ, 80211n_5GHZ, 80211ac, 80211ax_2_4GHZ, 80211ax_5GHZ
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate6Mbps"),
                                "RtsCtsThreshold", UintegerValue (0));
  meshDevices = wifi.Install (wifiPhy, wifiMac, apNodes); 

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
AodvExample::CreateWifiDevices ()
{
  WifiMacHelper wifiMac;
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  //80211n_2_4GHZ, 80211n_5GHZ, 80211ac, 80211ax_2_4GHZ, 80211ax_5GHZ
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate6Mbps"),
                                "RtsCtsThreshold", UintegerValue (0));

  apDevices.reserve (apNum);
  clDevices.reserve (apNum);
  for (uint32_t i = 0; i < apNum; ++i)
    {
      std::ostringstream os;
      os << "ap-" << i ;
      Ssid ssid = Ssid (os.str ());
      // AP
      wifiMac.SetType ("ns3::ApWifiMac",
                       "EnableBeaconJitter", BooleanValue (false),
                       "Ssid", SsidValue (ssid));
      apDevices.at (i) = wifi.Install (wifiPhy, wifiMac, apNodes.Get (i)); 
      // client
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
      clDevices.at (i) = wifi.Install (wifiPhy, wifiMac, clNodes.at (i));
    }

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
AodvExample::InstallInternetStack ()
{
  InstallMeshInternetStack ();
  InstallWifiInternetStack ();
}

void
AodvExample::InstallMeshInternetStack ()
{
  AodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (apNodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  meshInterfaces = address.Assign (meshDevices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("./output-aodv/aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
AodvExample::InstallWifiInternetStack ()
{
  AodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  Ipv4AddressHelper address;

  apInterfaces.reserve (apNum);
  clInterfaces.reserve (clNum);
  for (uint32_t i = 0; i < apNum; ++i)
    {
      stack.Install (clNodes.at (i));
      std::ostringstream os;
      os << "192.168." << 1+i << ".0";
      address.SetBase (os.str ().c_str (), "255.255.255.0");
      apInterfaces.at (i) = address.Assign (apDevices.at (i));
      clInterfaces.at (i) = address.Assign (clDevices.at (i));
    }

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("./output-aodv/aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
AodvExample::InstallApplications ()
{
  if (udp)
    {
      // UDP flow
      uint16_t port = 9;
      UdpServerHelper server (port);
      serverApp = server.Install (apNodes.Get (0));
      serverApp.Start (Seconds (1.0));
      serverApp.Stop (Seconds (totalTime + 0.1));

      UdpClientHelper client (meshInterfaces.GetAddress (0), port);
      client.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
      client.SetAttribute ("Interval", TimeValue (Time ("0.00001")));
      client.SetAttribute ("PacketSize", UintegerValue (1472));
      clientApp.reserve (apNum);
      for (uint32_t i = 0; i < apNum; ++i)
        {
          clientApp.at (i) = client.Install (clNodes.at (i));
          clientApp.at (i).Start (Seconds (1.0));
          clientApp.at (i).Stop (Seconds (totalTime + 0.1));
        }
    }
  else
    {
      // TCP flow
      uint16_t port = 50000;
      Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
      serverApp = server.Install (apNodes.Get (0));
      serverApp.Start (Seconds (1.0));
      serverApp.Stop (Seconds (totalTime + 0.1));

      OnOffHelper client ("ns3::TcpSocketFactory", Address ());
      client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      client.SetAttribute ("PacketSize", UintegerValue (1448));
      client.SetAttribute ("DataRate", StringValue ("5kb/s"));
      AddressValue remoteAddress (InetSocketAddress (meshInterfaces.GetAddress (0), port));
      client.SetAttribute ("Remote", remoteAddress);
      clientApp.reserve (apNum);
      for (uint32_t i = 0; i < apNum; ++i)
        {
          clientApp.at (i) = client.Install (clNodes.at (i));
          clientApp.at (i).Start (Seconds (1.0));
          clientApp.at (i).Stop (Seconds (totalTime + 0.1));
        }
    }
}

