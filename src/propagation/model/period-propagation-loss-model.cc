#include "propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <cmath>

#include "period-propagation-loss-model.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LinkBreakPropagationLossModel);

TypeId
LinkBreakPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkBreakPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<LinkBreakPropagationLossModel> ()
    .AddAttribute ("BreakProb",
                   "Link break probability",
                   DoubleValue (0.05),
                   MakeDoubleAccessor (&LinkBreakPropagationLossModel::m_breakProb),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Recover",
                   "The random variable used to pick a link recover time.",
                   StringValue ("ns3:: ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&LinkBreakPropagationLossModel::m_period),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

LinkBreakPropagationLossModel::LinkBreakPropagationLossModel () :
    m_breakProb (0.05)
{
  m_break = CreateObject<UniformRandomVariable> ();
  m_recover = new LinkBreakRecover ();
}

double
LinkBreakPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b) const
{
  double linkBreakRx = -1e3;

  auto it1 = m_recover->m_recoverTime.find (std::make_pair (a,b));
  if (it1 != m_recover->m_recoverTime.end ())
    {
      if (Simulator::Now ().GetSeconds () < it1->second)
        return linkBreakRx;
      else
        m_recover->m_recoverTime.erase (it1);
    }

  auto it2 = m_recover->m_recoverTime.find (std::make_pair (b,a));
  if (it2 != m_recover->m_recoverTime.end ())
    {
      if (Simulator::Now ().GetSeconds () < it2->second)
        return linkBreakRx;
      else
        m_recover->m_recoverTime.erase (it2);
    }

  if (m_break->GetValue () < m_breakProb)
    {
      double rt = Simulator::Now ().GetSeconds () + m_period->GetValue ();
      m_recover->m_recoverTime[std::make_pair (a,b)] = rt;
      return linkBreakRx;
    }
  else
    return txPowerDbm;
}

int64_t
LinkBreakPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (NodeDownPropagationLossModel);

TypeId
NodeDownPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NodeDownPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<NodeDownPropagationLossModel> ()
    .AddAttribute ("DownProb",
                   "Link break probability",
                   DoubleValue (0.01),
                   MakeDoubleAccessor (&NodeDownPropagationLossModel::m_downProb),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Recover",
                   "The random variable used to pick a link recover time.",
                   StringValue ("ns3:: ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&NodeDownPropagationLossModel::m_period),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

NodeDownPropagationLossModel::NodeDownPropagationLossModel () :
    m_downProb (0.01)
{
  m_down = CreateObject<UniformRandomVariable> ();
  m_recover = new NodeDownRecover ();
}

double
NodeDownPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b) const
{
  double linkBreakRx = -1e3;

  auto it = m_recover->m_recoverTime.find (a);
  if (it != m_recover->m_recoverTime.end ())
    {
      if (Simulator::Now ().GetSeconds () < it->second)
        return linkBreakRx;
      else
        m_recover->m_recoverTime.erase (it);
    }

  if (m_down->GetValue () < m_downProb)
    {
      double rt = Simulator::Now ().GetSeconds () + m_period->GetValue ();
      m_recover->m_recoverTime[a] = rt;
      return linkBreakRx;
    }
  else
    return txPowerDbm;
}

int64_t
NodeDownPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (ChannelChangePropagationLossModel);

TypeId
ChannelChangePropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ChannelChangePropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<ChannelChangePropagationLossModel> ()
    .AddAttribute ("Amplitude",
                   "Amplitude of channel variatiob",
                   StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=3.0|Bound=9.0]"),
                   MakeDoubleAccessor (&ChannelChangePropagationLossModel::m_amplitude),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Period",
                   "The random variable used to pick a link recover time.",
                   StringValue ("ns3:: ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&ChannelChangePropagationLossModel::m_period),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

ChannelChangePropagationLossModel::ChannelChangePropagationLossModel ()
{
  m_loss = 0;
  m_change = new ChannelChange ();
}

double
ChannelChangePropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b) const
{
  auto it1 = m_change->m_changeTime.find (std::make_pair (a,b));
  if (it1 != m_change->m_changeTime.end ())
    {
      if (Simulator::Now ().GetSeconds () >= it1->second)
        {
          m_loss = m_amplitude->GetValue ();
          m_change->m_changeTime[std::make_pair (a,b)] = Simulator::Now ().GetSeconds () + m_period->GetValue ();
        }
      return m_loss;
    }

  auto it2 = m_change->m_changeTime.find (std::make_pair (b,a));
  if (it2 != m_change->m_changeTime.end ())
    {
      if (Simulator::Now ().GetSeconds () >= it2->second)
        {
          m_loss = m_amplitude->GetValue ();
          m_change->m_changeTime[std::make_pair (b,a)] = Simulator::Now ().GetSeconds () + m_period->GetValue ();
        }
      return m_loss;
    }

  m_loss = m_amplitude->GetValue ();
  m_change->m_changeTime[std::make_pair (a,b)] = Simulator::Now ().GetSeconds () + m_period->GetValue ();
  return m_loss;
}

int64_t
ChannelChangePropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

// ------------------------------------------------------------------------- //

}
