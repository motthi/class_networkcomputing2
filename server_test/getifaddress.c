#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

struct ifaddrs *ifa_list, *ifa;
char addrstr[256];

int main() {
	getifaddrs(&ifa_list);
	for(ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next) {
		printf("%s\n", ifa->ifa_name);
		memset(addrstr, 0, sizeof(addrstr));

		if(ifa->ifa_addr->sa_family == AF_INET) {
			inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, addrstr, sizeof(addrstr));
			printf("IPv4 : %s\n", addrstr);
		} else if(ifa->ifa_addr->sa_family == AF_INET6) {
			inet_ntop(AF_INET6, &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr, addrstr, sizeof(addrstr));
			printf("IPv6 : %s\n", addrstr);
		}
		printf("\n");
	}
	freeifaddrs(ifa_list);
	return 0;
}
