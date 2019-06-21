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
}

double
LinkBreakPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b) const
{
  double linkBreakRx = -1e3;
  auto it1 = LinkBreakPropagationLossModel::m_recover.find (std::make_pair (a,b));
  auto it2 = LinkBreakPropagationLossModel::m_recover.find (std::make_pair (b,a));

  if (it1 != LinkBreakPropagationLossModel::m_recover.end ())
    return it1->second ? linkBreakRx : txPowerDbm;
  else if (it2 != LinkBreakPropagationLossModel::m_recover.end ())
    return it2->second ? linkBreakRx : txPowerDbm;
  else
    {
      UpdateLinkBreak (m_breakProb, m_period, a, b);
      it1 = LinkBreakPropagationLossModel::m_recover.find (std::make_pair (a,b));
      return  it1->second ? linkBreakRx : txPowerDbm;
    }

  return 0;
}

int64_t
LinkBreakPropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_period->SetStream (stream);
  return 1;
}

void
LinkBreakPropagationLossModel::UpdateLinkBreak (double m_breakProb,
                                                Ptr<RandomVariableStream> m_period,
                                                Ptr<MobilityModel> a,
                                                Ptr<MobilityModel> b)
{
  Ptr<UniformRandomVariable> m_break = CreateObject<UniformRandomVariable> ();
  bool temp = (m_break->GetValue () < m_breakProb);
  m_recover[std::make_pair (a,b)] = temp;
//  if (temp)
//    std::cout << "Link Down= " << a->GetPosition () << "-" << b->GetPosition () << std::endl;
//  else
//    std::cout << "Link Up= " << a->GetPosition () << "-" << b->GetPosition () << std::endl;
  Simulator::Schedule (Seconds (m_period->GetValue ()), &UpdateLinkBreak, m_breakProb, m_period, a, b);
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
}

double
NodeDownPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                             Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b) const
{
  double NodeDownRx = -1e3;
  auto it = m_recover.find (a);

  if (it != m_recover.end ())
    return it->second ? NodeDownRx : txPowerDbm;
  else
    {
      UpdateNodeDown (m_downProb, m_period, a);
      it = m_recover.find (a);
      return it->second ? NodeDownRx : txPowerDbm;
    }

  return 0;
}

int64_t
NodeDownPropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_period->SetStream (stream);
  return 1;
}

void
NodeDownPropagationLossModel::UpdateNodeDown (double m_downProb,
                                              Ptr<RandomVariableStream> m_period,
                                              Ptr<MobilityModel> a)
{
  Ptr<UniformRandomVariable> m_down = CreateObject<UniformRandomVariable> ();
  bool temp = (m_down->GetValue () < m_downProb);
  m_recover[a] = temp;
//  if (temp)
//    std::cout << "Node Down= " << a->GetPosition () << std::endl;
//  else
//    std::cout << "Node Up= " << a->GetPosition () << std::endl;
  Simulator::Schedule (Seconds (m_period->GetValue ()), &UpdateNodeDown, m_downProb, m_period, a);
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
  auto it1 = m_change.find (std::make_pair (a,b));
  auto it2 = m_change.find (std::make_pair (b,a));

  if (it1 != m_change.end ())
    return txPowerDbm - it1->second;
  else if (it2 != m_change.end ())
    return txPowerDbm - it2->second;
  else
    {
      UpdateChannelChange (m_amplitude, m_period, a, b);
      it1 = m_change.find (std::make_pair (a,b));
      return txPowerDbm - it1->second;
    }

  return 0;
}

int64_t
ChannelChangePropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_amplitude->SetStream (stream);
  m_period->SetStream (stream + 1);
  return 2;
}

void
ChannelChangePropagationLossModel::UpdateChannelChange (Ptr<RandomVariableStream> m_amplitude,
                                                        Ptr<RandomVariableStream> m_period,
                                                        Ptr<MobilityModel> a,
                                                        Ptr<MobilityModel> b)
{
  double temp = m_amplitude->GetValue ();
  m_change[std::make_pair (a,b)] = temp;
//  std::cout << "Extra loss of " << a->GetPosition () << "-" << b->GetPosition ()
//      << " = " << temp << std::endl;
  Simulator::Schedule (Seconds (m_period->GetValue ()), &UpdateChannelChange, m_amplitude, m_period, a, b);
}

// ------------------------------------------------------------------------- //

}
