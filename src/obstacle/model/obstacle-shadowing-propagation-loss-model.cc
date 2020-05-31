/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 North Carolina State University
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
 * Author: Scott E. Carpenter <scarpen@ncsu.edu>
 *
 */

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/mobility-model.h"
#include <cmath>
#include "ns3/topology.h"

#include "obstacle-shadowing-propagation-loss-model.h"

NS_LOG_COMPONENT_DEFINE ("ObstacleShadowingPropagationLossModel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ObstacleShadowingPropagationLossModel);

TypeId
ObstacleShadowingPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ObstacleShadowingPropagationLossModel")

  .SetParent<PropagationLossModel> ()
  .AddConstructor<ObstacleShadowingPropagationLossModel> ();

  return tid;
}

ObstacleShadowingPropagationLossModel::ObstacleShadowingPropagationLossModel ()
  : PropagationLossModel ()
{
}

ObstacleShadowingPropagationLossModel::~ObstacleShadowingPropagationLossModel ()
{
}

double
ObstacleShadowingPropagationLossModel::GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);

  // initialize = no loss

  double L_obs = 0.0;

  // get the topology instance, to search for obstacles
  Topology * topology = Topology::GetTopology();
  NS_ASSERT(topology != 0);

  if (topology->HasObstacles() == true)
    {
      // additional loss for obstacles
      double p1_x = a->GetPosition ().x;
      double p1_y = a->GetPosition ().y;
      double p2_x = b->GetPosition ().x;
      double p2_y = b->GetPosition ().y;
      // for two points, p1 and p2
      Point p1(p1_x, p1_y);
      Point p2(p2_x, p2_y);
      // and testing for obstacles within r=200m
      double r = 200.0;

      // get the obstructed loss, from the topology class
      L_obs = topology->GetObstructedLossBetween(p1, p2, r);
      // std::cout<<"p1_x"<<p1_x<<"\n";
      // std::cout<<"p1_y"<<p1_y<<"\n";
      // std::cout<<"p2_x"<<p2_x<<"\n";
      // std::cout<<"p2_y"<<p2_y<<"\n";

    }
    // std::cout<<"L_obs の値は"<<L_obs<<"\n";
    // std::cout<<"L_obs"<<L_obs<<"\n";

  return L_obs;
}

double 
ObstacleShadowingPropagationLossModel::DoCalcRxPower (double txPowerDbm,
						Ptr<MobilityModel> a,
						Ptr<MobilityModel> b) const
{
  double retVal = 0.0;
  double loss = GetLoss (a, b);
  retVal = txPowerDbm - loss;
  // std::cout<<"txPowerDbm"<<txPowerDbm<<"\n";
  // std::cout<<"-----"<<"\n";
  // std::cout<<"loss"<<loss<<"\n";
  // std::cout<<"==";
  // std::cout<<"retVal"<<"\n";
  return (retVal);
}

int64_t
ObstacleShadowingPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

} // namespace ns3
