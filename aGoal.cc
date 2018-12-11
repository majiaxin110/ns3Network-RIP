#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/netanim-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PoiRipRouting");

void TearDownLink (Ptr<Node> nodeA, Ptr<Node> nodeB, uint32_t interfaceA, uint32_t interfaceB)
{
  nodeA->GetObject<Ipv4> ()->SetDown (interfaceA);
  nodeB->GetObject<Ipv4> ()->SetDown (interfaceB);
}

void MoveOutNode (Ptr<Node> nodeA)
{
  ListPositionAllocator nodesPositionAllocator;
  Vector newPos(800, 300, 0);
  nodesPositionAllocator.Add(newPos);
  MobilityHelper nodesmobility;
  nodesmobility.SetPositionAllocator(&nodesPositionAllocator);
  nodesmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  nodesmobility.Install(nodeA);
}

int main (int argc, char **argv)
{
  bool verbose = false;
  bool printRoutingTables = true;
  bool showPings = true;
  int unusefulAmount = 4;
  std::string SplitHorizon ("PoisonReverse");

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("printRoutingTables", "Print routing tables at 30, 60 and 90 seconds", printRoutingTables);
  cmd.AddValue ("showPings", "Show Ping6 reception", showPings);
  cmd.AddValue ("splitHorizonStrategy", "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)", SplitHorizon);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnableAll (LogLevel (LOG_PREFIX_TIME | LOG_PREFIX_NODE));
      LogComponentEnable ("RipSimpleRouting", LOG_LEVEL_INFO);
      LogComponentEnable ("Rip", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv4L4Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("ArpCache", LOG_LEVEL_ALL);
      LogComponentEnable ("V4Ping", LOG_LEVEL_ALL);
    }

  if (SplitHorizon == "NoSplitHorizon")
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::NO_SPLIT_HORIZON));
    }
  else if (SplitHorizon == "SplitHorizon")
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::SPLIT_HORIZON));
    }
  else
    {
      Config::SetDefault ("ns3::Rip::SplitHorizon", EnumValue (RipNg::POISON_REVERSE));
    }


  // Create nodes compelete
  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> src = CreateObject<Node> ();
  Names::Add ("SrcNode", src);
  Ptr<Node> dst = CreateObject<Node> ();
  Names::Add ("DstNode", dst);
  Ptr<Node> Ap = CreateObject<Node> ();
  Names::Add ("Wifi", Ap);
  Ptr<Node> a = CreateObject<Node> ();
  Names::Add ("RouterA", a);
  Ptr<Node> b = CreateObject<Node> ();
  Names::Add ("RouterB", b);
  Ptr<Node> c = CreateObject<Node> ();
  Names::Add ("RouterC", c);
  Ptr<Node> d = CreateObject<Node> ();
  Names::Add ("RouterD", d);
  Ptr<Node> e = CreateObject<Node> ();
  Names::Add ("RouterE", e);
  Ptr<Node> f = CreateObject<Node> ();
  Names::Add ("RouterF", f);
  Ptr<Node> g = CreateObject<Node> ();
  Names::Add ("RouterG", g);

  Ptr<Node> Asuna = CreateObject<Node> ();
  Names::Add ("Asuna", Asuna);

  Ptr<Node> Kazuto = CreateObject<Node> ();
  Names::Add ("Kazuto", Kazuto);

  NodeContainer net1 (src, a); // a->src is 1
  NodeContainer net2 (a, f);
  NodeContainer net3 (f, g);
  NodeContainer net4 (g, b);
  NodeContainer net5 (b, e);
  NodeContainer net6 (e, d);
  NodeContainer net7 (d, c);
  NodeContainer net8 (c, a);
  NodeContainer net9(b, dst); // b->dst is 3
  NodeContainer net10(a, Ap); // a->Ap is 4
  NodeContainer net11(Ap, b); // b->Ap is 4
  NodeContainer routers1 (a, b, c, d, e); // NodeContainer's Constructor can only receive up to 5 parameters
  NodeContainer routers2 (f, g);
  NodeContainer nodes (src, dst);
  NodeContainer wifiAPContainer (Ap);
  NodeContainer wifiStaContainer (a,b,Asuna,Kazuto);
  NodeContainer wifiSAOContainer (Asuna,Kazuto);

  NodeContainer unusefulAddtions;
  unusefulAddtions.Create(unusefulAmount);
  NodeContainer unusefulCSMANodes;
  unusefulCSMANodes.Add(c);
  unusefulCSMANodes.Add(unusefulAddtions);

  // Create channels exclude wifi
  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer ndc1 = csma.Install (net1);
  NetDeviceContainer ndc2 = csma.Install (net2);
  NetDeviceContainer ndc3 = csma.Install (net3);
  NetDeviceContainer ndc4 = csma.Install (net4);
  NetDeviceContainer ndc5 = csma.Install (net5);
  NetDeviceContainer ndc6 = csma.Install (net6);
  NetDeviceContainer ndc7 = csma.Install (net7);
  NetDeviceContainer ndc8 = csma.Install (net8);
  NetDeviceContainer ndc9 = csma.Install (net9);

  NetDeviceContainer unusefulNdc = csma.Install(unusefulCSMANodes);

  // Begin : Wifi Channel settings
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());

  WifiHelper wifi;
  wifi.SetRemoteStationManager("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid-poi");
  mac.SetType("ns3::StaWifiMac",
		  "Ssid", SsidValue(ssid),
		  "ActiveProbing",BooleanValue(false));

  NetDeviceContainer wifiStaDevices;
  wifiStaDevices = wifi.Install(wifiPhy,mac,wifiStaContainer);

  mac.SetType("ns3::ApWifiMac",
		  "Ssid", SsidValue(ssid));

  NetDeviceContainer wifiApDevices;
  wifiApDevices = wifi.Install(wifiPhy,mac,wifiAPContainer);

  // Begin : RIP routing settings
  NS_LOG_INFO ("Create IPv4 and routing");
  RipHelper ripRouting;

  // Rule of thumb:
  // Interfaces are added sequentially, starting from 0
  // However, interface 0 is always the loopback...
  ripRouting.ExcludeInterface (a, 1);
  ripRouting.ExcludeInterface (b, 3);
  ripRouting.ExcludeInterface (c, 3);

//  ripRouting.SetInterfaceMetric (c, 3, 10);
//  ripRouting.SetInterfaceMetric (d, 1, 10);

  Ipv4ListRoutingHelper listRH;
  listRH.Add (ripRouting, 0);
  //  Ipv4StaticRoutingHelper staticRh;
  //  listRH.Add (staticRh, 5);

  InternetStackHelper internet;
  internet.SetIpv6StackInstall (false);
  internet.SetRoutingHelper (listRH);
  internet.Install (routers1);
  internet.Install (routers2);

  InternetStackHelper internetNodes;
  internetNodes.SetIpv6StackInstall (false);
  internetNodes.Install (nodes);
  internetNodes.Install(unusefulAddtions);
  internetNodes.Install(wifiAPContainer);
  internetNodes.Install(wifiSAOContainer);

  // set concrete static position
  ListPositionAllocator nodesPositionAllocator;
  Vector srcPos(100, 300, 0);
  Vector dstPos(500, 300, 0);
  nodesPositionAllocator.Add(srcPos);
  nodesPositionAllocator.Add(dstPos);
  MobilityHelper nodesmobility;
  nodesmobility.SetPositionAllocator(&nodesPositionAllocator);
  nodesmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  nodesmobility.Install(nodes);

  ListPositionAllocator routers1PositionAllocator;
  Vector aPos(260, 300, 0);
  Vector bPos(340, 300, 0);
  Vector cPos(200, 400, 0);
  Vector dPos(300, 400, 0);
  Vector ePos(400, 400, 0);
  routers1PositionAllocator.Add(aPos);
  routers1PositionAllocator.Add(bPos);
  routers1PositionAllocator.Add(cPos);
  routers1PositionAllocator.Add(dPos);
  routers1PositionAllocator.Add(ePos);
  MobilityHelper routers1mobility;
  routers1mobility.SetPositionAllocator(&routers1PositionAllocator);
  routers1mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  routers1mobility.Install(routers1);

  ListPositionAllocator routers2PositionAllocator;
  Vector fPos(200, 200, 0);
  Vector gPos(400, 200, 0);
  routers2PositionAllocator.Add(fPos);
  routers2PositionAllocator.Add(gPos);
  MobilityHelper routers2mobility;
  routers2mobility.SetPositionAllocator(&routers2PositionAllocator);
  routers2mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  routers2mobility.Install(routers2);

  ListPositionAllocator wifiPositionAllocator;
  Vector ApPos(300, 300, 0);
  wifiPositionAllocator.Add(ApPos);
  MobilityHelper wifimobility;
  wifimobility.SetPositionAllocator(&wifiPositionAllocator);
  wifimobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  wifimobility.Install(wifiAPContainer);

  ListPositionAllocator unusefulPositionAllocator;
  for(int i=0;i<unusefulAmount;i++)
  {
	  Vector currentPos(150+i*35,460,0);
	  unusefulPositionAllocator.Add(currentPos);
  }
  MobilityHelper unusefulMobility;
  unusefulMobility.SetPositionAllocator(&unusefulPositionAllocator);
  unusefulMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  unusefulMobility.Install(unusefulAddtions);

  Vector AsunaPos(280, 250, 0);
  ListPositionAllocator AsunaPositionAllocator;
  AsunaPositionAllocator.Add(AsunaPos);
  MobilityHelper AsunaMobility;
  AsunaMobility.SetPositionAllocator(&AsunaPositionAllocator);
  AsunaMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  AsunaMobility.Install(Asuna);

  MobilityHelper KazutoMobility;

  KazutoMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
							   "MinX", DoubleValue (320.0),
							   "MinY", DoubleValue (250.0),
							   "DeltaX", DoubleValue (15.0),
							   "DeltaY", DoubleValue (20.0),
							   "GridWidth", UintegerValue (3),
							   "LayoutType", StringValue ("RowFirst"));

  KazutoMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
						   "Bounds", RectangleValue (Rectangle (270, 370, 200, 300)));
  KazutoMobility.Install (Kazuto);

  // Assign addresses

  // Assign addresses.
  // The source and destination networks have global addresses
  // The "core" network just needs link-local addresses for routing.
  // We assign global addresses to the routers as well to receive
  // ICMPv6 errors.
  NS_LOG_INFO ("Assign IPv4 Addresses.");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase (Ipv4Address ("10.0.1.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic1 = ipv4.Assign (ndc1);

  ipv4.SetBase (Ipv4Address ("10.0.2.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic2 = ipv4.Assign (ndc2);

  ipv4.SetBase (Ipv4Address ("10.0.3.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic3 = ipv4.Assign (ndc3);

  ipv4.SetBase (Ipv4Address ("10.0.4.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic4 = ipv4.Assign (ndc4);

  ipv4.SetBase (Ipv4Address ("10.0.5.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic5 = ipv4.Assign (ndc5);

  ipv4.SetBase (Ipv4Address ("10.0.6.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic6 = ipv4.Assign (ndc6);

  ipv4.SetBase (Ipv4Address ("10.0.7.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic7 = ipv4.Assign (ndc7);

  ipv4.SetBase (Ipv4Address ("10.0.8.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic8 = ipv4.Assign (ndc8);

  ipv4.SetBase (Ipv4Address ("10.0.9.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer iic9 = ipv4.Assign (ndc9);

  // Wifi Address Assign
  ipv4.SetBase(Ipv4Address ("10.1.1.0"), Ipv4Mask ("255.255.255.0"));
  ipv4.Assign(wifiStaDevices);
  ipv4.Assign(wifiApDevices);

  // Unuseful Address Assign
  ipv4.SetBase(Ipv4Address ("10.8.1.0"), Ipv4Mask ("255.255.255.0"));
  ipv4.Assign(unusefulNdc);

  /*******************start*************************/
  Ptr<Ipv4StaticRouting> staticRouting;
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (src->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.0.1.2", 1 );
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (dst->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.0.9.1", 1 );

  if (printRoutingTables)
    {
      RipHelper routingHelper;

      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);

      routingHelper.PrintRoutingTableAt (Seconds (30.0), a, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (30.0), b, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (30.0), c, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (30.0), d, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (60.0), a, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (60.0), b, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (60.0), c, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (60.0), d, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (90.0), a, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (90.0), b, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (90.0), c, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (90.0), d, routingStream);
    }

  NS_LOG_INFO ("Create Applications.");
  uint32_t packetSize = 1024;
  Time interPacketInterval = Seconds (1.0);
  V4PingHelper ping ("10.0.9.2");
//  V4PingHelper pingWifi ("10.1.1.3");

  ping.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ping.SetAttribute ("Size", UintegerValue (packetSize));

//  pingWifi.SetAttribute ("Interval", TimeValue (interPacketInterval));
//  pingWifi.SetAttribute ("Size", UintegerValue (packetSize));

  if (showPings)
    {
      ping.SetAttribute ("Verbose", BooleanValue (true));
//      pingWifi.SetAttribute("Verbose", BooleanValue (true));
    }
  ApplicationContainer apps = ping.Install (src);
//  ApplicationContainer wifiApps = pingWifi.Install(a);

  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (200.0));

//  wifiApps.Start (Seconds (2.0));
//  wifiApps.Stop (Seconds (150.0));

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("rip-poi-routing.tr"));
  csma.EnablePcapAll ("rip-poi-routing", true);


  Simulator::Schedule(Seconds (20),&MoveOutNode,Ap);
  Simulator::Schedule(Seconds (70), &TearDownLink,a,Ap,4,0);
  Simulator::Schedule(Seconds (70), &TearDownLink,b,Ap,4,1); //83s reconnect

  /*********************end*********************/

  /* Now, do the actual simulation. */
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (200.0));

  AnimationInterface anim ("xmls/poi-rip.xml");
  anim.UpdateNodeDescription(src,"source");
  anim.UpdateNodeDescription(dst,"destination");
  anim.UpdateNodeDescription(Ap,"AP");
  anim.UpdateNodeDescription(Asuna,"Q1");
  anim.UpdateNodeDescription(Kazuto,"Q2");
  std::string descrip;
  for (int i=0;i<5;i++)
  {
	  descrip = 'A' + i;
	  anim.UpdateNodeDescription(routers1.Get(i),descrip);
  }
  anim.UpdateNodeDescription(f,"F");
  anim.UpdateNodeDescription(g,"G");
  anim.UpdateNodeColor(Ap,128,109,158);
  anim.UpdateNodeColor(src,10,240,10);
  anim.UpdateNodeColor(dst,10,10,240);
  anim.UpdateNodeColor(Asuna,249,125,28);
  anim.UpdateNodeColor(Kazuto,249,125,28);
  for(int i=0;i<unusefulAmount;i++)
  {
	  anim.UpdateNodeColor(unusefulAddtions.Get(i),80,80,80);
  }

  csma.EnablePcapAll("pcap/mycsma");

  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
