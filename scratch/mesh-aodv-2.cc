/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Using flow monitor counting throughput of each application
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
#include "ns3/mesh-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

void
PrintThroughputLine (Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier, double monitorInterval, Ipv4Address sourceAddress, uint16_t sourcePort, bool title)
{
  // Print per flow statistics
  monitor->CheckForLostPackets ();
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

  if (title)
    {
      std::cout << "-------------------------------------------\n";
      std::cout << "AP0 -> APm -> CL0-0 -> CL0-n -> CLm-0 -> CLm-n\n";
    }

  std::cout << Simulator::Now ().GetSeconds ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      if (i->first > 0)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          if (!t.destinationAddress.IsEqual(sourceAddress) || (t.destinationPort != sourcePort))
            continue;
          std::cout << '\t' << i->second.rxBytes;
        }
    }
  std::cout << '\n';

  Simulator::Schedule (Seconds (monitorInterval), &PrintThroughputLine, monitor, classifier, monitorInterval, sourceAddress, sourcePort, false);
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
  uint32_t rndSeed;
  /// Layout
  uint32_t gridSize;

  /// Number of nodes
  uint32_t apNum;
  uint32_t clNum;

  /// Distance between nodes, meters
  double apStep;
  double clStep;

  /// Simulation time, seconds
  double startTime;
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
  uint32_t gateway;
  std::string app;
  std::string datarate;
  ApplicationContainer serverApp;
  std::vector<ApplicationContainer> clientApp;

  /// throughput monitor
  double monitorInterval;
  Ptr<FlowMonitor> monitor;
  Ptr<Ipv4FlowClassifier> classifier;
  bool anim;

  /// test-only operations
  bool aptx;

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
  rndSeed (1),
  gridSize (3),
  apNum (0),
  clNum (1),
  apStep (50),
  clStep (20),
  startTime (1),
  totalTime (100),
  pcap (false),
  printRoutes (true),
  gateway (99999),
  app ("udp"),
  datarate ("1Mbps"),
  monitorInterval (0.5),
  anim (false),
  aptx (false)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  CommandLine cmd;

  cmd.AddValue ("rndSeed", "Random Seed", rndSeed);
  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("gridSize", "Size of AP grid.", gridSize);
  cmd.AddValue ("apNum", "Number of AP nodes.", apNum);
  cmd.AddValue ("clNum", "Number of CL nodes for each Service Set.", clNum);
  cmd.AddValue ("startTime", "Application start time, s.", startTime);
  cmd.AddValue ("totalTime", "Simulation time, s.", totalTime);
  cmd.AddValue ("monitorInterval", "Monitor interval, s.", monitorInterval);
  cmd.AddValue ("apStep", "AP grid step, m", apStep);
  cmd.AddValue ("clStep", "CL grid step, m", clStep);
  cmd.AddValue ("gateway", "Mount PacketSink on which AP.", gateway);
  cmd.AddValue ("app", "ping or UDP or TCP", app);
  cmd.AddValue ("datarate", "tested application datarate", datarate);
  cmd.AddValue ("monitorInterval", "Time between throughput updates.", monitorInterval);
  cmd.AddValue ("anim", "Output netanim .xml file or not.", anim);
  cmd.AddValue ("aptx", "Mount OnOffApplication on AP or not, for test.", aptx);

  cmd.Parse (argc, argv);

  SeedManager::SetSeed (rndSeed);

  if (apNum == 0)
    apNum = gridSize*gridSize;
  if (gateway > apNum)
    gateway = uint32_t (apNum/2);
  return true;
}

void
AodvExample::Run ()
{
  CreateVariables ();
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  AnimationInterface netanim ("./output-netanim/mesh-aodv-2.xml");
  // netanim.SetMaxPktsPerTraceFile (50000);
  if (!anim)
    netanim.SetStopTime (Seconds (0));

  // Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  Simulator::Schedule (Seconds (startTime+monitorInterval), &PrintThroughputLine, monitor, classifier, monitorInterval, apInterfaces[gateway].GetAddress (0), 50000, true);

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
  uint32_t txNum;
  if (aptx)
    txNum = apNum*clNum+apNum;
  else
    txNum = apNum*clNum;

  clNodes.reserve (apNum);
  apDevices.reserve (apNum);
  clDevices.reserve (apNum);
  apInterfaces.reserve (apNum);
  clInterfaces.reserve (apNum);
  clientApp.reserve (txNum);

  for (uint32_t i = 0; i < apNum; ++i)
    {
      clNodes.push_back (NodeContainer ());
      apDevices.push_back (NetDeviceContainer ());
      clDevices.push_back (NetDeviceContainer ());
      apInterfaces.push_back (Ipv4InterfaceContainer ());
      clInterfaces.push_back (Ipv4InterfaceContainer ());
    }
  for (uint32_t i = 0; i < txNum; ++i)
    {
      clientApp.push_back (ApplicationContainer ());
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
                                 "DeltaY", DoubleValue (apStep),
                                 "GridWidth", UintegerValue (gridSize),
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

      Vector center = apNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ();
      std::ostringstream randomRho;
      randomRho << "ns3::UniformRandomVariable[Min=0|Max=" << clStep << "]";
      MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                     "X", DoubleValue (center.x),
                                     "Y", DoubleValue (center.y),
             "Rho", StringValue (randomRho.str ()));
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
  MeshHelper mesh = MeshHelper::Default ();
  mesh.SetStackInstaller ("ns3::Dot11sStack");
  mesh.SetMacType ("RandomStart", TimeValue (Seconds (0.0)));

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("ChannelWidth", UintegerValue (40));
//  wifiPhy.Set ("Antennas", UintegerValue (4));
//  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (1));
//  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (1));
//  wifiPhy.Set ("TxGain", DoubleValue (10.0));
//  wifiPhy.Set ("RxGain", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerStart", DoubleValue (30.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (30.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  WifiHelper wifi;
  //80211n_2_4GHZ, 80211n_5GHZ, 80211ac, 80211ax_2_4GHZ, 80211ax_5GHZ
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager",
                                "RtsCtsThreshold", UintegerValue (5000));
//  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
//                                "ControlMode", StringValue ("HtMcs9"),
//                                "DataMode", StringValue ("HtMcs9"),
//                                "RtsCtsThreshold", UintegerValue (0));

  meshDevices = mesh.Install (wifiPhy, apNodes);

  if (pcap)
    wifiPhy.EnablePcapAll (std::string ("aodv"));

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
  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");
//  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
//                                "ControlMode", StringValue ("HtMcs7"),
//                                "DataMode", StringValue ("HtMcs7"),
//                                "RtsCtsThreshold", UintegerValue (0));

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
    wifiPhy.EnablePcapAll (std::string ("aodv"));

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
  aodv.Set ("TtlStart", UintegerValue (1));

  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (apNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  meshInterfaces = address.Assign (meshDevices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("./output-aodv/aodv.routes."+std::to_string (apNum)+"."+std::to_string (uint32_t (apStep))+"."+std::to_string (rndSeed), std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (totalTime - 0.01), routingStream);
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
    os << "10.1." << 11+i << ".0";
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
  if (app == std::string("udp"))
    {
      // UDP flow

      uint16_t port = 50000;
      Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
      serverApp = server.Install (apNodes.Get (gateway)); //
      serverApp.Start (Seconds (1.0));
      serverApp.Stop (Seconds (totalTime + 0.1));

      for (uint32_t i = 0; i < apNum; ++i)
        {
          for (uint32_t j = 0; j < clNum; ++j)
            {
              OnOffHelper client ("ns3::UdpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1472));
              client.SetAttribute ("DataRate", StringValue (datarate));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (apInterfaces[gateway].GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              clientApp[i*clNum+j].Start (Seconds (startTime));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (i == gateway)
            continue;

          if (aptx)
            {
              OnOffHelper client ("ns3::UdpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1472));
              client.SetAttribute ("DataRate", StringValue (datarate));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (apInterfaces[gateway].GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[apNum*clNum+i] = client.Install (apNodes.Get (i));
              clientApp[apNum*clNum+i].Start (Seconds (startTime));
              clientApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
            }
        }
    }

  if (app == std::string("tcp"))
    {
      // TCP flow

      uint16_t port = 50000;
      Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
      PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
      serverApp = server.Install (apNodes.Get (gateway)); //
      serverApp.Start (Seconds (1.0));
      serverApp.Stop (Seconds (totalTime + 0.1));

      for (uint32_t i = 0; i < apNum; ++i)
        {
          for (uint32_t j = 0; j < clNum; ++j)
            {
              OnOffHelper client ("ns3::TcpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1448));
              client.SetAttribute ("DataRate", StringValue (datarate));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (apInterfaces[gateway].GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              clientApp[i*clNum+j].Start (Seconds (startTime));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (i == gateway)
            continue;

          if (aptx)
            {
              OnOffHelper client ("ns3::TcpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1448));
              client.SetAttribute ("DataRate", StringValue (datarate));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (apInterfaces[gateway].GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[apNum*clNum+i] = client.Install (apNodes.Get (i));
              clientApp[apNum*clNum+i].Start (Seconds (startTime));
              clientApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
            }
        }
    }

  if (app == std::string("ping"))
    {
      V4PingHelper ping (apInterfaces[0].GetAddress (0));
      ping.SetAttribute ("Verbose", BooleanValue (true));

      ApplicationContainer p = ping.Install (apNodes.Get (apNum-1));
      p.Start (Seconds (0));
      p.Stop (Seconds (totalTime) - Seconds (0.001));
    }

  std::cout << "Pick " << gateway << "as gateway !!!\n";
  std::cout << "InstallApplications () DONE !!!\n";
}
