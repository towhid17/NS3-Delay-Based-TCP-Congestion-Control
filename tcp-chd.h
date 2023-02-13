#ifndef TCPCHD_H
#define TCPCHD_H

#include "tcp-congestion-ops.h"

namespace ns3 {

class TcpChd : public TcpNewReno
{
public:

  static TypeId GetTypeId (void);

  TcpChd ();

  TcpChd (const TcpChd& sock);

  ~TcpChd ();

  virtual std::string GetName () const;

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual Ptr<TcpCongestionOps> Fork ();
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);

  virtual void PckLossWindowUpdate(Ptr<TcpSocketState> tcb) ;

protected:
  virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t CongestionAvoidanceChd (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t CongestionAvoidanceNewReno (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

private:
  double CalcBackoffProbability() const;

private:
  Time m_minRtt;  
  Time m_maxRtt;
  uint32_t m_RttCount;   
  Time m_sumRtt;         
  Time m_minQD;
  Time m_threshQD;       
  double m_maxBP;
  uint32_t m_cWndNR;     
  Time m_qDelay;
  Time m_maxQDelay; 
  uint32_t m_isPckLoss;
};

}

#endif