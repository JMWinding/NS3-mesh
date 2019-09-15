/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Washington
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "mesh-obss-pd-algorithm.h"
#include "sta-wifi-mac.h"
#include "wifi-utils.h"
#include "wifi-phy.h"
#include "wifi-net-device.h"
#include "he-configuration.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MeshObssPdAlgorithm");
NS_OBJECT_ENSURE_REGISTERED (MeshObssPdAlgorithm);

MeshObssPdAlgorithm::MeshObssPdAlgorithm ()
  : ObssPdAlgorithm ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MeshObssPdAlgorithm::GetTypeId (void)
{
  static ns3::TypeId tid = ns3::TypeId ("ns3::MeshObssPdAlgorithm")
    .SetParent<ObssPdAlgorithm> ()
    .SetGroupName ("Wifi")
    .AddConstructor<MeshObssPdAlgorithm> ()
  ;
  return tid;
}

void
MeshObssPdAlgorithm::ConnectWifiNetDevice (const Ptr<WifiNetDevice> device)
{
  Ptr<WifiPhy> phy = device->GetPhy ();
  phy->TraceConnectWithoutContext ("EndOfHePreamble", MakeCallback (&MeshObssPdAlgorithm::ReceiveHeSig, this));
  ObssPdAlgorithm::ConnectWifiNetDevice (device);
}

void
MeshObssPdAlgorithm::ReceiveHeSig (HePreambleParameters params)
{
  NS_LOG_FUNCTION (this << +params.bssColor << WToDbm (params.rssiW));

  Ptr<StaWifiMac> mac = m_device->GetMac ()->GetObject<StaWifiMac>();
  if (mac && !mac->IsAssociated ())
    {
      NS_LOG_DEBUG ("This is not an associated STA: skip OBSS PD algorithm");
      return;
    }

  // Ptr<HeConfiguration> heConfiguration = m_device->GetHeConfiguration ();
  // NS_ASSERT (heConfiguration);
  // UintegerValue bssColorAttribute;
  // heConfiguration->GetAttribute ("BssColor", bssColorAttribute);
  // uint8_t bssColor = bssColorAttribute.Get ();

  //####
  //set bsscolor as self mac address(last number)
  uint8_t addrs[6];
  m_device->GetMac()->GetAddress().CopyTo(addrs);
  uint8_t bssColor = addrs[5];

  if (bssColor == 0)
    {
      NS_LOG_DEBUG ("BSS color is 0");
      return;
    }
  if (params.bssColor == 0)
    {
      NS_LOG_DEBUG ("Received BSS color is 0");
      return;
    }
  //TODO: SRP_AND_NON-SRG_OBSS-PD_PROHIBITED=1 => OBSS_PD SR is not allowed

  bool isObss = (bssColor != params.bssColor);
  if (isObss)
    {
      if (WToDbm (params.rssiW) < m_obssPdLevel)
        {
          NS_LOG_DEBUG ("Frame is OBSS and RSSI " << WToDbm(params.rssiW) << " is below OBSS-PD level of " << m_obssPdLevel << "; reset PHY to IDLE");
          ResetPhy (params);
        }
      else
        {
          NS_LOG_DEBUG ("Frame is OBSS and RSSI is above OBSS-PD level");
        }
    }
}

} //namespace ns3
