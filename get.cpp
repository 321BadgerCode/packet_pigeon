#include <iostream>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

#define VERSION "1.0.0-alpha"
#define SNAP_LEN 1518

// Callback function for packet handling.
void packet_handler(u_char* args,const struct pcap_pkthdr* header,const u_char* packet){
	struct ip* iph=(struct ip*)(packet+14); // Skip Ethernet header
	struct udphdr* udph=(struct udphdr*)(packet+14+(iph->ip_hl*4)); // Skip IP header

	if(ntohs(udph->uh_dport)==12345){ // Check for the target port
		const char* data=(const char*)(packet+14+(iph->ip_hl*4)+sizeof(struct udphdr));
		cout<<"Received message: "<<data<<endl;
	}
}

int main(int argc,char** argv){
	pcap_t*handle;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct bpf_program fp;
	char filter_exp[]="udp dst port 12345";
	for(int i=1;i<argc;i++){
		if((string)argv[i]=="-h"||(string)argv[i]=="--help"){
			cout<<"Usage: "<<argv[0]<<" [options].";
			cout<<	"\nOptions:"<<
				"\n-h|--help:\t\tDisplays the help menu."<<
				"\n--version:\t\tDisplays the version number."<<
				"\n-p <port>:\t\tEnter the port number."<<endl;
			return 0;
		}
		else if((string)argv[i]=="--version"){
			cout<<VERSION<<endl;
			return 0;
		}
		else if((string)argv[i]=="-p"){
			i++;
			if(i>=argc){
				cerr<<"Error: Port number not provided."<<endl;
				return 1;
			}
			else{
				string port=argv[i];
				memset(filter_exp,0,sizeof(filter_exp));
				strncpy(filter_exp,"udp dst port ",13);
				strncat(filter_exp,port.c_str(),port.length());
			}
		}
	}
	bpf_u_int32 net;

	// Open live pcap session on NIC with name eth0.
	handle=pcap_open_live("lo",SNAP_LEN,1,1000,errbuf);
	if(handle==nullptr){
		cerr<<"Couldn't open device: "<<errbuf<<endl;
		return 1;
	}

	// Compile and apply the filter.
	if(pcap_compile(handle,&fp,filter_exp,0,net)==-1){
		cerr<<"Couldn't parse filter "<<filter_exp<<": "<<pcap_geterr(handle)<<endl;
		return 1;
	}
	if(pcap_setfilter(handle,&fp)==-1){
		cerr<<"Couldn't install filter "<<filter_exp<<": "<<pcap_geterr(handle)<<endl;
		return 1;
	}

	// Capture packets.
	pcap_loop(handle,0,packet_handler,nullptr);

	// Close the handle.
	pcap_close(handle);
	return 0;
}
