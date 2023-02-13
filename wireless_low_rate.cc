// Network Topology
//
//              Lrwpan 2022:e00e::
//                                       
//  *    *    *    *    *     *     *     *   
//  |    |    |    |    |     |     |     |       2022:c00c::
// n9   n10  n11  n12  n13   n14   n15   n0 --------n16---------- n1   n2   n3   n4   n5  n6  n7  n8 
//                                                  csma          |    |    |    |   |   |   |   |
//                                                                *    *    *    *   *   *   *   *
//                                                                          Lrwpan 2022:a00a::



#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include <ns3/lr-wpan-error-model.h>


using namespace ns3;


int main (int argc, char** argv) {
  uint32_t simulationTime = 50;
  uint32_t speed = 15;
  uint32_t Csma_Nodes_count = 1;
  uint32_t Lw_nodes_count = 14;
  uint32_t Lw_nodes1_count = 14;

  std::string dataRate = "200Kbps";                  
  uint32_t payloadSize = 80;   


  std::string tcpVariants = "TcpNewReno";
  tcpVariants = "ns3::" + tcpVariants;
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 
                      TypeIdValue (TypeId::LookupByName (tcpVariants)));

  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName ("ns3::TcpClassicRecovery")));

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));


  NodeContainer Lw_nodes;
  Lw_nodes.Create (Lw_nodes_count);

  NodeContainer Lw_nodes1;
  Lw_nodes1.Create (Lw_nodes1_count);

  NodeContainer Csma_Nodes;
  Csma_Nodes.Create (Csma_Nodes_count);
  Csma_Nodes.Add (Lw_nodes.Get (0));
  Csma_Nodes.Add (Lw_nodes1.Get (0));

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (80),
                                 "DeltaY", DoubleValue (80),
                                 "GridWidth", UintegerValue (10),
                                 "LayoutType", StringValue ("RowFirst"));
                              
                
  mobility.Install (Lw_nodes);
  mobility.Install (Lw_nodes1);

  for(int i=1; i<static_cast<int>(Lw_nodes_count)-1; i++){
    Ptr<ConstantVelocityMobilityModel> mob = Lw_nodes.Get(i)->GetObject<ConstantVelocityMobilityModel>();
    mob->SetVelocity(Vector(speed, 0, 0));
  }

  for(int i=1; i<static_cast<int>(Lw_nodes1_count)-1; i++){
    Ptr<ConstantVelocityMobilityModel> mob1 = Lw_nodes1.Get(i)->GetObject<ConstantVelocityMobilityModel>();
    mob1->SetVelocity(Vector(speed, 0, 0));
  }


  LrWpanHelper lh;
  NetDeviceContainer lr_devices = lh.Install (Lw_nodes);
  lh.AssociateToPan (lr_devices, 0);

  LrWpanHelper lh1;
  NetDeviceContainer lr_devices1 = lh1.Install (Lw_nodes1);
  lh1.AssociateToPan (lr_devices1, 0);

  InternetStackHelper ISv6;
  ISv6.Install (Lw_nodes);
  ISv6.Install (Lw_nodes1);
  ISv6.Install (Csma_Nodes.Get(0));


  SixLowPanHelper sl;
  NetDeviceContainer sl_devices = sl.Install (lr_devices);

  SixLowPanHelper sl1;
  NetDeviceContainer sl_devices1 = sl1.Install (lr_devices1);

  CsmaHelper ch;
  ch.SetChannelAttribute ("DataRate", StringValue (dataRate));
  NetDeviceContainer c_devices = ch.Install (Csma_Nodes);

  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("2022:a00a::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer lr_interfaces;
  lr_interfaces = ipv6.Assign (sl_devices);
  lr_interfaces.SetForwarding (0, true);
  lr_interfaces.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2022:c00c::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer c_interfaces;
  c_interfaces = ipv6.Assign (c_devices);
  c_interfaces.SetForwarding (1, true);
  c_interfaces.SetDefaultRouteInAllNodes (1);

  ipv6.SetBase (Ipv6Address ("2022:e00e::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer lr_interfaces1;
  lr_interfaces1 = ipv6.Assign (sl_devices1);
  lr_interfaces1.SetForwarding (2, true);
  lr_interfaces1.SetDefaultRouteInAllNodes (2);

  for (uint32_t i = 0; i < sl_devices.GetN (); i++) {
    Ptr<NetDevice> device = sl_devices.Get (i);
    device->SetAttribute ("UseMeshUnder", BooleanValue (true));
    device->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  for (uint32_t i = 0; i < sl_devices1.GetN (); i++) {
    Ptr<NetDevice> device = sl_devices1.Get (i);
    device->SetAttribute ("UseMeshUnder", BooleanValue (true));
    device->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  uint32_t ports = 9;

  for( uint32_t i=1; i<=Lw_nodes_count-1; i++ ) {
    // BulkSendHelper sourceApp ("ns3::TcpSocketFactory",
    //                           Inet6SocketAddress (c_interfaces.GetAddress (0, 1), 
    //                           ports));
    // sourceApp.SetAttribute ("SendSize", UintegerValue (payloadSize));

    OnOffHelper server ("ns3::TcpSocketFactory", (Inet6SocketAddress (c_interfaces.GetAddress (0, 1), ports)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));

    ApplicationContainer sourceApps = server.Install (Lw_nodes.Get (i));
    sourceApps.Start (Seconds(0));
    sourceApps.Stop (Seconds(simulationTime));

    PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",
    Inet6SocketAddress (Ipv6Address::GetAny (), ports));
    sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    ApplicationContainer sinkApps = sinkApp.Install (Csma_Nodes.Get(0));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (simulationTime));
    
    ports++;
  }


  for( uint32_t i=1; i<=Lw_nodes1_count-1; i++ ) {
    // BulkSendHelper sourceApp ("ns3::TcpSocketFactory",
    //                           Inet6SocketAddress (lr_interfaces1.GetAddress (i, 1), 
    //                           ports+i));
    // sourceApp.SetAttribute ("SendSize", UintegerValue (payloadSize));

    OnOffHelper server ("ns3::TcpSocketFactory", (Inet6SocketAddress (c_interfaces.GetAddress (0, 1), ports)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));


    ApplicationContainer sourceApps = server.Install (Csma_Nodes.Get (0));
    sourceApps.Start (Seconds(0));
    sourceApps.Stop (Seconds(simulationTime));

    PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",
    Inet6SocketAddress (Ipv6Address::GetAny (), ports+i));
    sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    ApplicationContainer sinkApps = sinkApp.Install (Lw_nodes1.Get(i));
    sinkApps.Start (Seconds (10.0));
    sinkApps.Stop (Seconds (simulationTime));
    
    ports++;
  }


  FlowMonitorHelper flowHelper;
  flowHelper.InstallAll ();

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  flowHelper.SerializeToXmlFile ("./mySimOutput/paperNewReno.xml", false, false);

  Simulator::Destroy ();

  return 0;
}

