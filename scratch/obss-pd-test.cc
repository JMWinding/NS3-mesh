/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Input locations of routers
 * Mesh with 802.11n/ac fixed
 * WIFI_MAC_MGT_ACTION for MESH and BLOCK_ACK modulation changed
 * Flow monitor - delay
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

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
  double apStep;
  double clStep;

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
  apNum (2),
  clNum (0),
  apStep (40),
  clStep (20),
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
  obssLevel(-100)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
//   LogComponentEnable("TsHtWifiManager", LOG_LEVEL_ALL);
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
  cmd.AddValue ("apStep", "AP grid step, m", apStep);
  cmd.AddValue ("clStep", "CL grid step, m", clStep);
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
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

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
  wifiPhy.Set ("Antennas", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (4));
  wifiPhy.DisablePreambleDetectionModel ();
   wifiPhy.Set ("TxPowerStart", DoubleValue (21.0));
   wifiPhy.Set ("TxPowerEnd", DoubleValue (21.0));
  //  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
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
                                  "DataMode", StringValue ("HeMcs4"),
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
  //  wifiPhy.Set ("TxPowerStart", DoubleValue (21.0));
  //  wifiPhy.Set ("TxPowerEnd", DoubleValue (21.0));
  //  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
//   wifiPhy.Set ("ShortGuardEnabled", BooleanValue (true));

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
                                  "ControlMode", StringValue ("VhtMcs0"),
                                  "DataMode", StringValue ("VhtMcs7"),
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
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (csmaNodes);

  // csmaNodes.Add (apNodes.Get ((uint32_t) apNum/2));
  csmaNodes.Add (apNodes.Get ((uint32_t) apNum -1)); // change to a line
  for (uint32_t i = 1; i < gateways; ++i)
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
 // else // static
  // list.Add(staticRoutingHelper,10);

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
    std::ostringstream os;
    os << "10.1.1." << (int)( apNum/2 +1);
    for (iter = apNodes.Begin (); iter != apNodes.End (); iter++)
      {
        Ptr<Ipv4StaticRouting> staticRouting;
        staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((*iter)->GetObject<Ipv4> ()->GetRoutingProtocol ());
        staticRouting->SetDefaultRoute (os.str().c_str(), 1 ); // 0 for loopback , 1 for mesh
      }
    // set csmaNodes(0) , the server
    iter = csmaNodes.Begin();
    Ptr<Ipv4StaticRouting> staticRouting;
    staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((*iter)->GetObject<Ipv4> ()->GetRoutingProtocol ());
    staticRouting->SetDefaultRoute ("10.2.1.2", 1 );

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

// void
// AodvExample::TxSetColor (std::string context, Ptr<const Packet> p, double txPowerW)
// {
//   uint32_t idx = ConvertContextToNodeId (context);
//   std::cout <<"Node: "<<idx <<"  "<<  p << std::endl;
//   // p->Print(std::cout);
//   // std::cout<<std::endl;
//   WifiMacHeader head;
//   p->PeekHeader(head);
//   std::cout<< head.GetAddr1() <<std::endl;
//   Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (adhocDevices.Get (idx));
//   Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
//   if(idx)
//     heConfiguration->SetAttribute ("BssColor", UintegerValue (42));
//   else
//     heConfiguration->SetAttribute ("BssColor", UintegerValue (40));
//   return;
// }