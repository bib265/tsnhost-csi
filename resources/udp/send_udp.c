/* Description
 * Socket for sending UDP packets with test data
 * Uses the txtime feature of tc-etf (https://man7.org/linux/man-pages/man8/tc-etf.8.html)
 * The time the messages were sent is saved using timestamps
 * SO_TIMESTAMPING is used for hardware and software timestamping
 * see: https://www.kernel.org/doc/Documentation/networking/timestamping.txt
 * between hardware and software timestamps can be selected
 * Socket periodically sends out messages
 * Start time and period can be set
 * Test data is sent in the payload of the message
 * This test data included:
 * IP address/hostname + port of the sender and destination,
 * pcp-priority, Streamid, count, tx timestamp
 * Contains options for manipulating the process
*/

/* Compile
 * compile with: $ gcc -pthread -o /usr/local/bin/send_udp send_udp.c -lz
 *
 * zlib1g-dev required
 * sudo apt-get install zlib1g-dev
*/

/* Flags
 * ############################################################################
 * mandatory:
 *
 * -i [name]
 * specify interface on which the packet will be send
 * If not set, an error is issued
 *
 * ############################################################################
 * set destination /(source)
 *
 *  -d [ip/host]
 * set destination IP address or host
 * unicast only (Multiple entries not allowed)
 * If not set then loopback is used
 *
 * -u [port]
 * set udp destination port
 *
 * -s [port]
 * set udp source port
 *
 * ############################################################################
 * 802.1Q Header
 *
 * -t priority
 * set pcp priority (skb priority which is mapped by the vlan interface)
 * A priority is defined for the packets in the IEEE802.1Q header in the pcp
 * To do this, an skb priority is set in the socket
 * The skb priority is mapped between 0 and 7 to the corresponding pcp
 * Values outside this range are mapped to pcp priority 0
 * To use the PCP priority, the VLAN must be configured with mapping from
 * skb->pcp in the socket, only the skb priority is set
 *
 * ############################################################################
 * Streamid
 *
 * -x
 * set streamid
 * is sent in the payload of the message
 *
 * ############################################################################
 * TXTime (used tc-etf https://man7.org/linux/man-pages/man8/tc-etf.8.html)
 *
 * -P [num]
 * period in nanoseconds
 * Time that is added to the basetime after each transmission
 * and determines the time of the next packet
 * If the value is too small, it can happen that period<processing time
 * If this happens, errors occur because the transmission time can
 * no longer be met (Packets are dropped by tc-etf)
 *
 * -b [tstamp]
 * txtime of 1st packet
 * Default: now + ~2seconds\n"
 * specification as uint64_t
 * If time is specified in the past, packets up to the
 * present time will be created and discarded (Leads to errors)
 *
 *  -E
 * enable error reporting
 * reads the socket error queue for SO_TXTIME errors
 * If an error occurs, returns the type of error and exits the program
 *
 * -w [num]
 * wake up time
 * Time before the scheduled send time that the process
 * should wake up and start processing the packets
 * delta from wake up to txtime in nanoseconds
 * If too low, errors will occur
 *
 * -n
 * deactivate TXTime feature
 * Period and basetime are still used, but the time is not set in socketoption
 *
 * -D
 * set deadline mode for SO_TXTIME
 * Packets are not sent at send-time,
 * but are sent when they arrive and discarded after send-time
 *
 * ############################################################################
 * Timestamping
 * source:
 * (https://www.kernel.org/doc/Documentation/networking/timestamping.txt)
 *
 * -H
 * Enable hardware time stamping (enabled by default)
 * uses SOF_TIMESTAMPING_TX_HARDWARE |
 * SOF_TIMESTAMPING_RX_HARDWARE |
 * SOF_TIMESTAMPING_RAW_HARDWARE
 * from SO_TIMESTAMPING
 * Cannot be used with -S
 *
 * -S
 * Enable software time stamping
 * uses SOF_TIMESTAMPING_TX_SOFTWARE |
 * SOF_TIMESTAMPING_RX_SOFTWARE |
 * SOF_TIMESTAMPING_SOFTWARE
 * from SO_TIMESTAMPING
 * Cannot be used with -H
 *
 * -I interface
 * Setting of the interface on which the driver options
 * for hardware time stamping should be set
 * If a vlan device is used, the corresponding interface
 * must be used on which the vlan interface is based
 *
 * ############################################################################
 * Commands for setting the process on the host
 *
 * -q
 * quit mode which suppresses the output via stdout, errors are still thrown
 *
 * -f
 * write received messages to file
 * Output is appended to file if file already exists
 * If file does not exist then file will be created
 * Directory of the file must exist
 *
 * -h
 * print usage
 *
 * -c [num]
 *  Select CPU on which the process is running
 *
 * -p [num]
 * set linux realtime priority
 * 1(min) to 99(max)
 *
 * ############################################################################
 * example
 * $ send_udp -i t1.42 -I t1 -d t2 -H
*/


/* copyrights
 * This program demonstrates transmission of UDP packets using the
 * system TAI timer.
 *
 * Copyright (C) 2017 linutronix GmbH
 *
 * Large portions taken from the linuxptp stack.
 * Copyright (C) 2011, 2012 Richard Cochran <richardcochran@gmail.com>
 *
 * Some portions taken from the sgd test program.
 * Copyright (C) 2015 linutronix GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* Payload adjustment
 * Explanation of how the payload is structured and how it can be changed.
 *
 * The payload is made up of 2 parts in the run_nanosleep method.
 *
 * The first part is the constant part of the payload,
 * this is formed once and remains the same in all messages.
 * search for the comment: build constant msg
 * To expand this, an example is given at the appropriate place.
 *
 * The second part of the variable part.
 * This is formed anew for each message and attached to the constant part
 * search for the comment: build variable msg
 * To expand this, an example is given at the appropriate place.
*/

#define _GNU_SOURCE /*for CPU_SET*/
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/errqueue.h>
#include <linux/ethtool.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <zlib.h>

#define ONE_SEC				1000000000ULL
#define DEFAULT_PERIOD		1000000000
#define DEFAULT_DELAY		1000000
#define DEFAULT_PRIORITY	0
#define DEFAULT_IPADDR		"127.0.0.1" //if no destination use loopback
#define DST_PORT			4242
#define SRC_PORT			42424
#define STREAM_ID			0
#define BUF 				2048
#define MARKER				'.' /* default padding symbol (currently not used and deactivated,
							   	to activate search for comment "set padding symbol" and uncomment the command */
//#define MAX_LENGTH_UDP		1454 // set maximal allowed with FRER
//#define MAX_LENGTH_UDP		1458 // set maximal allowed without FRER
#define MAX_LENGTH_ETHERNET	1522 // max length of Ethernet frame with one 802.1Q Header set (without R-tag)
#define MIN_LENGTH_ETHERNET	64 	// max length of Ethernet frame with one 802.1Q Header set (without R-tag)
#define UDP_IP_ETHERNET_HEADER_SIZE_BYTE 46 // header length of udp, ip and ethernet size with vlan header

#define PRIO 				80	// set default priority (-1 means not set)
#define MIN_CYCLE_TIME 	    1	// set minimal cycle time for -P

#ifndef SO_TXTIME
#define SO_TXTIME		61
#define SCM_TXTIME		SO_TXTIME
#endif

#ifndef SO_EE_ORIGIN_TXTIME
#define SO_EE_ORIGIN_TXTIME		6
#define SO_EE_CODE_TXTIME_INVALID_PARAM	1
#define SO_EE_CODE_TXTIME_MISSED	2
#endif

#define pr_err(s)	fprintf(stderr, s "\n")
#define pr_info(s)	fprintf(stdout, s "\n")

#define BUFFER_SIZE 1500

// TODO
#define COMPRESSION_ACTIVE -1 // 0 true

/* The API for SO_TXTIME is the below struct and enum, which will be
 * provided by uapi/linux/net_tstamp.h in the near future.
 */

static short sk_events = 0;
static short sk_revents = POLLERR;
static uint64_t base_time = 0;
static int running = 1, use_so_txtime = 1;
static int period_nsec = DEFAULT_PERIOD;
static int waketx_delay = DEFAULT_DELAY;
static int so_priority = DEFAULT_PRIORITY;
static int dst_port = DST_PORT;
static int src_port = SRC_PORT;
static int use_deadline_mode = 0;
static int receive_errors = SOF_TXTIME_REPORT_ERRORS;
static int exit_on_error = 0;
static int msg_length_byte_ethernet = -1;
static int msg_length_byte_udp = -1;
static int stream_id = STREAM_ID;
static int count = 0;
static int mode_set = 0;
static int hardware_mode = 1;
static int quiet_mode = 0;
static int set_timestamping = 0;
static int child_pid = -1;
static int parent_pid = -1;
static int file_set = 0;
static char *file;
static char *dst_ip = DEFAULT_IPADDR;
static char *viface = NULL;
static char *piface = NULL;
static char *progname;
static struct in_addr ucast_addr;
static struct hostent *hp;
static struct sock_txtime sk_txtime;
static unsigned char tx_buffer[BUFFER_SIZE];
static unsigned char con_msg[BUFFER_SIZE];

/* ############################################################################*/
/* signal handler*/
/* ############################################################################*/

// signal handler to end the program without error
static void sig_handler_norm(int sig) {
	if(sig == SIGTERM || sig == SIGINT){
		int pid=getpid();
		if( pid==child_pid){
			kill(parent_pid, SIGUSR2);
			exit(0);
		}
		if( pid==parent_pid && child_pid != -1){
			kill(child_pid, SIGUSR2);
			exit(0);
		}
		if( pid==parent_pid && child_pid == -1){
			exit(0);
		}
		printf("Signal could not be processed\n");
    }else{
		printf("signal not defined for processing\n");
	}
}
// signal handler to end the program with error
static void sig_handler_err(int sig) {
	if(sig == SIGUSR1){
		int pid=getpid();
		if( pid==child_pid){
			kill(parent_pid, SIGUSR2);
			exit(EXIT_FAILURE);
		}
		if( pid==parent_pid && child_pid != -1){
			kill(child_pid, SIGUSR2);
			exit(EXIT_FAILURE);
		}
		if( pid==parent_pid && child_pid == -1){
			exit(EXIT_FAILURE);
		}
		printf("Signal could not be processed\n");
    }else{
		printf("signal not defined for processing\n");
	}
}

// signal handler to kill only current process
static void sig_handler_kys(int sig) {
	if(sig == SIGUSR2){
		int pid=getpid();
		exit(0);
    }else{
		printf("signal not defined for processing\n");
	}
}

static void exit_with_thread_handling(int err) {
	int pid=getpid();
	/*if am the child send kill signal to parent*/
	if( pid==child_pid){
		kill(parent_pid, SIGUSR2);
	}
	/*if i am the parent and child exists, send signal to kill the brat*/
	if( pid==parent_pid && child_pid != -1){
		kill(child_pid, SIGUSR2);
	}
	/* exit with error code and msg*/
	if(err == 0){
		printf("Exit script on purpose\n");
		exit(EXIT_SUCCESS);
	} else {
		pr_err("Script stopped on error");
		exit(EXIT_FAILURE);
	}
}

/* ############################################################################*/
/* function to minimize string with zlib*/
/* ############################################################################*/
int compress_buffer(const unsigned char *input, size_t input_length, unsigned char *output, size_t *output_length) {
    /*compress string with zlib*/
	/*compress string*/
	uLongf compressed_size = BUFFER_SIZE;
    int result = compress(output, &compressed_size, input, input_length);
	/* check if compression is successful*/
    if (result != Z_OK) {
        printf("error by string compression: %d\n", result);
        return -1;
    }
    return 0; 
}

/* ############################################################################*/
/* math funktions*/
/* ############################################################################*/
static void normalize(struct timespec *ts)
{
	while (ts->tv_nsec > 999999999) {
		ts->tv_sec += 1;
		ts->tv_nsec -= ONE_SEC;
	}

	while (ts->tv_nsec < 0) {
		ts->tv_sec -= 1;
		ts->tv_nsec += ONE_SEC;
	}
}

/* ############################################################################*/
/* socket functions*/
/* ############################################################################*/
static int sk_interface_index(int fd, const char *name)
{
	struct ifreq ifreq;
	int err;

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name) - 1);
	err = ioctl(fd, SIOCGIFINDEX, &ifreq);
	if (err < 0) {
		pr_err("ioctl SIOCGIFINDEX failed: %m");
		return err;
	}
	return ifreq.ifr_ifindex;
}

static int open_socket(const char *name, struct in_addr dst_addr, short port, clockid_t clkid)
{
	struct sockaddr_in addr;
	int fd, index, on = 1;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		pr_err("socket failed: %m");
		goto no_socket;
	}
	index = sk_interface_index(fd, name);
	if (index < 0)
		goto no_option;

	if (setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &so_priority, sizeof(so_priority))) {
		pr_err("Couldn't set priority");
		goto no_option;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		pr_err("setsockopt SO_REUSEADDR failed: %m");
		goto no_option;
	}
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
		pr_err("bind failed: %m");
		goto no_option;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, name, strlen(name))) {
		pr_err("setsockopt SO_BINDTODEVICE failed: %m");
		goto no_option;
	}
	addr.sin_addr = dst_addr;

	sk_txtime.clockid = clkid;
	sk_txtime.flags = (use_deadline_mode | receive_errors);
	if (use_so_txtime && setsockopt(fd, SOL_SOCKET, SO_TXTIME, &sk_txtime, sizeof(sk_txtime))) {
		pr_err("setsockopt SO_TXTIME failed: %m");
		goto no_option;
	}

	/*#######################################################################*/
	/* activate hardware timestamping at the driver */
	if (set_timestamping == 1) {
		struct ifreq hwtstamp;
		struct hwtstamp_config hwconfig, hwconfig_requested;
		memset(&hwtstamp, 0, sizeof(hwtstamp));
		strncpy(hwtstamp.ifr_name, piface, sizeof(hwtstamp.ifr_name));
		hwtstamp.ifr_data = (void *)&hwconfig;
		memset(&hwconfig, 0, sizeof(hwconfig));
		hwconfig.tx_type = HWTSTAMP_TX_ON;
		hwconfig.rx_filter = HWTSTAMP_FILTER_ALL;
		hwconfig_requested = hwconfig;

		if (ioctl(fd, SIOCSHWTSTAMP, &hwtstamp) < 0) {
			if ((errno == EINVAL || errno == ENOTSUP) &&
				hwconfig_requested.tx_type == HWTSTAMP_TX_OFF &&
				hwconfig_requested.rx_filter == HWTSTAMP_FILTER_NONE)
				printf("SIOCSHWTSTAMP: disabling hardware time stamping not possible\n");
			else
				printf ("%s: SIOCSHWTSTAMP error: (%s)\n",
				progname, strerror(errno));
				goto no_option;;
		}
		if(quiet_mode == 0){
			printf("SIOCSHWTSTAMP: tx_type %d requested, got %d; rx_filter %d requested, got %d\n",
			hwconfig_requested.tx_type, hwconfig.tx_type,
			hwconfig_requested.rx_filter, hwconfig.rx_filter);
		}
	}

	/*#######################################################################*/
	/* set socketoption for timestamping */
	int so_timestamping_flags=0;
	int rc;
	
	if (hardware_mode == 1) {
		/* Timestamp options from ptp4l -H mode, get raw hardware timestamps from NIC with SO_TIMESTAMPING */
		so_timestamping_flags |= SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
		so_timestamping_flags |= SOF_TIMESTAMPING_TX_SOFTWARE; //activate Software for counter
	}else{
		/* Timestamp options from ptp4l -S mode, get software timestamps with SO_TIMESTAMPING */
		so_timestamping_flags |= SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
	}
	
	/* activate counter*/
	so_timestamping_flags |= SOF_TIMESTAMPING_OPT_ID | SOF_TIMESTAMPING_OPT_TSONLY;

	rc = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &so_timestamping_flags, sizeof(so_timestamping_flags));
	if (0 > rc){
    	printf ("%s: can not set socket option(%s)\n",
        progname, strerror(errno));
    	goto no_option;
  	}

  	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof on);
  	if (0 > rc) {
    	printf ("%s: can not set socket option(%s)\n",
        progname, strerror(errno));
    	goto no_option;
  	}

	/*#######################################################################*/
	/* check timestamping socket option*/
	int val,len;
	len = sizeof(val);
	if (getsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &val, &len) < 0) {
		printf("%s: %s\n", "getsockopt SO_TIMESTAMPING",
		       strerror(errno));
	} else {
		if(quiet_mode == 0){
			printf("timestamping is active with the following SO_TIMESTAMPING value: %d\n", val);
		}
		if (val != so_timestamping_flags)
			printf("Setting the timestamping failed with the following SO_TIMESTAMPING value: %d\n",
			       so_timestamping_flags);
	}

	/*#######################################################################*/
	
	return fd;
no_option:
	close(fd);
no_socket:
	return -1;
}

static int udp_open(const char *name, clockid_t clkid)
{
	int fd;

	fd = open_socket(name, ucast_addr, src_port, clkid);

	return fd;
}

static int udp_send(int fd, void *buf, int len, __u64 txtime)
{
	char control[CMSG_SPACE(sizeof(txtime))] = {};
	struct sockaddr_in sin;
	struct cmsghdr *cmsg;
	struct msghdr msg;
	struct iovec iov;
	ssize_t cnt;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr = ucast_addr;
	sin.sin_port = htons(dst_port);

	iov.iov_base = buf;
	iov.iov_len = len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &sin;
	msg.msg_namelen = sizeof(sin);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	/*
	 * specify the transmission time in the CMSG.
	 */
	if (use_so_txtime) {
		msg.msg_control = control;
		msg.msg_controllen = sizeof(control);

		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_TXTIME;
		cmsg->cmsg_len = CMSG_LEN(sizeof(__u64));
		*((__u64 *) CMSG_DATA(cmsg)) = txtime;
	}
	cnt = sendmsg(fd, &msg, 0);
	if (cnt < 0) {
		pr_err("sendmsg failed: %m");
		return cnt;
	}
	//pr_err("send");
	return cnt;
}

/* ############################################################################*/
/* read send timestamps msg*/
/* ############################################################################*/
static int read_cmsg(clockid_t clkid, int fd, struct pollfd p_fd, int period_nsec, struct timespec ts_sleep_true)
{
	/* init timespec for waiting if poll timeout is 0, see else in while loop */
	struct timespec ts_sleep_else;
	ts_sleep_else.tv_sec = 0;
	ts_sleep_else.tv_nsec=100000;
	if ((period_nsec / 2) < 100000){
		ts_sleep_else.tv_nsec = period_nsec / 2;
	}
	normalize(&ts_sleep_else);

	while (running) {
		int count=-1;
		int err;
		int txtime_err_detect=0;
		/* Check if errors are pending on the error queue.
		-1 for infinite timeout */
		err = poll(&p_fd, 1, 0);
		if (err > 0 && p_fd.revents & POLLERR) {
			char err_buffer[BUF];
			char msg_control[BUF];
			char txt_err[BUF];
			struct sock_extended_err *serr;
			struct cmsghdr *cmsg;
			__u64 tstamp = 0;
			int recv_length;
			long int cmsg_txtime=-1;

			struct iovec iov = {
					.iov_base = err_buffer,
					.iov_len = sizeof(err_buffer)
			};
			struct msghdr msg = {
					.msg_iov = &iov,
					.msg_iovlen = 1,
					.msg_control = msg_control,
					.msg_controllen = sizeof(msg_control)
			};
			
			recv_length = recvmsg(fd, &msg, MSG_ERRQUEUE);
			if (recv_length == -1) {
				pr_err("recvmsg failed");
					kill(parent_pid, SIGUSR1);
					exit(EXIT_FAILURE);
					continue;
			}
			cmsg = CMSG_FIRSTHDR(&msg);
			while (cmsg != NULL) {
				serr = (void *) CMSG_DATA(cmsg);
				if (serr->ee_origin == SO_EE_ORIGIN_TXTIME) {
					tstamp = ((__u64) serr->ee_data << 32) + serr->ee_info;

					switch(serr->ee_code) {
					case SO_EE_CODE_TXTIME_INVALID_PARAM:
						//fprintf(stderr, "packet with tstamp %llu dropped due to invalid params\n", tstamp);
						txtime_err_detect = 1;
						strcpy(txt_err,"INVALID_PARAM");
					case SO_EE_CODE_TXTIME_MISSED:
						//fprintf(stderr, "packet with tstamp %llu dropped due to missed deadline\n", tstamp);
						txtime_err_detect = 1;
						strcpy(txt_err,"MISSED_DEADLINE");
					default:
						txtime_err_detect = 1;
					}
				}
				/* get timestamp */
				if (serr->ee_origin == 0) {
					struct timespec *ts = (struct timespec *)CMSG_DATA(cmsg);
					if (hardware_mode == 1) {
						cmsg_txtime = (long)ts[2].tv_sec * ONE_SEC + (long)ts[2].tv_nsec;
					}else{
						cmsg_txtime = (long)ts[0].tv_sec * ONE_SEC + (long)ts[0].tv_nsec;
					}
				}

				/* read counter */
				if (serr->ee_origin == 4) {
					count=serr->ee_data;
				}

				/* jump too next cmsg*/
				cmsg = CMSG_NXTHDR(&msg, cmsg);
			}
			/* print timestamp via stdout*/
			if(quiet_mode == 0){
				if(txtime_err_detect == 0){
					printf("%s,COUNT=%d,SEND_TIME=%ld\n",con_msg,count,cmsg_txtime);
				} else {
					printf("%s,COUNT=%d,SEND_TIME=%lld,ERROR=%s\n",con_msg,count,tstamp,txt_err);
				}
			}

			if(file_set == 1){
				FILE * fp;
				fp = fopen(file, "a");
				if (fp == NULL) {
					exit (EXIT_FAILURE);
				}
				if(txtime_err_detect == 0){
					fprintf(fp,"%s,COUNT=%d,SEND_TIME=%ld\n",con_msg,count,cmsg_txtime);
				} else {
					fprintf(fp,"%s,COUNT=%d,SEND_TIME=%lld,ERROR=%s\n",con_msg,count,tstamp,txt_err);
				}
				fclose(fp);
			}

			if (txtime_err_detect == 1 && exit_on_error == 1) {
				kill(parent_pid, SIGUSR1);
				exit(EXIT_FAILURE);
			}
			/* sleep until the next packet */
			ts_sleep_true.tv_sec += period_nsec/ ONE_SEC;
			ts_sleep_true.tv_nsec += (period_nsec % ONE_SEC);
			normalize(&ts_sleep_true);
			clock_nanosleep(clkid, TIMER_ABSTIME, &ts_sleep_true, NULL);
		}else{
			/* if poll timeout is 0/bugged and the while loop becomes a while true loop
			or until the first package is send protect system from while true loop
			is only a protection not necessary for working */
			clock_nanosleep(clkid, 0, &ts_sleep_else, NULL);
		}
	}
	return 0;
}

/* ############################################################################*/
/* send and read periodic msg */
/* ############################################################################*/
static int run_nanosleep(clockid_t clkid, int fd)
{
	struct timespec ts;
	int cnt, err;
	int tmp_length=0;
	__u64 txtime;
	struct pollfd p_fd = { fd, sk_events, 0 };
	char buffer[BUFFER_SIZE];
	count=0; //reset count (precaution for further extensions, not necessary)

	/* init compress var*/
	size_t tx_buffer_length;
	size_t compressed_length;
	//memset(tx_buffer, MARKER, sizeof(tx_buffer));
	/* set padding symbol
	command to fill the whole buffer with a symbol,
	serves to change the padding
	*/

	/* build constant msg */
	
	strcpy(con_msg, "D=");
	sprintf(buffer, "%s", hp->h_name); //read hostname from hp
  	strcat(con_msg, buffer);
  	strcat(con_msg, ",DP=");
	sprintf(buffer, "%d", dst_port);
  	strcat(con_msg, buffer);
	strcat(con_msg, ",S=");
	sprintf(buffer, "%s", viface);
  	strcat(con_msg, buffer);
	strcat(con_msg, ",SP=");
	sprintf(buffer, "%d", src_port);
  	strcat(con_msg, buffer);
  	strcat(con_msg, ",P=");
	sprintf(buffer, "%d", so_priority);
  	strcat(con_msg, buffer);
  	strcat(con_msg, ",ID=");
	sprintf(buffer, "%d", stream_id);
  	strcat(con_msg, buffer);
	/* example how to extend message
	 * strcpy(con_msg, "VARIABLE_NAME="); 	Set the name of the variable by changing VARIABLE_NAME
	 *										command inserts the string into the buffer
	 * sprintf(buffer, "%d", variable); 	write variable into tmp buffer, replace variable
	 * 									  	(change %d depending on the data type of the variable)
	 * strcat(con_msg, buffer);				command inserts the variable as a string into the buffer
	*/

	/* get length of con_msg to prevent calculation in each iteration */
	int con_msg_length=strlen(con_msg);

	/* If no base-time was specified, start one to two seconds in the
	 * future.
	 */
	if (base_time == 0) {
		clock_gettime(clkid, &ts);
		ts.tv_sec += 1;
		ts.tv_nsec = ONE_SEC - waketx_delay;
	} else {
		ts.tv_sec = base_time / ONE_SEC;
		ts.tv_nsec = (base_time % ONE_SEC) - waketx_delay;
	}

	normalize(&ts);

	txtime = ts.tv_sec * ONE_SEC + ts.tv_nsec;
	txtime += waketx_delay;

	/* print txtime of the first package*/
	if(quiet_mode == 0){
		fprintf(stderr, "\ntxtime of 1st packet is: %llu\n", txtime);
	}

	/* fork new proccess for timestamp and error reciving */
	child_pid = fork();
	if (child_pid== 0){
		/* Check if errors are pending on the error queue. */
		child_pid = getpid();
		read_cmsg(clkid,fd,p_fd,period_nsec,ts);
	}
	
	while (running) {

		/* clear buffer and copy constant buffer into tx_buffer */
		memset(tx_buffer, 0, (int)strlen(tx_buffer)); // clear buffer

		/* build variable msg */
		sprintf(buffer, "COUNT=%d,TX_TIME=%lld,", count, txtime);
		strcpy(tx_buffer, buffer);

		strncat(tx_buffer, con_msg, con_msg_length);
		

		if (COMPRESSION_ACTIVE == 0){
			/* compress string in buffer*/
			tx_buffer_length = strlen((const char *)tx_buffer) + 1;
			int result = compress_buffer(tx_buffer, tx_buffer_length, con_msg, &compressed_length);
			if (result != Z_OK) {
				fprintf(stderr, "cannot compress string in cout %d\n",count);
			}
		}

		err = clock_nanosleep(clkid, TIMER_ABSTIME, &ts, NULL);
		switch (err) {
		case 0:
			/* call send_udp function with msg in tx_buffer and the message length
			if message length is -1 (not set) set string length of tx_buffer
			if message length is set, set message length directly (truncation of content possible) */
			if (msg_length_byte_udp == -1) {
				/* send msg with car msg based on length of msg or min ethernet size*/
				tmp_length = (int)strlen(tx_buffer);
				udp_send(fd, tx_buffer, tmp_length , txtime);
			} else {
				/* send msg with fix msg size*/
				cnt = udp_send(fd, tx_buffer, msg_length_byte_udp, txtime);
			}
			ts.tv_nsec += period_nsec;
			normalize(&ts);
			txtime += period_nsec;
			printf("%lld\n", txtime);
			count++; // increase counter
			break;
		case EINTR:
			continue;
		default:
			fprintf(stderr, "clock_nanosleep returned %d: %s",
				err, strerror(err));
			return err;
		}
	}
	return 0;
}

static int set_realtime(pthread_t thread, int priority, int cpu)
{
	cpu_set_t cpuset;
	struct sched_param sp;
	int err, policy;

	int min = sched_get_priority_min(SCHED_FIFO);
	int max = sched_get_priority_max(SCHED_FIFO);

	/* print min and max prio*/
	if(quiet_mode == 0){
		fprintf(stderr, "min %d max %d\n", min, max);
	}
	if (priority < 0) {
		return 0;
	}

	err = pthread_getschedparam(thread, &policy, &sp);
	if (err) {
		fprintf(stderr, "pthread_getschedparam: %s\n", strerror(err));
		return -1;
	}

	sp.sched_priority = priority;

	err = pthread_setschedparam(thread, SCHED_FIFO, &sp);
	if (err) {
		fprintf(stderr, "pthread_setschedparam: %s\n", strerror(err));
		return -1;
	}

	if (cpu < 0) {
		return 0;
	}
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (err) {
		fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(err));
		return -1;
	}

	return 0;
}

/* ############################################################################*/
/* help */
/* ############################################################################*/
static void usage(char *progname)
{
	fprintf(stderr,
		"\n"
		"usage: %s [options]\n"
		"\n"
		" -c [num]      run on CPU 'num'\n"
		" -w [num]      delta from wake up to txtime in nanoseconds (default %d)\n"
		" -i [name]     set interface 'name'\n"
		" -I [name]     set physical interface 'name', to configure the hardware timestamps on driver level\n"
		" -p [num]      run with RT priority 'num'\n"
		" -P [num]      period in nanoseconds (default %d)\n"
		" -n            do not use SO_TXTIME\n"
		" -t [num]      set SO_PRIORITY to 'num' (default %d)\n"
		" -D            set deadline mode for SO_TXTIME\n"
		" -E            enable error reporting on the socket error queue for SO_TXTIME, exit when error occurs\n"
		" -b [tstamp]   txtime of 1st packet as a 64bit [tstamp]. Default: now + ~2seconds\n"
		" -u [port]     set udp destination port (default %d)\n"
		" -s [port]     set udp source port (default %d)\n"
		" -d [ip/host]  set destination (ip or hostname)\n"
		" -l [num]      set ethernet frame size (no length set by default)\n"
		" -x [num]      set streamid (default %d)\n"
		" -H            Select the hardware time stamping\n"
		" -S            Select the software time stamping\n"
		" -q            suppress message output\n"
		" -f [path]     append output to a file\n"
		" -h            prints this message and exits\n"
		"\n",
		progname, DEFAULT_DELAY, DEFAULT_PERIOD, DEFAULT_PRIORITY, DST_PORT, SRC_PORT,STREAM_ID);
}

/* ############################################################################*/
/* main */
/* ############################################################################*/
int main(int argc, char *argv[])
{
	int c, cpu = -1, err, fd, priority = PRIO;
	parent_pid = getpid();
	clockid_t clkid = CLOCK_TAI;
	/* create signal handler */
	signal(SIGINT,sig_handler_norm);
	signal(SIGTERM,sig_handler_norm);
	signal(SIGUSR1,sig_handler_err);
	signal(SIGUSR2,sig_handler_kys);
	/* Process the command line arguments. */
	progname = strrchr(argv[0], '/');
	progname = progname ? 1 + progname : argv[0];
	while (EOF != (c = getopt(argc, argv, "c:w:i:I:p:P:nt:DEb:u:s:d:l:x:HSqf:h"))) {
		switch (c) {
		case 'c':
			cpu = atoi(optarg);
			break;
		case 'w':
			waketx_delay = atoi(optarg);
			break;
		case 'i':
			viface = optarg;
			break;
		case 'I':
			piface = optarg;
			set_timestamping = 1;
			break;
		case 'p':
			priority = atoi(optarg);
			break;
		case 'P':
			period_nsec = atoi(optarg);
			break;
		case 'n':
			use_so_txtime = 0;
			break;
		case 't':
			so_priority = atoi(optarg);
			break;
		case 'D':
			use_deadline_mode = SOF_TXTIME_DEADLINE_MODE;
			break;
		case 'E':
			exit_on_error = 1;
			break;
		case 'b':
			base_time = atoll(optarg);
			break;
		case 'u':
			dst_port = atoi(optarg);
			break;
		case 's':
			src_port = atoi(optarg);
			break;
		case 'd':
			dst_ip = optarg;
			break;
		case 'l':
			msg_length_byte_ethernet = atoi(optarg); //read in msg length
			break;
		case 'x':
			stream_id = atoi(optarg); //read in stream id
			break;
		case 'H':
			if (mode_set == 1) {
    			printf ("hardware/software modus can only be set once\n");
    			exit (EXIT_FAILURE);
  				}
			mode_set = 1;
			hardware_mode =1;
			break;
		case 'S':
			if (mode_set == 1) {
    			printf ("hardware/software modus can only be set once\n");
    			exit (EXIT_FAILURE);
  				}
			mode_set = 1;
			hardware_mode =0;
			break;
		case 'q':
			quiet_mode = 1;
			break;
		case 'f':
			file = optarg;
			file_set = 1;
			break;
		case 'h':
			usage(progname);
			return 0;
		case '?':
			usage(progname);
			return -1;
		}
	}

	/* convert hostname to ip */
	hp = gethostbyname(dst_ip);
	if (!hp) { // print error if error occur
		printf("Destination specified in -d was not resolved\n");
		exit(EXIT_FAILURE);
	}
	ucast_addr = *(struct in_addr *)(hp->h_addr); // set ip addr

	if (waketx_delay > 999999999 || waketx_delay < 0) {
		pr_err("Bad wake up to transmission delay.");
		usage(progname);
		return -1;
	}

	/* print error, if period is to low */
	if (period_nsec < MIN_CYCLE_TIME) {
		pr_err("Bad period.");
		usage(progname);
		return -1;
	}

	/* print error, if period is to large for int */
	if (period_nsec > 2147483647) {
		pr_err("Bad period.");
		usage(progname);
		return -1;
	}

	/* print error, if interface is not set */
	if (!viface) {
		pr_err("Need a network interface.");
		usage(progname);
		return -1;
	}

	/* check if message length is illegal*/
	if (msg_length_byte_ethernet != -1) {
		/* print error if input is out of msg length*/
		if (msg_length_byte_ethernet > MAX_LENGTH_ETHERNET || msg_length_byte_ethernet < MIN_LENGTH_ETHERNET) {
			pr_err("illegal message length specified with -l");
			usage(progname);
			exit(EXIT_FAILURE);
		}else{
			/* calculate udp payload size if size is in range of a single ethernet frame*/
			msg_length_byte_udp = msg_length_byte_ethernet - UDP_IP_ETHERNET_HEADER_SIZE_BYTE;
		}
	}

	/* print error, if linux realtime priority can not set */
	if (set_realtime(pthread_self(), priority, cpu)) {
		return -1;
	}

	/* open upd with the interface and clock specified*/
	fd = udp_open(viface, clkid);
	if (fd < 0) {
		return -1;
	}

	/* send messages periodically */
	err = run_nanosleep(clkid, fd);
	close(fd);
	/* exit with thread handling */
	exit_with_thread_handling(err);
}