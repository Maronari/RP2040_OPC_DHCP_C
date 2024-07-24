/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 *
 */
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include <stdint.h>

extern void set_system_time(uint32_t sec);

/* Prevent having to link sys_arch.c (we don't test the API layers in unit tests) */
#define NO_SYS                      0
#define MEM_ALIGNMENT               4
#define LWIP_RAW                    0
#define LWIP_NETCONN                1
#define LWIP_SOCKET                 1
#define LWIP_DHCP                   1
#define LWIP_DNS                    1
#define LWIP_ICMP                   1
#define LWIP_UDP                    1
#define LWIP_TCP                    1
#define MEM_SIZE                    2048

// disable ACD to avoid build errors
// http://lwip.100.n7.nabble.com/Build-issue-if-LWIP-DHCP-is-set-to-0-td33280.html
#define LWIP_DHCP_DOES_ACD_CHECK    0

#define ETH_PAD_SIZE                0
#define LWIP_IP_ACCEPT_UDP_PORT(p)  ((p) == PP_NTOHS(67))

#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_STATUS_CALLBACK  1

#define TCP_MSS                     (1500 /*mtu*/ - 20 /*iphdr*/ - 20 /*tcphhr*/)
#define TCP_SND_BUF                 (2 * TCP_MSS)

#define LWIP_HTTPD_CGI              0
#define LWIP_HTTPD_SSI              0
#define LWIP_HTTPD_SSI_INCLUDE_TAG  0

#define LWIP_RAND_WIZ()             ((u32_t)rand())

#define MEMP_NUM_SYS_TIMEOUT        (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 2)
#define LWIP_ARP                    2

#if 1
#define LWIP_DEBUG                  1
#define TCP_DEBUG                   LWIP_DBG_OFF
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define SNTP_DEBUG                  LWIP_DBG_OFF
#endif

#define LWIP_COMPAT_SOCKETS         0
#define SO_REUSE                    1
#define LWIP_TIMEVAL_PRIVATE        0

#define LWIP_PROVIDE_ERRNO
#define LWIP_IPV6                   1

#if !NO_SYS
#define TCPIP_MBOX_SIZE             20
#define TCPIP_THREAD_NAME           "TCPIP_Task"
#define TCPIP_THREAD_STACKSIZE      1024
#define LWIP_TIMEVAL_PRIVATE        0
#define DEFAULT_RAW_RECVMBOX_SIZE   8
#define MEMP_NUM_NETCONN            10
#define PBUF_POOL_SIZE              16
#define MEMP_NUM_PBUF               16
#define MEMP_NUM_TCP_SEG            16
#define MEMP_NUM_TCP_WND            6
#define DEFAULT_UDP_RECVMBOX_SIZE   6
#define DEFAULT_TCP_RECVMBOX_SIZE   6
#define DEFAULT_ACCEPTMBOX_SIZE     6
#endif

#define MEMP_NUM_SYS_TIMEOUT        LWIP_NUM_SYS_TIMEOUT_INTERNAL
#define SNTP_SERVER_DNS             1
#define SNTP_SERVER_ADDRESS         "ntp.msk-ix.ru"
#define SNTP_UPDATE_DELAY           600000
#define SNTP_SET_SYSTEM_TIME(s)     set_system_time((int32_t) s);

#endif /* __LWIPOPTS_H__ */
