#include <fstream>
#include <string>
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
  bool printRoutingTables = false;
  bool showPings = true;
  int routersAmount = 4;
  std::string SplitHorizon ("PoisonReverse");

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("printRoutingTables", "Print routing tables at 30, 60 and 90 seconds", printRoutingTables);
  cmd.AddValue ("showPings", "Show Ping6 reception", showPings);
  cmd.AddValue ("splitHorizonStrategy", "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)", SplitHorizon);
  cmd.AddValue ("amount","The amount of routers", routersAmount);
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


  // Create source and destination nodes
  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> src = CreateObject<Node> ();
  Names::Add ("SrcNode", src);
  Ptr<Node> dst = CreateObject<Node> ();
  Names::Add ("DstNode", dst);

  // Create routers
  NodeContainer routers;
  routers.Create(routersAmount);

  NodeContainer allNodes(src,dst);
  allNodes.Add(routers);

  NodeContainer nodesContainer(src,dst);
  NodeContainer srcNodes(src,routers.Get(0));
  NodeContainer dstNodes(routers.Get(routersAmount - 1),dst);

  // Begin : RIP routing settings
  NS_LOG_INFO ("Create IPv4 and routing");
  RipHelper ripRouting;

  // Rule of thumb:
  // Interfaces are added sequentially, starting from 0
  // However, interface 0 is always the loopback...
  ripRouting.ExcludeInterface (allNodes.Get(2), 2);
  ripRouting.ExcludeInterface (allNodes.Get(routersAmount + 1), 2);

  Ipv4ListRoutingHelper listRH;
  listRH.Add (ripRouting, 0);

  InternetStackHelper internet;
  internet.SetIpv6StackInstall (false);
  internet.SetRoutingHelper (listRH);
  internet.Install (routers);

  InternetStackHelper internetNodes;
  internetNodes.SetIpv6StackInstall (false);
  internetNodes.Install (src);
  internetNodes.Install(dst);


  // set concrete static position
  ListPositionAllocator routersPositionAllocator;
  for(int i = 0;i<routersAmount;i++)
  {
	  Vector currentPos(50+i*10,i % 2 == 0 ? 30 : 50,0);
	  routersPositionAllocator.Add(currentPos);
  }
  MobilityHelper routersMobility;
  routersMobility.SetPositionAllocator(&routersPositionAllocator);
  routersMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  routersMobility.Install(routers);

  ListPositionAllocator nodesPositionAllocator;
  Vector srcPos(30, 40, 0);
  Vector dstPos(50 + routersAmount * 10, 40, 0);
  nodesPositionAllocator.Add(srcPos);
  nodesPositionAllocator.Add(dstPos);
  MobilityHelper nodesMobility;
  nodesMobility.SetPositionAllocator(&nodesPositionAllocator);
  nodesMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  nodesMobility.Install(nodesContainer);

  // Create channels
  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  // Assign addresses and channels
  // The source and destination networks have global addresses
  // The "core" network just needs link-local addresses for routing.
  // We assign global addresses to the routers as well to receive
  // ICMPv6 errors.
  NS_LOG_INFO ("Assign IPv4 Addresses.");
  Ipv4AddressHelper ipv4;

  std::string currentAddr("10.0.");

  for(int i = 0;i < routersAmount - 1;i++)
  {
	  NodeContainer currentRouters(routers.Get(i),routers.Get(i+1));
	  NetDeviceContainer currentDevice = csma.Install(currentRouters);
	  currentAddr += char('0' + i);
	  currentAddr.append(".0");
	  ipv4.SetBase(Ipv4Address(currentAddr.c_str()), Ipv4Mask("255.255.255.0"));
	  Ipv4InterfaceContainer currentIIC = ipv4.Assign(currentDevice);

	  currentAddr = "10.0.";
  }

  NetDeviceContainer srcConn = csma.Install(srcNodes);
  NetDeviceContainer dstConn = csma.Install(dstNodes);

  ipv4.SetBase(Ipv4Address("10.6.0.0"), Ipv4Mask("255.255.255.0"));
  Ipv4InterfaceContainer srcIIC = ipv4.Assign(srcConn);

  ipv4.SetBase(Ipv4Address("10.7.0.0"), Ipv4Mask("255.255.255.0"));
  Ipv4InterfaceContainer dstIIC = ipv4.Assign(dstConn);

  Ptr<Ipv4StaticRouting> staticRouting;
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (src->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.6.0.2", 1 );
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (dst->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.7.0.1", 1 );

  //TO-DO: modify table output
  if (printRoutingTables)
    {
      RipHelper routingHelper;

      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);

    }

  NS_LOG_INFO ("Create Applications.");
//  uint32_t packetSize = 1024;
  Time interPacketInterval = Seconds (1.0);
//  V4PingHelper ping ("10.0.1.1");
//  V4PingHelper pingWifi ("10.1.1.3");

//  ping.SetAttribute ("Interval", TimeValue (interPacketInterval));
//  ping.SetAttribute ("Size", UintegerValue (packetSize));

//  pingWifi.SetAttribute ("Interval", TimeValue (interPacketInterval));
//  pingWifi.SetAttribute ("Size", UintegerValue (packetSize));

  if (showPings)
    {
//      ping.SetAttribute ("Verbose", BooleanValue (true));
//      pingWifi.SetAttribute("Verbose", BooleanValue (true));
    }
//  ApplicationContainer apps = ping.Install (src);
//  ApplicationContainer wifiApps = pingWifi.Install(a);

//  apps.Start (Seconds (1.0));
//  apps.Stop (Seconds (200.0));

//  wifiApps.Start (Seconds (2.0));
//  wifiApps.Stop (Seconds (150.0));

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("rip-poi-B-project.tr"));
  csma.EnablePcapAll ("rip-poi-B-project", true);


  /*********************end*********************/

  /* Now, do the actual simulation. */
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (200.0));

  AnimationInterface anim ("xmls/poi-B-project.xml");
  anim.UpdateNodeDescription(src,"source");
  anim.UpdateNodeDescription(dst,"destination");

  anim.UpdateNodeColor(src,10,240,10);
  anim.UpdateNodeColor(dst,10,10,240);

  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
