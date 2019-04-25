/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Input locations of routers and throughput
 * Routing selection
 * Mesh antennas not fixed
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/dsdv-module.h"
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

/// throughput monitor
std::vector<uint64_t> lastTotalRx;
std::vector<double> throughput;
std::vector<Ptr<PacketSink>> packetSink;

double
CalculateSingleStreamThroughput (Ptr<PacketSink> sink, uint64_t &lastTotalRx, double monitorInterval)
{
  double thr = (sink->GetTotalRx () - lastTotalRx) * (double) 8/1024/1024/monitorInterval;
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
        throughput[i] = -1;
      else
        throughput[i] = CalculateSingleStreamThroughput (packetSink[i], lastTotalRx[i], monitorInterval);
      std::cout << '\t' << throughput[i];
    }
  std::cout << std::endl;
  Simulator::Schedule (Seconds (monitorInterval), &CalculateThroughput, monitorInterval);
}

void
PrintThroughputTitle (uint32_t apNum, uint32_t clNum, bool aptx)
{
  std::cout << "-------------------------------------------------\n";
  std::cout << "Time[s]";
  for (uint32_t i = 0; i < apNum; ++i)
    for (uint32_t j = 0; j < clNum; ++j)
      std::cout << '\t' << "cl-" << i << '-' << j;
  if (aptx)
    for (uint32_t i = 0; i < apNum; ++i)
      std::cout << '\t' << "ap-" << i;
  std::cout << std::endl;
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
  std::string app;
  std::string datarate;
  std::vector<ApplicationContainer> serverApp;
  std::vector<ApplicationContainer> clientApp;

  /// throughput monitor
  double monitorInterval;

  /// netanim
  bool anim;

  /// test-only operations
  bool aptx;

  /// specified real implementation
  std::string locationFile;
  std::vector<std::vector<double>> locations;
  uint32_t gateways;
  double scale;
  NodeContainer csmaNodes;
  NetDeviceContainer csmaDevices;
  Ipv4InterfaceContainer csmaInterfaces;

  std::string route;


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

  /// Connect gateways
  void CreateCsmaDevices ();
  void InstallCsmaInternetStack ();
  void ReadLocations ();
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
  apNum (2),
  clNum (0),
  apStep (50),
  clStep (20),
  startTime (1),
  totalTime (20),
  pcap (false),
  printRoutes (false),
  app ("udp"),
  datarate ("1Mbps"),
  monitorInterval (1.0),
  anim (false),
  aptx (false),
  locationFile (""),
  gateways (1),
  scale (100),
  route ("aodv")
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
  cmd.AddValue ("app", "ping or UDP or TCP", app);
  cmd.AddValue ("datarate", "tested application datarate", datarate);
  cmd.AddValue ("monitorInterval", "Time between throughput updates.", monitorInterval);
  cmd.AddValue ("anim", "Output netanim .xml file or not.", anim);
  cmd.AddValue ("aptx", "Mount OnOffApplication on AP or not, for test.", aptx);
  cmd.AddValue ("locationFile", "Location file name.", locationFile);
  cmd.AddValue ("gateways", "Number of gateway AP.", gateways);
  cmd.AddValue ("scale", "Ratio between experiment and simulation.", scale);
  cmd.AddValue ("route", "Routing protocol", route);

  cmd.Parse (argc, argv);

  SeedManager::SetSeed (rndSeed);

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

  AnimationInterface netanim ("./my-simulations/temp-anim.xml");
  // netanim.SetMaxPktsPerTraceFile (50000);
  if (!anim)
    netanim.SetStopTime (Seconds (0));

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
  serverApp.reserve (txNum);
  clientApp.reserve (txNum);
  lastTotalRx.reserve (txNum);
  throughput.reserve (txNum);
  packetSink.reserve (txNum);

  for (uint32_t i = 0; i < apNum; ++i)
    {
      clNodes.push_back (NodeContainer ());
      apDevices.push_back (NetDeviceContainer ());
      clDevices.push_back (NetDeviceContainer ());
      apInterfaces.push_back (Ipv4InterfaceContainer ());
      clInterfaces.push_back (Ipv4InterfaceContainer ());
      locations.push_back (std::vector<double> (4, 0));
    }
  for (uint32_t i = 0; i < txNum; ++i)
    {
      serverApp.push_back (ApplicationContainer ());
      clientApp.push_back (ApplicationContainer ());
      lastTotalRx.push_back (0);
      throughput.push_back (0);
      packetSink.push_back (NULL);
    }

  if (aptx && (clNum > 0))
    {
      std::cout << "Concurrent AP and CL application does not make sense !!!\n";
      std::exit (0);
    }

  ReadLocations ();

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
  if (locationFile.empty ())
    {
      mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (0.0),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (apStep),
                                     "DeltaY", DoubleValue (apStep),
                                     "GridWidth", UintegerValue (gridSize),
                                     "LayoutType", StringValue ("RowFirst"));
    }
  else
    {
      if (locations.size () < apNum)
        {
          std::cout << "Number of locations is not enough !!!\n";
          std::exit (0);
        }
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
      for (uint32_t i = 0; i < apNum; i++)
        positionAlloc->Add (Vector (locations[i][0] * scale/100, locations[i][1] * scale/100, locations[i][2]));
      mobility.SetPositionAllocator (positionAlloc);
    }
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (apNodes);

  std::cout << "CreateApNodes () DONE !!!\n";
}

void
AodvExample::CreateClNodes ()
{
  std::cout << "Creating " << (unsigned)clNum << " CLs " << clStep << " m away from each AP.\n";
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
  CreateCsmaDevices ();

  std::cout << "CreateDevices () DONE !!!\n";
}

void
AodvExample::CreateMeshDevices ()
{
  MeshHelper mesh = MeshHelper::Default ();
  mesh.SetStackInstaller ("ns3::Dot11sStack");
  mesh.SetMacType ("RandomStart", TimeValue (Seconds (startTime)),
                   "BeaconInterval", TimeValue (Seconds (startTime)));
  mesh.SetStandard (WIFI_PHY_STANDARD_80211ac);
//  mesh.SetRemoteStationManager ("ns3::IdealWifiManager");
  mesh.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "ControlMode", StringValue ("VhtMcs0"),
                                "DataMode", StringValue ("VhtMcs7"),
                                "RtsCtsThreshold", UintegerValue (99999));

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("ChannelWidth", UintegerValue (40));
  wifiPhy.Set ("Antennas", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (4));
  wifiPhy.Set ("TxPowerStart", DoubleValue (30.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (30.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set ("ShortGuardEnabled", BooleanValue (true));

  meshDevices = mesh.Install (wifiPhy, apNodes);

//  for (uint32_t i = 0; i < meshDevices.GetN (); ++i)
//    {
//      Ptr<NetDevice> nd = meshDevices.Get (i);
//      Ptr<MeshPointDevice> mpd = DynamicCast<MeshPointDevice> (nd);
//      std::vector<Ptr<NetDevice>> nds = mpd->GetInterfaces ();
//      Ptr<WifiNetDevice> wnd = DynamicCast<WifiNetDevice> (nds[0]);
//      Ptr<WifiPhy> wp = wnd->GetPhy ();
//      std::cout << "Channel Width = " << wp->GetChannelWidth () << " !!!!!!!!!!!!!!!!!!\n";
//      std::cout << "Antennas = " << wp->GetNumberOfAntennas () << " !!!!!!!!!!!!!!!!!!\n";
//      std::cout << "Tx Support = " << wp->GetMaxSupportedTxSpatialStreams () << " !!!!!!!!!!!!!!!!!!\n";
//      std::cout << "Rx Support = " << wp->GetMaxSupportedRxSpatialStreams () << " !!!!!!!!!!!!!!!!!!\n";
//    }

  if (pcap)
    wifiPhy.EnablePcapAll (std::string ("aodv"));

  std::cout << "CreateMeshDevices () DONE !!!\n";
}

void
AodvExample::CreateCsmaDevices ()
{
  csmaNodes.Create (1);
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (csmaNodes);

  for (uint32_t i = 0; i < gateways; ++i)
    csmaNodes.Add (apNodes.Get (i));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("9999Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  csmaDevices = csma.Install (csmaNodes);

  std::cout << "CreateCsmaDevices () DONE !!!\n";
}

void
AodvExample::CreateWifiDevices ()
{
  WifiMacHelper wifiMac;

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("ChannelWidth", UintegerValue (20));
//  wifiPhy.Set ("Antennas", UintegerValue (4));
//  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (4));
//  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (4));
//  wifiPhy.Set ("TxPowerStart", DoubleValue (30.0));
//  wifiPhy.Set ("TxPowerEnd", DoubleValue (30.0));
//  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
//  wifiPhy.Set ("ShortGuardEnabled", BooleanValue (true));

  WifiHelper wifi;
  // 80211n_2_4GHZ, 80211n_5GHZ, 80211ac, 80211ax_2_4GHZ, 80211ax_5GHZ
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
//  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "ControlMode", StringValue ("HtMcs0"),
                                "DataMode", StringValue ("HtMcs7"),
                                "RtsCtsThreshold", UintegerValue (99999));

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
  InstallCsmaInternetStack ();

  std::cout << "InstallInternetStack () DONE !!!\n";
}

void
AodvExample::InstallMeshInternetStack ()
{
  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;

  InternetStackHelper stack;
  if (route == std::string ("dsdv"))
    stack.SetRoutingHelper (dsdv);
  else if (route == std::string ("olsr"))
    stack.SetRoutingHelper (olsr);
  else
    stack.SetRoutingHelper (aodv);
  stack.Install (apNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  meshInterfaces = address.Assign (meshDevices);

  if (printRoutes)
    {
//      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("./output-olsr/olsr.routes."+std::to_string (apNum)+"."+std::to_string (uint32_t (apStep))+"."+std::to_string (rndSeed), std::ios::out);
//      olsr.PrintRoutingTableAllAt (Seconds (totalTime - 0.01), routingStream);
    }

  std::cout << "InstallMeshInternetStack () DONE !!!\n";
}

void
AodvExample::InstallCsmaInternetStack ()
{
  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;

  InternetStackHelper stack;
  if (route == std::string ("dsdv"))
    stack.SetRoutingHelper (dsdv);
  else if (route == std::string ("olsr"))
    stack.SetRoutingHelper (olsr);
  else
    stack.SetRoutingHelper (aodv);
  stack.Install (csmaNodes.Get (0));

  Ipv4AddressHelper address;
  address.SetBase ("10.2.1.0", "255.255.255.0");
  csmaInterfaces = address.Assign (csmaDevices);

  std::cout << "InstallCsmaInternetStack () DONE !!!\n";
}

void
AodvExample::InstallWifiInternetStack ()
{
  for (uint32_t i = 0; i < apNum; ++i)
    {
      AodvHelper aodv;
      OlsrHelper olsr;
      DsdvHelper dsdv;

      InternetStackHelper stack;
      if (route == std::string ("dsdv"))
        stack.SetRoutingHelper (dsdv);
      else if (route == std::string ("olsr"))
        stack.SetRoutingHelper (olsr);
      else
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

      for (uint32_t i = 0; i < apNum; ++i)
        {
          for (uint32_t j = 0; j < clNum; ++j)
            {
              uint16_t port = 50000+i*100+j;
              Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
              PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
              serverApp[i*clNum+j] = server.Install (csmaNodes.Get (0)); //
              serverApp[i*clNum+j].Start (Seconds (1.0));
              serverApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
              packetSink[i*clNum+j] = StaticCast<PacketSink> (serverApp[i*clNum+j].Get (0));

              OnOffHelper client ("ns3::UdpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1472));
              client.SetAttribute ("DataRate", StringValue (datarate));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              clientApp[i*clNum+j].Start (Seconds (startTime));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (aptx)
            {
              uint16_t port = 40000+i*100;
              Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
              PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
              serverApp[apNum*clNum+i] = server.Install (csmaNodes.Get (0)); //
              serverApp[apNum*clNum+i].Start (Seconds (1.0));
              serverApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
              packetSink[apNum*clNum+i] = StaticCast<PacketSink> (serverApp[apNum*clNum+i].Get (0));

              OnOffHelper client ("ns3::UdpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1472));
              client.SetAttribute ("DataRate", StringValue (std::to_string (locations[i][3]) + "Mbps"));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
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

      for (uint32_t i = 0; i < apNum; ++i)
        {
          for (uint32_t j = 0; j < clNum; ++j)
            {
              uint16_t port = 50000+i*100+j;
              Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
              PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
              serverApp[i*clNum+j] = server.Install (csmaNodes.Get (0)); //
              serverApp[i*clNum+j].Start (Seconds (1.0));
              serverApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
              packetSink[i*clNum+j] = StaticCast<PacketSink> (serverApp[i*clNum+j].Get (0));

              OnOffHelper client ("ns3::TcpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1448));
              client.SetAttribute ("DataRate", StringValue (datarate));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              clientApp[i*clNum+j].Start (Seconds (startTime));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (aptx)
            {
              uint16_t port = 40000+i*100;
              Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
              PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
              serverApp[apNum*clNum+i] = server.Install (csmaNodes.Get (0)); //
              serverApp[apNum*clNum+i].Start (Seconds (1.0));
              serverApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
              packetSink[apNum*clNum+i] = StaticCast<PacketSink> (serverApp[apNum*clNum+i].Get (0));

              OnOffHelper client ("ns3::TcpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1448));
              client.SetAttribute ("DataRate", StringValue (datarate));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[apNum*clNum+i] = client.Install (apNodes.Get (i));
              clientApp[apNum*clNum+i].Start (Seconds (startTime));
              clientApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
            }
        }
    }

  std::cout << "Gateway is connect to AP 0\n";
  std::cout << "InstallApplications () DONE !!!\n";

  Simulator::Schedule (Seconds (startTime), &PrintThroughputTitle, apNum, clNum, aptx);
  Simulator::Schedule (Seconds (startTime+monitorInterval), &CalculateThroughput, monitorInterval);
}

void
AodvExample::ReadLocations ()
{
  if (locationFile.empty ())
    return;

  std::ifstream fin (locationFile);
  if (fin.is_open ())
    {
      for (uint32_t i = 0; i < apNum; ++i)
        fin >> locations[i][0] >> locations[i][1] >> locations[i][2] >> locations[i][3];
      fin.close ();
    }
}
