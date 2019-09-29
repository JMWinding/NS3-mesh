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
PrintThroughputTitle (uint32_t apNum)
{
  std::cout << "-------------------------------------------------\n";
  std::cout << "Time[s]";
  for (uint32_t i = 0; i < apNum; ++i)
      std::cout << '\t' << "cl-" << i;
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
  // Simulation parameters
  double totalTime;
  double monitorInterval;
  bool anim;
  std::string flowout;
  bool pcap;
  // Layout parameters
  std::string locationFile;
  double scale;
  uint32_t gridSize;
  uint32_t apNum;
  double apXStep;
  double apYStep;
  double clStep;
  std::string gateways;
  // MAC parameters
  bool linkFail;
  std::string mac;
  // Rate adaptation parameters
  std::string rateControl;
  std::string constantRate;
  // Routing parameters
  std::string route;
  bool printRoutes;
  // App parameters
  std::string app;
  std::string appl;
  bool aptx;
  double startTime;
  double datarate;
  bool datarateUp;
  // obss pd
  bool isObss;
  double obssLevel;
  // network
  /// nodes used in the example
  NodeContainer apNodes;
  NodeContainer clNodes;
  /// devices used i0n the example
  NetDeviceContainer meshDevices;
  NetDeviceContainer apDevices;
  NetDeviceContainer clDevices;
  /// interfaces used in the example
  Ipv4InterfaceContainer meshInterfaces;
  Ipv4InterfaceContainer apInterfaces;
  Ipv4InterfaceContainer clInterfaces;
  /// application
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;
  /// specified real implementation
  std::vector<std::vector<double>> locations;
  NodeContainer csmaNodes;
  NetDeviceContainer csmaDevices;
  Ipv4InterfaceContainer csmaInterfaces;

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
  /// Create the simulation applications
  void InstallApplications ();
  /// Connect gateways
  void CreateCsmaDevices ();
  void ReadLocations ();
  void PreSetStationManager ();
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
  // Simulation parameters
  totalTime (100),
  monitorInterval (1),
  anim (false),
  flowout ("out-flow.xml"),
  pcap (false),
  // Layout parameters
  locationFile (""),
  scale (1),
  gridSize (3), 
  apNum (2),
  apXStep (20),
  apYStep (30),
  clStep (10),
  gateways ("0"),
  // MAC parameters
  linkFail (false),
  mac ("mesh"),
  // Rate adaptation parameters
  rateControl ("constant"),
  constantRate ("VhtMcs0"),
  // Routing parameters
  route ("aodv"),
  printRoutes ("false"),
  // App parameters
  app ("tcp"),
  appl ("1"),
  aptx (true),
  startTime (10),
  datarate (1e6),
  datarateUp (false),
  isObss(false),
  obssLevel(-62)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  Packet::EnablePrinting ();
  // LogComponentEnable("MeshObssPdAlgorithm", LOG_LEVEL_ALL);
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  // LogComponentEnable("ObssWifiManager", LOG_LEVEL_ALL);

  CommandLine cmd;

  cmd.AddValue ("totalTime", "Simulation time, s.", totalTime);
  cmd.AddValue ("monitorInterval", "Monitor interval, s.", monitorInterval);
  cmd.AddValue ("anim", "Output netanim .xml file or not.", anim);
  cmd.AddValue ("flowout", "Result output directory", flowout);
  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);

  cmd.AddValue ("locationFile", "Location file name.", locationFile);
  cmd.AddValue ("scale", "Ratio between experiment and simulation.", scale);
  cmd.AddValue ("gridSize", "Size of AP grid.", gridSize);
  cmd.AddValue ("apNum", "Number of AP nodes.", apNum);
  cmd.AddValue ("apXStep", "AP grid X step, m.", apXStep);
  cmd.AddValue ("apYStep", "AP grid Y step, m.", apYStep);
  cmd.AddValue ("clStep", "CL range around AP.", clStep);
  cmd.AddValue ("gateways", "Index of gateway AP.", gateways);

  cmd.AddValue ("linkFail", "Enable link failure model or not.", linkFail);
  cmd.AddValue ("mac", "MAC type", mac);

  cmd.AddValue ("rateControl", "Rate control--constant/ideal/minstrel.", rateControl);
  cmd.AddValue ("constantRate", "Rate used for ConstantRateManager.", constantRate);

  cmd.AddValue ("route", "Routing protocol.", route);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);

  cmd.AddValue ("app", "UDP or TCP.", app);
  cmd.AddValue ("appl", "Index of nodes with application.", appl);
  cmd.AddValue ("aptx", "Install application on AP or CL.", aptx);
  cmd.AddValue ("startTime", "Application start time, s.", startTime);
  cmd.AddValue ("datarate", "Tested application datarate.", datarate);
  cmd.AddValue ("datarateUp", "Increase datarate or not.", datarateUp);
  cmd.AddValue ("isObss", "Use obss pd or not", isObss);
  cmd.AddValue ("obssLevel", "Obss pd thershold level", obssLevel);

  cmd.Parse (argc, argv);

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

  AnimationInterface netanim ("out-anim.xml");
  // netanim.SetMaxPktsPerTraceFile (50000);
  if (!anim)
    netanim.SetStopTime (Seconds (0));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Schedule (Seconds (startTime), &PrintThroughputTitle, apNum);
  Simulator::Schedule (Seconds (startTime+monitorInterval), &CalculateThroughput, monitorInterval);

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
  lastTotalRx.reserve (apNum);
  throughput.reserve (apNum);
  packetSink.reserve (apNum);

  for (uint32_t i = 0; i < apNum; ++i)
    {
      locations.push_back (std::vector<double> (4, 0));
      lastTotalRx.push_back (0);
      throughput.push_back (0);
      packetSink.push_back (NULL);
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
  std::cout << "Creating " << (unsigned)apNum << " APs "
            << apXStep << " and " << apYStep << " m apart.\n";
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
                                     "DeltaX", DoubleValue (apXStep),
                                     "DeltaY", DoubleValue (apYStep),
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
  std::cout << "Creating CL " << clStep << " m away from each AP.\n";
  for (uint32_t i = 0; i < apNum; ++i)
    {
      NodeContainer nodes;
      nodes.Create (1);
      std::ostringstream os;
      os << "cl-" << i;
      Names::Add (os.str(), nodes.Get (0));

      Vector center = apNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ();
      std::ostringstream randomRho;
      randomRho << "ns3::UniformRandomVariable[Min=0|Max=" << clStep << "]";
      MobilityHelper mobility;
      mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                     "X", DoubleValue (center.x),
                                     "Y", DoubleValue (center.y),
                                     "Rho", StringValue (randomRho.str ()));
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (nodes);

      clNodes.Add (nodes);
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
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  if (linkFail)
    {
      wifiChannel.AddPropagationLoss ("ns3::LinkBreakPropagationLossModel",
                                      "Start", DoubleValue (60),
                                      "End", DoubleValue (1e3),
                                      "BreakProb", DoubleValue (0.05),
                                      "Period", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"));
      wifiChannel.AddPropagationLoss ("ns3::NodeDownPropagationLossModel",
                                      "Start", DoubleValue (60),
                                      "End", DoubleValue (1e3),
                                      "DownProb", DoubleValue (0.05),
                                      "Period", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"));
      wifiChannel.AddPropagationLoss ("ns3::ChannelChangePropagationLossModel",
                                      "Start", DoubleValue (60),
                                      "End", DoubleValue (1e3),
                                      "Amplitude", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=3.0|Bound=6.0]"),
                                      "Period", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"));
    }

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("ChannelNumber", UintegerValue (38));
  wifiPhy.Set ("Antennas", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (4));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (4));
 wifiPhy.Set ("TxPowerStart", DoubleValue (1.0));
 wifiPhy.Set ("TxPowerEnd", DoubleValue (21.0));
 wifiPhy.Set ("TxPowerLevels", UintegerValue (10));
  wifiPhy.Set ("ShortGuardEnabled", BooleanValue (true));

  // if(isObss)
  // {
  //     wifiPhy.DisablePreambleDetectionModel ();
  // }

  if (pcap)
    wifiPhy.EnablePcapAll (std::string ("aodv"));

  if (mac == std::string ("mesh"))
    {
      MeshHelper mesh = MeshHelper::Default ();

      mesh.SetStackInstaller ("ns3::Dot11sStack");
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
      Config::SetDefault ("ns3::dot11s::HwmpProtocol::MaxTtl", UintegerValue (2));

      mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
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
      else
        mesh.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "ControlMode", StringValue ("HtMcs0"),
                                      "DataMode", StringValue (constantRate),
                                      "RtsCtsThreshold", UintegerValue (99999));
      meshDevices = mesh.Install (wifiPhy, apNodes);
    }
  else
    {
      WifiMacHelper wifiMac;
      wifiMac.SetType ("ns3::AdhocWifiMac");

      WifiHelper wifi;
      wifi.SetStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
      if (rateControl == std::string ("ideal"))
        wifi.SetRemoteStationManager ("ns3::IdealWifiManager",
                                      "RtsCtsThreshold", UintegerValue (99999));
      else if (rateControl == std::string ("obss"))
        wifi.SetRemoteStationManager ("ns3::ObssWifiManager",
                                      "RtsCtsThreshold", UintegerValue (99999),
                                      "DefaultTxPowerLevel", UintegerValue(9));
      else if (rateControl == std::string ("minstrel"))
        wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager",
                                      "RtsCtsThreshold", UintegerValue (99999));
      else if (rateControl == std::string ("arf"))
        wifi.SetRemoteStationManager ("ns3::ArfHtWifiManager",
                                      "RtsCtsThreshold", UintegerValue (99999));
      else if (rateControl == std::string ("aarf"))
        wifi.SetRemoteStationManager ("ns3::AarfHtWifiManager",
                                      "RtsCtsThreshold", UintegerValue (99999));
      else
        wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "ControlMode", StringValue ("HeMcs0"),
                                      "DataMode", StringValue (constantRate),
                                      "RtsCtsThreshold", UintegerValue (99999));

      if(isObss)
      {
        wifi.SetObssPdAlgorithm ("ns3::MeshObssPdAlgorithm",
                      "ObssPdLevel", DoubleValue (obssLevel));  
      }
      meshDevices = wifi.Install (wifiPhy, wifiMac, apNodes);
    }

    if(isObss)
    {
      // set HE BSS color
      for (uint32_t i = 0; i < meshDevices.GetN (); i++)
      {
          Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (meshDevices.Get (i));
          //print mac address
          std::cout<< device->GetMac()->GetAddress()<<std::endl;
          Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
          heConfiguration->SetAttribute ("BssColor", UintegerValue (1));
      }
    }

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

  std::replace (gateways.begin (), gateways.end (), '+', ' ');
  std::vector<int> array;
  std::stringstream ss(gateways);
  uint32_t temp;
  while (ss >> temp)
    array.push_back (temp);

  for (uint32_t i = 0; i < array.size (); ++i)
    csmaNodes.Add (apNodes.Get (array[i]));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("9999Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  csmaDevices = csma.Install (csmaNodes);

  std::cout << "CreateCsmaDevices () DONE !!!\n";
}

void
AodvExample::CreateWifiDevices ()
{

  for (uint32_t i = 0; i < apNum; ++i)
    {
      YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
      YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
      wifiPhy.SetChannel (wifiChannel.Create ());
      wifiPhy.Set ("ChannelNumber", UintegerValue (3));

      WifiHelper wifi;
      wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
      wifi.SetRemoteStationManager ("ns3::IdealWifiManager",
                                "RtsCtsThreshold", UintegerValue (99999));

      if (pcap)
        wifiPhy.EnablePcapAll (std::string ("aodv"));

      NetDeviceContainer devices;

      std::ostringstream os;
      os << "ap-" << i ;
      Ssid ssid = Ssid (os.str ());
      WifiMacHelper wifiMac;
      // AP
      wifiMac.SetType ("ns3::ApWifiMac",
                       "EnableBeaconJitter", BooleanValue (false),
                       "Ssid", SsidValue (ssid));
      devices = wifi.Install (wifiPhy, wifiMac, apNodes.Get (i));
      apDevices.Add (devices);
      // client
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
      devices = wifi.Install (wifiPhy, wifiMac, clNodes.Get (i));
      clDevices.Add (devices);
    }

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

  DsdvHelper dsdv;

  Ipv4ListRoutingHelper list;
  if (route == std::string ("olsr"))
    list.Add (olsr, 100);
  else if (route == std::string ("dsdv"))
    list.Add (dsdv, 100);
  else
    list.Add (aodv, 100);

  InternetStackHelper stack;
  stack.SetRoutingHelper (list); // has effect on the next Install ()
  stack.Install (apNodes);
  stack.SetRoutingHelper (list); // has effect on the next Install ()
  stack.Install (clNodes);
  stack.SetRoutingHelper (list); // has effect on the next Install ()
  stack.Install (csmaNodes.Get (0));

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  meshInterfaces = address.Assign (meshDevices);
  address.SetBase ("10.2.1.0", "255.255.255.0");
  csmaInterfaces = address.Assign (csmaDevices);
  for (uint32_t i = 0; i < apNum; ++i)
    {
      Ipv4InterfaceContainer interfaces;

      std::ostringstream os;
      os << "10.1." << 11+i << ".0";
      address.SetBase (os.str ().c_str (), "255.255.255.0");
      interfaces = address.Assign (apDevices.Get (i));
      apInterfaces.Add (interfaces);
      interfaces = address.Assign (clDevices.Get (i));
      clInterfaces.Add (interfaces);
    }

  std::cout << "InstallInternetStack () DONE !!!\n";
}

void
AodvExample::InstallApplications ()
{
  ApplicationContainer apps;
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (0.0));
  x->SetAttribute ("Max", DoubleValue (0.1));

  std::replace (appl.begin (), appl.end (), '+', ' ');
  std::vector<int> array;
  std::stringstream ss(appl);
  uint32_t temp;
  while (ss >> temp)
    array.push_back (temp);

  if (app == std::string("udp"))
    {
      for (uint32_t j = 0; j < array.size(); ++j)
        {
          uint32_t i = array[j];
          uint16_t port = 50000+i;
          Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
          PacketSinkHelper server ("ns3::UdpSocketFactory", localAddress);
          apps = server.Install (csmaNodes.Get (0)); //
          apps.Start (Seconds (0.1));
          apps.Stop (Seconds (totalTime + 0.1));
          packetSink[i] = StaticCast<PacketSink> (apps.Get (0));
          serverApps.Add (apps);

          OnOffHelper client ("ns3::UdpSocketFactory", Address ());
          client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          client.SetAttribute ("PacketSize", UintegerValue (1472));
          client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
          client.SetAttribute ("MaxBytes", UintegerValue (0));
          AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
          client.SetAttribute ("Remote", remoteAddress);
          if (aptx)
            apps = client.Install (apNodes.Get (i));
          else
            apps = client.Install (clNodes.Get (i));
          apps.Start (Seconds (startTime + x->GetValue ()));
          apps.Stop (Seconds (totalTime + 0.1));
          clientApps.Add (apps);
        }
    }

  if (app == std::string("tcp"))
    {
      for (uint32_t j = 0; j < array.size(); ++j)
        {
          uint32_t i = array[j];
          uint16_t port = 50000+i;
          Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
          PacketSinkHelper server ("ns3::TcpSocketFactory", localAddress);
          apps = server.Install (csmaNodes.Get (0)); //
          apps.Start (Seconds (0.1));
          apps.Stop (Seconds (totalTime + 0.1));
          packetSink[i] = StaticCast<PacketSink> (apps.Get (0));
          serverApps.Add (apps);

          OnOffHelper client ("ns3::TcpSocketFactory", Address ());
          client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
          client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          client.SetAttribute ("PacketSize", UintegerValue (1448));
          client.SetAttribute ("DataRate", DataRateValue (DataRate ((uint64_t) (datarate))));
          client.SetAttribute ("MaxBytes", UintegerValue (0));
          AddressValue remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (0), port)); //
          client.SetAttribute ("Remote", remoteAddress);
          if (aptx)
            apps = client.Install (apNodes.Get (i));
          else
            apps = client.Install (clNodes.Get (i));
          apps.Start (Seconds (startTime + x->GetValue ()));
          apps.Stop (Seconds (totalTime + 0.1));
          clientApps.Add (apps);
        }
    }

  if (datarateUp)
    for (uint32_t i = 1; i < (totalTime/10); ++i)
      Simulator::Schedule (Seconds (startTime+10*i), Config::Set, "/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/DataRate",
                           DataRateValue (DataRate ((uint64_t) (datarate*(i+1)))));

  std::cout << "InstallApplications () DONE !!!\n";
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
