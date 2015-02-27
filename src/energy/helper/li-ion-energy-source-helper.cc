/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
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
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "li-ion-energy-source-helper.h"
#include "ns3/energy-source.h"

namespace ns3 {

LiIonEnergySourceHelper::LiIonEnergySourceHelper ()
{
  m_liIonEnergySource.SetTypeId ("ns3::LiIonEnergySource");
}

LiIonEnergySourceHelper::~LiIonEnergySourceHelper ()
{
}

void
LiIonEnergySourceHelper::Set (std::string name, const AttributeValue &v)
{
  m_liIonEnergySource.Set (name, v);
}

Ptr<EnergySource>
LiIonEnergySourceHelper::DoInstall (Ptr<Node> node) const
{
  NS_ASSERT (node != NULL);
  // check if energy source already exists
  Ptr<EnergySource> source = node->GetObject<EnergySource> ();
  if (source != NULL)
    {
      NS_FATAL_ERROR ("Energy source already installed!");
    }
  source = m_liIonEnergySource.Create<EnergySource> ();
  NS_ASSERT (source != NULL);
  source->SetNode (node);
  return source;
}

} // namespace ns3
