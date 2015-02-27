/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef RON_HEADER_H
#define RON_HEADER_H

#include "ns3/ptr.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/address.h"
#include <iostream>

namespace ns3 {

/* A Header for the Resilient Overlay Network (RON) client and server.
 */
class RonHeader : public Header 
{
public:
  RonHeader ();
  //RonHeader (Ipv4Address destination);
  explicit RonHeader (Ipv4Address destination, Ipv4Address intermediate = Ipv4Address((uint32_t)0));
  virtual ~RonHeader ();

  Ipv4Address GetFinalDest (void) const; 
  Ipv4Address GetNextDest (void) const;
  Ipv4Address GetOrigin (void) const;
  uint32_t GetSeq (void) const;
  uint8_t GetHop (void) const;
  //Ipv4Address PopNextDest (void); 
  uint8_t IncrHops (void);
  bool IsForward (void) const;
  void AddDest (Ipv4Address addr);
  void ReversePath (void);
  void SetDestination (Ipv4Address dest);
  void SetOrigin (Ipv4Address origin);
  void SetSeq (uint32_t seq);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

private:
  bool m_forward;
  uint8_t m_nHops;
  uint8_t m_nIps;
  uint32_t m_seq;
  uint32_t m_dest;
  uint32_t m_origin;
  uint32_t *m_ips;
};

#define RON_HEADER_SIZE(n) 15 + n*4

} //namespace ns3

#endif
