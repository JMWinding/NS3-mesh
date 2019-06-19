#include "propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <cmath>

#include "link-failure-propagation-loss-model.h"

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
    .AddAttribute ("Recover", "The random variable used to pick a link recover time.",
                   StringValue ("ns3:: ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&LinkBreakPropagationLossModel::m_recover),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("RecoverTime", "Storage recover time.",
                   StringValue ("ns3:: ExponentialRandomVariable[Mean=1.0]"),
                   MakePointerAccessor (&LinkBreakPropagationLossModel::m_recover),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

LinkBreakPropagationLossModel::LinkBreakPropagationLossModel ()
{
  m_break = CreateObject<UniformRandomVariable> ();
}

double
LinkBreakPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                              Ptr<MobilityModel> a,
                                              Ptr<MobilityModel> b)
{
  double linkBreakRx = -1e3;

  auto it1 = m_recoverTime.find (std::make_pair (a,b));
  if (it1 != m_recoverTime.end ())
    {
      if (Simulator::Now ().GetSeconds () < it1->second)
        return linkBreakRx;
      else
        m_recoverTime.erase (it1);
    }

  auto it2 = m_recoverTime.find (std::make_pair (b,a));
  if (it2 != m_recoverTime.end ())
    {
      if (Simulator::Now ().GetSeconds () < it2->second)
        return linkBreakRx;
      else
        m_recoverTime.erase (it2);
    }

  if (m_break->GetValue () < m_breakProb)
    {
      double rt = Simulator::Now ().GetSeconds () + m_recover->GetValue ();
      m_recoverTime.insert (std::make_pair (std::make_pair (a,b), rt));
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
