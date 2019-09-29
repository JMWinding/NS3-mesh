/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/log.h"
#include "obss-wifi-manager.h"
#include "wifi-phy.h"

namespace ns3 {
/**
 * \brief hold per-remote-station state for Ideal Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the Ideal Wifi manager
 */
struct ObssWifiRemoteStation : public WifiRemoteStation
{
  double m_lastSnrObserved;  //!< SNR of most recently reported packet sent to the remote station
  double m_lastSnrCached;    //!< SNR most recently used to select a rate
  uint8_t m_nss;             //!< Number of spatial streams
  WifiMode m_lastMode;       //!< Mode most recently used to the remote station
};

/// To avoid using the cache before a valid value has been cached
static const double CACHE_INITIAL_VALUE = -100;

static ObssWifiManager::PathLossPairs globalLossPairs; // (dst+src, loss)

NS_OBJECT_ENSURE_REGISTERED (ObssWifiManager);

NS_LOG_COMPONENT_DEFINE ("ObssWifiManager");

TypeId
ObssWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ObssWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ObssWifiManager> ()
    .AddAttribute ("BerThreshold",
                   "The maximum Bit Error Rate acceptable at any transmission mode",
                   DoubleValue (1e-5),
                   MakeDoubleAccessor (&ObssWifiManager::m_ber),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("Rate",
                     "Traced value for rate changes (b/s)",
                     MakeTraceSourceAccessor (&ObssWifiManager::m_currentRate),
                     "ns3::TracedValueCallback::Uint64")
  ;
  return tid;
}

ObssWifiManager::ObssWifiManager ()
  : m_currentRate (0)
{
  NS_LOG_FUNCTION (this);
}

ObssWifiManager::~ObssWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

void
ObssWifiManager::SetupPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  WifiRemoteStationManager::SetupPhy (phy);
}

uint16_t
ObssWifiManager::GetChannelWidthForMode (WifiMode mode) const
{
  NS_ASSERT (mode.GetModulationClass () != WIFI_MOD_CLASS_HT
             && mode.GetModulationClass () != WIFI_MOD_CLASS_VHT
             && mode.GetModulationClass () != WIFI_MOD_CLASS_HE);
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS
      || mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
    {
      return 22;
    }
  else
    {
      return 20;
    }
}

void
ObssWifiManager::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  WifiMode mode;
  WifiTxVector txVector;
  uint8_t nss = 1;
  uint8_t nModes = GetPhy ()->GetNModes ();
  for (uint8_t i = 0; i < nModes; i++)
    {
      mode = GetPhy ()->GetMode (i);
      txVector.SetChannelWidth (GetChannelWidthForMode (mode));
      txVector.SetNss (nss);
      txVector.SetMode (mode);
      NS_LOG_DEBUG ("Initialize, adding mode = " << mode.GetUniqueName ());
      AddSnrThreshold (txVector, GetPhy ()->CalculateSnr (txVector, m_ber));
    }
  // Add all HT, VHT and HE MCSes
  if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
    {
      nModes = GetPhy ()->GetNMcs ();
      for (uint8_t i = 0; i < nModes; i++)
        {
          for (uint16_t j = 20; j <= GetPhy ()->GetChannelWidth (); j *= 2)
            {
              txVector.SetChannelWidth (j);
              mode = GetPhy ()->GetMcs (i);
              if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
                {
                  uint16_t guardInterval = GetShortGuardIntervalSupported () ? 400 : 800;
                  txVector.SetGuardInterval (guardInterval);
                  //derive NSS from the MCS index
                  nss = (mode.GetMcsValue () / 8) + 1;
                  NS_LOG_DEBUG ("Initialize, adding mode = " << mode.GetUniqueName () <<
                                " channel width " << j <<
                                " nss " << +nss <<
                                " GI " << guardInterval);
                  NS_LOG_DEBUG ("In SetupPhy, adding mode = " << mode.GetUniqueName ());
                  txVector.SetNss (nss);
                  txVector.SetMode (mode);
                  AddSnrThreshold (txVector, GetPhy ()->CalculateSnr (txVector, m_ber));
                }
              else //VHT or HE
                {
                  uint16_t guardInterval;
                  if (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
                    {
                      guardInterval = GetShortGuardIntervalSupported () ? 400 : 800;
                    }
                  else
                    {
                      guardInterval = GetGuardInterval ();
                    }
                  txVector.SetGuardInterval (guardInterval);
                  for (uint8_t k = 1; k <= GetPhy ()->GetMaxSupportedTxSpatialStreams (); k++)
                    {
                      NS_LOG_DEBUG ("Initialize, adding mode = " << mode.GetUniqueName () <<
                                    " channel width " << j <<
                                    " nss " << +k <<
                                    " GI " << guardInterval);
                      if (mode.IsAllowed (j, k))
                        {
                          NS_LOG_DEBUG ("In SetupPhy, adding mode = " << mode.GetUniqueName ());
                          txVector.SetNss (k);
                          txVector.SetMode (mode);
                          AddSnrThreshold (txVector, GetPhy ()->CalculateSnr (txVector, m_ber));
                        }
                      else
                        {
                          NS_LOG_DEBUG ("In SetupPhy, mode = " << mode.GetUniqueName () << " disallowed");
                        }
                    }
                }
            }
        }
    }
  
  //######
  // connect end of preamble trace source
  GetPhy()->TraceConnectWithoutContext ("EndOfHePreamble", MakeCallback (&ObssWifiManager::ReceiveHeSig, this));
  m_device = DynamicCast<WifiNetDevice> (GetPhy()->GetDevice());
  m_obssRestricted = false;
  uint8_t mymac[6];
  m_device->GetMac()->GetAddress().CopyTo(mymac);
  m_myMac = mymac[5];

}

double
ObssWifiManager::GetSnrThreshold (WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << txVector.GetMode ().GetUniqueName ());
  for (Thresholds::const_iterator i = m_thresholds.begin (); i != m_thresholds.end (); i++)
    {
      NS_LOG_DEBUG ("Checking " << i->second.GetMode ().GetUniqueName () <<
                    " nss " << +i->second.GetNss () <<
                    " GI " << i->second.GetGuardInterval () <<
                    " width " << i->second.GetChannelWidth ());
      NS_LOG_DEBUG ("against TxVector " << txVector.GetMode ().GetUniqueName () <<
                    " nss " << +txVector.GetNss () <<
                    " GI " << txVector.GetGuardInterval () <<
                    " width " << txVector.GetChannelWidth ());
      if (txVector.GetMode () == i->second.GetMode ()
          && txVector.GetNss () == i->second.GetNss ()
          && txVector.GetChannelWidth () == i->second.GetChannelWidth ())
        {
          return i->first;
        }
    }
  NS_ASSERT (false);
  return 0.0;
}

void
ObssWifiManager::AddSnrThreshold (WifiTxVector txVector, double snr)
{
  NS_LOG_FUNCTION (this << txVector.GetMode ().GetUniqueName () << snr);
  m_thresholds.push_back (std::make_pair (snr, txVector));
}

WifiRemoteStation *
ObssWifiManager::DoCreateStation (void) const
{
  NS_LOG_FUNCTION (this);
  ObssWifiRemoteStation *station = new ObssWifiRemoteStation ();
  station->m_lastSnrObserved = 0.0;
  station->m_lastSnrCached = CACHE_INITIAL_VALUE;
  station->m_lastMode = GetDefaultMode ();
  station->m_nss = 1;
  return station;
}


void
ObssWifiManager::DoReportRxOk (WifiRemoteStation *station, double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}

void
ObssWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
ObssWifiManager::DoReportDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
ObssWifiManager::DoReportRtsOk (WifiRemoteStation *st,
                                 double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << st << ctsSnr << ctsMode.GetUniqueName () << rtsSnr);
  ObssWifiRemoteStation *station = (ObssWifiRemoteStation *)st;
  station->m_lastSnrObserved = rtsSnr;
}

void
ObssWifiManager::DoReportDataOk (WifiRemoteStation *st,
                                  double ackSnr, WifiMode ackMode, double dataSnr)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode.GetUniqueName () << dataSnr);
  std::cout<<"myMac: "<< +m_myMac <<" nexthop: "<< +m_nexthopMac<< " dataSnr:  "<< WToDbm(dataSnr) << std::endl;

  ObssWifiRemoteStation *station = (ObssWifiRemoteStation *)st;
  if (dataSnr == 0)
    {
      NS_LOG_WARN ("DataSnr reported to be zero; not saving this report.");
      return;
    }
  station->m_lastSnrObserved = dataSnr;
}

void
ObssWifiManager::DoReportAmpduTxStatus (WifiRemoteStation *st, uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus, double rxSnr, double dataSnr)
{
  NS_LOG_FUNCTION (this << st << +nSuccessfulMpdus << +nFailedMpdus << rxSnr << dataSnr);
  ObssWifiRemoteStation *station = (ObssWifiRemoteStation *)st;
  if (dataSnr == 0)
    {
      NS_LOG_WARN ("DataSnr reported to be zero; not saving this report.");
      return;
    }
  station->m_lastSnrObserved = dataSnr;
}


void
ObssWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
ObssWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

WifiTxVector
ObssWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  ObssWifiRemoteStation *station = (ObssWifiRemoteStation *)st;
  //We search within the Supported rate set the mode with the
  //highest data rate for which the snr threshold is smaller than m_lastSnr
  //to ensure correct packet delivery.
  WifiMode maxMode = GetDefaultMode ();
  WifiTxVector txVector;
  WifiMode mode;
  uint64_t bestRate = 0;
  uint8_t selectedNss = 1;
  uint16_t guardInterval;
  uint16_t channelWidth = std::min (GetChannelWidth (station), GetPhy ()->GetChannelWidth ());
  txVector.SetChannelWidth (channelWidth);
  if (station->m_lastSnrCached != CACHE_INITIAL_VALUE && station->m_lastSnrObserved == station->m_lastSnrCached)
    {
      // SNR has not changed, so skip the search and use the last
      // mode selected
      maxMode = station->m_lastMode;
      selectedNss = station->m_nss;
      NS_LOG_DEBUG ("Using cached mode = " << maxMode.GetUniqueName () <<
                    " last snr observed " << station->m_lastSnrObserved <<
                    " cached " << station->m_lastSnrCached <<
                    " nss " << +selectedNss);
    }
  else
    {
      if ((GetHtSupported () || GetVhtSupported () || GetHeSupported ())
          && (GetHtSupported (st) || GetVhtSupported (st) || GetHeSupported (st)))
        {
          for (uint8_t i = 0; i < GetNMcsSupported (station); i++)
            {
              mode = GetMcsSupported (station, i);
              txVector.SetMode (mode);
              if (mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
                {
                  guardInterval = static_cast<uint16_t> (std::max (GetShortGuardIntervalSupported (station) ? 400 : 800, GetShortGuardIntervalSupported () ? 400 : 800));
                  txVector.SetGuardInterval (guardInterval);
                  // If the node and peer are both VHT capable, only search VHT modes
                  if (GetVhtSupported () && GetVhtSupported (station))
                    {
                      continue;
                    }
                  // If the node and peer are both HE capable, only search HE modes
                  if (GetHeSupported () && GetHeSupported (station))
                    {
                      continue;
                    }
                  // Derive NSS from the MCS index. There is a different mode for each possible NSS value.
                  uint8_t nss = (mode.GetMcsValue () / 8) + 1;
                  txVector.SetNss (nss);
                  if (!txVector.IsValid ()
                      || nss > std::min (GetMaxNumberOfTransmitStreams (), GetNumberOfSupportedStreams (st)))
                    {
                      NS_LOG_DEBUG ("Skipping mode " << mode.GetUniqueName () <<
                                    " nss " << +nss <<
                                    " width " << txVector.GetChannelWidth ());
                      continue;
                    }
                  double threshold = GetSnrThreshold (txVector);
                  uint64_t dataRate = mode.GetDataRate (txVector.GetChannelWidth (), txVector.GetGuardInterval (), nss);
                  NS_LOG_DEBUG ("Testing mode " << mode.GetUniqueName () <<
                                " data rate " << dataRate <<
                                " threshold " << threshold  << " last snr observed " <<
                                station->m_lastSnrObserved << " cached " <<
                                station->m_lastSnrCached);
                  if (dataRate > bestRate && threshold < station->m_lastSnrObserved)
                    {
                      NS_LOG_DEBUG ("Candidate mode = " << mode.GetUniqueName () <<
                                    " data rate " << dataRate <<
                                    " threshold " << threshold  <<
                                    " last snr observed " <<
                                    station->m_lastSnrObserved);
                      bestRate = dataRate;
                      maxMode = mode;
                      selectedNss = nss;
                    }
                }
              else if (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
                {
                  guardInterval = static_cast<uint16_t> (std::max (GetShortGuardIntervalSupported (station) ? 400 : 800, GetShortGuardIntervalSupported () ? 400 : 800));
                  txVector.SetGuardInterval (guardInterval);
                  // If the node and peer are both HE capable, only search HE modes
                  if (GetHeSupported () && GetHeSupported (station))
                    {
                      continue;
                    }
                  // If the node and peer are not both VHT capable, only search HT modes
                  if (!GetVhtSupported () || !GetVhtSupported (station))
                    {
                      continue;
                    }
                  for (uint8_t nss = 1; nss <= std::min (GetMaxNumberOfTransmitStreams (), GetNumberOfSupportedStreams (station)); nss++)
                    {
                      txVector.SetNss (nss);
                      if (!txVector.IsValid ())
                        {
                          NS_LOG_DEBUG ("Skipping mode " << mode.GetUniqueName () <<
                                        " nss " << +nss <<
                                        " width " << txVector.GetChannelWidth ());
                          continue;
                        }
                      double threshold = GetSnrThreshold (txVector);
                      uint64_t dataRate = mode.GetDataRate (txVector.GetChannelWidth (), txVector.GetGuardInterval (), nss);
                      NS_LOG_DEBUG ("Testing mode = " << mode.GetUniqueName () <<
                                    " data rate " << dataRate <<
                                    " threshold " << threshold << " last snr observed " <<
                                    station->m_lastSnrObserved << " cached " <<
                                    station->m_lastSnrCached);
                      if (dataRate > bestRate && threshold < station->m_lastSnrObserved)
                        {
                          NS_LOG_DEBUG ("Candidate mode = " << mode.GetUniqueName () <<
                                        " data rate " << dataRate <<
                                        " threshold " << threshold  <<
                                        " last snr observed " <<
                                        station->m_lastSnrObserved);
                          bestRate = dataRate;
                          maxMode = mode;
                          selectedNss = nss;
                        }
                    }
                }
              else //HE
                {
                  guardInterval = std::max (GetGuardInterval (station), GetGuardInterval ());
                  txVector.SetGuardInterval (guardInterval);
                  // If the node and peer are not both HE capable, only search (V)HT modes
                  if (!GetHeSupported () || !GetHeSupported (station))
                    {
                      continue;
                    }
                  for (uint8_t nss = 1; nss <= std::min (GetMaxNumberOfTransmitStreams (), GetNumberOfSupportedStreams (station)); nss++)
                    {
                      txVector.SetNss (nss);
                      if (!txVector.IsValid ())
                        {
                          NS_LOG_DEBUG ("Skipping mode " << mode.GetUniqueName () <<
                                        " nss " << +nss <<
                                        " width " << +txVector.GetChannelWidth ());
                          continue;
                        }
                      double threshold = GetSnrThreshold (txVector);
                      uint64_t dataRate = mode.GetDataRate (txVector.GetChannelWidth (), txVector.GetGuardInterval (), nss);
                      NS_LOG_DEBUG ("Testing mode = " << mode.GetUniqueName () <<
                                    " data rate " << dataRate <<
                                    " threshold " << threshold  << " last snr observed " <<
                                    station->m_lastSnrObserved << " cached " <<
                                    station->m_lastSnrCached);
                      if (dataRate > bestRate && threshold < station->m_lastSnrObserved)
                        {
                          NS_LOG_DEBUG ("Candidate mode = " << mode.GetUniqueName () <<
                                        " data rate " << dataRate <<
                                        " threshold " << threshold  <<
                                        " last snr observed " <<
                                        station->m_lastSnrObserved);
                          bestRate = dataRate;
                          maxMode = mode;
                          selectedNss = nss;
                        }
                    }
                }
            }
        }
      else
        {
          // Non-HT selection
          selectedNss = 1;
          for (uint8_t i = 0; i < GetNSupported (station); i++)
            {
              mode = GetSupported (station, i);
              txVector.SetMode (mode);
              txVector.SetNss (selectedNss);
              txVector.SetChannelWidth (GetChannelWidthForMode (mode));
              double threshold = GetSnrThreshold (txVector);
              uint64_t dataRate = mode.GetDataRate (txVector.GetChannelWidth (), txVector.GetGuardInterval (), txVector.GetNss ());
              NS_LOG_DEBUG ("mode = " << mode.GetUniqueName () <<
                            " threshold " << threshold  <<
                            " last snr observed " <<
                            station->m_lastSnrObserved);
              if (dataRate > bestRate && threshold < station->m_lastSnrObserved)
                {
                  NS_LOG_DEBUG ("Candidate mode = " << mode.GetUniqueName () <<
                                " data rate " << dataRate <<
                                " threshold " << threshold  <<
                                " last snr observed " <<
                                station->m_lastSnrObserved);
                  bestRate = dataRate;
                  maxMode = mode;
                }
            }
        }
      NS_LOG_DEBUG ("Updating cached values for station to " <<  maxMode.GetUniqueName () << " snr " << station->m_lastSnrObserved);
      station->m_lastSnrCached = station->m_lastSnrObserved;
      station->m_lastMode = maxMode;
      station->m_nss = selectedNss;
    }
  NS_LOG_DEBUG ("Found maxMode: " << maxMode << " channelWidth: " << channelWidth);
  if (maxMode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      guardInterval = std::max (GetGuardInterval (station), GetGuardInterval ());
    }
  else if ((maxMode.GetModulationClass () == WIFI_MOD_CLASS_HT) || (maxMode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
    {
      guardInterval = static_cast<uint16_t> (std::max (GetShortGuardIntervalSupported (station) ? 400 : 800, GetShortGuardIntervalSupported () ? 400 : 800));
    }
  else
    {
      guardInterval = 800;
    }
  if (m_currentRate != maxMode.GetDataRate (channelWidth, guardInterval, selectedNss))
    {
      NS_LOG_DEBUG ("New datarate: " << maxMode.GetDataRate (channelWidth, guardInterval, selectedNss));
      m_currentRate = maxMode.GetDataRate (channelWidth, guardInterval, selectedNss);
    }

  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetPreambleForTransmission (maxMode.GetModulationClass (), GetShortPreambleEnabled (), UseGreenfieldForDestination (GetAddress (station))), guardInterval, GetNumberOfAntennas (), selectedNss, 0, GetChannelWidthForTransmission (maxMode, channelWidth), GetAggregation (station), false);
}

WifiTxVector
ObssWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  ObssWifiRemoteStation *station = (ObssWifiRemoteStation *)st;
  //We search within the Basic rate set the mode with the highest
  //snr threshold possible which is smaller than m_lastSnr to
  //ensure correct packet delivery.
  double maxThreshold = 0.0;
  WifiTxVector txVector;
  WifiMode mode;
  uint8_t nss = 1;
  WifiMode maxMode = GetDefaultMode ();
  //avoid to use legacy rate adaptation algorithms for IEEE 802.11n/ac/ax
  //RTS is sent in a legacy frame; RTS with HT/VHT/HE is not yet supported
  for (uint8_t i = 0; i < GetNBasicModes (); i++)
    {
      mode = GetBasicMode (i);
      txVector.SetMode (mode);
      txVector.SetNss (nss);
      txVector.SetChannelWidth (GetChannelWidthForMode (mode));
      double threshold = GetSnrThreshold (txVector);
      if (threshold > maxThreshold && threshold < station->m_lastSnrObserved)
        {
          maxThreshold = threshold;
          maxMode = mode;
        }
    }
  return WifiTxVector (maxMode, GetDefaultTxPowerLevel (), GetPreambleForTransmission (maxMode.GetModulationClass (), GetShortPreambleEnabled (), UseGreenfieldForDestination (GetAddress (station))), 800, GetNumberOfAntennas (), nss, 0, GetChannelWidthForMode (maxMode), GetAggregation (station), false);
}

bool
ObssWifiManager::IsLowLatency (void) const
{
  return true;
}

void
ObssWifiManager::UpdatePathLoss(HePreambleParameters params)
{
  NS_LOG_FUNCTION (this<< +m_myMac);

  double loss = WToDbm(params.rssiW) - params.txpower/10.0 ; // negative value(dbm)

  //TO DO: more complex updating method
    for (PathLossPairs::const_iterator i = m_lossPairs.begin (); i != m_lossPairs.end (); i++)
    {
      if(params.src == i->first)
      {
        if(i->second != loss)
        {
          std::cout<<"different loss, old: "<< i->second << " new: "<< loss<< std::endl; 
        }
        return;
      }
    }

    // no match
    m_lossPairs.push_back(std::make_pair(params.src, loss));
    std::cout<<"Node mac "<< +m_myMac << " pushed src="<< +params.src <<" loss= "<<loss<<std::endl;

    // update global loss pairs

    for (PathLossPairs::const_iterator i = globalLossPairs.begin (); i != globalLossPairs.end (); i++)
    {
      uint8_t i_dst = i->first / (1<<8);
      uint8_t i_src = i->first % (1<<8);
      if(params.src == i_src && m_myMac == i_dst )
      {
        if(i->second != loss)
        {
          std::cout<<"global different loss, old: "<< i->second << " new: "<< loss<< std::endl; 
        }
        return;
      }
    }
    globalLossPairs.push_back(std::make_pair(((uint16_t)m_myMac<<8) + params.src, loss));
    std::cout<<"Global pushed : dst= "<< +m_myMac << "  src="<< +params.src <<" loss= "<<loss<<std::endl;

    return;
}

void
ObssWifiManager::ReceiveHeSig(HePreambleParameters params)
{
  NS_LOG_FUNCTION (this<< +m_myMac);
  std::cout<< "Mymac "<<+m_myMac <<"\t Dst "<<(int)params.dst <<" Src "<< (int)params.src<< \
   "  Power " << +params.txpower<<" Rssi: "<<WToDbm (params.rssiW) <<" Duration" << +params.time<< "  Mcs "<< +params.mcs <<std::endl;

  UpdatePathLoss(params);
  UpdateObssTransStatus(params);
  CheckObssStatus(params);

  return;
}

void
ObssWifiManager::UpdateObssTransStatus(HePreambleParameters params)
{
  NS_LOG_FUNCTION (this<< +m_myMac);

  ObssTran tran(params.dst, params.src, Simulator::Now().ToInteger(Time::Unit::NS),\
   (uint64_t)params.time * 1e4, (double)params.txpower / 10.0, params.mcs);
  // tran.dst = params.dst;
  // tran.src = params.src;
  // tran.startTime = Simulator::Now().ToInteger(Time::Unit::NS);
  // tran.duration = (uint64_t)params.time * 1e4 ; // ns 
  // tran.txPower = (double)params.txpower / 10.0;
  m_obssTrans.push_back(tran);
}

void
ObssWifiManager::CheckObssStatus(HePreambleParameters params)
{
  NS_LOG_FUNCTION (this<< +m_myMac);

  std::cout<<"checkObssStatus"<<std::endl;
  //check timer
  for(int idx=0; idx<m_obssTrans.size(); idx++)
  {
    double startTime = std::get<2>(m_obssTrans[idx]);
    double duration = std::get<3>(m_obssTrans[idx]);
    std::cout<<"start "<<startTime<< " duration "<<duration<<"  Now "<< +Simulator::Now().ToInteger(Time::Unit::NS)<<std::endl;
    if(startTime+duration < Simulator::Now().ToInteger(Time::Unit::NS))
    {
      // time past, delete it
      m_obssTrans.erase(m_obssTrans.begin()+idx);
    }

  }
  std::cout<<"m_obssTrans.size= "<<m_obssTrans.size()<<std::endl;
  if(m_obssTrans.size()==0 || !CheckRouting(params))
  {
    m_obssRestricted = false;
    return;
  }
  // #######
  // TO DO: Update power and rate limit

  double interference = 0;
  ReceiverInfos recvinfos;

  // sum up interference
  //for nexthop
  for(int idx=0; idx<m_obssTrans.size(); idx++)
  {
    uint8_t temp_src = std::get<1>(m_obssTrans[idx]);
    double temp_txpower = std::get<4>(m_obssTrans[idx]);
    double temp_loss = GetPathLoss(m_nexthopMac, temp_src);
    std::cout<<"dst: "<<+m_nexthopMac<<" src: "<<+temp_src<<" loss: "<<temp_loss<<std::endl;
    if(temp_loss>0) // no path loss yet
    {
      m_obssRestricted = false;
      return;
    }
    interference += DbmToW(temp_txpower + temp_loss);
  }
  ReceiverInfo recvInfo(m_nexthopMac, interference, 0, -1); // first one
  recvinfos.push_back(recvInfo);

  // add other receivers
  double signal = 0;
  for(int idx=0; idx<m_obssTrans.size(); idx++)
  {
    uint8_t temp_dst = std::get<0>(m_obssTrans[idx]);
    uint8_t temp_mcs = std::get<5>(m_obssTrans[idx]);
    interference = 0;
    signal = 0;
    for(int idx2=0; idx2<m_obssTrans.size(); idx2++)
    {
      uint8_t temp_src = std::get<1>(m_obssTrans[idx2]);
      uint8_t temp_dst2 = std::get<0>(m_obssTrans[idx2]);
      double temp_txpower = std::get<4>(m_obssTrans[idx2]);
      double temp_loss = GetPathLoss(temp_dst, temp_src);
      if(temp_loss>0) // no path loss yet
      {
        m_obssRestricted = false;
        return;
      }
      if(temp_dst2 == temp_dst) // not inf but signal !
      {
        signal = DbmToW(temp_txpower + temp_loss);
      }
      else
      {
        interference += DbmToW(temp_txpower + temp_loss); 
      }
    }
    ReceiverInfo recvInfo(temp_dst, interference, signal, temp_mcs);
    recvinfos.push_back(recvInfo);
  }

  bool isOk;
  double myTxpower;
  int level;
  std::cout<<"we have "<<recvinfos.size()<<" recv info"<<std::endl;
  for(level=GetPhy()->GetNTxPower();level>=0;level--)
  {
    myTxpower = GetPhy()->GetPowerDbm(level); //dbm
    isOk=true;

    // must not disturb on-going transimission
    for(int idx=1;idx<recvinfos.size();idx++)
    {
      uint8_t temp_recv = std::get<0>(recvinfos[idx]);
      double temp_inf = std::get<1>(recvinfos[idx]);
      double temp_signal = std::get<2>(recvinfos[idx]);
      int temp_mcs = std::get<3>(recvinfos[idx]);
      double temp_loss = GetPathLoss(temp_recv, m_myMac);
      if(temp_loss>0) // no path loss yet
      {
        m_obssRestricted = false;
        return;
      }
      
      WifiMode mode = GetPhy()->GetMcs(temp_mcs + 42); // Use HeMcs
      std::cout<<"mcs: "<< temp_mcs << " name: "<< mode.GetUniqueName()<< std::endl;

      double myI = DbmToW(myTxpower+ temp_loss); // the interference I will introduce (W)
      uint16_t channelWidth = GetPhy ()->GetChannelWidth ();
      std::cout<<"loss= "<<temp_loss<<" interference= "<<WToDbm(myI+temp_inf)<<" signal= "<<WToDbm(temp_signal)<<" ch= "<<channelWidth<<std::endl;
      double dstSNR = WToDbm(CalculateSnr(temp_signal, myI+temp_inf, channelWidth));

      WifiTxVector txVector;
      txVector.SetChannelWidth (channelWidth);
      txVector.SetNss (1);
      txVector.SetMode (mode);
      double SNRlimit = WToDbm( GetPhy()->CalculateSnr(txVector, m_ber) );

      std::cout<<"mypower= "<<myTxpower<<" SNRlimit= "<<SNRlimit<<" dstSNR= "<<dstSNR<<std::endl;

      if(dstSNR<SNRlimit)
      {
        isOk=false;
        break;
      }
    }

    if(isOk)
    {
      std::cout<<"Found right txpower!"<<myTxpower<<std::endl;
      break;
    }

  }

  uint8_t temp_recv = std::get<0>(recvinfos[0]);
  double temp_inf = std::get<1>(recvinfos[0]);
  double temp_loss = GetPathLoss(temp_recv, m_myMac);
  if(temp_loss>0) // no path loss yet
  {
    m_obssRestricted = false;
    return;
  }
  uint16_t channelWidth = GetPhy ()->GetChannelWidth ();
  std::cout<<"loss= "<<temp_loss<<" interference= "<<WToDbm(temp_inf)<<" signal= "<<(myTxpower+temp_loss)<<" ch= "<<channelWidth<<std::endl;
  double SNR = WToDbm(CalculateSnr(DbmToW(myTxpower+temp_loss), temp_inf, channelWidth));
  int mcs;
  double SNRlimit;
  for(mcs=9;mcs>=0;mcs--)
  {
    WifiMode mode = GetPhy()->GetMcs(mcs + 42);
    WifiTxVector txVector;
    txVector.SetChannelWidth (channelWidth);
    txVector.SetNss (1);
    txVector.SetMode (mode);
    SNRlimit = WToDbm( GetPhy()->CalculateSnr(txVector, m_ber) );
    if(SNRlimit<SNR)break;
  }
  std::cout<<"final mcs: "<<mcs<<" SNRlimit:"<<SNRlimit<<" SNR:"<<SNR<<std::endl;
  m_obssPowerLimit = level;
  m_obssMcsLimit = mcs;
  m_obssRestricted = true; // ?? When to set true
}

double 
ObssWifiManager::CalculateSnr (double signal, double noiseInterference, uint16_t channelWidth)
{
  double noiseFigure = 7; // default value of wifi-phy attribute

  //thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.3803e-23;
  //Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290 * channelWidth * 1e6;
  //receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noiseFloor = noiseFigure * Nt;
  double noise = noiseFloor + noiseInterference;
  double snr = signal / noise; //linear scale
  NS_LOG_DEBUG ("bandwidth(MHz)=" << channelWidth << ", signal(W)= " << signal << ", noise(W)=" << noiseFloor << ", interference(W)=" << noiseInterference << ", snr=" << RatioToDb(snr) << "dB");
  
  return snr;
}

double
ObssWifiManager::GetPathLoss(uint8_t dst, uint8_t src)
{

  for (PathLossPairs::const_iterator i = globalLossPairs.begin (); i != globalLossPairs.end (); i++)
  {
    uint8_t i_dst = i->first / (1<<8);
    uint8_t i_src = i->first % (1<<8);
    if(src == i_src && dst == i_dst )
    {
      return i->second;
    }
  }
  std::cout<<"No loss for dst "<<+dst<<" src "<<+src<<std::endl;
  return 1; // loss must be negative, so use 1 to indicate no match #####
}


// check routing table, 
// return false if no nexthop or the params.src==myNexthop || params.dst == myMac
bool
ObssWifiManager::CheckRouting(HePreambleParameters params)
{
  uint8_t addrs2[4]; // nexthop ip

  Ptr<Ipv4RoutingProtocol> aodvRouting;
  aodvRouting = Ipv4RoutingHelper::GetRouting <Ipv4RoutingProtocol> (m_device->GetNode()->GetObject<Ipv4> ()->GetRoutingProtocol ());
  // aodvRouting = m_device->GetNode()->GetObject<Ipv4> ()->GetRoutingProtocol ();
  Ipv4Address sourceAddr = m_device->GetNode()->GetObject<Ipv4> ()->GetAddress(1,0).GetLocal();
  Ipv4Header ipHead;
  ipHead.SetDestination("10.2.1.1");
  ipHead.SetSource(sourceAddr);
  Socket::SocketErrno err;
  Packet ptemp;
  Ptr<Ipv4Route> routeEntry = aodvRouting->RouteOutput(&ptemp, ipHead, m_device, err);

  if(!err)
  {
      // std::cout<<"  myAddr= "<<sourceAddr<<"  nextHop= "<<routeEntry->GetGateway() <<std::endl;
      routeEntry->GetGateway().Serialize(addrs2);
      m_nexthopMac = addrs2[3];

      // check all on-going transmission
      for(int idx=0; idx<m_obssTrans.size(); idx++)
      {
        uint8_t tran_dst = std::get<0>(m_obssTrans[idx]);
        uint8_t tran_src = std::get<1>(m_obssTrans[idx]);

        if(tran_src == m_nexthopMac || tran_dst == m_nexthopMac || tran_dst ==m_myMac || addrs2[0]==127)
        {
          NS_LOG_DEBUG("loopback addr or not a obss frame.");
          // std::cout<<"loopback addr or not a obss frame."<<std::endl;
          return false;
        }
      }

      return true;      
  }
  else
  {
    // std::cout<<"  myAddr= "<<sourceAddr<<"  error routing , no nexthop"<<std::endl;  
    return false;
  }
  
}

} //namespace ns3
