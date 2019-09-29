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

#ifndef OBSS_WIFI_MANAGER_H
#define OBSS_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-net-device.h"
#include "sta-wifi-mac.h"
#include "wifi-remote-station-manager.h"
#include "ns3/internet-module.h"
#include "wifi-utils.h"

namespace ns3 {

/**
 * \brief Ideal rate control algorithm
 * \ingroup wifi
 *
 * This class implements an 'ideal' rate control algorithm
 * similar to RBAR in spirit (see <i>A rate-adaptive MAC
 * protocol for multihop wireless networks</i> by G. Holland,
 * N. Vaidya, and P. Bahl.): every station keeps track of the
 * snr of every packet received and sends back this snr to the
 * original transmitter by an out-of-band mechanism. Each
 * transmitter keeps track of the last snr sent back by a receiver
 * and uses it to pick a transmission mode based on a set
 * of snr thresholds built from a target ber and transmission
 * mode-specific snr/ber curves.
 */
class ObssWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  ObssWifiManager ();
  virtual ~ObssWifiManager ();

  void SetupPhy (const Ptr<WifiPhy> phy);

  typedef std::vector<std::pair<uint16_t, double> > PathLossPairs;

private:
  //overridden from base class
  void DoInitialize (void);
  WifiRemoteStation* DoCreateStation (void) const;
  void DoReportRxOk (WifiRemoteStation *station,
                     double rxSnr, WifiMode txMode);
  void DoReportRtsFailed (WifiRemoteStation *station);
  void DoReportDataFailed (WifiRemoteStation *station);
  void DoReportRtsOk (WifiRemoteStation *station,
                      double ctsSnr, WifiMode ctsMode, double rtsSnr);
  void DoReportDataOk (WifiRemoteStation *station,
                       double ackSnr, WifiMode ackMode, double dataSnr);
  void DoReportAmpduTxStatus (WifiRemoteStation *station,
                              uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus,
                              double rxSnr, double dataSnr);
  void DoReportFinalRtsFailed (WifiRemoteStation *station);
  void DoReportFinalDataFailed (WifiRemoteStation *station);
  WifiTxVector DoGetDataTxVector (WifiRemoteStation *station);
  WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station);
  bool IsLowLatency (void) const;

  /**
   * Return the minimum SNR needed to successfully transmit
   * data with this WifiTxVector at the specified BER.
   *
   * \param txVector WifiTxVector (containing valid mode, width, and nss)
   *
   * \return the minimum SNR for the given WifiTxVector
   */
  double GetSnrThreshold (WifiTxVector txVector) const;
  /**
   * Adds a pair of WifiTxVector and the minimum SNR for that given vector
   * to the list.
   *
   * \param txVector the WifiTxVector storing mode, channel width, and nss
   * \param snr the minimum SNR for the given txVector
   */
  void AddSnrThreshold (WifiTxVector txVector, double snr);

  /**
   * Convenience function for selecting a channel width for legacy mode
   * \param mode legacy WifiMode
   * \return the channel width (MHz) for the selected mode
   */
  uint16_t GetChannelWidthForMode (WifiMode mode) const;

  void ReceiveHeSig (HePreambleParameters params);

  void UpdatePathLoss(HePreambleParameters params);

  void UpdateObssTransStatus(HePreambleParameters params);

  void CheckObssStatus(HePreambleParameters params);

  double GetSINR(uint8_t dst, double myTxpower);

  double CalculateSnr (double signal, double noiseInterference, uint16_t channelWidth);

  double GetPathLoss(uint8_t dst, uint8_t src);

  bool CheckRouting(HePreambleParameters params);

  /**
   * A vector of <snr, WifiTxVector> pair holding the minimum SNR for the
   * WifiTxVector
   */
  typedef std::vector<std::pair<double, WifiTxVector> > Thresholds;

  // typedef struct ObssTran
  // {
  //   uint8_t dst;
  //   uint8_t src;
  //   uint64_t startTime;
  //   uint64_t duration;
  //   double txPower; //dbm
  // };

  typedef std::tuple<uint8_t, uint8_t, uint64_t, uint64_t, double, uint8_t> ObssTran; //dst, src, startTime, duration, txPower, mcs
  typedef std::vector<ObssTran> ObssTrans;
  typedef std::tuple<uint8_t, double, double, int> ReceiverInfo; // mac(dst), interference(W), signal(W), Mcs
  typedef std::vector<ReceiverInfo> ReceiverInfos;

  double m_ber;             //!< The maximum Bit Error Rate acceptable at any transmission mode
  Thresholds m_thresholds;  //!< List of WifiTxVector and the minimum SNR pair

  TracedValue<uint64_t> m_currentRate; //!< Trace rate changes

  // #####
  // obss pd
  PathLossPairs m_lossPairs; // (src, loss)
  Ptr<WifiNetDevice> m_device;
  bool m_obssRestricted;
  ObssTrans m_obssTrans;
  uint8_t m_myMac;
  uint8_t m_nexthopMac;

  uint8_t m_obssPowerLimit;
  uint8_t m_obssMcsLimit;
  uint8_t m_obssHeMcsLimit;

};

} //namespace ns3

#endif /* OBSS_WIFI_MANAGER_H */
