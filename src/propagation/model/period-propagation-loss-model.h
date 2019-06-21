#ifndef LINK_FAILURE_PROPAGATION_LOSS_MODEL_H
#define LINK_FAILURE_PROPAGATION_LOSS_MODEL_H

#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-environment.h>

namespace ns3 {

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
  static void UpdateLinkBreak (double m_breakProb,
                               Ptr<RandomVariableStream> m_period,
                               Ptr<MobilityModel> a,
                               Ptr<MobilityModel> b);

private:
  double m_breakProb;
  Ptr<RandomVariableStream> m_period;
  static std::map<std::pair<Ptr<MobilityModel>,Ptr<MobilityModel>>, bool> m_recover;

};

std::map<std::pair<Ptr<MobilityModel>,Ptr<MobilityModel>>, bool> LinkBreakPropagationLossModel::m_recover;

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
  static void UpdateNodeDown (double m_downProb,
                              Ptr<RandomVariableStream> m_period,
                              Ptr<MobilityModel> a);

private:
  double m_downProb;
  Ptr<RandomVariableStream> m_period;
  static std::map<Ptr<MobilityModel>, bool> m_recover;

};

std::map<Ptr<MobilityModel>, bool> NodeDownPropagationLossModel::m_recover;

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
  static void UpdateChannelChange (Ptr<RandomVariableStream> m_amplitude,
                                   Ptr<RandomVariableStream> m_period,
                                   Ptr<MobilityModel> a,
                                   Ptr<MobilityModel> b);

private:
  Ptr<RandomVariableStream> m_amplitude;
  Ptr<RandomVariableStream> m_period;
  static std::map<std::pair<Ptr<MobilityModel>,Ptr<MobilityModel>>, double> m_change;

};

std::map<std::pair<Ptr<MobilityModel>,Ptr<MobilityModel>>, double> ChannelChangePropagationLossModel::m_change;

} // namespace ns3


#endif // ITU_R_1411_NLOS_OVER_ROOFTOP_PROPAGATION_LOSS_MODEL_H
