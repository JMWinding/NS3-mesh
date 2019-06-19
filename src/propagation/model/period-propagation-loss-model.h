#ifndef LINK_FAILURE_PROPAGATION_LOSS_MODEL_H
#define LINK_FAILURE_PROPAGATION_LOSS_MODEL_H

#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-environment.h>

namespace ns3 {

class LinkBreakRecover
{
public:
  LinkBreakRecover ();

public:
  std::map<std::pair<Ptr<MobilityModel>,Ptr<MobilityModel>>, double> m_recoverTime;
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
  double m_breakProb;
  Ptr<UniformRandomVariable> m_break;
  Ptr<RandomVariableStream> m_period;
  Ptr<LinkBreakRecover> m_recover;

};

class NodeDownRecover
{
public:
  NodeDownRecover ();

public:
  std::map<Ptr<MobilityModel>, double> m_recoverTime;
}

class NodeDownPropagationLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  NodeDownPropagationLossModel ();

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  NodeDownPropagationLossModel (const NodeDownPropagationLossModel&);
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
  double m_downProb;
  Ptr<UniformRandomVariable> m_down;
  Ptr<RandomVariableStream> m_period;
  Ptr<NodeDownRecover> m_recover;

};

class ChannelChange
{
public:
  ChannelChange ();

public:
  std::map<std::pair<Ptr<MobilityModel>,Ptr<MobilityModel>>, double> m_changeTime;
}

class ChannelChangePropagationLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  ChannelChangePropagationLossModel ();

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  ChannelChangePropagationLossModel (const ChannelChangePropagationLossModel&);
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
  double m_loss;
  Ptr<RandomVariableStream> m_amplitude;
  Ptr<RandomVariableStream> m_period;
  Ptr<ChannelChange> m_change;

};

} // namespace ns3


#endif // ITU_R_1411_NLOS_OVER_ROOFTOP_PROPAGATION_LOSS_MODEL_H
