
// Network Topology
//
//              Wifi 172.1.3.0
//                                       AP
//  *    *    *    *    *     *     *     *   
//  |    |    |    |    |     |     |     |       172.1.1.0
// n9   n10  n11  n12  n13   n14   n15   n0 -------------------- n1   n2   n3   n4   n5  n6  n7  n8 
//                                             point-to-point     |    |    |    |   |   |   |   |
//                                                                ================================
//                                                                          LAN 172.1.2.0


#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#include "ns3/flow-monitor-helper.h"
NS_LOG_COMPONENT_DEFINE ("mysim");

using namespace ns3;

std::ofstream goodputfile("./mysimOutput/goodputfile.txt");
uint64_t lastTotalRx = 0;                    
Ptr<PacketSink> sink;   

void
CalcGoodput ()
{
  Time now = Simulator::Now ();                                         
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5; 
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbps" << std::endl;
  goodputfile << now.GetSeconds () <<" "<< cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalcGoodput);
}

int
main (int argc, char *argv[])
{
  uint32_t packetSize = 1500;                       
  std::string dataRate = "100Mbps";                  
  std::string tcpVariant = "TcpNewReno";             
  std::string phyRate = "HtMcs7";                    
  double sim_time = 5;                     

  tcpVariant = std::string ("ns3::") + tcpVariant;
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetSize));
   
  uint32_t nCsma = 7;
  uint32_t nWifi = 7;

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);

  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (10.0),
                                 "DeltaY", DoubleValue (20.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("172.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("172.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("172.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevices);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  for(int i = 0; i < 7; i++){
    csmaDevices.Get (i+1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));


    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9+i));
    ApplicationContainer sinkApp = sinkHelper.Install (wifiStaNodes.Get(i)); 
    sink = StaticCast<PacketSink> (sinkApp.Get (0));

    OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (staInterface.GetAddress (i), 9+i)));
    server.SetAttribute ("PacketSize", UintegerValue (packetSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    ApplicationContainer serverApp = server.Install (csmaNodes.Get(i+1));

    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));
  }

  Simulator::Schedule (Seconds (1.1), &CalcGoodput);


  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor=flowHelper.InstallAll();
   

  Simulator::Stop (Seconds (sim_time + 1));
  Simulator::Run ();

  flowMonitor->SerializeToXmlFile("./mysimOutput/mysimflow.xml",false,false);

  double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * sim_time));

  Simulator::Destroy ();

  std::cout << "\nGoodput : " << averageThroughput << " Mbps" << std::endl;
  return 0;
}
