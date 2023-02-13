/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
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
 */
#include "tcp-chd.h"
#include "ns3/log.h"
#include "tcp-socket-state.h"

namespace ns3 {

// Libra
NS_LOG_COMPONENT_DEFINE ("TcpChd");
NS_OBJECT_ENSURE_REGISTERED (TcpChd);

TypeId
TcpChd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpChd")
    .SetParent<TcpNewReno> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpChd> ()
  ;
  return tid;
}

TcpChd::TcpChd (void) 
: TcpNewReno (),
    m_minRtt (Time::Max ()),
    m_maxRtt (Time::Min ()),
	m_RttCount (0),
	m_sumRtt (Time(0)),
	m_minQD (Time(0.005)),
	m_threshQD (Time(0.030)),
	m_maxBP (0.25),
	m_cWndNR (1),
	m_qDelay (Time(0)),
	m_maxQDelay (Time(0)),
  m_isPckLoss (0)
{
  NS_LOG_FUNCTION (this);
}

TcpChd::TcpChd (const TcpChd& sock)
  : TcpNewReno (sock),
    m_minRtt (sock.m_minRtt),
    m_maxRtt (sock.m_maxRtt),
	m_RttCount (sock.m_RttCount),
	m_minQD (sock.m_minQD),
	m_threshQD (sock.m_threshQD),
	m_maxBP (sock.m_maxBP),
	m_cWndNR (sock.m_cWndNR),
	m_qDelay (sock.m_qDelay),
	m_maxQDelay (sock.m_maxQDelay),
  m_isPckLoss (sock.m_isPckLoss)
{
  NS_LOG_FUNCTION (this);
}

TcpChd::~TcpChd (void)
{
}

uint32_t
TcpChd::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      m_cWndNR = tcb->m_cWnd;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}

uint32_t
TcpChd::CongestionAvoidanceNewReno (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / m_cWndNR;
      adder = std::max (1.0, adder);
      m_cWndNR += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In New Reno CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);

	  return m_cWndNR;
    }
	 
  return 0;
}

uint32_t
TcpChd::CongestionAvoidanceChd (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
	  double pb = CalcBackoffProbability();

	  double x = (std::rand()%10)/10;

	  if(x<pb){
		  return (static_cast<uint32_t>(tcb->m_cWnd/2));
	  }
	  else {
		  return (tcb->m_cWnd+static_cast<uint32_t>(1/tcb->m_cWnd));
	  }

    }

  return 0;
}

void
TcpChd::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
    
	  if(m_isPckLoss==0){
		  if(m_qDelay<=m_minQD){
        uint32_t wind = CongestionAvoidanceNewReno (tcb, segmentsAcked);
        tcb->m_cWnd = wind;
      }
      else{
        uint32_t wind = CongestionAvoidanceChd (tcb, segmentsAcked);
        tcb->m_cWnd = wind;

        uint32_t Swind = CongestionAvoidanceNewReno (tcb, segmentsAcked);
        m_cWndNR = std::max(wind, Swind);
      }
	  }
    else{
      m_isPckLoss = 0;
    }

    }
}

std::string
TcpChd::GetName () const
{
  return "TcpChd";
}

void
TcpChd::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                        const Time &rtt)
{
  NS_LOG_FUNCTION (this << tcb << packetsAcked << rtt);

  if (rtt.IsZero ())
    {
      return;
    }

  m_minRtt = std::min (m_minRtt, rtt);
  NS_LOG_DEBUG ("Updated m_minRtt = " << m_minRtt);

  m_maxRtt = std::max (rtt, m_maxRtt);
  NS_LOG_DEBUG ("Updated m_baseRtt = " << m_maxRtt);

  ++m_RttCount;
  NS_LOG_DEBUG ("Updated m_cntRtt = " << m_RttCount);

  m_sumRtt += rtt;
  NS_LOG_DEBUG ("Updated m_sumRtt = " << m_sumRtt);

  m_qDelay = m_sumRtt/m_RttCount - m_minRtt;
  NS_LOG_DEBUG ("Updated m_qDelay = " << m_maxQDelay);

  m_maxQDelay = m_maxRtt - m_maxRtt;
  NS_LOG_DEBUG ("Updated m_maxQDelay = " << m_maxQDelay);

}


double
TcpChd::CalcBackoffProbability() const
{
	
	double qDelayVal = static_cast<double> (m_qDelay.GetMilliSeconds());
	double maxQDelayVal = static_cast<double> (m_maxQDelay.GetMilliSeconds());
	double minQDelayVal = static_cast<double> (m_minQD.GetMilliSeconds());
	double threshVal = static_cast<double> (m_threshQD.GetMilliSeconds());

	if(qDelayVal==m_threshQD.GetMilliSeconds()){
		return m_maxBP;
	}
	else if(qDelayVal<m_threshQD.GetMilliSeconds()){
		double slope = m_maxBP/(threshVal-minQDelayVal);
		double c = (-1)*(slope*minQDelayVal);

		double prob = slope*qDelayVal + c;
		
		return prob;
	}
	else {
		double slope = (-1)*(m_maxBP/(maxQDelayVal-threshVal));
		double c = (-1)*(slope*maxQDelayVal);

		double prob = slope*qDelayVal + c;

		return prob;
	}
}

void
TcpChd::PckLossWindowUpdate(Ptr<TcpSocketState> tcb) 
{
  if(m_qDelay > m_threshQD){
    uint32_t wind = static_cast<uint32_t>(tcb->m_cWnd);	
    wind = std::max(m_cWndNR, wind);
    tcb->m_cWnd = static_cast<uint32_t> (wind/2);
  }
}


uint32_t
TcpChd::GetSsThresh (Ptr< TcpSocketState> state,
                         uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  //std::cout<<"bytesInFlight "<< bytesInFlight <<" max: "<< std::max (2 * state->m_segmentSize, bytesInFlight / 2);
  return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
}

Ptr<TcpCongestionOps>
TcpChd::Fork ()
{
  return CopyObject<TcpChd> (this);
}

}

