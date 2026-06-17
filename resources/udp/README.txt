Instructions for using the socket

installation:
Sockets must be installed on avamini02 and avamini03
compile send_udp with: $ gcc -pthread -o /usr/local/bin/send_udp send_udp.c -lz
compile recv_udp with: $ gcc -pthread -o /usr/local/bin/rec_udp rec_udp.c -lz

Requirements:
vlan device must be set with mapping from sbk to pcp
see vlan on avamini02 and avamini03

#######################################################################################################################
#######################################################################################################################
send_udp

_______________________________________________________________________________
Description
Socket for sending UDP packets with test data
Uses the txtime feature of tc-etf 
 (https://man7.org/linux/man-pages/man8/tc-etf.8.html)
The time the messages were sent is saved using timestamps
SO_TIMESTAMPING is used for hardware and software timestamping
see: https://www.kernel.org/doc/Documentation/networking/timestamping.txt
between hardware and software timestamps can be selected
Socket periodically sends out messages
Start time and period can be set
Test data is sent in the payload of the message
This test data included: 
IP address/hostname + port of the sender and destination,
pcp-priority, Streamid, count, tx timestamp
Contains options for manipulating the process


_______________________________________________________________________________
Compile

compile with: $ gcc -pthread -o /usr/local/bin/send_udp send_udp.c -lz
_______________________________________________________________________________
Flags 
############################################################################
mandatory:

-i [name]
    specify interface on which the packet will be send
    If not set, an error is issued

############################################################################
set destination /(source)

-d [ip/host]  
    set destination IP address or host
    unicast only (Multiple entries not allowed)
    If not set then loopback is used

-u [port]     
    set udp destination port 

-s [port]     
    set udp source port

############################################################################
802.1Q Header

-t priority
    set pcp priority (skb priority which is mapped by the vlan interface)
    A priority is defined for the packets in the IEEE802.1Q header in the pcp
    To do this, an skb priority is set in the socket
    The skb priority is mapped between 0 and 7 to the corresponding pcp
    Values outside this range are mapped to pcp priority 0
    To use the PCP priority, the VLAN must be configured with mapping from 
    skb->pcp in the socket, only the skb priority is set

############################################################################
Streamid

-x
set streamid
is sent in the payload of the message

############################################################################
TXTime (used tc-etf https://man7.org/linux/man-pages/man8/tc-etf.8.html)

-P [num]      
    period in nanoseconds
    Time that is added to the basetime after each transmission 
    and determines the time of the next packet
    If the value is too small, it can happen that period<processing time
    If this happens, errors occur because the transmission time can 
    no longer be met (Packets are dropped by tc-etf)

-b [tstamp]   
    txtime of 1st packet 
    Default: now + ~2seconds\n"
    specification as uint64_t
    If time is specified in the past, packets up to the 
    present time will be created and discarded (Leads to errors)

-E            
    enable error reporting 
    reads the socket error queue for SO_TXTIME errors
    If an error occurs, returns the type of error and exits the program

-w [num]      
    wake up time 
    Time before the scheduled send time that the process 
    should wake up and start processing the packets
    delta from wake up to txtime in nanoseconds 
    If too low, errors will occur

-n            
    deactivate TXTime feature
    Period and basetime are still used, but the time is not set in socketoption

-D            
    set deadline mode for SO_TXTIME
    Packets are not sent at send-time, 
    but are sent when they arrive and discarded after send-time

############################################################################
Timestamping 
source:
(https://www.kernel.org/doc/Documentation/networking/timestamping.txt)

-H
    Enable hardware time stamping (enabled by default)
    uses SOF_TIMESTAMPING_TX_HARDWARE | 
    SOF_TIMESTAMPING_RX_HARDWARE | 
    SOF_TIMESTAMPING_RAW_HARDWARE
    from SO_TIMESTAMPING
    Cannot be used with -S

-S
    Enable software time stamping
    uses SOF_TIMESTAMPING_TX_SOFTWARE | 
    SOF_TIMESTAMPING_RX_SOFTWARE | 
    SOF_TIMESTAMPING_SOFTWARE
    from SO_TIMESTAMPING
    Cannot be used with -H

-I interface
    Setting of the interface on which the driver options 
    for hardware time stamping should be set
    If a vlan device is used, the corresponding interface 
    must be used on which the vlan interface is based

############################################################################
Commands for setting the process on the host

-q
    quit mode which suppresses the output via stdout, errors are still thrown

-f
    write received messages to file
    Output is appended to file if file already exists
    If file does not exist then file will be created
    Directory of the file must exist

-h
    print usage

-c [num]     
    Select CPU on which the process is running

-p [num]      
    set linux realtime priority 
    1(min) to 99(max)

############################################################################
example
$ send_udp -i t1.42 -I t1 -d t2 -H


#######################################################################################################################
#######################################################################################################################
rec_udp

_______________________________________________________________________________
Description
Socket for receiving UDP packets with software and hardware time stamps
socket for IPv4 UDP packets is opened to receive the packets
Used to receive the packets from send_udp and output them to standard output or a file
SO_TIMESTAMPING is used for hardware and software timestamping
see: https://www.kernel.org/doc/Documentation/networking/timestamping.txt
between hardware and software timestamps can be selected
The payload of the received messages is extended by the timestamp of receipt

_______________________________________________________________________________
Compile

compile with $ gcc -pthread -o /usr/local/bin/rec_udp rec_udp.c -lz
_______________________________________________________________________________
Flags 
There are various setting options, as listed below:

-i interface
    Set the interface on which the packets should be received
    Must be set

-I interface
    Setting of the interface on which the driver options 
    for hardware time stamping should be set
    If a vlan device is used, the corresponding interface 
    must be used on which the vlan interface is based

-p port
    Set the UDP port
    Standard port is 4242

-H
    Enable hardware time stamping (enabled by default)
    uses SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE
    from SO_TIMESTAMPING
    Cannot be used with -S

-S
    Enable software time stamping
    uses SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
    from SO_TIMESTAMPING
    Cannot be used with -H

-q
    quit mode which suppresses the output via stdout, errors are still thrown

-f
    write received messages to file
    Output is appended to file if file already exists
    If file does not exist then file will be created
    Directory of the file must exist

-h
    print usage

example command line call
$ rec_udp -i t2.42 -I t2 -H
