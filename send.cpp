#include <iostream>
#include <cstring>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

using namespace std;

#define VERSION "1.0.0-alpha"
#define PACKET_SIZE 512

// Function to compute checksum.
unsigned short checksum(void* b,int len){
	unsigned short* buf=(unsigned short*)b;
	unsigned int sum=0;
	unsigned short result;

	for (sum=0;len>1;len-=2){
		sum+=*buf++;
	}
	if(len==1){
		sum+=*(unsigned char*)buf;
	}
	sum=(sum>>16)+(sum&0xFFFF);
	sum+=(sum>>16);
	result=~sum;
	return result;
}

int main(int argc,char** argv){
	int sock;
	struct sockaddr_in dest;
	char buffer[PACKET_SIZE];
	struct ip* iph=(struct ip*)buffer;
	struct udphdr* udph=(struct udphdr*)(buffer+sizeof(struct ip));
	char* data=buffer+sizeof(struct ip)+sizeof(struct udphdr);
	const char* target_ip="127.0.0.1";
	int target_port=12345;
	const char* message="Hello world!";
	for(int i=1;i<argc;i++){
		if((string)argv[i]=="-h"||(string)argv[i]=="--help"){
			cout<<"Usage: "<<argv[0]<<" [options].";
			cout<<	"\nOptions:"<<
				"\n-h|--help:\t\tDisplays the help menu."<<
				"\n--version:\t\tDisplays the version number."<<
				"\n-i <IP>:\t\tEnter the IP address."<<
				"\n-p <port>:\t\tEnter the port number."<<
				"\n-m <message>:\t\tEnter the message."<<endl;
			return 0;
		}
		else if((string)argv[i]=="--version"){
			cout<<VERSION<<endl;
			return 0;
		}
		else if((string)argv[i]=="-i"){
			i++;
			if(i>=argc){
				cout<<"Error: IP address not provided."<<endl;
				return 1;
			}
			target_ip=argv[i];
		}
		else if((string)argv[i]=="-p"){
			i++;
			if(i>=argc){
				cout<<"Error: Port number not provided."<<endl;
				return 1;
			}
			int port=atoi(argv[i]);
			if(port<0||port>65535){
				cout<<"Error: Invalid port number."<<endl;
				return 1;
			}
			target_port=port;
		}
		else if((string)argv[i]=="-m"){
			i++;
			if(i>=argc){
				cout<<"Error: Message not provided."<<endl;
				return 1;
			}
			message=argv[i];
		}
	}
	string packet_id_tmp=to_string(target_port);
	reverse(packet_id_tmp.begin(),packet_id_tmp.end());
	int packet_id=atoi(packet_id_tmp.c_str());

	// Create raw socket.
	sock=socket(AF_INET,SOCK_RAW,IPPROTO_RAW);
	if(sock<0){
		perror("socket");
		return 1;
	}

	// Destination address.
	dest.sin_family=AF_INET;
	dest.sin_port=htons(target_port);
	dest.sin_addr.s_addr=inet_addr(target_ip);

	// Fill the IP header.
	memset(buffer,0,PACKET_SIZE);
	iph->ip_hl=5;
	iph->ip_v=4;
	iph->ip_tos=0;
	iph->ip_len=sizeof(struct ip)+sizeof(struct udphdr)+strlen(message);
	iph->ip_id=htonl(packet_id); // ID of this packet
	iph->ip_off=0;
	iph->ip_ttl=255;
	iph->ip_p=IPPROTO_UDP;
	iph->ip_sum=0; // Set to 0 before calculating checksum
	iph->ip_src.s_addr=inet_addr("127.0.0.1"); // Source IP
	iph->ip_dst.s_addr=dest.sin_addr.s_addr;

	// Fill the UDP header.
	udph->uh_sport=htons(packet_id); // Source port
	udph->uh_dport=htons(target_port); // Destination port
	udph->uh_ulen=htons(sizeof(struct udphdr)+strlen(message));
	udph->uh_sum=0;

	// Copy the message.
	strcpy(data,message);

	// Calculate checksums.
	iph->ip_sum=checksum(buffer,iph->ip_len);

	// Send the packet.
	if(sendto(sock, buffer,iph->ip_len,0,(struct sockaddr*)&dest,sizeof(dest))<0){
		perror("sendto");
		return 1;
	}

	close(sock);
	cout<<"Message sent: "<<message<<endl;
	return 0;
}