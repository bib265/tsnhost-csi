/* Description
 * Socket for receiving UDP packets with software and hardware time stamps
 * socket for IPv4 UDP packets is opened to receive the packets
 * Used to receive the packets from send_udp and output them to standard output or a file
 * SO_TIMESTAMPING is used for hardware and software timestamping
 * see: https://www.kernel.org/doc/Documentation/networking/timestamping.txt
 * between hardware and software timestamps can be selected
 * The payload of the received messages is extended by the timestamp of receipt
*/

/* Compile
 * compile with $ gcc -pthread -o /usr/local/bin/rec_udp rec_udp.c -lz
 *
 * zlib1g-dev required
 * sudo apt-get install zlib1g-dev
*/

/* Flags 
 * There are various setting options, as listed below:
 * 
 * -i interface
 * Set the interface on which the packets should be received
 * Must be set
 * 
 * -I interface
 * Setting of the interface on which the driver options 
 * for hardware time stamping should be set
 * If a vlan device is used, the corresponding interface 
 * must be used on which the vlan interface is based
 * 
 * -p port
 * Set the UDP port
 * Standard port is 4242
 * 
 * -H
 * Enable hardware time stamping (enabled by default)
 * uses SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE
 * from SO_TIMESTAMPING
 * Cannot be used with -S
 * 
 * -S
 * Enable software time stamping
 * uses SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
 * from SO_TIMESTAMPING
 * Cannot be used with -H
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
 * example command line call
 * $ rec_udp -i t2.42 -I t2 -H
*/

/*#######################################################################*/
/*includes*/
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/sockios.h> // import code for SO_TIMESTAMPING
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <net/if.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <zlib.h>

/*#######################################################################*/
/* define constant variables as default values */
/* default values are changeable */
#define ONE_SEC         1000000000ULL
#define SRC_PORT        4242
#define BUFFER_SIZE     1500
// TODO
#define COMPRESSION_ACTIVE -1 // 0 true
/*#######################################################################*/
/* define code wide variable */
static char *progname;

/*#######################################################################*/
/* decompress input */
/*#######################################################################*/
int decompress_buffer(const unsigned char *input, size_t input_length, unsigned char *output, size_t *output_length) {
    uLongf decompressed_size = BUFFER_SIZE;

    // uncompress buffer
    int result = uncompress(output, &decompressed_size, input, input_length);
    if (result == Z_OK) {
        *output_length = decompressed_size;
        return 0;  
    } else if (result == Z_BUF_ERROR) {
        printf("Buffer size too small for decompressing\n");
        return -1;
    } else if (result == Z_DATA_ERROR) {
        printf("Error decompressing message: Z_DATA_ERROR (possibly corrupted or invalid data)\n");
        return -1;
    } else {
        printf("Error decompressing message: %d\n", result);
        return -1;
    }
}

/*#######################################################################*/
/*function to print usage of the program with -h flag*/
static void usage(char *progname)
{
    fprintf(stderr,
        "\n"
        "usage: %s [options]\n"
        "\n"
        " -i [iface]    set interface\n"
        " -I [name]     set physical interface 'name', to configure the hardware timestamps on driver level\n"
        " -p [num]      set UDP receiving port\n"
        " -H            activate hardware timestamping (default)\n"
        " -S            activate software timestamping\n"
        " -q            suppress message output\n"
        " -f [path]     append output to a file\n"
        " -h            prints this message and exits\n"
        "\n",
        progname);
}

// signal handler to end the program without error
static void sig_handler_norm(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        exit(0);
    } else {
        printf("Signal not defined for processing\n");
    }
}

// signal handler to end the program with error
static void sig_handler_err(int sig) {
    if (sig == SIGUSR1) {    
        exit(EXIT_FAILURE);
    } else {
        printf("Signal not defined for processing\n");
    }
}

int main (int argc, char **argv) {
    /*#######################################################################*/
    /*initialize variables*/
    
    int c, port = SRC_PORT;
    char *viface = NULL;
    char *piface = NULL;
    char *file = NULL;
    FILE *fp = NULL;

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int s, rc, n;
    long int txtime;
    int mode_set = 0;
    int hardware_mode = 1;
    int quiet_mode = 0;
    int file_set = 0;
    int set_timestamping = 0;
    /*#######################################################################*/
    /* init traps for terminating*/

    signal(SIGINT, sig_handler_norm);
    signal(SIGTERM, sig_handler_norm);
    signal(SIGUSR1, sig_handler_err);

    /*#######################################################################*/
    /* Read Input*/
    /* Process the command line arguments. */
    progname = strrchr(argv[0], '/');
    progname = progname ? 1 + progname : argv[0];
    while (EOF != (c = getopt(argc, argv, "i:I:p:HSqf:h"))) {
        switch (c) {
        case 'i':
            viface = optarg;
            break;
        case 'I':
            piface = optarg;
            set_timestamping = 1;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'H':
            if (mode_set == 1) {
                printf("Hardware/software mode can only be set once\n");
                exit(EXIT_FAILURE);
            }
            mode_set = 1;
            hardware_mode = 1;
            break;
        case 'S':
            if (mode_set == 1) {
                printf("Hardware/software mode can only be set once\n");
                exit(EXIT_FAILURE);
            }
            mode_set = 1;
            hardware_mode = 0;
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

    /*#######################################################################*/
    /* build data structure for receipt of message and timestamps */

    /* For the message to receive */
    char buf[BUFFER_SIZE];
    char decompressed_data[BUFFER_SIZE];
    size_t decompressed_length;

    /* initialize structure for scatter/gather I/O. */
    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    /* setup control message */
    char ctrl[BUFFER_SIZE];
    struct msghdr msg;
    struct cmsghdr *cmsg = (struct cmsghdr *)ctrl;

    msg.msg_control = (char *)ctrl;
    msg.msg_controllen = sizeof(ctrl);
    msg.msg_name = &servAddr;
    msg.msg_namelen = sizeof(servAddr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    /*#######################################################################*/
    /* create socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        printf("%s: Cannot open socket(%s)\n", progname, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*#######################################################################*/
    /* set device */
    struct ifreq device;
    memset(&device, 0, sizeof(device));
    if (viface) {
        strncpy(device.ifr_name, viface, sizeof(device.ifr_name));
        if (ioctl(s, SIOCGIFADDR, &device) < 0) {
            printf("%s: Cannot get IP address: (%s)\n", progname, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /*#######################################################################*/
    /* set socket option for timestamping */
    int so_timestamping_flags = 0;
    if (hardware_mode == 1) {
        /* Timestamp options for hardware timestamps */
        so_timestamping_flags |= SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
    } else {
        /* Timestamp options for software timestamps */
        so_timestamping_flags |= SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
    }

    rc = setsockopt(s, SOL_SOCKET, SO_TIMESTAMPING, &so_timestamping_flags, sizeof(so_timestamping_flags));
    if (rc < 0) {
        printf("%s: Cannot set socket option(%s)\n", progname, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int on = 1;
    rc = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    if (rc < 0) {
        printf("%s: Cannot set socket option(%s)\n", progname, strerror(errno));
        exit(EXIT_FAILURE);
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

        if (ioctl(s, SIOCSHWTSTAMP, &hwtstamp) < 0) {
            if ((errno == EINVAL || errno == ENOTSUP) &&
                hwconfig_requested.tx_type == HWTSTAMP_TX_OFF &&
                hwconfig_requested.rx_filter == HWTSTAMP_FILTER_NONE)
                printf("SIOCSHWTSTAMP: Disabling hardware time stamping not possible\n");
            else
                printf("%s: SIOCSHWTSTAMP error: (%s)\n", progname, strerror(errno));
                exit(EXIT_FAILURE);
        }
        if (quiet_mode == 0) {
            printf("SIOCSHWTSTAMP: tx_type %d requested, got %d; rx_filter %d requested, got %d\n",
                hwconfig_requested.tx_type, hwconfig.tx_type,
                hwconfig_requested.rx_filter, hwconfig.rx_filter);
        }
    }

    /*#######################################################################*/
    /* check socket option */
    int val, len;
    len = sizeof(val);
    if (getsockopt(s, SOL_SOCKET, SO_TIMESTAMPING, &val, &len) < 0) {
        printf("%s: %s\n", "getsockopt SO_TIMESTAMPING", strerror(errno));
    } else {
        if (quiet_mode == 0) {
            printf("Timestamping is active with the following SO_TIMESTAMPING value: %d\n", val);
        }
        if (val != so_timestamping_flags)
            printf("Setting the timestamping failed with the following SO_TIMESTAMPING value: %d\n", so_timestamping_flags);
    }

    /*#######################################################################*/
    /* bind port */
    int y = 1;
    servAddr.sin_port = htons(port);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
    rc = bind(s, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if (rc < 0) {
        printf("%s: Cannot bind port %d (%s)\n", progname, port, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*#######################################################################*/
    /* endless loop for receiving */
    while (1) {
        /* receive message */
        n = recvmsg(s, &msg, 0);
        if (n < 0) {
            printf("%s: Can't receive data... (%s)\n", progname, strerror(errno));
            continue;
        }

		if (COMPRESSION_ACTIVE == 0){
			/* decompress buffer */
        int ok = decompress_buffer((unsigned char *)buf, n, decompressed_data, &decompressed_length) == 0;
		}

        /* get timestamps from control message */
        struct timespec *ts = (struct timespec *)CMSG_DATA(cmsg);

        /* convert timestamp from control message to a single long value */
        if (hardware_mode == 1) {
            txtime = (long)ts[2].tv_sec * ONE_SEC + (long)ts[2].tv_nsec;
        } else {
            txtime = (long)ts[0].tv_sec * ONE_SEC + (long)ts[0].tv_nsec;
        }

        /* print received message */
        if (quiet_mode == 0) {
			if (COMPRESSION_ACTIVE == 0){
				printf("REC_TIME=%ld, UDP_LENGTH=%d, %s\n", txtime, n, decompressed_data);
			}else{
				printf("REC_TIME=%ld, UDP_LENGTH=%d, %s\n", txtime, n, buf);
			}
        }

        /* append message to a file */
        if (file_set == 1) {
            fp = fopen(file, "a");
            if (fp == NULL) {
                printf("%s: Cannot open file %s (%s)\n", progname, file, strerror(errno));
                exit(EXIT_FAILURE);
            }

			if (COMPRESSION_ACTIVE == 0){
				fprintf(fp, "REC_TIME=%ld, UDP_LENGTH=%d, %s\n", txtime, n, decompressed_data);
			}else{
				fprintf(fp, "REC_TIME=%ld, UDP_LENGTH=%d, %s\n", txtime, n, buf);
			}
            fclose(fp);
        }
    }
}
