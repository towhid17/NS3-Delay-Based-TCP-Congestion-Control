
// Network Topology
//
//              LAN 172.1.3.0
//                                       
// ========================================   
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

std::ofstream goodputfile("./mySimOutput/goodputfileNewReno.txt");
uint64_t lastTotalRx = 0;                   
Ptr<PacketSink> sink;                         

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1000;   
  double errorRate = 0.00001;                    
  std::string dataRate = "4Mbps";                  
  std::string tcpVariant = "TcpNewReno";             
  std::string phyRate = "HtMcs7";                    
  double simulationTime = 10;    
     
  uint32_t nCsma = 10;
  uint32_t nCsma1 = 10;                    

  CommandLine cmd (__FILE__);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data ate", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpNewReno, TcpChd ", tcpVariant);
  cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);

  tcpVariant = std::string ("ns3::") + tcpVariant;
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

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

  NodeContainer csmaNodes1;
  csmaNodes1.Add (p2pNodes.Get (0));
  csmaNodes1.Create (nCsma1);

  CsmaHelper csma1;
  csma1.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma1.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices1;
  csmaDevices1 = csma1.Install (csmaNodes1);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (csmaNodes1);
  Ipv4AddressHelper address;

  address.SetBase ("172.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("172.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("172.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces1;
  csmaInterfaces1 = address.Assign (csmaDevices1);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (errorRate));
  for(int i = 0; i < static_cast<int>(nCsma); i++){
    csmaDevices.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    TypeId id;
    std::stringstream nodeId1;
    if(i%2==0){
      id = TypeId::LookupByName ("ns3::TcpNewReno");
    }
    else{
      id = TypeId::LookupByName ("ns3::TcpChd");
    }

    nodeId1 << csmaNodes1.Get(i)->GetId ();
    std::string specificNode = "/NodeList/" + nodeId1.str () + "/$ns3::TcpL4Protocol/SocketType";
    Config::Set (specificNode, TypeIdValue (id));

    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9+i));
    ApplicationContainer sinkApp = sinkHelper.Install (csmaNodes1.Get(i)); 
    sink = StaticCast<PacketSink> (sinkApp.Get (0));

    OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (csmaInterfaces1.GetAddress (i), 9+i)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    ApplicationContainer serverApp = server.Install (csmaNodes.Get(i));

    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));
  }

  Ptr<FlowMonitor> fm;
  FlowMonitorHelper fmh;
  fm=fmh.InstallAll();
   

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

  fm->SerializeToXmlFile("./mySimOutput/paperNewReno.xml",false,false);

  double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));

  Simulator::Destroy ();

  std::cout << tcpVariant <<": Error Rate: "<< errorRate<<"\nAverage throughput : " << averageThroughput << " Mbit/s" << std::endl;
  return 0;

}

