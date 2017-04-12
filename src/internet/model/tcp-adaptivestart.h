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
#ifndef TCPADAPTIVESTART_H
#define TCPADAPTIVESTART_H

#include "ns3/object.h"
#include "ns3/timer.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-congestion-ops.h"

namespace ns3 {

/**
 * \brief The NewReno implementation
 *
 * New Reno introduces partial ACKs inside the well-established Reno algorithm.
 * This and other modifications are described in RFC 6582.
 *
 * \see IncreaseWindow
 */
class TcpAdaptiveStart : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpAdaptiveStart ();

  /**
   * \brief Copy constructor.
   * \param sock object to copy.
   */
  TcpAdaptiveStart (const TcpAdaptiveStart& sock);

  ~TcpAdaptiveStart ();

  std::string GetName () const;

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);

  virtual Ptr<TcpCongestionOps> Fork ();

protected:
  TracedValue<double>    m_currentERE;             //!< Current value of the ERE (Eligible Rate Estimate)
  TracedValue<double>    m_bestRate;               //!< The best sending rate. Determined by using the value of m_minRtt   
  Time                   m_interval;               //!< The time interval over which ERE is calculated, as given in WestWood ABSE
  
  double                 m_lastSampleERE;          //!< Last ERE sample after being filtered
  double                 m_lastERE;                //!< Will become the previous instantaneous ERE value    
  
  
  Time                   m_minRtt;                 //!< Minimum RTT
  
  Time                   m_Rtt;                    //!< RTT   

  
  int                    m_ackedSinceT;            //!< The number of segments ACKed between T values in Westwood ABSE
 
  Time                   m_lastAck;                //!< Time at which the previous ACK arrived

  double                 m_deltaT;                 //!< Inter-arrival ACK time
 
  bool                   flag;                     //!< flag used to call CalculateERE() for the very first time
 
  EventId                m_ereEstimateEvent;  
 
 
private:

  /**
   * Calculate the Eligible Rate Estimate
   *
   * \param [in] tcb the socket state.
   */
  void CalculateERE (Ptr<TcpSocketState> tcb);
  
  /**
   * \brief Calculate the best allowable sending rate
   *
   * \param [in] m_minRtt the minimum RTT seen so far
   * \param [in] tcb the socket state.
   */

  void CalculateBestRate (const Time& m_minRtt, Ptr<TcpSocketState> tcb);
        

};

} // namespace ns3

#endif // TCPADAPTIVESTART_H
