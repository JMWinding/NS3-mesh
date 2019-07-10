/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004,2005,2006 INRIA
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
#include "aarf-ht-wifi-manager.h"
#include "wifi-phy.h"
#include "wifi-tx-vector.h"

#define Min(a,b) ((a < b) ? a : b)
#define Max(a,b) ((a > b) ? a : b)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AarfHtWifiManager");

/**
 * \brief hold per-remote-station state for AARF Wifi manager.
 *
 * This struct extends from WifiRemoteStation struct to hold additional
 * information required by the AARF Wifi manager
 */
struct AarfHtWifiRemoteStation : public WifiRemoteStation
{
  uint32_t m_timer; ///< timer
  uint32_t m_success; ///< success
  uint32_t m_failed; ///< failed
  bool m_recovery; ///< recovery
  uint32_t m_retry; ///< retry
  uint32_t m_timerTimeout; ///< timer timeout
  uint32_t m_successThreshold; ///< success threshold
  uint8_t m_rate; ///< rate
};

NS_OBJECT_ENSURE_REGISTERED (AarfHtWifiManager);

TypeId
AarfHtWifiManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AarfHtWifiManager")
    .SetParent<WifiRemoteStationManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<AarfHtWifiManager> ()
    .AddAttribute ("SuccessK", "Multiplication factor for the success threshold in the AARF algorithm.",
                   DoubleValue (2.0),
                   MakeDoubleAccessor (&AarfHtWifiManager::m_successK),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TimerK",
                   "Multiplication factor for the timer threshold in the AARF algorithm.",
                   DoubleValue (2.0),
                   MakeDoubleAccessor (&AarfHtWifiManager::m_timerK),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MaxSuccessThreshold",
                   "Maximum value of the success threshold in the AARF algorithm.",
                   UintegerValue (60),
                   MakeUintegerAccessor (&AarfHtWifiManager::m_maxSuccessThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MinTimerThreshold",
                   "The minimum value for the 'timer' threshold in the AARF algorithm.",
                   UintegerValue (15),
                   MakeUintegerAccessor (&AarfHtWifiManager::m_minTimerThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MinSuccessThreshold",
                   "The minimum value for the success threshold in the AARF algorithm.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&AarfHtWifiManager::m_minSuccessThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Rate",
                     "Traced value for rate changes (b/s)",
                     MakeTraceSourceAccessor (&AarfHtWifiManager::m_currentRate),
                     "ns3::TracedValueCallback::Uint64")
  ;
  return tid;
}

AarfHtWifiManager::AarfHtWifiManager ()
  : WifiRemoteStationManager (),
    m_currentRate (0)
{
  NS_LOG_FUNCTION (this);
}

AarfHtWifiManager::~AarfHtWifiManager ()
{
  NS_LOG_FUNCTION (this);
}

WifiRemoteStation *
AarfHtWifiManager::DoCreateStation (void) const
{
  NS_LOG_FUNCTION (this);
  AarfHtWifiRemoteStation *station = new AarfHtWifiRemoteStation ();

  station->m_successThreshold = m_minSuccessThreshold;
  station->m_timerTimeout = m_minTimerThreshold;
  station->m_rate = 0;
  station->m_success = 0;
  station->m_failed = 0;
  station->m_recovery = false;
  station->m_retry = 0;
  station->m_timer = 0;

  return station;
}

uint16_t
AarfHtWifiManager::GetChannelWidthForMode (WifiMode mode) const
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

uint8_t
AarfHtWifiManager::GetIncreaseMcs (WifiRemoteStation *st, uint8_t rate)
{
  WifiMode mode = GetMcsSupported (st, rate);

  WifiTxVector txVector;
  uint8_t nss = std::min (GetMaxNumberOfTransmitStreams (), GetNumberOfSupportedStreams (st));
  txVector.SetNss (nss);
  uint16_t channelWidth = std::min (GetChannelWidth (st), GetPhy ()->GetChannelWidth ());
  txVector.SetChannelWidth (channelWidth);
  uint16_t guardInterval;

  WifiMode modeNew;
  for (uint8_t i = rate + 1; i < GetNMcsSupported (st); i++)
    {
      modeNew = GetMcsSupported (st, i);
      if (modeNew.GetModulationClass () != mode.GetModulationClass ())
        break;
      else
        {
          txVector.SetMode (modeNew);

          if (modeNew.GetModulationClass () == WIFI_MOD_CLASS_HT)
            if ((modeNew.GetMcsValue () / 8) + 1 != nss)
              break;
            else
              guardInterval = static_cast<uint16_t> (std::max (GetShortGuardInterval (st) ? 400 : 800, GetPhy ()->GetShortGuardInterval () ? 400 : 800));
          else if (modeNew.GetModulationClass () == WIFI_MOD_CLASS_VHT)
            guardInterval = static_cast<uint16_t> (std::max (GetShortGuardInterval (st) ? 400 : 800, GetPhy ()->GetShortGuardInterval () ? 400 : 800));
          else
            guardInterval = std::max (GetGuardInterval (st), static_cast<uint16_t> (GetPhy ()->GetGuardInterval ().GetNanoSeconds ()));
          txVector.SetGuardInterval (guardInterval);

          if (txVector.IsValid ())
            {
              NS_LOG_DEBUG ("INCREASE MOD: " << modeNew.GetUniqueName ());
              return i;
            }
          else
            continue;
        }
    }

  NS_LOG_DEBUG ("INCREASE MOD UNCHANGE: " << mode.GetUniqueName ());
  return rate;
}

uint8_t
AarfHtWifiManager::GetDecreaseMcs (WifiRemoteStation *st, uint8_t rate)
{
  WifiMode mode = GetMcsSupported (st, rate);

  WifiTxVector txVector;
  uint8_t nss = std::min (GetMaxNumberOfTransmitStreams (), GetNumberOfSupportedStreams (st));
  txVector.SetNss (nss);
  uint16_t channelWidth = std::min (GetChannelWidth (st), GetPhy ()->GetChannelWidth ());
  txVector.SetChannelWidth (channelWidth);
  uint16_t guardInterval;

  WifiMode modeNew;
  for (uint8_t i = rate - 1; i >= 0; i--)
    {
      modeNew = GetMcsSupported (st, i);
      if (modeNew.GetModulationClass () != mode.GetModulationClass ())
          break;
      else
        {
          txVector.SetMode (modeNew);

          if (modeNew.GetModulationClass () == WIFI_MOD_CLASS_HT)
            {
              if ((modeNew.GetMcsValue () / 8) + 1 != nss)
                  break;
              else
                guardInterval = static_cast<uint16_t> (std::max (GetShortGuardInterval (st) ? 400 : 800, GetPhy ()->GetShortGuardInterval () ? 400 : 800));
            }
          else if (modeNew.GetModulationClass () == WIFI_MOD_CLASS_VHT)
            guardInterval = static_cast<uint16_t> (std::max (GetShortGuardInterval (st) ? 400 : 800, GetPhy ()->GetShortGuardInterval () ? 400 : 800));
          else
            guardInterval = std::max (GetGuardInterval (st), static_cast<uint16_t> (GetPhy ()->GetGuardInterval ().GetNanoSeconds ()));
          txVector.SetGuardInterval (guardInterval);

          if (txVector.IsValid ())
            {
              NS_LOG_DEBUG ("DECREASE MOD: " << modeNew.GetUniqueName ());
              return i;
            }
          else
            continue;
        }
    }

  NS_LOG_DEBUG ("DECREASE MOD UNCHANGE: " << mode.GetUniqueName ());
  return rate;
}

void
AarfHtWifiManager::DoReportRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}
/**
 * It is important to realize that "recovery" mode starts after failure of
 * the first transmission after a rate increase and ends at the first successful
 * transmission. Specifically, recovery mode transcends retransmissions boundaries.
 * Fundamentally, ARF handles each data transmission independently, whether it
 * is the initial transmission of a packet or the retransmission of a packet.
 * The fundamental reason for this is that there is a backoff between each data
 * transmission, be it an initial transmission or a retransmission.
 *
 * \param st the station that we failed to send DATA
 */
void
AarfHtWifiManager::DoReportDataFailed (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  AarfHtWifiRemoteStation *station = (AarfHtWifiRemoteStation *)st;
  station->m_timer++;
  station->m_failed++;
  station->m_retry++;
  station->m_success = 0;

  if (station->m_recovery)
    {
      NS_ASSERT (station->m_retry >= 1);
      if (station->m_retry == 1)
        {
          //need recovery fallback
          station->m_successThreshold = (int)(Min (station->m_successThreshold * m_successK,
                                                   m_maxSuccessThreshold));
          station->m_timerTimeout = (int)(Max (station->m_timerTimeout * m_timerK,
                                               m_minSuccessThreshold));
          if ((HasVhtSupported () == true || HasHtSupported () == true || HasHeSupported () == true)
              && (GetHtSupported (st) == true || GetVhtSupported (st) == true || GetHeSupported (st) == true))
            {
              station->m_rate = GetDecreaseMcs (st, station->m_rate);
            }
          else
            {
              if (station->m_rate != 0)
                {
                  station->m_rate--;
                }
            }
        }
      station->m_timer = 0;
    }
  else
    {
      NS_ASSERT (station->m_retry >= 1);
      if (((station->m_retry - 1) % 2) == 1)
        {
          //need normal fallback
          station->m_timerTimeout = m_minTimerThreshold;
          station->m_successThreshold = m_minSuccessThreshold;
          if ((HasVhtSupported () == true || HasHtSupported () == true || HasHeSupported () == true)
              && (GetHtSupported (st) == true || GetVhtSupported (st) == true || GetHeSupported (st) == true))
            {
              station->m_rate = GetDecreaseMcs (st, station->m_rate);
            }
          else
            {
              if (station->m_rate != 0)
                {
                  station->m_rate--;
                }
            }
        }
      if (station->m_retry >= 2)
        {
          station->m_timer = 0;
        }
    }
}

void
AarfHtWifiManager::DoReportRxOk (WifiRemoteStation *station,
                                 double rxSnr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << station << rxSnr << txMode);
}

void
AarfHtWifiManager::DoReportRtsOk (WifiRemoteStation *station,
                                  double ctsSnr, WifiMode ctsMode, double rtsSnr)
{
  NS_LOG_FUNCTION (this << station << ctsSnr << ctsMode << rtsSnr);
  NS_LOG_DEBUG ("station=" << station << " rts ok");
}

void
AarfHtWifiManager::DoReportDataOk (WifiRemoteStation *st,
                                   double ackSnr, WifiMode ackMode, double dataSnr)
{
  NS_LOG_FUNCTION (this << st << ackSnr << ackMode << dataSnr);
  AarfHtWifiRemoteStation *station = (AarfHtWifiRemoteStation *) st;
  station->m_timer++;
  station->m_success++;
  station->m_failed = 0;
  station->m_recovery = false;
  station->m_retry = 0;
  NS_LOG_DEBUG ("station=" << station << " data ok success=" << station->m_success << ", timer=" << station->m_timer);
  if (station->m_success == station->m_successThreshold
      || station->m_timer == station->m_timerTimeout)
      {
        if ((HasHtSupported () || HasVhtSupported () || HasHeSupported ())
            && (GetHtSupported (st) || GetVhtSupported (st) || GetHeSupported (st)))
          {
            station->m_rate = GetIncreaseMcs (st, station->m_rate);
          }
        else
          {
            if (station->m_rate < (station->m_state->m_operationalRateSet.size () - 1))
              {
                NS_LOG_DEBUG ("station=" << station << " inc rate");
                station->m_rate++;
              }
          }
        station->m_timer = 0;
        station->m_success = 0;
        station->m_recovery = true;
      }
}

void
AarfHtWifiManager::DoReportFinalRtsFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

void
AarfHtWifiManager::DoReportFinalDataFailed (WifiRemoteStation *station)
{
  NS_LOG_FUNCTION (this << station);
}

WifiTxVector
AarfHtWifiManager::DoGetDataTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  AarfHtWifiRemoteStation *station = (AarfHtWifiRemoteStation *) st;

  WifiTxVector txVector;
  WifiMode mode;
  uint8_t selectedNss;
  uint16_t guardInterval;
  uint16_t channelWidth;

  if ((HasHtSupported () || HasVhtSupported () || HasHeSupported ())
      && (GetHtSupported (st) || GetVhtSupported (st) || GetHeSupported (st)))
    {
      mode = GetMcsSupported (station, station->m_rate);
      selectedNss = std::min (GetMaxNumberOfTransmitStreams (), GetNumberOfSupportedStreams (station));
      channelWidth = std::min (GetChannelWidth (station), GetPhy ()->GetChannelWidth ());

      txVector.SetMode (mode);
      txVector.SetNss (selectedNss);
      txVector.SetChannelWidth (channelWidth);

      if (HasHeSupported () && GetHeSupported (st))
        {
          if (mode.GetModulationClass () != WIFI_MOD_CLASS_HE || !txVector.IsValid ())
            {
              NS_LOG_DEBUG ("WRONG HE MOD");
              for (uint8_t i = 0; i < this->GetNMcsSupported (station); i++)
                {
                  mode = GetMcsSupported (station, i);
                  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
                    {
                      station->m_rate = i;
                      break;
                    }
                }
            }
        }
      else if (HasVhtSupported () && GetVhtSupported (st))
        {
          if (mode.GetModulationClass () != WIFI_MOD_CLASS_VHT || !txVector.IsValid ())
            {
              NS_LOG_DEBUG ("WRONG VHT MOD");
              for (uint8_t i = 0; i < this->GetNMcsSupported (station); i++)
                {
                  mode = GetMcsSupported (station, i);
                  if (mode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
                    {
                      station->m_rate = i;
                      break;
                    }
                }
            }
        }
      else
        {
          if (mode.GetModulationClass () != WIFI_MOD_CLASS_HT
                || !txVector.IsValid ()
                || (mode.GetMcsValue () / 8) + 1 != selectedNss)
            {
              NS_LOG_DEBUG ("WRONG HT MOD");
              for (uint8_t i = 0; i < this->GetNMcsSupported (station); i++)
                {
                  mode = GetMcsSupported (station, i);
                  if ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT)
                      && ((mode.GetMcsValue () / 8) + 1 == selectedNss))
                    {
                      station->m_rate = i;
                      break;
                    }
                }
            }
        }
    }
  else
    {
      mode = GetSupported (station, station->m_rate);
      selectedNss = 1;
      channelWidth = GetChannelWidthForMode (mode);
    }

  if (mode.GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      guardInterval = std::max (GetGuardInterval (station), static_cast<uint16_t> (GetPhy ()->GetGuardInterval ().GetNanoSeconds ()));
    }
  else
    {
      guardInterval = static_cast<uint16_t> (std::max (GetShortGuardInterval (station) ? 400 : 800, GetPhy ()->GetShortGuardInterval () ? 400 : 800));
    }

  if (m_currentRate != mode.GetDataRate (channelWidth, guardInterval, selectedNss))
    {
      NS_LOG_DEBUG ("New datarate: " << mode.GetDataRate (channelWidth, guardInterval, selectedNss));
      m_currentRate = mode.GetDataRate (channelWidth, guardInterval, selectedNss);
    }

  return WifiTxVector (mode, GetDefaultTxPowerLevel (), GetPreambleForTransmission (mode, GetAddress (station)), guardInterval, GetNumberOfAntennas (), selectedNss, 0, GetChannelWidthForTransmission (mode, channelWidth), GetAggregation (station), false);
}

WifiTxVector
AarfHtWifiManager::DoGetRtsTxVector (WifiRemoteStation *st)
{
  NS_LOG_FUNCTION (this << st);
  /// \todo we could/should implement the Aarf algorithm for
  /// RTS only by picking a single rate within the BasicRateSet.
  AarfHtWifiRemoteStation *station = (AarfHtWifiRemoteStation *) st;
  uint16_t channelWidth = GetChannelWidth (station);
  if (channelWidth > 20 && channelWidth != 22)
    {
      //avoid to use legacy rate adaptation algorithms for IEEE 802.11n/ac
      channelWidth = 20;
    }
  WifiTxVector rtsTxVector;
  WifiMode mode;
  if (GetUseNonErpProtection () == false)
    {
      mode = GetSupported (station, 0);
    }
  else
    {
      mode = GetNonErpSupported (station, 0);
    }
  rtsTxVector = WifiTxVector (mode, GetDefaultTxPowerLevel (), GetPreambleForTransmission (mode, GetAddress (station)), 800, 1, 1, 0, channelWidth, GetAggregation (station), false);
  return rtsTxVector;
}

bool
AarfHtWifiManager::IsLowLatency (void) const
{
  return true;
}

} //namespace ns3
