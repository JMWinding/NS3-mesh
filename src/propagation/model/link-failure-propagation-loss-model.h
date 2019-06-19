#ifndef LINK_FAILURE_PROPAGATION_LOSS_MODEL_H
#define LINK_FAILURE_PROPAGATION_LOSS_MODEL_H

#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-environment.h>

namespace ns3 {

class LinkBreakRecover
{
public:
  std::map<std::pair<Ptr<MobilityModel>,Ptr<MobilityModel>>, double> m_linkRecoverTime;
  std::map<Ptr<MobilityModel>, double> m_nodeRecoverTime;
}

class LinkBreakPropagationLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  LinkBreakPropagationLossModel ();

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  LinkBreakPropagationLossModel (const LinkBreakPropagationLossModel&);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  LinkBreakPropagationLossModel& operator= (const LinkBreakPropagationLossModel&);
  virtual double DoCalcRxPower (double txPowerDbm,
                                Ptr<MobilityModel> a,
                                Ptr<MobilityModel> b) const;
  virtual int64_t DoAssignStreams (int64_t stream);

private:
  Ptr<UniformRandomVariable> m_break;
  double m_linkBreakProb;
  Ptr<RandomVariableStream> m_linkRecover;
  double m_nodeDownProb;
  Ptr<RandomVariableStream> m_nodeRecover;

};

} // namespace ns3


#endif // ITU_R_1411_NLOS_OVER_ROOFTOP_PROPAGATION_LOSS_MODEL_H
