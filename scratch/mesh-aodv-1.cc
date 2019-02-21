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
#include <iomanip>
#include <cmath>
#include <vector>

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

/// throughput monitor
std::vector<uint64_t> lastTotalRx;
std::vector<double> throughput;
std::vector<Ptr<PacketSink>> packetSink;

double
CalculateSingleStreamThroughput (Ptr<PacketSink> sink, uint64_t &lastTotalRx, double monitorInterval)
{
  double thr = (sink->GetTotalRx () - lastTotalRx) * (double) 8/1e6/monitorInterval;
  lastTotalRx = sink->GetTotalRx ();
  return thr;
}

void
CalculateThroughput (double monitorInterval)
{
  std::cout << Simulator::Now ().GetSeconds ();
  for (uint32_t i = 0; i < packetSink.size (); ++i)
    {
	  if (packetSink[i] == NULL)
	    {
		  throughput[i] = -1;
	    }
	  else
	    {
          throughput[i] = CalculateSingleStreamThroughput (packetSink[i], lastTotalRx[i], monitorInterval);
	    }
      std::cout << '\t' << throughput[i];
    }
  std::cout << std::endl;
  Simulator::Schedule (Seconds (monitorInterval), &CalculateThroughput, monitorInterval);
}

/// class
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

  /// devices used i0n the example
  NetDeviceContainer meshDevices;
  std::vector<NetDeviceContainer> apDevices;
  std::vector<NetDeviceContainer> clDevices;

  /// interfaces used in the example
  Ipv4InterfaceContainer meshInterfaces;
  std::vector<Ipv4InterfaceContainer> apInterfaces;
  std::vector<Ipv4InterfaceContainer> clInterfaces;

  /// application
  bool udp;
  std::vector<ApplicationContainer> serverApp;
  std::vector<ApplicationContainer> clientApp;

  /// throughput monitor
  double monitorInterval;

private:
  void CreateVariables ();

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

  std::cout << std::setprecision (2) << std::fixed;

  test.Run ();
  // test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodvExample::AodvExample () :
  apNum (10),
  clNum (1),
  apStep (50),
  clStep (20),
  totalTime (100),
  pcap (false),
  printRoutes (true),
  udp (true),
  monitorInterval (0.5)
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
  cmd.AddValue ("totalTime", "Simulation time, s.", totalTime);
  cmd.AddValue ("monitorInterval", "Monitor interval, s.", monitorInterval);
  cmd.AddValue ("apStep", "AP grid step, m", apStep);
  cmd.AddValue ("clStep", "CL grid step, m", clStep);
  cmd.AddValue ("udp", "UDP or TCP", udp);

  cmd.Parse (argc, argv);
  return true;
}

void
AodvExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateVariables ();
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";
  std::cout << "-------------------------------------------------\n";
  std::cout << "Time[s]";
  for (uint32_t i = 0; i < apNum; ++i)
    {
      for (uint32_t j = 0; j < clNum; ++j)
	    {
          std::cout << '\t' << "cl-" << i << '-' << j;
	    }
    }
  for (uint32_t i = 0; i < apNum; ++i)
    {
	  std::cout << '\t' << "ap-" << i;
    }
  std::cout << std::endl;

  Simulator::Schedule (Seconds (1.0), &CalculateThroughput, monitorInterval);
  AnimationInterface anim ("./output-netanim/mesh-aodv-2.xml");
  // anim.SetMaxPktsPerTraceFile (50000);

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
AodvExample::Report (std::ostream &)
{ 
}

void
AodvExample::CreateVariables ()
{
  clNodes.reserve (apNum);
  apDevices.reserve (apNum);
  clDevices.reserve (apNum);
  apInterfaces.reserve (apNum);
  clInterfaces.reserve (apNum);
  serverApp.reserve (apNum*clNum+apNum);
  clientApp.reserve (apNum*clNum+apNum);
  lastTotalRx.reserve (apNum*clNum+apNum);
  throughput.reserve (apNum*clNum+apNum);
  packetSink.reserve (apNum*clNum+apNum);

  for (uint32_t i = 0; i < apNum; ++i)
    {
      clNodes.push_back (NodeContainer ());
      apDevices.push_back (NetDeviceContainer ());
      clDevices.push_back (NetDeviceContainer ());
      apInterfaces.push_back (Ipv4InterfaceContainer ());
      clInterfaces.push_back (Ipv4InterfaceContainer ());
    }
  for (uint32_t i = 0; i < apNum*clNum+apNum; ++i)
    {
	  serverApp.push_back (ApplicationContainer ());
	  clientApp.push_back (ApplicationContainer ());
	  lastTotalRx.push_back (0);
	  throughput.push_back (0);
	  packetSink.push_back (NULL);
    }


  std::cout << "CreateVariables () DONE !!!\n";
}

void
AodvExample::CreateNodes ()
{
  CreateApNodes ();
  CreateClNodes ();
  std::cout << "CreateNodes () DONE !!!\n";
}

void
AodvExample::CreateApNodes ()
{

  std::cout << "Creating " << (unsigned)apNum << " APs " << apStep << " m apart.\n";
  apNodes.Create (apNum);
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
  std::cout << "CreateApNodes () DONE !!!\n";
}

void
AodvExample::CreateClNodes ()
{

  std::cout << "Creating " << (unsigned)clNum << " CLs " << clStep << " m apart for each AP.\n";
  for (uint32_t i = 0; i < apNum; ++i)
    {
      clNodes[i].Create (clNum);

      for (uint32_t j = 0; j < clNum; ++j)
        {
          std::ostringstream os;
          os << "cl-" << i << "-" << j;
          Names::Add (os.str(), clNodes[i].Get (j));
        }

      MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                     "X", DoubleValue (apStep * i),
                                     "Y", DoubleValue (0),
									 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=10]"));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (clNodes[i]);
    }
  std::cout << "CreateClNodes () DONE !!!\n";
}

void
AodvExample::CreateDevices ()
{
  CreateMeshDevices ();
  CreateWifiDevices ();
  std::cout << "CreateDevices () DONE !!!\n";
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
                                "ControlMode", StringValue ("HtMcs7"),
	  	  	  	                "DataMode", StringValue ("HtMcs7"),
                                "RtsCtsThreshold", UintegerValue (0));
  meshDevices = wifi.Install (wifiPhy, wifiMac, apNodes); 

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
  std::cout << "CreateMeshDevices () DONE !!!\n";
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
		  	  	  	  	  	 	"ControlMode", StringValue ("HtMcs7"),
		  	  	  	  	  	 	"DataMode", StringValue ("HtMcs7"),
                                "RtsCtsThreshold", UintegerValue (0));

  for (uint32_t i = 0; i < apNum; ++i)
    {
      std::ostringstream os;
      os << "ap-" << i ;
      Ssid ssid = Ssid (os.str ());
      // AP
      wifiMac.SetType ("ns3::ApWifiMac",
                       "EnableBeaconJitter", BooleanValue (false),
                       "Ssid", SsidValue (ssid));
      apDevices[i] = wifi.Install (wifiPhy, wifiMac, apNodes.Get (i));
      // client
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
      clDevices[i] = wifi.Install (wifiPhy, wifiMac, clNodes[i]);
    }

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
  std::cout << "CreateWifiDevices () DONE !!!\n";
}

void
AodvExample::InstallInternetStack ()
{
  InstallMeshInternetStack ();
  InstallWifiInternetStack ();
  std::cout << "InstallInternetStack () DONE !!!\n";
}

void
AodvExample::InstallMeshInternetStack ()
{
  OlsrHelper olsr;
  AodvHelper aodv;
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (apNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  meshInterfaces = address.Assign (meshDevices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("./output-aodv/aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
  std::cout << "InstallMeshInternetStack () DONE !!!\n";
}

void
AodvExample::InstallWifiInternetStack ()
{
  for (uint32_t i = 0; i < apNum; ++i)
    {
	  OlsrHelper olsr;
	  AodvHelper aodv;
	  InternetStackHelper stack;
	  stack.SetRoutingHelper (aodv);
      stack.Install (clNodes[i]);

      std::ostringstream os;
      os << "10.1." << 10+i << ".0";
	  Ipv4AddressHelper address;
      address.SetBase (os.str ().c_str (), "255.255.255.0");
      apInterfaces[i] = address.Assign (apDevices[i]);
      clInterfaces[i] = address.Assign (clDevices[i]);
    }
  std::cout << "InstallWifiInternetStack () DONE !!!\n";
}

void
AodvExample::InstallApplications ()
{
  uint32_t k = 2;
  if (udp)
    {
      // UDP flow
      for (uint32_t i = 0; i < apNum; ++i)
        {
          for (uint32_t j = 0; j < clNum; ++j)
            {
              uint16_t port = i*clNum+j+50000;
              Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
              PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
              serverApp[i*clNum+j] = server.Install (apNodes.Get (k)); //
              serverApp[i*clNum+j].Start (Seconds (1.0));
              serverApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
         	  packetSink[i*clNum+j] = StaticCast<PacketSink> (serverApp[i*clNum+j].Get (0));

              OnOffHelper client ("ns3::UdpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1472));
              client.SetAttribute ("DataRate", StringValue ("1Mbps"));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (apInterfaces[k].GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              clientApp[i*clNum+j].Start (Seconds (1.0));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (i == k) //
            {
        	  continue;
            }

          uint16_t port = i+40000;
          Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
          PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
          serverApp[apNum*clNum+i] = server.Install (apNodes.Get (k)); //
          serverApp[apNum*clNum+i].Start (Seconds (1.0));
          serverApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
     	  packetSink[apNum*clNum+i] = StaticCast<PacketSink> (serverApp[apNum*clNum+i].Get (0));

          OnOffHelper client ("ns3::UdpSocketFactory", Address ());
          client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          client.SetAttribute ("PacketSize", UintegerValue (1472));
          client.SetAttribute ("DataRate", StringValue ("2Mbps"));
          client.SetAttribute ("MaxBytes", UintegerValue (0));
          AddressValue remoteAddress (InetSocketAddress (apInterfaces[k].GetAddress (0), port)); //
          client.SetAttribute ("Remote", remoteAddress);
          clientApp[apNum*clNum+i] = client.Install (apNodes.Get (i));
          clientApp[apNum*clNum+i].Start (Seconds (1.0));
          clientApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
        }
    }
  else
    {
      // TCP flow
      for (uint32_t i = 0; i < apNum; ++i)
        {
          for (uint32_t j = 0; j < clNum; ++j)
            {
              uint16_t port = i*clNum+j+50000;
              Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
              PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
              serverApp[i*clNum+j] = server.Install (apNodes.Get (k)); //
              serverApp[i*clNum+j].Start (Seconds (1.0));
              serverApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
         	  packetSink[i*clNum+j] = StaticCast<PacketSink> (serverApp[i*clNum+j].Get (0));

              OnOffHelper client ("ns3::TcpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1448));
              client.SetAttribute ("DataRate", StringValue ("1Mbps"));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (apInterfaces[k].GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              clientApp[i*clNum+j].Start (Seconds (1.0));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (i == k) //
            {
        	  continue;
            }

          uint16_t port = i+40000;
          Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
          PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
          serverApp[apNum*clNum+i] = server.Install (apNodes.Get (k)); //
          serverApp[apNum*clNum+i].Start (Seconds (1.0));
          serverApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
     	  packetSink[apNum*clNum+i] = StaticCast<PacketSink> (serverApp[apNum*clNum+i].Get (0));

          OnOffHelper client ("ns3::TcpSocketFactory", Address ());
          client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          client.SetAttribute ("PacketSize", UintegerValue (1448));
          client.SetAttribute ("DataRate", StringValue ("2Mbps"));
          client.SetAttribute ("MaxBytes", UintegerValue (0));
          AddressValue remoteAddress (InetSocketAddress (apInterfaces[k].GetAddress (0), port)); //
          client.SetAttribute ("Remote", remoteAddress);
          clientApp[apNum*clNum+i] = client.Install (apNodes.Get (i));
          clientApp[apNum*clNum+i].Start (Seconds (1.0));
          clientApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
        }
    }
  std::cout << "InstallApplications () DONE !!!\n";
}
