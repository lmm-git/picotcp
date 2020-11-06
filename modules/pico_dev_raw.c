/*********************************************************************
   PicoTCP. Copyright (c) 2012-2017 Altran Intelligent Systems. Some rights reserved.
   See COPYING, LICENSE.GPLv2 and LICENSE.GPLv3 for usage.

   Authors: Daniele Lacamera
 *********************************************************************/


#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include "pico_device.h"
#include "pico_dev_raw.h"
#include "pico_stack.h"

#include <sys/poll.h>

struct pico_device_raw {
    struct pico_device dev;
    int fd;
    int if_idx;
};

#define RAW_MTU 1500

static int pico_raw_send(struct pico_device *dev, void *buf, int len)
{
    struct pico_device_raw *raw = (struct pico_device_raw *) dev;
    
    struct sockaddr_ll sock_addr;
    memset((void*)&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sll_family = PF_PACKET;
    sock_addr.sll_ifindex = raw->if_idx;
    sock_addr.sll_halen = ETH_ALEN;
    memcpy(sock_addr.sll_addr, buf, ETH_ALEN);

    
    //dbg("I AM HEREEEEE WITH ETH %d\n", raw->dev.eth->mac.addr[0]);
    //for (size_t i = 0; i < len; i++) {
    //	printf("%02X", ((uint8_t*)buf)[i]);
    //}
    //printf("\n");
    int retcode = (int)sendto(raw->fd, buf, (uint32_t)len, 0, &sock_addr, sizeof(sock_addr));
    //dbg("send code: %d\n", retcode);
    //perror("send");
    return retcode;
}

static int pico_raw_poll(struct pico_device *dev, int loop_score)
{
    struct pico_device_raw *raw = (struct pico_device_raw *) dev;
    struct pollfd pfd;
    unsigned char buf[RAW_MTU];
    int len;
    pfd.fd = raw->fd;
    pfd.events = POLLIN;
    do  {
        //if (poll(&pfd, 1, 0) <= 0)
        //    return loop_score;
   	//dbg("RECEIIIVVVIIINNNNGGG FOR YOU\n");


        len = (int)recv(raw->fd, buf, RAW_MTU, MSG_DONTWAIT);
        if (len > 0) {
            loop_score--;
            pico_stack_recv(dev, buf, (uint32_t)len);
        } else if (len <= 0) {
	    return loop_score;
	}
    } while(loop_score > 0);
    return 0;
}

/* Public interface: create/destroy. */

void pico_raw_destroy(struct pico_device *dev)
{
    struct pico_device_raw *raw = (struct pico_device_raw *) dev;
    if(raw->fd > 0)
        close(raw->fd);
}


static int raw_open()
{
    int raw_fd;
    if((raw_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        return(-1);
    }

    return raw_fd;
}



struct pico_device *pico_raw_create(char *name, char *ifname)
{
    //printf("CREATING RAW DEV\n");
    struct pico_device_raw *raw = PICO_ZALLOC(sizeof(struct pico_device_raw));

    if (!raw)
        return NULL;

    uint8_t the_mac[6] = {0x9A, 0x5C, 0xA4, 0x7B, 0xB6, 0xA9};
    if( 0 != pico_device_init((struct pico_device *)raw, name, the_mac)) {
        dbg("RAW init failed.\n");
        pico_raw_destroy((struct pico_device *)raw);
        return NULL;
    }

    raw->dev.overhead = 0;
    raw->fd = raw_open(name);
    if (raw->fd < 0) {
        dbg("RAW creation failed.\n");
        pico_raw_destroy((struct pico_device *)raw);
        return NULL;
    }

    int name_len = strlen(ifname);
    if (setsockopt(raw->fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, name_len) < 0) {
	perror("SO_BINDTODEVICE");
	close(raw->fd);
	return NULL;
    }


    struct ifreq ifr;

    memset (&ifr, 0, sizeof (ifr));
    strncpy (ifr.ifr_name, ifname, name_len);

    if (ioctl (raw->fd, SIOCGIFINDEX, &ifr) < 0)
        perror ("SIOCGIFINDEX");
    raw->if_idx = ifr.ifr_ifindex;

    raw->dev.send = pico_raw_send;
    raw->dev.poll = pico_raw_poll;
    raw->dev.destroy = pico_raw_destroy;
    dbg("Device %s created.\n", raw->dev.name);
    return (struct pico_device *)raw;
}

