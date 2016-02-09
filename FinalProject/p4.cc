#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-dumbbell.h"
#include <iostream>
#include <iomanip>
#include <map>

#include "WormApp.h"
#include "ns3/object-factory.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;
using namespace std;

#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <vector>
#include <time.h>
#include <iomanip>
#include <assert.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/stats-module.h"
#include "ns3/position-allocator.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-address.h"
#include <map>

#define worm 1
#define onoff 0

#define debug 0

// ------------ Define worm types    ---------------
#define UdpWORMTYPE  1
#define UDPWORMTYPE  2
#define WORMTYPE     UDPWORMTYPE

// ------------ Define the topology  ---------------
#define TREES        4
#define FANOUT1      2
#define FANOUT2      32
#define TREELEGPROB  0.85

#define LINKBW       "1Mbps"
#define HLINKBW      "10Mbps"
#define BLINKBW      "100Mbps"

// ------------ Worm parameters -----------------------
#define VULNERABILITY  1.0
#define SCANRATE       100
#define SCANRANGE      0
#define PAYLOAD        1000
#define NUMCONN        1

// ----------- Simulation settings -------------------
#define SIMTIME        0.0
#define SEEDVALUE      1

using namespace ns3;
using namespace std;

ApplicationContainer sinkApps;	
ApplicationContainer container;
vector<NodeContainer> fanout2Nodes;

AnimationInterface * anim;
int total_infected=0;

typedef map< int,Ptr<Node> > node_map_t;
node_map_t node_map;
static void IntTrace( const Ptr< const Packet > packet, const Address &address )
	{

	     uint32_t recvSize =packet->GetSize();
 		 uint32_t* buffer= new uint32_t [recvSize];
 		 packet->CopyData((uint8_t *)buffer,recvSize);   
		

		int node_id;	
		istringstream(string((char*)buffer)) >> node_id;
		
		//cout<< node_id << "data " << endl;	
		
		Ptr <Node> ptr=node_map[node_id];
				
		Ptr <Application> app = container.Get (node_id-1);
		Ptr <WormApp> wapp = DynamicCast <WormApp> (app);
	
		if(ptr!=NULL && wapp!=NULL)
{		
		if(!wapp->m_infected)		
			{
				wapp->m_infected=1;
				total_infected++;
				wapp->infectnodes();
				cout<<total_infected <<" " <<Simulator::Now()<<endl;
			}			
	 	//change color
		
		//Ptr <Node> ptr= fanout2Nodes.Get(node_id-1);
		
		anim->UpdateNodeColor (ptr,0,0,0);
		//AnimationInterface::UpdateNodeColor(ptr,0,0,0);
		
		//anim.UpdateNodeColor(ptr,0,0,0);
		
		//wapp ->set_pkt_size(1024);
		//Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);	    
		 //pktSink->TraceConnectWithoutContext ("Rx", MakeCallback (&IntTrace)); 
}		       
	}



int main(int argc, char* argv[])
{
  uint32_t wormtype = UdpWORMTYPE;
  uint32_t nt = TREES;
  uint32_t nf1 = FANOUT1;
  uint32_t nf2 = FANOUT2;
  string linkbw  = LINKBW;
  string hlinkbw = HLINKBW;
  string blinkbw = BLINKBW;
  uint32_t scanrate = SCANRATE;
  uint32_t payload = PAYLOAD;
  uint32_t seedValue = SEEDVALUE;
  uint32_t numConn = NUMCONN;
  double vulnerability = VULNERABILITY;
  double treelegprob = TREELEGPROB;
  double simtime = SIMTIME;
  bool logTop = 0;
  std::string dataFileName = "p4.data";
  uint16_t port = 5001;
  
  CommandLine cmd;
  cmd.AddValue ("wormtype",      "Type of worm: UDP or Udp",     wormtype);
  cmd.AddValue ("trees",         "Number of trees",              nt);
  cmd.AddValue ("fanout1",       "First fanout of trees",        nf1);
  cmd.AddValue ("fanout2",       "Second fanout of trees",       nf2);
  cmd.AddValue ("linkbw",        "Link bandwidth",               linkbw);
  cmd.AddValue ("hlinkbw",       "HLink bandwidth",              hlinkbw);
  cmd.AddValue ("blinkbw",       "BLink bandwidth",              blinkbw);
  cmd.AddValue ("scanrate",      "Scan rate",                    scanrate);
  cmd.AddValue ("payload",       "Payload",                      payload);
  cmd.AddValue ("seedvalue",     "Seed value for RNG",           seedValue);
  cmd.AddValue ("vulnerability", "Vulnerability to infection",   vulnerability);
  cmd.AddValue ("numConn",       "Number of Udp connections",    numConn);
  cmd.AddValue ("treelegprob",   "Probability of tree legs",     treelegprob);
  cmd.AddValue ("simtime",       "Simulator time in seconds",    simtime);
  cmd.AddValue ("logTop",        "Display the topology stats",   logTop);
  cmd.AddValue ("filename",      "Name of output file",          dataFileName);

  cmd.Parse (argc,argv);

  // Set the random number generator
  SeedManager::SetSeed (seedValue);
  //SeedManager::SetSeed ((int) clock());
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetAttribute ("Stream", IntegerValue (6110));
  uv->SetAttribute ("Min", DoubleValue (0.0));
  uv->SetAttribute ("Max", DoubleValue (1.0));

  //Config::SetDefault("ns3::UdpL4Protocol::SocketType", TypeIdValue (UdpTahoe::GetTypeId()));
  
  // Create nodes.
  vector<NodeContainer> fanout1Nodes(nt);
  vector<NodeContainer> fanout2Nodes;
  if (logTop) cout << "Topology: " << endl;
  for (uint32_t i = 0; i < nt; ++i)
    {
       if (logTop) cout << "Tree " << i << endl;
       for (uint32_t j = 0; j < nf1; ++j)
         {
           double uv1 = uv->GetValue ();
           if (uv1 <= treelegprob)
             {
               // Create a fanout node for this tree
               if (logTop) cout << "\tFanout " << fanout1Nodes[i].GetN() << ": ";
               fanout1Nodes[i].Create(1);
               Ptr<Node> temp=fanout1Nodes[i].Get(0);
              
               fanout2Nodes.push_back(NodeContainer ());
               for (uint32_t k = 0; k < nf2; ++k)
                 {
                   double uv2 = uv->GetValue ();
                   if (uv2 <= treelegprob)
                     {
                       // Create a fanout node for this fanout node
                       fanout2Nodes[fanout2Nodes.size()-1].Create(1);
                       Ptr<Node> temp1=fanout2Nodes[fanout2Nodes.size()-1].Get(0);
                       //int id1=temp1->GetId();
                       //node_map[id1]=temp1;
                     }
                 }
               if (logTop) cout << fanout2Nodes[fanout2Nodes.size()-1].GetN() << " ";
               if (logTop) cout << endl;
             }
         }
    }

  NodeContainer treeNodes;
  treeNodes.Create(nt);

  // Consolidate all nodes to install on the stack.
  NodeContainer nodes;
  nodes.Create(1); // Root node
  nodes.Add(treeNodes);
  for (uint32_t i = 0; i < nt; ++i)
    {
      nodes.Add(fanout1Nodes[i]);
    }

  for (uint32_t i = 0; i < fanout2Nodes.size(); ++i)
    {
      nodes.Add(fanout2Nodes[i]);
    }

  // Install nodes on internet stack.
  InternetStackHelper stack;
  stack.Install (nodes);

  // Create channels and devices.
  PointToPointHelper tree;
  tree.SetDeviceAttribute ("DataRate", StringValue (blinkbw));
  tree.SetChannelAttribute ("Delay", StringValue ("20ms"));
  vector<NetDeviceContainer> treeDevices(nt);

  PointToPointHelper fanout1;
  fanout1.SetDeviceAttribute ("DataRate", StringValue (hlinkbw));
  fanout1.SetChannelAttribute ("Delay", StringValue ("20ms"));
  vector<NetDeviceContainer> fanout1Devices;

  PointToPointHelper fanout2;
  fanout2.SetDeviceAttribute ("DataRate", StringValue (linkbw));
  fanout2.SetChannelAttribute ("Delay", StringValue ("20ms"));
  vector<NetDeviceContainer> fanout2Devices;

  uint32_t next = 0;
  uint32_t nNext = 0;
  for (uint32_t i = 0; i < nt; ++i)
    {
      // Connect the root node to each tree node
      treeDevices[i] = tree.Install (nodes.Get(0),
                                     treeNodes.Get(i));

      for (uint32_t j = 0; j < fanout1Nodes[i].GetN(); ++j)
        {
          // Connect each tree node to its fanout nodes
          fanout1Devices.push_back (fanout1.Install (treeNodes.Get(i),
                                                     fanout1Nodes[i].Get(j)));

          for (uint32_t k = 0; k < fanout2Nodes[next+j].GetN(); ++k)
            {
              fanout2Devices.push_back (fanout2.Install (fanout1Nodes[i].Get(j),
                                                         fanout2Nodes[next+j].Get(k)));
            }
        }
      next += fanout1Nodes[i].GetN();
    }

  // Assign IP addresses
  ostringstream oss;
  Ipv4AddressHelper ipv4;
  vector<Ipv4InterfaceContainer> iTree(nt);
  vector<Ipv4InterfaceContainer> iFanout1;
  vector<Ipv4InterfaceContainer> iFanout2;
  next = 0;
  nNext = 0;
  for (uint32_t i = 0; i < nt; ++i)
    {
      oss.str ("");
      oss << i + 1 << ".1.0.0";
      ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");

      iTree[i] = ipv4.Assign(treeDevices[i]);

      for (uint32_t j = 0; j < fanout1Nodes[i].GetN(); ++j)
        {
          oss.str ("");
          oss << "192." << 1 + next + j << ".1.0";
          ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");

          iFanout1.push_back(ipv4.Assign(fanout1Devices[next+j]));
          for (uint32_t k = nNext; k < nNext + fanout2Nodes[next+j].GetN(); ++k)
            {
              oss.str ("");
              oss << "192." << 1 + next + j << "." << k + 2 - nNext << ".0";
              ipv4.SetBase (oss.str ().c_str (), "255.255.255.0");
              iFanout2.push_back(ipv4.Assign(fanout2Devices[k]));
            }
          nNext += fanout2Nodes[next+j].GetN();
        }
      next += fanout1Nodes[i].GetN();
    }

  // Populate routing tables.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

MobilityHelper mobility;
mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (100.0),
                                     "MinY", DoubleValue (0.0),
                                     "DeltaX", DoubleValue (10.0),
                                     "DeltaY", DoubleValue (10.0),
                                     "GridWidth", UintegerValue (200),
                                     "LayoutType", StringValue ("RowFirst"));
mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
mobility.Install (nodes.Get(0));


    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (50.0),
                                     "MinY", DoubleValue (10.0),
                                     "DeltaX", DoubleValue (100.0),
                                     "DeltaY", DoubleValue (0.0),
                                     "GridWidth", UintegerValue (200),
                                     "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (treeNodes);
    


mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (0.0),
                                     "MinY", DoubleValue (20.0),
                                     "DeltaX", DoubleValue (25.0),
                                     "DeltaY", DoubleValue (0.0),
                                     "GridWidth", UintegerValue (200),
                                     "LayoutType", StringValue ("RowFirst"));
mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  for (uint32_t i = 0; i < nt; ++i)
    {
      mobility.Install(fanout1Nodes[i]);
    }


mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue (0.0),
                                     "MinY", DoubleValue (30.0),
                                     "DeltaX", DoubleValue (10.0),
                                     "DeltaY", DoubleValue (0.0),
                                     "GridWidth", UintegerValue (200),
                                     "LayoutType", StringValue ("RowFirst"));
mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  for (uint32_t i = 0; i < fanout2Nodes.size(); ++i)
    {
      mobility.Install(fanout2Nodes[i]);
    }

////////////install the sinks
	
	 
	Address sinkLocalAddress1 (InetSocketAddress (Ipv4Address::GetAny (), port));
	  	PacketSinkHelper packetSinkHelper1 ("ns3::UdpSocketFactory", sinkLocalAddress1);
	  	
		  for(uint32_t i=0;i<fanout2Nodes.size();i++)

	{
		for(uint32_t j=0;j<fanout2Nodes[i].GetN();j++){ 	
		sinkApps.Add (packetSinkHelper1.Install (fanout2Nodes[i].Get(j)));
		//sinkApps.Add (packetSinkHelper1.Install (nodes.Get(i)));
		//cout<<" Node id is "<< fanout2Nodes[i].Get(j) ->GetId()<< endl;			
		Ptr<Node> ptr= fanout2Nodes[i].Get(j);
	        Ptr<Ipv4> ipv4ptr= ptr->GetObject<Ipv4>();
	       // cout<< " node IP is "<< ipv4ptr->GetAddress(1,0).GetLocal()<<"  ";
		
       
	}
	  	//sinkApps1.Add (packetSinkHelper1.Install (d2.GetRight (1)));
			
	}	
sinkApps.Start (Seconds (0.0));
	  	sinkApps.Stop (Seconds (60.0));

///////ipv4 addresses part
      
	 

if(onoff)
{
    	 OnOffHelper UdpclientHelper ("ns3::UdpSocketFactory", Address ());
  	  UdpclientHelper.SetAttribute ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
  	  UdpclientHelper.SetAttribute ("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
        
  
	  ApplicationContainer Udp_client;
	 
	  //AddressValue remoteAddress (InetSocketAddress (ipv4_tier2[0].GetAddress(0), port));
AddressValue remoteAddress (InetSocketAddress ("192.1.2.2", port));
	  UdpclientHelper.SetAttribute ("Remote", remoteAddress);
	  UdpclientHelper.SetAttribute ("PacketSize", UintegerValue (512));
	  
	  for(uint32_t k=0;k<fanout2Nodes.size();k++)
 {
	  for(uint32_t i=0;i<fanout2Nodes[k].GetN();i++)

	{	
		
		//AddressValue remoteAddress (InetSocketAddress (ipv4_tier2[i].GetAddress(0), port));
		  //UdpclientHelper.SetAttribute ("Remote", remoteAddress);
	  
		//UdpclientHelper.SetAttribute ("PacketSize", UintegerValue (512));
	  	 	
		//if(i==0)
	  	//UdpclientHelper.SetAttribute ("PacketSize", UintegerValue (1024));

		
	  	Udp_client.Add (UdpclientHelper.Install (fanout2Nodes[k].Get(i)));
	  	Udp_client.Start (Seconds (1.0)); // Start 1 second after sink
	  	Udp_client.Stop (Seconds (60.0)); // Stop before the sink
	}
}
	
}
if (worm)
{	WormApp worm_app;


	//helper for the worm class
	ObjectFactory m_factory;
	
	m_factory.SetTypeId (worm_app.GetTypeId());
	m_factory.Set ("OnTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
 	m_factory.Set("OffTime", StringValue ("ns3::UniformRandomVariable[Min=0.,Max=1.]"));
 	m_factory.Set("MaxBytes", UintegerValue(50000));
	

	Ptr<WormApp> client_app = m_factory.Create<WormApp> ();
	client_app ->set_number_connections(4);
	
	client_app ->set_pkt_size(512);
	client_app ->m_infected = 1;	
	client_app ->set_max_packets(1)	;
	
	
	 
  	nodes.Get(0)->AddApplication (client_app);
	client_app ->initialize("ns3::UdpSocketFactory", port);	
	
	//cout<<client_app->m_maxBytes<<endl;
	//cout<<client_app->m_pktSize<<endl;
	//cout<<client_app->m_onTime<<endl;	
	//cout<<client_app->m_offTime<<endl;
	//cout<<client_app->m_cbrRate<<endl;
	//cout<<client_app->m_maxBytes<<endl;
	//cout<<client_app->m_port << endl;

	container.Add(client_app);
	
		for(uint32_t i=0;i<fanout2Nodes.size();i++)

	{
		for(uint32_t j=0;j<fanout2Nodes[i].GetN();j++)
	{ 		
		Ptr< WormApp> new_app = m_factory.Create<WormApp> ();
		new_app ->set_number_connections(4);
		new_app ->set_pkt_size(512);		
		new_app ->set_max_packets(1)	;
		new_app->m_infected=0;
	
		fanout2Nodes[i].Get(j) ->AddApplication (new_app);
		new_app ->initialize("ns3::UdpSocketFactory", port);	
		container.Add(new_app);
				
		//sinkApps.Add (packetSinkHelper1.Install (nodes.Get(i)));
		//cout<<" Node id is "<< fanout2Nodes[i].Get(j) ->GetId()<< endl;	 
               node_map[fanout2Nodes[i].Get(j) ->GetId()]=fanout2Nodes[i].Get(j) ;     
	}
	  	//sinkApps1.Add (packetSinkHelper1.Install (d2.GetRight (1)));
			
	}	

	
	container.Start (Seconds (0.0));
	container.Stop (Seconds (60.0));
}	//fget
	
		
		//attach contexts
		for(uint32_t i=0;i<sinkApps.GetN();i++)
	{
		Ptr <Application> app = sinkApps.Get (i);
		 Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);	    
		 pktSink->TraceConnectWithoutContext ("Rx", MakeCallback (&IntTrace)); 
	}	
		  
		  //AnimationInterface::SetConstantPosition (nodes.Get (0), 10, 10);
		  //AnimationInterface::SetConstantPosition (nodes.Get (1), 15, 10);		
		  //AnimationInterface::SetConstantPosition (nodes.Get (2), 20, 10);
		  //AnimationInterface::SetConstantPosition (nodes.Get (3), 25, 10); 
		
		anim = new AnimationInterface("worm.xml");	
		
		
		//anim->EnablePacketMetadata();		
		Simulator::Stop(Seconds(60.0));
	  	Simulator::Run ();

		//if (onoff)cout<<"Total bytes recived was" << pktSink->GetTotalRx() <<endl;
		//if (worm)  cout<<"Total infected worms were  " << total_infected <<endl;
	
}

