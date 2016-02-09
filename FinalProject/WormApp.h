#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/onoff-application.h"


#define disp 0

//THINGS To be done:
/*At start application, start only if node has been affected
Cancel events commented out in start application?
*/

/*
I think are complete
StartApplication
StopApplication
StartSending
StopSending
ScheduleStartEvent
ScheduletStopEvent
*/

/*CHANGES
1. sending to multiple random addresses
2. 
*/


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("ns3-worm");


//global functions
string findIP();			

class WormApp: public Application
{	
	public:
  	virtual void DoDispose (void);
	
	
	
	virtual void StartApplication (void);    // Called at time specified by Start
 	virtual void StopApplication (void);     // Called at time specified by Stop
	void CancelEvents ();
	void StartSending (uint32_t);
	void StopSending ();
  	void SendPacket (uint32_t);
		  
	void setInfected();
	void infectnodes();
	static TypeId GetTypeId(void);	
	//string findIP();
	 //WORM SPECIFIC CONNECTIONS
	 uint32_t m_nconnections;
	 uint32_t m_port;
	 
	  std::vector< ns3::Ptr<ns3::Socket> > m_socket;       //!< Associated sockets, send to m_nconnections
	  Address         m_peer;         //!< Peer address
	  bool            m_connected;    //!< True if connected
	  Ptr<RandomVariableStream>  m_onTime;       //!< rng for On Time
	  Ptr<RandomVariableStream>  m_offTime;      //!< rng for Off Time
	  DataRate        m_cbrRate;      //!< Rate that data is generated
	  DataRate        m_cbrRateFailSafe;      //!< Rate that data is generated (check copy)
	  uint32_t        m_pktSize;      //!< Size of packets
	  uint32_t        m_residualBits; //!< Number of generated, but not sent, bits
	  bool m_infected;  
	  Time            m_lastStartTime; //!< Time last packet sent
	  uint32_t        m_maxBytes;     //!< Limit total number of bytes sent
	  Ptr<ns3::Socket> m_sinkSocket; 
   	 uint32_t        m_totBytes;     //!< Total bytes sent so far
	  EventId         m_startStopEvent;     //!< Event id for next start or stop event
	  EventId         m_sendEvent;    //!< Event id of pending "send packet" event
	  TypeId          m_typeId;          //!< Type of the socket used
	 


 	  //TracedCallback<Ptr<const Packet> > m_txTrace;
	  string m_protocol;
	  
	 
         void ScheduleNextTx (uint32_t);
   	 void ScheduleStartEvent (uint32_t i);
   	 void ScheduleStopEvent ();
   	 void ConnectionSucceeded (Ptr<Socket> socket);
   	 void ConnectionFailed (Ptr<Socket> socket);	
	 void set_number_connections(int);
         void set_pkt_size(int);
	 void set_attributes();
	void set_max_packets(uint32_t bytesmax)	;
	void Listen(ns3::Ptr<ns3::Socket> socket);
	public:
	WormApp();
	~WormApp();
	void initialize(string, uint32_t infectionport);
	//this  are the public variables	
	/*void SetMaxBytes (uint32_t maxBytes);
	int64_t AssignStreams (int64_t stream);
	void DoDispose (void);	
	*/
	

}; 


ns3::TypeId WormApp::GetTypeId(void)
{
	static ns3::TypeId tid = ns3::TypeId ("ns3::WormApp")
	.SetParent<ns3::Application> ()
	.AddConstructor<WormApp> ()
	.AddAttribute ("DataRate", "The data rate in on state.",
	ns3::DataRateValue (ns3::DataRate ("500kb/s")),
	ns3::MakeDataRateAccessor (&WormApp::m_cbrRate),
	ns3::MakeDataRateChecker ())
	.AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
	ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
	ns3::MakePointerAccessor (&WormApp::m_onTime),
	ns3::MakePointerChecker <ns3::RandomVariableStream>())
	.AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
	ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
	ns3::MakePointerAccessor (&WormApp::m_offTime),
	ns3::MakePointerChecker <ns3::RandomVariableStream>())
	.AddAttribute ("MaxBytes","The total number of bytes to send. Once these bytes are sent, no packet is sent again, even in on state. The value zero means that there is no limit.",
	ns3::UintegerValue (50000),
	ns3::MakeUintegerAccessor (&WormApp::m_maxBytes),
	ns3::MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("Protocol", "The type of protocol to use.",
	ns3::TypeIdValue (TcpSocketFactory::GetTypeId ()),
	ns3::MakeTypeIdAccessor (&WormApp::m_typeId),
	ns3::MakeTypeIdChecker ());
	//.AddTraceSource ("Tx", "A new packet is created and is sent",
	//ns3::MakeTraceSourceAccessor (&WormApp::m_txTrace));
	return tid;
}


void WormApp::CancelEvents ()
{
	NS_LOG_FUNCTION (this);
	if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
	{ // Cancel the pending send packet event
	// Calculate residual bits since last packet sent
	ns3::Time delta (ns3::Simulator::Now () - m_lastStartTime);
	ns3::int64x64_t bits = delta.To (ns3::Time::S) * m_cbrRate.GetBitRate ();
	m_residualBits += bits.GetHigh ();
	}
	m_cbrRateFailSafe = m_cbrRate;
	ns3::Simulator::Cancel (m_sendEvent);
	ns3::Simulator::Cancel (m_startStopEvent);
}


WormApp::WormApp ()
  : m_connected (false),
    m_residualBits (0),
    m_infected(0),
    m_lastStartTime (Seconds (0)),
    //m_sinkSocket(0),	
    m_totBytes (0)

{
  NS_LOG_FUNCTION (this);
}

WormApp::~WormApp()
{
  NS_LOG_FUNCTION (this);
}

void WormApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

for(uint32_t i=0;i<m_nconnections;i++) 
	 m_socket[i] = 0;
  // chain up
  Application::DoDispose ();
}

void WormApp::StartSending (uint32_t index)
{
	//cout <<GetNode() -> m_maxBytes << GetNode()->m_infected <<" I am node " << endl;
	//std::cerr << "[" << m_name << "] Starting sending" << std::endl;
	NS_LOG_FUNCTION (this);
	if(disp) cout <<" I have scheduled next packet "<<endl;
	m_lastStartTime = ns3::Simulator::Now ();
	ScheduleNextTx (index); // Schedule the send packet event
	//ScheduleStopEvent ();
}

void WormApp::StopSending ()
{
	NS_LOG_FUNCTION (this);
	CancelEvents ();
	//ScheduleStartEvent ();
}



void WormApp::ScheduleStartEvent (uint32_t index)
{ // Schedules the event to start sending data (switch to the "On" state)
	NS_LOG_FUNCTION (this);
	if(disp)  cout<<" I am at schedule start event "<<endl;
	ns3::Time offInterval = ns3::Seconds (m_offTime->GetValue ());
	NS_LOG_LOGIC ("start at " << offInterval);
	ns3::Simulator::ScheduleNow(&WormApp::StartSending, this, index);
}

void WormApp::ScheduleStopEvent ()
{ // Schedules the event to stop sending data (switch to "Off" state)
	NS_LOG_FUNCTION (this);
	ns3::Time onInterval = ns3::Seconds (m_onTime->GetValue ());
	NS_LOG_LOGIC ("stop at " << onInterval);
	ns3::Simulator::Schedule (onInterval, &WormApp::StopSending, this);
}

void WormApp::Listen(ns3::Ptr<ns3::Socket> socket)
{	
	cout<<" I have listeneded "<< endl;
	while (socket->GetRxAvailable () > 0)
	{
	
		m_infected = true;
		cout<<" One more infected node "<< endl;
		//start infecting nodes
		//and if count already greate than node count, stop everything	

	}
	
}


void WormApp::infectnodes()
{
	for(uint32_t i=0;i<m_nconnections;i++)
		{
			
			      m_socket[i] = Socket::CreateSocket (GetNode (), m_typeId);
			     	
			     //cout<<m_peer<<endl;
			 m_socket[i]->Bind ();     
			 string s= findIP();
		         if (disp) cout<<" Addresses are  " << s<< endl;
			 //m_socket[i]->Connect (InetSocketAddress(s.c_str(),m_port));
			    m_socket[i]->Connect (InetSocketAddress(s.c_str(),m_port));  
			     
				
			      m_socket[i]->SetAllowBroadcast (true);
			      m_socket[i]->ShutdownRecv ();					//doubtful

			      m_socket[i]->SetConnectCallback (
				MakeCallback (&WormApp::ConnectionSucceeded, this),
				MakeCallback (&WormApp::ConnectionFailed, this));
				ScheduleStartEvent (i);		
		}

			
		 			//set up scheduling nodes for the multiple sockets
		 //m_totBytes =0 ;
		  Simulator::Schedule(ns3::Seconds(2.0),&WormApp::infectnodes, this);
		
		if (disp) cout<<" next generation "<< endl;
		  m_cbrRateFailSafe = m_cbrRate;

		  // Insure no pending event
		  CancelEvents ();				//
		  // If we are not yet connected, there is nothing to do here
		  // The ConnectionComplete upcall will start timers at that time
		  //if (!m_connected) cout<<" bad :( "<< endl; 
		 if(disp)  cout<<m_nconnections<<endl;
		  return;
		 


}

// Application Methods
void WormApp::StartApplication () // Called at time specified by Start
{
	//make the sink socket here
	
	
  if(disp)  cout<<" infection status is " << m_infected <<"  " <<endl;	
  NS_LOG_FUNCTION (this);		//start sending if affected
	if(m_infected)
	{	
		infectnodes();	
	}
		//repeat this event after 2 seconds
}



void WormApp::ScheduleNextTx (uint32_t index)
{
	
 	 if(disp)  cout<<" I am going to send packet "<< endl;
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      Time nextTime (Seconds (bits /
                              static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      if(disp)  cout<<" Time for next packet is " << nextTime<< endl;
      m_sendEvent = Simulator::Schedule (nextTime, &WormApp::SendPacket, this, index);
}

void WormApp::SendPacket(uint32_t index)
{	//cout<<"total bytes so far " << m_totBytes<< " and max bytes is " << m_maxBytes<< endl;
	//TO FILL
		//if(m_totBytes< m_maxBytes)
	{		

			Ptr<Packet> packet = Create<Packet> (m_pktSize);
		  	//m_txTrace (packet);
	
			m_socket[index]->Send(packet);
	
			//m_socket[index]->SendTo(packet, 0, ns3::InetSocketAddress("10.1..2", m_port));
		  	m_totBytes += m_pktSize;
			m_lastStartTime = ns3::Simulator::Now();
			m_residualBits = 0;
			if(disp)  cout<<" I ahve sent packet , total packets sent till now "<< m_totBytes/ m_pktSize << endl;
			ScheduleNextTx (index);

			//cout<<" bytes sent to "<< m_totBytes/ m_pktSize <<endl;
	
	}
	//else
	//StopApplication ();
}




void WormApp::StopApplication () // Called at time specified by Stop
{
  	NS_LOG_FUNCTION (this);

 	 CancelEvents ();
	 //if (m_sinkSocket != 0) 
	//{
	//m_sinkSocket->Close();
	//}						//close all the associated sockets
	  for(uint32_t i=0;i<m_nconnections;i++)
	{
		  if(m_socket[i] != 0)
		    {
		      m_socket[i]->Close ();
		     
		    }
		  else
		    {
		      NS_LOG_WARN ("OnOffApplication found null socket to close in StopApplication");
		    }
	}
}



string findIP()
{	
	//static int a=6110;	
	Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
	static int svalue=0; 	
	
	U->SetAttribute ("Stream", IntegerValue (svalue++));
  	U->SetAttribute ("Min", DoubleValue (0.0));
  	U->SetAttribute ("Max", DoubleValue (0.7));  
  
	Ptr<UniformRandomVariable> V = CreateObject<UniformRandomVariable> ();
	//static int svalue=0; 		
	V->SetAttribute ("Stream", IntegerValue (svalue++));
  	V->SetAttribute ("Min", DoubleValue (0.0));
  	V->SetAttribute ("Max", DoubleValue (4.0)); 
 
	int x= 10 * U->GetValue();
	int y= 10 * V->GetValue();
	char xchar[10];
	char ychar[10];
	sprintf(xchar, "%d",x);
	sprintf(ychar, "%d",y);	
	//cout<< "192."+ string(xchar)+"."+string(ychar)+"."+"0"<<endl;
	return "192."+ string(xchar)+"."+string(ychar)+"."+"2";
}

void WormApp::initialize (string typeofsocket, uint32_t infected_port) // Called at time specified by Stop
{	SeedManager::SetSeed (11223344);
	m_port = infected_port;
	m_protocol= typeofsocket;
	m_typeId=  ns3::TypeId::LookupByName(m_protocol);

	//sink socket for receiving
	//m_sinkSocket = ns3::Socket::CreateSocket (GetNode (), m_typeId); 
	//ns3::InetSocketAddress local = ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(),m_port);
	//m_sinkSocket->Bind(local);
	//m_sinkSocket->Listen();
	//m_sinkSocket->SetRecvCallback (MakeCallback (&WormApp::Listen, this));
	// Initialize number of simultaneous connections
	m_socket = std::vector< ns3::Ptr<ns3::Socket> > (m_nconnections);
	
	m_peer = ns3::InetSocketAddress("10.1.3.2", m_port);
	
	//initialize the listening socket
        //m_sinkSocket = ns3::Socket::CreateSocket (GetNode (), m_typeId); 
	//ns3::InetSocketAddress local = ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(),m_port);
	//m_sinkSocket->Bind(local);
	//int a = m_sinkSocket->Listen();	
	//m_sinkSocket->SetRecvCallback (MakeCallback (&WormApp::Listen, this));

}
void WormApp::ConnectionSucceeded (Ptr<Socket> socket)
{	NS_LOG_FUNCTION (this << socket);
	m_connected = true;
	if (disp)cout<<" Connection established!" <<endl;
}
void WormApp::ConnectionFailed (Ptr<Socket> socket)
{	
	if (disp) cout<<" Could not open socket" <<endl;
	NS_LOG_FUNCTION (this << socket);
}

void WormApp::setInfected()
{
	m_infected= true;

}

void WormApp:: set_number_connections(int a)
{
	m_nconnections = a;
}

void WormApp::set_pkt_size(int a)
{
	m_pktSize = a;
}

void WormApp::set_attributes()
{
	/*
	m_onTime= ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]");
	m_offTime= ns3::StringValue ("ns3::ConstantRandomVariable[Constant=0.5]");
	m_cbrRate= ns3::DataRateValue (ns3::DataRate ("500kb/s"));
	m_maxBytes= ns3::UintegerValue (50000);
	*/
}
void WormApp:: set_max_packets(uint32_t packets)
{
	m_maxBytes = packets * m_pktSize; 
}

