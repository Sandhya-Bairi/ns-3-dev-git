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
#include "tcp-adaptivestart.h"
#include "tcp-socket-base.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpAdaptiveStart");

// AdaptiveStart

NS_OBJECT_ENSURE_REGISTERED (TcpAdaptiveStart);

TypeId
TcpAdaptiveStart::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpAdaptiveStart")
    .SetParent<TcpNewReno> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpAdaptiveStart> ()
    .AddTraceSource("EREstimate", "The eligible rate estimate",
                    MakeTraceSourceAccessor(&TcpAdaptiveStart::m_currentERE),
                    "ns3::TracedValueCallback::Double")
    .AddTraceSource("EstimatedBestRate", "The best allowable sending rate",
                    MakeTraceSourceAccessor(&TcpAdaptiveStart::m_bestRate),
                    "ns3::TracedValueCallback::Double")
     .AddTraceSource("EstimatedERE", "The eligible rate estimate",
                    MakeTraceSourceAccessor(&TcpAdaptiveStart::m_currentERE),
                    "ns3::TracedValueCallback::Double")
  
  ;
  return tid;
}

TcpAdaptiveStart::TcpAdaptiveStart (void) : TcpNewReno (),
m_currentERE (0),
  m_bestRate (0),
  m_interval (Seconds (0.10)),
  m_lastSampleERE (0),
  m_lastERE (0),
  m_minRtt (Time (0)),
  m_Rtt (Time (0)),
  m_ackedSinceT (0),
  m_lastAck (Time::Min ()),
    flag(false)
{
  NS_LOG_FUNCTION (this);
}

TcpAdaptiveStart::TcpAdaptiveStart (const TcpAdaptiveStart& sock)
  : TcpNewReno (sock),
m_currentERE (sock.m_currentERE),
  m_bestRate (sock.m_bestRate),
  m_lastSampleERE (sock.m_lastSampleERE),
  m_lastERE (sock.m_lastERE),
  m_minRtt (Time (0)),
  m_Rtt (Time (0)),
   flag(sock.flag)
{
  NS_LOG_FUNCTION (this);
}

TcpAdaptiveStart::~TcpAdaptiveStart (void)
{
}


/**
 * \brief Try to increase the cWnd following the NewReno specification
 *
 * \see SlowStart
 * \see CongestionAvoidance
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpAdaptiveStart::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  /* Follwoing the Adaptive Start algorithm, the m_ssThresh is a maximum of the following three values:
   *         (2 * state->m_segmentSize) this is equivalent to two segments. m_ssThresh should never go below this
   *         (state->m_ssThresh)        the current m_ssThresh value
   *         (m_currentERE * m_minRtt)  this metric is as proposed in the paper
   */
  tcb->m_ssThresh = std::max (2 * tcb->m_segmentSize, uint32_t (m_currentERE ));

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }

  /* At this point, we could have segmentsAcked != 0. This because RFC says
   * that in slow start, we should increase cWnd by min (N, SMSS); if in
   * slow start we receive a cumulative ACK, it counts only for 1 SMSS of
   * increase, wasting the others.
   *
   * // Uncorrect assert, I am sorry
   * NS_ASSERT (segmentsAcked == 0);
   */
}



/**
 * \brief This function is called on receipt of every ACK
 *
 * In this function, we calculate m_minRtt and m_interval, and call the function CalculateERE() which calculates the ERE     
 * (m_currentERE) value
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 * \param rtt last round-trip-time
 */

void
TcpAdaptiveStart::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
      
       double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
       adder = std::max (1.0, adder);
      
       /**
         If CongestionAvoidance() has been called not because of loss event, rather because of our comparison with ssthresh,
         then tcb->m_cWnd should continue from its previous value, and not start from 1.
        */
       if(tcb->m_congState!=4)
         tcb->m_cWnd = std::max(tcb->m_ssThresh, tcb->m_cWnd);
      
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }
}


void
TcpAdaptiveStart::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                        const Time& rtt)
{
   
  
  NS_LOG_FUNCTION (this << tcb << packetsAcked << rtt);

  if (rtt.IsZero ())
    {
      NS_LOG_WARN ("RTT measured is zero!");
      return;
    }
    
 /**
   * Setting the value of m_deltaT. Initially, m_lastAck is set to Time::Min in which case m_lastAck is set to 0 
   * ( because no ACK has been received previously )
   */
   
  m_lastAck==Time::Min ()? m_deltaT = 0: m_deltaT = Simulator::Now (). GetSeconds () - m_lastAck. GetSeconds ();     
  
  m_lastAck = Simulator::Now ();

  m_ackedSinceT += packetsAcked;
  
  m_Rtt = rtt;
  
  if (m_minRtt.IsZero ())
    {
      m_minRtt = rtt;
    }
  else
    {
      if (rtt < m_minRtt)
        {
          m_minRtt = rtt;
        }
    }

  NS_LOG_LOGIC ("MinRtt: " << m_minRtt.GetMilliSeconds () << "ms");


  //uint32_t substitute = std::max (2 * tcb->m_segmentSize, uint32_t (tcb->m_ssThresh));
  //tcb->m_ssThresh = std::max (substitute, uint32_t (m_currentERE * static_cast<double> (m_minRtt.GetSeconds ())));
  
  tcb->m_ssThresh = std::max (2 * tcb->m_segmentSize, uint32_t (m_currentERE ));
  
  NS_LOG_LOGIC ("currentERE : " << m_currentERE <<"\nssthresh set to " << tcb->m_ssThresh);

         
  CalculateBestRate(m_minRtt, tcb);
       
  if (!(flag))
    {
      flag = true;
      CalculateERE(tcb);   
    }

}


/**
 * \brief To calculate the best allowable sending rate
 *
 * This function calculates m_bestRate by using minRtt, which is the smallest time in which an ACK has been received.
 *
 * \param minRtt minimum round-trip-time seen so far
 * \param tcb internal congestion state
 */
void
TcpAdaptiveStart::CalculateBestRate (const Time &minRtt, Ptr<TcpSocketState> tcb)
{

  NS_LOG_FUNCTION (this);

  NS_ASSERT (!minRtt.IsZero ());
  
  m_bestRate = double(tcb->m_cWnd / (double ( (minRtt.GetSeconds ()))));

  NS_LOG_LOGIC ("Estimated Best Rate: " << m_bestRate);
}


/**
 * \brief To calculate the Eligible Rate Estimate (ERE), denoted by the variable m_currentERE
 *
 * \param tcb internal congestion state
 */
void
TcpAdaptiveStart::CalculateERE (Ptr<TcpSocketState> tcb)
{
    
  NS_LOG_FUNCTION (this);

  NS_ASSERT (!m_interval.IsZero ());

  m_currentERE = m_ackedSinceT * tcb->m_segmentSize / m_interval.GetSeconds ();
  
  m_ackedSinceT = 0;

  NS_LOG_LOGIC ("Instantaneous ERE: " << m_currentERE);

  static double u = 0;
  static double umax = 0;
  
  if(m_currentERE > m_lastSampleERE)  
    u = 0.6 * u + 0.4 * (m_currentERE - m_lastSampleERE);
  else
    u = 0.6 * u + 0.4 * (m_lastSampleERE - m_currentERE);
  
  if(u>umax)
    umax = u;
 
  double tk;
  tk = m_Rtt.GetSeconds() + 10*m_Rtt.GetSeconds()*(u/umax);   
  
  // Filter the ERE sample
  double alpha = 0.9;

  alpha = (2*tk - m_deltaT)/(2*tk + m_deltaT);

  double instantaneousERE = m_currentERE;

  m_currentERE = (alpha * m_lastERE) + ((1 - alpha) * instantaneousERE);

  m_lastSampleERE = instantaneousERE;

  m_lastERE = m_currentERE;
   
   
       
  if (0.01 > (m_Rtt.GetSeconds() * ((m_bestRate - m_lastERE) / m_bestRate )))
    m_interval = Seconds(0.01);
  else
    m_interval = Seconds(m_Rtt.GetSeconds() *( (m_bestRate - m_lastERE)/ m_bestRate));
  
  m_ereEstimateEvent = Simulator::Schedule (m_interval, &TcpAdaptiveStart::CalculateERE,
                                                   this,tcb);

}


std::string
TcpAdaptiveStart::GetName () const
{
  return "TcpAdaptiveStart";
}


uint32_t
TcpAdaptiveStart::GetSsThresh (Ptr<const TcpSocketState> tcb,
                         uint32_t bytesInFlight)
{
  (void) bytesInFlight;
  NS_LOG_LOGIC ("Current ERE: " << m_currentERE << " minRtt: " <<
                m_minRtt << " ssthresh: " <<
                m_currentERE * static_cast<double> (m_minRtt.GetSeconds ()));
 
  return std::max (2 * tcb->m_segmentSize, uint32_t (m_currentERE));
}



Ptr<TcpCongestionOps>
TcpAdaptiveStart::Fork ()
{
  return CopyObject<TcpAdaptiveStart> (this);
}

} // namespace ns3
