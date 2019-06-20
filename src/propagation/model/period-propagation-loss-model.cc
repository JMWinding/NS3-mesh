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
    .AddAttribute ("BreakProb", "Link break probability.",
                   DoubleValue (0.05),
                   MakeDoubleAccessor (&LinkBreakPropagationLossModel::m_breakProb),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Period", "The random variable used to pick a link recover time.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&LinkBreakPropagationLossModel::m_period),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

LinkBreakPropagationLossModel::LinkBreakPropagationLossModel () :
    m_breakProb (0.05)
{
  m_break = CreateObject<UniformRandomVariable> ();
}

double
LinkBreakPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b) const
{
  double linkBreakRx = -1e3;

  auto it1 = LinkBreakPropagationLossModel::m_recover.find (std::make_pair (a,b));
  if (it1 != LinkBreakPropagationLossModel::m_recover.end ())
    {
      if (Simulator::Now ().GetSeconds () < it1->second)
        return linkBreakRx;
      else
        LinkBreakPropagationLossModel::m_recover.erase (it1);
    }

  auto it2 = LinkBreakPropagationLossModel::m_recover.find (std::make_pair (b,a));
  if (it2 != LinkBreakPropagationLossModel::m_recover.end ())
    {
      if (Simulator::Now ().GetSeconds () < it2->second)
        return linkBreakRx;
      else
        LinkBreakPropagationLossModel::m_recover.erase (it2);
    }

  if (m_break->GetValue () < m_breakProb)
    {
      double rt = Simulator::Now ().GetSeconds () + m_period->GetValue ();
      LinkBreakPropagationLossModel::m_recover[std::make_pair (a,b)] = rt;
      return linkBreakRx;
    }
  else
    return txPowerDbm;

  return 0;
}

int64_t
LinkBreakPropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_period->SetStream (stream);
  return 1;
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
    .AddAttribute ("DownProb", "Node down probability.",
                   DoubleValue (0.01),
                   MakeDoubleAccessor (&NodeDownPropagationLossModel::m_downProb),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Period", "The random variable used to pick a link recover time.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&NodeDownPropagationLossModel::m_period),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

NodeDownPropagationLossModel::NodeDownPropagationLossModel () :
    m_downProb (0.01)
{
  m_down = CreateObject<UniformRandomVariable> ();
}

double
NodeDownPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b) const
{
  double linkBreakRx = -1e3;

  auto it = m_recover.find (a);
  if (it != m_recover.end ())
    {
      if (Simulator::Now ().GetSeconds () < it->second)
        return linkBreakRx;
      else
        m_recover.erase (it);
    }

  if (m_down->GetValue () < m_downProb)
    {
      double rt = Simulator::Now ().GetSeconds () + m_period->GetValue ();
      m_recover[a] = rt;
      return linkBreakRx;
    }
  else
    return txPowerDbm;

  return 0;
}

int64_t
NodeDownPropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_period->SetStream (stream);
  return 1;
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
    .AddAttribute ("Amplitude", "The random variable used to pick amplitude of channel variation.",
                   StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=3.0|Bound=9.0]"),
                   MakePointerAccessor (&ChannelChangePropagationLossModel::m_amplitude),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("Period", "The random variable used to pick a link recover time.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&ChannelChangePropagationLossModel::m_period),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

ChannelChangePropagationLossModel::ChannelChangePropagationLossModel ()
{
}

double
ChannelChangePropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b) const
{
  double loss = m_amplitude->GetValue ();
  double next = Simulator::Now ().GetSeconds () + m_period->GetValue ();
  auto it1 = m_change.find (std::make_pair (a,b));
  if (it1 != m_change.end ())
    {
      if (Simulator::Now ().GetSeconds () >= it1->second.second)
        it1->second = std::make_pair (loss, next);
      return loss;
    }

  auto it2 = m_change.find (std::make_pair (b,a));
  if (it2 != m_change.end ())
    {
      if (Simulator::Now ().GetSeconds () >= it2->second.second)
        it2->second = std::make_pair (loss, next);
      return loss;
    }

  m_change[std::make_pair (a,b)] = std::make_pair (loss, next);
  return loss;

  return 0;
}

int64_t
ChannelChangePropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_amplitude->SetStream (stream);
  m_period->SetStream (stream + 1);
  return 2;
}

// ------------------------------------------------------------------------- //

}
