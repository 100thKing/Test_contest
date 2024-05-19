#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Create LTE Helper
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  // Create EPC Helper
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  // Create Node Container for eNB and UEs
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (1);
  ueNodes.Create (2);

  // Install Mobility Model
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);
  mobility.Install (ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  InternetStackHelper internet;
  internet.Install (ueNodes);

  // Assign IP addresses to UEs
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("10.1.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = ipv4h.Assign (ueLteDevs);

  // Attach UEs to eNB
  lteHelper->Attach (ueLteDevs, enbLteDevs.Get (0));

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  UdpClientHelper dlClient (ueIpIface.GetAddress (0), dlPort);
  dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds (10)));
  dlClient.SetAttribute ("PacketSize", UintegerValue (1024)); // Размер пакета 1024 байт
  dlClient.SetAttribute ("MaxPackets", UintegerValue (1000000));

  UdpServerHelper dlServer (dlPort);
  serverApps.Add (dlServer.Install (ueNodes.Get (0)));

  clientApps.Add (dlClient.Install (ueNodes.Get (1)));

  UdpClientHelper ulClient (ueIpIface.GetAddress (1), ulPort);
  ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds (10)));
  ulClient.SetAttribute ("PacketSize", UintegerValue (1024)); // Размер пакета 1024 байт
  ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));

  UdpServerHelper ulServer (ulPort);
  serverApps.Add (ulServer.Install (ueNodes.Get (1)));

  clientApps.Add (ulClient.Install (ueNodes.Get (0)));

  serverApps.Start (Seconds (1.0));
  clientApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  clientApps.Stop (Seconds (10.0));

  // Configure the scheduler
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");

  // Enable key performance metrics output
  lteHelper->EnableRlcTraces ();
  lteHelper->EnableMacTraces ();

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
