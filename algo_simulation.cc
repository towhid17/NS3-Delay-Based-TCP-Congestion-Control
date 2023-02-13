
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
uint64_t lastTotalRx = 0;                     /* The value of the last total received bytes */


Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */

void
CalcGoodput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
  std::cout <<"\n" <<now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  goodputfile << now.GetSeconds () <<" "<< cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (4000), &CalcGoodput);
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1500;   
  double errorRate = 0.00015;                    /* Transport layer payload size in bytes. */
  std::string dataRate = "10Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "TcpChd";             /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 5;                        /* Simulation time in seconds. */

  /* Command line argument parser setup. */
  CommandLine cmd (__FILE__);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data ate", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpNewReno, TcpVegas, TcpChd ", tcpVariant);
  cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);

  tcpVariant = std::string ("ns3::") + tcpVariant;
  // Select TCP variant
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));


  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
   
  uint32_t nCsma = 5;
  uint32_t nCsma1 = 5;

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("10Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);


  ///reciever

  NodeContainer csmaNodes1;
  csmaNodes1.Add (p2pNodes.Get (0));
  csmaNodes1.Create (nCsma1);

  CsmaHelper csma1;
  csma1.SetChannelAttribute ("DataRate", StringValue ("10Mbps"));
  csma1.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices1;
  csmaDevices1 = csma1.Install (csmaNodes1);

  /* Internet stack */
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

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Install TCP Receiver on the access point */
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (errorRate));
  for(int i = 0; i < 2; i++){
    csmaDevices.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));


    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9+i));
    ApplicationContainer sinkApp = sinkHelper.Install (csmaNodes1.Get(i)); 
    sink = StaticCast<PacketSink> (sinkApp.Get (0));

    /* Install TCP/UDP Transmitter on the station */
    OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (csmaInterfaces1.GetAddress (i), 9+i)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    ApplicationContainer serverApp = server.Install (csmaNodes.Get(i)); // server node assign

    /* Start Applications */
    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));
  }

  Simulator::Schedule (Seconds (1.1), &CalcGoodput);


  /* Flow Monitor */
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor=flowHelper.InstallAll();
   

  /* Start Simulation */
  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

   /* Flow Monitor File  */
  flowMonitor->SerializeToXmlFile("./mySimOutput/paperNewReno.xml",false,false);

  double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));

  Simulator::Destroy ();

  std::cout << tcpVariant <<": Error Rate: "<< errorRate<<"\nAverage throughput : " << averageThroughput << " Mbit/s" << std::endl;
  return 0;
}

