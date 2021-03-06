/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Input locations of routers
 * Mesh with 802.11n/ac fixed
 * WIFI_MAC_MGT_ACTION for MESH and BLOCK_ACK modulation changed
 * Flow monitor - delay
 * 
 * 
 * 2*3 = 6 Nodes & 1 csma server
 * 1 -- 2 -- 3  \
 *               csma node
 * 6 -- 5 -- 4  /
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

#include "ns3/test.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/config-store.h"
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
  double thr = (sink->GetTotalRx () - lastTotalRx) * (double) 8/1e6 / monitorInterval;
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

uint32_t
ConvertContextToNodeId (std::string context)
{
  std::string sub = context.substr (10);
  uint32_t pos = sub.find ("/Device");
  uint32_t nodeId = atoi (sub.substr (0, pos).c_str ());
  return nodeId;
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
  double lengthStep;
  double widthStep;

  /// Simulation time, seconds
  double startTime;
  double totalTime;
  // double startDelay;
  double beaconInterval;
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
  NetDeviceContainer adhocDevices;
  std::vector<NetDeviceContainer> apDevices;
  std::vector<NetDeviceContainer> clDevices;

  /// interfaces used in the example
  Ipv4InterfaceContainer meshInterfaces;
  Ipv4InterfaceContainer adhocInterfaces;
  std::vector<Ipv4InterfaceContainer> apInterfaces;
  std::vector<Ipv4InterfaceContainer> clInterfaces;

  /// application
  std::string app;
  double datarate;
  std::vector<ApplicationContainer> serverApp;
  std::vector<ApplicationContainer> clientApp;

  /// throughput monitor
  double monitorInterval;

  /// netanim
  bool anim;

  /// test-only operations
  bool aptx;
  uint32_t naptx;

  /// specified real implementation
  std::string locationFile;
  std::vector<std::vector<double>> locations;
  uint32_t gateways;
  double scale;
  NodeContainer csmaNodes;
  NetDeviceContainer csmaDevices;
  Ipv4InterfaceContainer csmaInterfaces;

  std::string route;
  std::string rateControl;
  std::string flowout;
    // mesh or adhoc
  std::string macType;

  // obss setting
  std::string obssAlgo;
  double obssLevel;
  std::string heMcs;

private:
  void CreateVariables ();

  /// Create the nodes
  void CreateNodes ();
  void CreateApNodes ();
  void CreateClNodes ();

  /// Create the devices
  void CreateDevices ();
  void CreateAdhocDevices();
  void CreateMeshDevices ();
  void CreateWifiDevices ();

  /// Create the network
  void InstallInternetStack ();

  /// Create the simulation applications
  void InstallApplications ();

  /// Connect gateways
  void CreateCsmaDevices ();
  void ReadLocations ();
  void PreSetStationManager ();

  // set BSS colotr
  void TxSetColor(std::string context, Ptr<const Packet> p, double txPowerW);
  void CheckPhyState (Ptr<WifiNetDevice> device, WifiPhyState expectedState);
  void SendOnePacket (Ptr<WifiNetDevice> tx_dev, Ptr<WifiNetDevice> rx_dev, uint32_t payloadSize);
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
  rndSeed (42),
  gridSize (3),
  apNum (6),
  clNum (0),
  lengthStep (140),
  widthStep (280),
  startTime (1),
  totalTime (20),
  // startDelay(0.0),
  beaconInterval (0.5),
  pcap (false),
  printRoutes (false),
  app ("udp"),
  datarate (1e6),
  monitorInterval (1.0),
  anim (false),
  aptx (true),
  naptx (0),
  locationFile (""),
  gateways (1),
  scale (100),
  route ("aodv"),
  rateControl ("ideal"),
  flowout ("test.xml"),
  macType("mesh"),
  obssAlgo("const"),
  obssLevel(-100),
  heMcs("HeMcs4")
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
//   LogComponentEnable("TsHtWifiManager", LOG_LEVEL_ALL);
  // LogComponentEnable("OlsrRoutingProtocol", LOG_LEVEL_ALL);
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  // LogComponentEnable("MeshObssPdAlgorithm", LOG_LEVEL_ALL);

  Packet::EnablePrinting ();

  CommandLine cmd;

  cmd.AddValue ("rndSeed", "Random Seed", rndSeed);
  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("gridSize", "Size of AP grid.", gridSize);
  cmd.AddValue ("apNum", "Number of AP nodes.", apNum);
  cmd.AddValue ("clNum", "Number of CL nodes for each Service Set.", clNum);
  cmd.AddValue ("startTime", "Application start time, s.", startTime);
  cmd.AddValue ("totalTime", "Simulation time, s.", totalTime);
  cmd.AddValue ("beaconInterval", "Mesh beacon interval, s.", beaconInterval);
  cmd.AddValue ("monitorInterval", "Monitor interval, s.", monitorInterval);
  cmd.AddValue ("lengthStep", "AP grid step, m", lengthStep);
  cmd.AddValue ("widthStep", "CL grid step, m", widthStep);
  cmd.AddValue ("app", "ping or UDP or TCP", app);
  cmd.AddValue ("datarate", "tested application datarate", datarate);
  cmd.AddValue ("anim", "Output netanim .xml file or not.", anim);
  cmd.AddValue ("aptx", "Mount OnOffApplication on AP or not, for test.", aptx);
  cmd.AddValue ("naptx", "Number of AP without throughput.", naptx);
  cmd.AddValue ("locationFile", "Location file name.", locationFile);
  cmd.AddValue ("gateways", "Number of gateway AP.", gateways);
  cmd.AddValue ("scale", "Ratio between experiment and simulation.", scale);
  cmd.AddValue ("route", "Routing protocol", route);
  cmd.AddValue ("rateControl", "Rate control--constant/ideal/minstrel", rateControl);
  cmd.AddValue ("flowout", "Result output directory", flowout);
  cmd.AddValue ("macType", "type of mac --mesh/adhoc", macType);
  cmd.AddValue ("obssAlgo", "chose obss pd algorithm", obssAlgo);
  cmd.AddValue ("obssLevel", "obss pd current level ", obssLevel);
  cmd.AddValue ("heMcs", "constant Mcs", heMcs);
  // cmd.AddValue ("startDelay", "delay after start time for transmiting tcp", startDelay);

  cmd.Parse (argc, argv);

  SeedManager::SetSeed (rndSeed);

  // #####
  // if(app == "tcp") startDelay = 10; // delay for tcp

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

  // #####
  // no need, if we do not use global routing helper
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // #####
  // may not work with mesh
  // Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin", MakeCallback (&AodvExample::TxSetColor, this));

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

//   AnimationInterface netanim ("./my-simulations/temp-anim.xml");
  // netanim.SetMaxPktsPerTraceFile (50000);
//   if (!anim)
//     netanim.SetStopTime (Seconds (0));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      if (!t.destinationAddress.IsEqual (csmaInterfaces.GetAddress (0)))
        continue;
      if ((t.destinationPort < 40000) || (t.destinationPort >= 40000 + apNodes.GetN ()))
        continue;
      std::cout << t.sourceAddress << '\t';
      if (i->second.rxPackets > 1)
        {
          std::cout << (double)i->second.rxBytes * 8/1e6 / (i->second.timeLastTxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) << '\t';
        }
      else
        std::cout << 0 << '\t';
      if (i->second.rxPackets > 0)
        {
          std::cout << (double)i->second.delaySum.GetMicroSeconds() / (double)i->second.rxPackets << '\n';
        }
      else
        std::cout << 0 << '\n';
    }

  monitor->SerializeToXmlFile(flowout, false, false);

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
      clNum = 0;
      std::cout << "Throughput aggregation has higher priority than individual clients!!!\n";
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
  std::cout << "Creating " << (unsigned)apNum << " APs " << lengthStep << " m length apart  "<< widthStep <<"m width apart.\n";
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
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        positionAlloc->Add (Vector (0.0, 0.0, 0.0));  // Node 1
        positionAlloc->Add (Vector (lengthStep, 0.0, 0.0));  // 2
        positionAlloc->Add (Vector (2*lengthStep, 0.0, 0.0));  // 3
        positionAlloc->Add (Vector (2*lengthStep, widthStep, 0.0));  // 4
        positionAlloc->Add (Vector (lengthStep, widthStep, 0.0));  // 5
        positionAlloc->Add (Vector (0.0, widthStep, 0.0));  // 6
        mobility.SetPositionAllocator (positionAlloc);
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
  std::cout << "Creating " << (unsigned)clNum << " CLs " << 30 << " m away from each AP.\n";
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
      randomRho << "ns3::UniformRandomVariable[Min=0|Max=" << 30 << "]";
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
  if(macType==std::string("adhoc")) CreateAdhocDevices();
  else CreateMeshDevices();

  CreateWifiDevices ();
  CreateCsmaDevices ();

  std::cout << "CreateDevices () DONE !!!\n";
}

void AodvExample::CreateAdhocDevices()
{
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("ChannelNumber", UintegerValue (38));
  wifiPhy.Set ("Antennas", UintegerValue (1));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (1));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (1));
  wifiPhy.DisablePreambleDetectionModel ();
  // wifiPhy.Set ("TxPowerStart", DoubleValue (13.0));
  // wifiPhy.Set ("TxPowerEnd", DoubleValue (13.0));
  // wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
//   wifiPhy.Set ("ShortGuardEnabled", BooleanValue (true));

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  if (rateControl == std::string ("ideal"))
    wifi.SetRemoteStationManager ("ns3::IdealWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("minstrel"))
    wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("arf"))
    wifi.SetRemoteStationManager ("ns3::ArfHtWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("aarf"))
    wifi.SetRemoteStationManager ("ns3::AarfHtWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("rraa"))
    wifi.SetRemoteStationManager ("ns3::RraaHtWifiManager");
  else
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "ControlMode", StringValue ("HeMcs0"),
                                  "DataMode", StringValue (heMcs),
                                  "RtsCtsThreshold", UintegerValue (99999));
                                  
  if(obssAlgo==std::string("const"))                                
    wifi.SetObssPdAlgorithm ("ns3::ConstantObssPdAlgorithm",
                          "ObssPdLevel", DoubleValue (obssLevel));
  else if(obssAlgo==std::string("mesh")) 
    wifi.SetObssPdAlgorithm ("ns3::MeshObssPdAlgorithm",
                          "ObssPdLevel", DoubleValue (obssLevel));  
  else
    std::cout<<"no obss pd"<<std::endl;
  

  WifiMacHelper wifiMac;
  // wifiMac.SetType ("ns3::AdhocWifiMac",
  //                 "BE_MaxAmduSize", UintegerValue(65535),
  //                 "BK_MaxAmpduSize", UintegerValue(65535));
  wifiMac.SetType ("ns3::AdhocWifiMac");


  adhocDevices = wifi.Install(wifiPhy, wifiMac, apNodes);

  // set HE BSS color
  for (uint32_t i = 0; i < adhocDevices.GetN (); i++)
    {
        Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (adhocDevices.Get (i));
        //print mac address
        std::cout<< device->GetMac()->GetAddress()<<std::endl;
        Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
        heConfiguration->SetAttribute ("BssColor", UintegerValue (1));
    }

  if (pcap)
      wifiPhy.EnablePcapAll (std::string ("adhoc"));

  std::cout << "CreateAdhocDevices () DONE !!!\n";
}

void
AodvExample::CreateMeshDevices ()
{
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("ChannelNumber", UintegerValue (38));
  wifiPhy.Set ("Antennas", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (4));
   wifiPhy.Set ("TxPowerStart", DoubleValue (21.0));
   wifiPhy.Set ("TxPowerEnd", DoubleValue (21.0));
   wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  //wifiPhy.Set ("ShortGuardEnabled", BooleanValue (true));

  Config::SetDefault ("ns3::dot11s::PeerLink::MaxBeaconLoss", UintegerValue (20));
  Config::SetDefault ("ns3::dot11s::PeerLink::MaxRetries", UintegerValue (4));
  Config::SetDefault ("ns3::dot11s::PeerLink::MaxPacketFailure", UintegerValue (5));
  Config::SetDefault ("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPactivePathTimeout", TimeValue (Seconds (100)));
  Config::SetDefault ("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPactiveRootTimeout", TimeValue (Seconds (100)));
  Config::SetDefault ("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPmaxPREQretries", UintegerValue (5));
  Config::SetDefault ("ns3::dot11s::HwmpProtocol::UnicastPreqThreshold", UintegerValue (10));
  Config::SetDefault ("ns3::dot11s::HwmpProtocol::UnicastDataThreshold", UintegerValue (5));
  Config::SetDefault ("ns3::dot11s::HwmpProtocol::DoFlag", BooleanValue (false));
  Config::SetDefault ("ns3::dot11s::HwmpProtocol::RfFlag", BooleanValue (true));

  MeshHelper mesh = MeshHelper::Default ();
  mesh.SetStackInstaller ("ns3::Dot11sStack");
  mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
  mesh.SetMacType ("RandomStart", TimeValue (Seconds (startTime)),
                   "BeaconInterval", TimeValue (Seconds (beaconInterval)));
  mesh.SetStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);

  if (rateControl == std::string ("ideal"))
    mesh.SetRemoteStationManager ("ns3::IdealWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("minstrel"))
    mesh.SetRemoteStationManager ("ns3::MinstrelHtWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("arf"))
    mesh.SetRemoteStationManager ("ns3::ArfHtWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("aarf"))
    mesh.SetRemoteStationManager ("ns3::AarfHtWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else if (rateControl == std::string ("rraa"))
    mesh.SetRemoteStationManager ("ns3::RraaHtWifiManager");
  else if (rateControl == std::string ("ts"))
    mesh.SetRemoteStationManager ("ns3::TsHtWifiManager",
                                  "RtsCtsThreshold", UintegerValue (99999));
  else
    mesh.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "ControlMode", StringValue ("HeMcs0"),
                                  "DataMode", StringValue (heMcs),
                                  "RtsCtsThreshold", UintegerValue (99999));

  std::cout << "before install \n";
  meshDevices = mesh.Install (wifiPhy, apNodes);

  //  WifiMacHelper wifiMac;
  //  wifiMac.SetType ("ns3::AdhocWifiMac");
  //
  //  WifiHelper wifi;
  //  wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
  //  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
  //                                "ControlMode", StringValue ("VhtMcs0"),
  //                                "DataMode", StringValue ("VhtMcs7"),
  //                                "RtsCtsThreshold", UintegerValue (99999));
  //  meshDevices = wifi.Install (wifiPhy, wifiMac, apNodes);

  if (pcap)
    wifiPhy.EnablePcapAll (std::string ("mesh"));

  std::cout << "CreateMeshDevices () DONE !!!\n";
}

void
AodvExample::CreateCsmaDevices ()
{
  csmaNodes.Create (1);
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (lengthStep*2, widthStep/2.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (csmaNodes);

  // csmaNodes.Add (apNodes.Get ((uint32_t) apNum/2));
  csmaNodes.Add (apNodes.Get ((uint32_t) apNum/2 - 1)); // Node 3
  csmaNodes.Add (apNodes.Get ((uint32_t) apNum/2)); // Node 4

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("9999Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  csmaDevices = csma.Install (csmaNodes);

  std::cout << "CreateCsmaDevices () DONE !!!\n";
}

void
AodvExample::CreateWifiDevices ()
{
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "ControlMode", StringValue ("HtMcs0"),
                                "DataMode", StringValue ("HtMcs7"),
                                "RtsCtsThreshold", UintegerValue (99999));

  WifiMacHelper wifiMac;
  for (uint32_t i = 0; i < apNum; ++i)
    {
      std::ostringstream os;
      os << "ap-" << i ;
      Ssid ssid = Ssid (os.str ());
      // AP
      wifiMac.SetType ("ns3::ApWifiMac",
                       "EnableBeaconJitter", BooleanValue (false),
                       "Ssid", SsidValue (ssid));
        // #######
      //may need to create channel for every ap
      wifiPhy.SetChannel (wifiChannel.Create ());
      wifiPhy.Set ("ChannelNumber", UintegerValue (3)); 
      // channelNumber can be the same, but may have larger interference

      apDevices[i] = wifi.Install (wifiPhy, wifiMac, apNodes.Get (i));
      // client
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
      clDevices[i] = wifi.Install (wifiPhy, wifiMac, clNodes[i]);
    }

  if (pcap)
    wifiPhy.EnablePcapAll (std::string ("wifi"));

  std::cout << "CreateWifiDevices () DONE !!!\n";
}

void
AodvExample::InstallInternetStack ()
{
  AodvHelper aodv;
  aodv.Set ("AllowedHelloLoss", UintegerValue (20));
  aodv.Set ("HelloInterval", TimeValue (Seconds (3)));
  aodv.Set ("RreqRetries", UintegerValue (5));
  aodv.Set ("ActiveRouteTimeout", TimeValue (Seconds (100)));
  aodv.Set ("DestinationOnly", BooleanValue (false));

  OlsrHelper olsr;
  // olsr.ExcludeInterface(apNodes.Get(apNum/2), 2);

  DsdvHelper dsdv;

  // ########
  // try StaticRouting
  Ipv4StaticRoutingHelper staticRoutingHelper;

  Ipv4ListRoutingHelper list;
  if (route == std::string ("olsr"))
    list.Add (olsr, 100);
  else if (route == std::string ("dsdv"))
    list.Add (dsdv, 100);
  else if(route == std::string("aodv"))
    list.Add (aodv, 100);
  else // static
    list.Add(staticRoutingHelper,10);

  InternetStackHelper stack;
  stack.SetRoutingHelper (list); // has effect on the next Install () and others!!
  stack.Install (apNodes);
  //######
  stack.SetRoutingHelper (list); // has effect on the next Install ()
  // InternetStackHelper csmastack;
  // csmastack.SetRoutingHelper(staticRoutingHelper);
  stack.Install (csmaNodes.Get (0));

  for (uint32_t i = 0; i < apNum; ++i)
    {
      // stack.SetRoutingHelper (list);
      stack.Install (clNodes[i]);
    }


  // Assign Ip addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  if(macType==std::string("adhoc"))
    adhocInterfaces = address.Assign(adhocDevices);
  else
    meshInterfaces = address.Assign (meshDevices);
  address.SetBase ("10.2.1.0", "255.255.255.0");
  csmaInterfaces = address.Assign (csmaDevices);
  for (uint32_t i = 0; i < apNum; ++i)
    {
      std::ostringstream os;
      os << "10.1." << 11+i << ".0";
      address.SetBase (os.str ().c_str (), "255.255.255.0");
      apInterfaces[i] = address.Assign (apDevices[i]);
      clInterfaces[i] = address.Assign (clDevices[i]);
    }

  // ######
  // set static routing rules
  if(route == std::string("static"))
  {
    NodeContainer::Iterator iter;
    Ptr<Ipv4StaticRouting> staticRouting;
    // std::ostringstream os;
    // os << "10.1.1." << (int)( apNum/2 +1);
    // for (iter = apNodes.Begin (); iter != apNodes.End (); iter++)
    //   {
    //     Ptr<Ipv4StaticRouting> staticRouting;
    //     staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((*iter)->GetObject<Ipv4> ()->GetRoutingProtocol ());
    //     staticRouting->SetDefaultRoute (os.str().c_str(), 1 ); // 0 for loopback , 1 for mesh
    //   }
    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting>  (apNodes.Get(0)->GetObject<Ipv4>()->GetRoutingProtocol() );
    staticRouting->AddHostRouteTo("10.2.1.1","10.1.1.2",1);
    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting>  (apNodes.Get(1)->GetObject<Ipv4>()->GetRoutingProtocol() );
    staticRouting->AddHostRouteTo("10.2.1.1","10.1.1.3",1);
    staticRouting->AddHostRouteTo("10.1.1.1", 1);
    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting>  (apNodes.Get(2)->GetObject<Ipv4>()->GetRoutingProtocol() );
    staticRouting->AddHostRouteTo("10.2.1.1",2);
    staticRouting->AddHostRouteTo("10.1.1.1", "10.1.1.2", 1);

    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting>  (apNodes.Get(5)->GetObject<Ipv4>()->GetRoutingProtocol() );
    staticRouting->AddHostRouteTo("10.2.1.1","10.1.1.5",1);
    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting>  (apNodes.Get(4)->GetObject<Ipv4>()->GetRoutingProtocol() );
    staticRouting->AddHostRouteTo("10.2.1.1","10.1.1.4",1);
    staticRouting->AddHostRouteTo("10.1.1.6", 1);
    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting>  (apNodes.Get(3)->GetObject<Ipv4>()->GetRoutingProtocol() );
    staticRouting->AddHostRouteTo("10.1.1.6","10.1.1.5",1);
    staticRouting->AddHostRouteTo("10.2.1.1",2);

    // set csmaNodes(0) , the server
    iter = csmaNodes.Begin();
    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((*iter)->GetObject<Ipv4> ()->GetRoutingProtocol ());
    staticRouting->AddHostRouteTo("10.1.1.1", "10.2.1.2", 1);
    staticRouting->AddHostRouteTo("10.1.1.6", "10.2.1.3", 1);

    std::cout<< "Static Routing Rules Set.\n";
  }

  std::cout << "InstallInternetStack () DONE !!!\n";
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
              client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              clientApp[i*clNum+j].Start (Seconds (startTime));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (aptx)
            {
              if (i < naptx)
                continue;

              uint16_t port = 40000+i;
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
              client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
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
              client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[i*clNum+j] = client.Install (clNodes[i].Get (j));
              // Ptr<OnOffApplication> tempApp = clientApp[i*clNum+j].Get(0);
              // Ptr<TcpSocket> TcpS = tempApp->GetSocket();
              // TcpS->SetSynRetries();
              clientApp[i*clNum+j].Start (Seconds (startTime));
              clientApp[i*clNum+j].Stop (Seconds (totalTime + 0.1));
            }

          if (aptx)
            {
              if (i < naptx)
                continue;

              uint16_t port = 40000+i;
              Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
              PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
              serverApp[apNum*clNum+i] = server.Install (csmaNodes.Get (0)); //
              serverApp[apNum*clNum+i].Start (Seconds (1.0));
              serverApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
              packetSink[apNum*clNum+i] = StaticCast<PacketSink> (serverApp[apNum*clNum+i].Get (0));

              OnOffHelper client ("ns3::TcpSocketFactory", Address ());
              client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
              client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
              client.SetAttribute ("PacketSize", UintegerValue (1448));
              client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
              client.SetAttribute ("MaxBytes", UintegerValue (0));
              AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
              client.SetAttribute ("Remote", remoteAddress);
              clientApp[apNum*clNum+i] = client.Install (apNodes.Get (i));
              clientApp[apNum*clNum+i].Start (Seconds (startTime));
              clientApp[apNum*clNum+i].Stop (Seconds (totalTime + 0.1));
            }
        }
    }
  else if(app == std::string("udp-2"))
    {
      uint64_t lowDatarate = 1e5;
        // Node 1
        uint16_t port = 40000;
        Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
        serverApp[apNum*clNum] = server.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum].Start (Seconds (1.0));
        serverApp[apNum*clNum].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum] = StaticCast<PacketSink> (serverApp[apNum*clNum].Get (0));

        OnOffHelper client ("ns3::UdpSocketFactory", Address ());
        client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client.SetAttribute ("PacketSize", UintegerValue (1472));
        client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (lowDatarate))));
        client.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
        client.SetAttribute ("Remote", remoteAddress);
        clientApp[apNum*clNum] = client.Install (apNodes.Get (0));
        clientApp[apNum*clNum].Start (Seconds (startTime));
        clientApp[apNum*clNum].Stop (Seconds (totalTime + 0.1));

        // Node 6
        uint16_t port2 = 40000+apNum-1;
        Address localAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port2));
        PacketSinkHelper server2 ("ns3::UdpSocketFactory", localAddress2);
        serverApp[apNum*clNum+apNum-1] = server2.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum+apNum-1].Start (Seconds (1.0));
        serverApp[apNum*clNum+apNum-1].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum+apNum-1] = StaticCast<PacketSink> (serverApp[apNum*clNum+apNum-1].Get (0));

        OnOffHelper client2 ("ns3::UdpSocketFactory", Address ());
        client2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client2.SetAttribute ("PacketSize", UintegerValue (1472));
        client2.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (lowDatarate))));
        client2.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress2 (InetSocketAddress (csmaInterfaces.GetAddress (0), port2)); //
        client2.SetAttribute ("Remote", remoteAddress2);
        clientApp[apNum*clNum+apNum-1] = client2.Install (apNodes.Get (apNum-1));
        clientApp[apNum*clNum+apNum-1].Start (Seconds (startTime));
        clientApp[apNum*clNum+apNum-1].Stop (Seconds (totalTime + 0.1));

      double dataDelay = 20;

        // Node 1
        uint16_t port3 = 40001;
        Address localAddress3 (InetSocketAddress (Ipv4Address::GetAny (), port3));
        PacketSinkHelper server3 ("ns3::UdpSocketFactory", localAddress3);
        serverApp[apNum*clNum+1] = server3.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum+1].Start (Seconds (1.0));
        serverApp[apNum*clNum+1].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum+1] = StaticCast<PacketSink> (serverApp[apNum*clNum+1].Get (0));

        OnOffHelper client3 ("ns3::UdpSocketFactory", Address ());
        client3.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client3.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client3.SetAttribute ("PacketSize", UintegerValue (1472));
        client3.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
        client3.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress3 (InetSocketAddress (csmaInterfaces.GetAddress (0), port3)); //
        client3.SetAttribute ("Remote", remoteAddress3);
        clientApp[apNum*clNum+1] = client3.Install (apNodes.Get (0));
        clientApp[apNum*clNum+1].Start (Seconds (startTime+dataDelay));
        clientApp[apNum*clNum+1].Stop (Seconds (totalTime + 0.1));

        // Node 6
        uint16_t port4 = 40000+apNum -2;
        Address localAddress4 (InetSocketAddress (Ipv4Address::GetAny (), port4));
        PacketSinkHelper server4 ("ns3::UdpSocketFactory", localAddress4);
        serverApp[apNum*clNum+apNum-2] = server4.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum+apNum-2].Start (Seconds (1.0));
        serverApp[apNum*clNum+apNum-2].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum+apNum-2] = StaticCast<PacketSink> (serverApp[apNum*clNum+apNum-2].Get (0));

        OnOffHelper client4 ("ns3::UdpSocketFactory", Address ());
        client4.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client4.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client4.SetAttribute ("PacketSize", UintegerValue (1472));
        client4.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
        client4.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress4 (InetSocketAddress (csmaInterfaces.GetAddress (0), port4)); //
        client4.SetAttribute ("Remote", remoteAddress4);
        clientApp[apNum*clNum+apNum-2] = client4.Install (apNodes.Get (apNum-1));
        clientApp[apNum*clNum+apNum-2].Start (Seconds (dataDelay+startTime));
        clientApp[apNum*clNum+apNum-2].Stop (Seconds (totalTime + 0.1));



    }
  else if(app == std::string("mini-test"))
    {
      uint8_t delay=2;
        // Node 1
        uint16_t port = 40000;
        Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
        serverApp[apNum*clNum] = server.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum].Start (Seconds (1.0));
        serverApp[apNum*clNum].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum] = StaticCast<PacketSink> (serverApp[apNum*clNum].Get (0));

        OnOffHelper client ("ns3::UdpSocketFactory", Address ());
        client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client.SetAttribute ("PacketSize", UintegerValue (1472));
        client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
        client.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
        client.SetAttribute ("Remote", remoteAddress);
        clientApp[apNum*clNum] = client.Install (apNodes.Get (0));
        clientApp[apNum*clNum].Start (Seconds (startTime));
        clientApp[apNum*clNum].Stop (Seconds (delay -1 + 0.1));

        // Node 6
        uint16_t port2 = 40000+apNum-1;
        Address localAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port2));
        PacketSinkHelper server2 ("ns3::UdpSocketFactory", localAddress2);
        serverApp[apNum*clNum+apNum-1] = server2.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum+apNum-1].Start (Seconds (1.0));
        serverApp[apNum*clNum+apNum-1].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum+apNum-1] = StaticCast<PacketSink> (serverApp[apNum*clNum+apNum-1].Get (0));

        OnOffHelper client2 ("ns3::UdpSocketFactory", Address ());
        client2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client2.SetAttribute ("PacketSize", UintegerValue (1472));
        client2.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
        client2.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress2 (InetSocketAddress (csmaInterfaces.GetAddress (0), port2)); //
        client2.SetAttribute ("Remote", remoteAddress2);
        clientApp[apNum*clNum+apNum-1] = client2.Install (apNodes.Get (apNum-1));
        clientApp[apNum*clNum+apNum-1].Start (Seconds (startTime));
        clientApp[apNum*clNum+apNum-1].Stop (Seconds (delay-1 + 0.1));





      uint32_t payloadSize =2500;
      Ptr<WifiNetDevice> netDevices[10];
      for(uint32_t i=0;i<apNum;i++)
      {
        netDevices[i] = DynamicCast<WifiNetDevice> (adhocDevices.Get (i));
      }

      // In order to have all ADDBA handshakes established, each AP and STA sends a packet
      Simulator::Schedule (Seconds (delay+0.25), &AodvExample::SendOnePacket, this, netDevices[0], netDevices[2], payloadSize);
      Simulator::Schedule (Seconds (delay+0.5), &AodvExample::SendOnePacket, this, netDevices[5], netDevices[4], payloadSize);

      // We test PHY state and verify whether a CCA reset did occur.

      // Node 1 sends a packet 0.5s later.
      Simulator::Schedule (Seconds (delay+2.0), &AodvExample::SendOnePacket, this, netDevices[0], netDevices[2], payloadSize);
      Simulator::Schedule (Seconds (delay+2.0) + MicroSeconds (1), &AodvExample::CheckPhyState, this, netDevices[0], WifiPhyState::TX);
      // All other PHYs should have stay idle until 4us (preamble detection time).
      Simulator::Schedule (Seconds (delay+2.0) + MicroSeconds (2), &AodvExample::CheckPhyState, this, netDevices[1], WifiPhyState::IDLE);
      Simulator::Schedule (Seconds (delay+2.0) + MicroSeconds (2), &AodvExample::CheckPhyState, this, netDevices[2], WifiPhyState::IDLE);
      Simulator::Schedule (Seconds (delay+2.0) + MicroSeconds (2), &AodvExample::CheckPhyState, this, netDevices[5], WifiPhyState::IDLE);
      // All PHYs should be receiving the PHY header if preamble has been detected (always the case in this test).
      Simulator::Schedule (Seconds (delay+2.0) + MicroSeconds (10), &AodvExample::CheckPhyState, this, netDevices[5], WifiPhyState::RX);
      // PHYs of AP1 and STA1 should be idle if they were reset by OBSS_PD SR, otherwise they should be receiving.
      Simulator::Schedule (Seconds (delay+2.0) + MicroSeconds (50), &AodvExample::CheckPhyState, this, netDevices[5], WifiPhyState::IDLE);
      // STA2 should be receiving
      Simulator::Schedule (Seconds (delay+2.0) + MicroSeconds (150), &AodvExample::CheckPhyState, this, netDevices[1], WifiPhyState::RX);

      // We test whether two networks can transmit simultaneously, and whether transmit power restrictions are applied.

      // Node 6 sends another packet 0.1s later.
      Simulator::Schedule (Seconds (delay+2.1), &AodvExample::SendOnePacket, this, netDevices[0], netDevices[1], payloadSize);
      // STA1 sends a packet 100us later. Even though AP2 is still transmitting, STA1 can transmit simultaneously if it's PHY was reset by OBSS_PD SR.
      Simulator::Schedule (Seconds (delay+2.1) + MicroSeconds (120), &AodvExample::SendOnePacket, this, netDevices[5], netDevices[4], payloadSize);
      // Check simultaneous transmissions
      Simulator::Schedule (Seconds (delay+2.1) + MicroSeconds (125), &AodvExample::CheckPhyState, this, netDevices[0],WifiPhyState::TX );
      Simulator::Schedule (Seconds (delay+2.1) + MicroSeconds (125), &AodvExample::CheckPhyState, this, netDevices[1], WifiPhyState::RX);
      Simulator::Schedule (Seconds (delay+2.1) + MicroSeconds (125), &AodvExample::CheckPhyState, this, netDevices[4], WifiPhyState::RX);
      Simulator::Schedule (Seconds (delay+2.1) + MicroSeconds (125), &AodvExample::CheckPhyState, this, netDevices[5], WifiPhyState::TX);
    }
  else if(app==std::string("udp-new"))
  {
    // Node 1
        uint16_t port = 40000;
        Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
        serverApp[apNum*clNum] = server.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum].Start (Seconds (1.0));
        serverApp[apNum*clNum].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum] = StaticCast<PacketSink> (serverApp[apNum*clNum].Get (0));

        OnOffHelper client ("ns3::UdpSocketFactory", Address ());
        client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client.SetAttribute ("PacketSize", UintegerValue (1472));
        client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (1e5))));
        client.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
        client.SetAttribute ("Remote", remoteAddress);
        clientApp[apNum*clNum] = client.Install (apNodes.Get (0));
        clientApp[apNum*clNum].Start (Seconds (startTime));
        clientApp[apNum*clNum].Stop (Seconds (totalTime + 0.1));

        // Node 6
        uint16_t port2 = 40000+apNum-1;
        Address localAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port2));
        PacketSinkHelper server2 ("ns3::UdpSocketFactory", localAddress2);
        serverApp[apNum*clNum+apNum-1] = server2.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum+apNum-1].Start (Seconds (1.0));
        serverApp[apNum*clNum+apNum-1].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum+apNum-1] = StaticCast<PacketSink> (serverApp[apNum*clNum+apNum-1].Get (0));

        OnOffHelper client2 ("ns3::UdpSocketFactory", Address ());
        client2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client2.SetAttribute ("PacketSize", UintegerValue (1472));
        client2.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (1e5))));
        client2.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress2 (InetSocketAddress (csmaInterfaces.GetAddress (0), port2)); //
        client2.SetAttribute ("Remote", remoteAddress2);
        clientApp[apNum*clNum+apNum-1] = client2.Install (apNodes.Get (apNum-1));
        clientApp[apNum*clNum+apNum-1].Start (Seconds (startTime));
        clientApp[apNum*clNum+apNum-1].Stop (Seconds (totalTime + 0.1));

        for(int idx=1;idx<=datarate/1e6;idx++)
        {
          Simulator::Schedule(Seconds(idx*10),Config::Set,"/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/DataRate",DataRateValue (DataRate ((uint64_t) (1e6*idx))));
        }
  }
  else if (app==std::string("tcp-new"))
  {
        // Node 1
        uint16_t port = 40000;
        Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
        serverApp[apNum*clNum] = server.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum].Start (Seconds (1.0));
        serverApp[apNum*clNum].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum] = StaticCast<PacketSink> (serverApp[apNum*clNum].Get (0));

        OnOffHelper client ("ns3::TcpSocketFactory", Address ());
        client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client.SetAttribute ("PacketSize", UintegerValue (1472));
        client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (1e5))));
        client.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
        client.SetAttribute ("Remote", remoteAddress);
        clientApp[apNum*clNum] = client.Install (apNodes.Get (0));
        clientApp[apNum*clNum].Start (Seconds (startTime));
        clientApp[apNum*clNum].Stop (Seconds (totalTime + 0.1));

        // Node 6
        uint16_t port2 = 40000+apNum-1;
        Address localAddress2 (InetSocketAddress (Ipv4Address::GetAny (), port2));
        PacketSinkHelper server2 ("ns3::TcpSocketFactory", localAddress2);
        serverApp[apNum*clNum+apNum-1] = server2.Install (csmaNodes.Get (0)); //
        serverApp[apNum*clNum+apNum-1].Start (Seconds (1.0));
        serverApp[apNum*clNum+apNum-1].Stop (Seconds (totalTime + 0.1));
        packetSink[apNum*clNum+apNum-1] = StaticCast<PacketSink> (serverApp[apNum*clNum+apNum-1].Get (0));

        OnOffHelper client2 ("ns3::TcpSocketFactory", Address ());
        client2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        client2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        client2.SetAttribute ("PacketSize", UintegerValue (1472));
        client2.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (1e5))));
        client2.SetAttribute ("MaxBytes", UintegerValue (0));
        AddressValue remoteAddress2 (InetSocketAddress (csmaInterfaces.GetAddress (0), port2)); //
        client2.SetAttribute ("Remote", remoteAddress2);
        clientApp[apNum*clNum+apNum-1] = client2.Install (apNodes.Get (apNum-1));
        clientApp[apNum*clNum+apNum-1].Start (Seconds (startTime));
        clientApp[apNum*clNum+apNum-1].Stop (Seconds (totalTime + 0.1));

        for(int idx=1;idx<=datarate/1e6;idx++)
        {
          Simulator::Schedule(Seconds(idx*10),Config::Set,"/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/DataRate",DataRateValue (DataRate ((uint64_t) (1e6*idx))));
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
        fin >> locations[i][0] >> locations[i][1] >> locations[i][2];
      fin.close ();
    }
}

void
AodvExample::CheckPhyState (Ptr<WifiNetDevice> device, WifiPhyState expectedState)
{
  WifiPhyState currentState;
  PointerValue ptr;
  Ptr<WifiPhy> phy = DynamicCast<WifiPhy> (device->GetPhy ());
  phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  if(currentState!=expectedState)
    std::cout <<"IP = "<< device->GetAddress()<<"  PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now () << std::endl;
  else
  {
    std::cout <<"IP = "<< device->GetAddress()<<"  PHY State " << currentState << " does match expected state " << expectedState << " at " << Simulator::Now () << "!!!!!!!"<< std::endl;
  }
  
}

void
AodvExample::SendOnePacket (Ptr<WifiNetDevice> tx_dev, Ptr<WifiNetDevice> rx_dev, uint32_t payloadSize)
{
  Ptr<Packet> p = Create<Packet> (payloadSize);
  tx_dev->Send (p, rx_dev->GetAddress (), 1);
}
