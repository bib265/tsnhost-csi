# Project Description

This is written in markdown and probably looks better in the project's
Git repository. Please also note that further tasks are only added in
the Git.

### Introduction
 
There are two worlds. In the data center, Software-Defined Networking (SDN)
is prevailing and has established a rich ecosystem of standards and tools
including those that support rapid prototyping of (virtual) networked
environments.  Usually, the network is managed by a so called SDN controller
that uses the OpenFlow protocol to configure all network elements (e.g.,
the network switches).  There are frameworks to build custom controller
software (e.g., Ryu), virtual switches (e.g., Open vSwith) to be controlled,
and virtual network environments (e.g., MiniNet) that allows you to run all
of these on a single computer.
 
In the world of factory automation networks, a series of standards called
Time Sensitive Networking (TSN) has been developed that makes products
(e.g., realtime-capable switches) of different vendors compatible to each
other.  Thus, factory network environments start to grow in size and
complexity requiring better means for configuration, for example, a TSN
controller.  First ideas are inspired by SDN and described in the standards.

In a former project, we had taught the OpenDaylight SDN controller some new 
ticks in order to manage a network of TSN-capable switches. In this project,
we want to bring the talkers and listeners better into the game.Talkers and 
listeners are the end stations that send and receive, respectively, the data 
streams in the network. In particular, we want to manage the Linux hosts in
a TSN testbed.


### Background on SDN

In the following, there are a lot of resource pointers into the world of SDN. 
You may optionally read some or all of them. Probably, you will have to 
consult some of them later.
 
* Initial idea of SDN and OpenFlow:
  Nick McKeown, Tom Anderson, Hari Balakrishnan, Guru Parulkar, Larry
  eterson, Jennifer Rexford, Scott Shenker, and Jonathan Turner. 2008.
  OpenFlow: enabling innovation in campus networks. SIGCOMM Comput. Commun.
  Rev. 38, 2 (April 2008), 69-74. DOI: https://doi.org/10.1145/1355734.1355746
* Home of OpenFlow (standard):
  https://opennetworking.org/software-defined-standards/specifications/
* OpenDaylight project home: https://www.opendaylight.org/
* lighty.io: https://lighty.io/
* YANG specifications list: https://en.wikipedia.org/wiki/YANG
* YANG RFC: https://datatracker.ietf.org/doc/html/rfc6020
* YANG 1.1 RFC: https://datatracker.ietf.org/doc/html/rfc7950
* YANG tutorial: https://ultraconfig.com.au/blog/learn-yang-full-tutorial-for-beginners/
* pyang: https://pypi.org/project/pyang/
* pyangbind: https://pypi.org/project/pyangbind/ 
* Video tutorial of SDN in practice by Telefonica:
  https://unibox.uni-rostock.de/getlink/fi93tfAJRZtG6PbHaQBRNnJC/TransportNetworkProgrammability.mkv


### Background on TSN

In the following, there are a lot of resource pointers into the world of SDN. 
You may optionally read some or all of them. Probably, you will have to 
consult some of them later.

* You find a section of a bachelor's thesis (written in German) in the
  files section at the unibox:
  https://unibox.uni-rostock.de/getlink/fiQPTLUUsZTh5obJbogJGV3t/TSN.pdf
* This is a scientific paper with a good introduction into TSN and traffic
  classes: https://doi.org/10.1109/ACCESS.2018.2883644
  You should read it until (and including) Sect. 3. (And if you are still
  interested, please continue.)
* These are our papers on the topic:  
  https://doi.org/10.1109/RTCSA50079.2020.9203662 and
  https://doi.org/10.1145/3139258.3139289
  Please read them until (and including) Sect. 2. (And if you are still
  interested, please continue.)
* The TSN standard is here: https://doi.org/10.1109/IEEESTD.2018.8403927
  Chapter 8.6 gives an overview about the Forwarding Process (cf. Fig.
  8-12). Schedule enforcement is described in Sect. 8.6.8.4 and 
  illustrated by Fig. 8-14. However, this should be nothing new w.r.t. the
  sources above. Hence, this is just for looking up very specific details
  as the length of nearly 2000 pages might not be the best idea for a quick
  read.
* IEEE has worked on a YANG data model for TSN on which we may build upon.
  The status is here: https://1.ieee802.org/tsn/802-1qcw/


### TSN Configuration
 
According to IEEE 802.1Qcc, there are three architectures: decentralized,
centralized, and hybrid. Reference: IEEE Standard for Local and
Metropolitan Area NetworksBridges and Bridged Networks, Amendment 31:
Stream Reservation Protocol (SRP) Enhancements and Performance Improvements.
In: IEEE Std 802.1Qcc-2018 (Amendment to IEEE Std 802.1Q-2018 as amended by
IEEE Std 802.1Qcp-2018) (2018), Oct, S. 1208. DOI:
https://doi.org/10.1109/IEEESTD.2018.8514112
 
All architectures comprise an interface between user end devices (talker and
listener) and network: User Network Interface (UNI) for the user to specify
the requirements of the data to be transmitted without knowing the network,
the network independently configures bridges considering the user 
requirements.
Reference: Gutiérrez, M. ; Ademaj, A. ; Steiner, W. ; Dobrin, R. ;
Punnekkat, S.: Self-configuration of IEEE 802.1 TSN networks. In: 2017 22nd
IEEE International Conference on Emerging Technologies and Factory
Automation (ETFA), 2017. ISSN 19460759, S. 18

We just consider the fully centralized design. There is a Centralized 
Network Configuration (CNC), which knows the network topology and bridge 
capabilities and manages the bridges over the Southbound API.  There is also
the Centralized User Configuration (CUC) to be able to configure end 
devices in a very limited way, talker and listener send their requirements 
to the CUC with a *non-specified* protocol. CUC anc CNC are connected by a
UNI interface that IEEE is trying to forget as both CUC and CNC are 
practically implemented in the same controller.
 
``` 
         |--------------------------->CUC<--------------------------|
         |                             |                            |
         |                            UNI                           |
         |                             |                            |
         |                    |------>CNC<------|                   |
         |                    |                 |                   |  
      Talker ------------> Brigde --> ... --> Bridge -----------> Listener
```
 
End device descriptions:
 
* CUC requests device descriptions, which data they deliver and which they
  need to derive network data flows
* CUC determines network data flows and sends them to the CNC via UNI
  Network data streams are characterized by:
  * Stream ID: 0...N
  * Source: IP address etc.
  * Target: IP address etc.
  * Cycle: period duration, after which the next message is transmitted
  * Msg size: size of Ethernet packet in bytes
 
Data exchange between CNC and bridges is done via NETCONF. This is the
protocol with which the TSN controller (CNC+CUC) talks to the bridges
and eventually also to our TSN/LINUX host. Exactly, this is missing
in the figure above and requires arcs in the opposite direction. 
 
* YANG models the configuration data for network management protocols
  Reference: M. Bjorklund, “YANG - A Data Modeling Language for the Network
  Configuration Protocol (NETCONF),” RFC 6020, Oct. 2010. [Online].
  Available: https://rfc-editor.org/rfc/rfc6020.txt
* Network management protocols such as NETCONF apply a specific encoding
  such as XML or JavaScript Object Notation (JSON)
  Reference: R. Enns, M. Bjorklund, A. Bierman, and J. Schönwälder, “Network
  Configuration Protocol (NETCONF),” RFC 6241, Jun. 2011. [Online].
  Available: https://rfc-editor.org/rfc/rfc6241.txt
* Properties of networks are specified by YANG modules
* A YANG module defines the organization and hierarchy, respectively, of
  data and rules


### Tasks

* Get a user account from the computer science department. 
  Engl.: https://www.informatik.uni-rostock.de/en/it-service/information/user-account/
  Germ.: https://www.informatik.uni-rostock.de/it-service/informationen/nutzeraccount/
* Log in to the Gitlab: https://git.informatik.uni-rostock.de
* Ask Helge to be invited to the repository for this project.
* For the next tasks, please use Git's feature to create issues. In 
  addition, please document the time estimated for and actually spent on
  your tasks. Git documentation for time tracking:
  https://docs.gitlab.com/ee/user/project/time_tracking.html 
* As everything runs under Linux, you may use a VM. For this you require a
  hypervisor such as Virtual Box: https://www.virtualbox.org/
  For MAC on ARM, you may test VMware Fusion: 
  https://www.vmware.com/products/desktop-hypervisor/workstation-and-fusion
* If in doubt, please use an Ubuntu 24.04 (LTS, Noble Numbat) image: 
  https://ubuntu.com/download 
* You may use the server version. It comes without a graphical desktop 
  environment. However, you can access it via ssh on the command line.
* If you like living on the edge, you may whether all network options are
  available in Microsoft's WSL2: 
  https://docs.microsoft.com/en-us/windows/wsl/install-win10
* Please set up your development environment (IDE) that you can work on
  the VM. If in doubt, please use the Visual Studio Code: 
  https://code.visualstudio.com/
* If using a server version as VM, you may want to leverage the Remote
  Code Development features of your IDE:
  https://code.visualstudio.com/docs/remote/remote-overview
* As NETCONF server, we will use Netopeer 2: 
  https://github.com/CESNET/netopeer2

* Netopeer 2 Resources:

  - Netopeer 2 project page: https://github.com/CESNET/Netopeer2
  - Netopeer 2 documentation: https://netopeer.liberouter.org/doc/netopeer2/
  - IETF RFC 6241 - Network Configuration Protocol (NETCONF): https://tools.ietf.org/html/rfc6241
  - IETF RFC 6242 - Using the NETCONF Protocol over Secure Shell (SSH): https://tools.ietf.org/html/rfc6242
  - YANG data modeling language: https://tools.ietf.org/html/rfc7950

  Netopeer 2 Setup and Usage:

    1. Install Netopeer 2 on your system following the instructions in the project documentation: https://netopeer.liberouter.org/doc/netopeer2/installation.html
    2. Familiarize yourself with the Netopeer 2 architecture and components: https://netopeer.liberouter.org/doc/netopeer2/architecture.html
    3. Explore the available YANG models and learn how to use them with Netopeer 2: https://netopeer.liberouter.org/doc/netopeer2/yang-models.html
    4. Try out the Netopeer 2 client and server usage examples: https://netopeer.liberouter.org/doc/netopeer2/usage.html


* As YANG datastore, we will use Sysrepo: https://www.sysrepo.org/

* Sysrepo Resources:

  - Sysrepo project page: https://www.sysrepo.org/
  - Sysrepo GitHub repository: https://github.com/sysrepo/sysrepo
  - Sysrepo documentation: https://www.sysrepo.org/documentation/
  - YANG data modeling language: https://tools.ietf.org/html/rfc7950
  - IETF RFC 6241 - Network Configuration Protocol (NETCONF): https://tools.ietf.org/html/rfc6241

  Sysrepo Setup and Usage:

  1. Install Sysrepo on your system following the instructions in the documentation: https://www.sysrepo.org/documentation/installation/
  2. Familiarize yourself with the Sysrepo architecture and components: https://www.sysrepo.org/documentation/architecture/
  3. Learn how to use Sysrepo's YANG data store and management capabilities:
    - Modeling data using YANG: https://www.sysrepo.org/documentation/data-modeling/
    - Accessing and managing data: https://www.sysrepo.org/documentation/data-access/
    - Subscribing to data changes: https://www.sysrepo.org/documentation/data-notifications/
  4. Explore Sysrepo's language bindings and integration with various programming languages:
    - C language binding: https://www.sysrepo.org/documentation/c-api/
    - Python language binding: https://www.sysrepo.org/documentation/python-api/
    - Other language bindings: https://www.sysrepo.org/documentation/language-bindings/
  5. Check out the Sysrepo examples and tutorials to get started: https://www.sysrepo.org/documentation/examples/

  
* There are several ways to get Netopeer 2 and Sysrepo running.
  One way is to compile them from the sources. To do this in a controlled
  environment that is identical for each of us, please use a Docker
  container! This requires you to write a Docker file. Docker:
  https://www.docker.com/


* Docker Resources for setup and usage:

  1. Install Docker on your operating system:
    - Docker for Windows: https://docs.docker.com/docker-for-windows/install/
    - Docker for macOS: https://docs.docker.com/docker-for-mac/install/
    - Docker for Linux: https://docs.docker.com/engine/install/
  2. Familiarize yourself with the Docker architecture and core concepts:
    - Containers: https://docs.docker.com/get-started/overview/#docker-objects
    - Images: https://docs.docker.com/get-started/overview/#docker-images
    - Networking: https://docs.docker.com/network/
    - Volumes: https://docs.docker.com/storage/volumes/
  3. Learn how to work with Docker containers:
    - Building Docker images: https://docs.docker.com/get-started/part2/
    - Running Docker containers: https://docs.docker.com/get-started/part3/
    - Managing Docker containers: https://docs.docker.com/engine/reference/run/
  4. Explore Docker Compose for multi-container applications:
    - Docker Compose overview: https://docs.docker.com/compose/
    - Docker Compose file reference: https://docs.docker.com/compose/compose-file/
  5. Check out Docker's security features and best practices:
    - Docker security: https://docs.docker.com/engine/security/
    - Docker security best practices: https://docs.docker.com/develop/develop-images/dockerfile_best-practices/
  6. Docker Tutorial for Beginners - A Full DevOps Course on How to Run Applications in Containers: 
  An end-to-end course with hands-on labs to get started using Docker. https://www.youtube.com/watch?v=fqMOX6JJhGo

* Maybe, look for different ways to install Netopeer and Sysrepo.
* Add a simple YANG model to the Sysrepo and modify its data via
  NETCONF. Thus, YANG, Netopeer 2, and Sysrepo works...
* Sysrepo provides several ways for applications to interact with.
  Analyze the possibilities, discuss strengths and weaknesses of
  each approach and finally make a good design decision on how to
  carry on... 
* One part of the necessary configuration is covered by the YANG models
  developed by IEEE. Please refer to "IEEE Standard for Local and 
  Metropolitan Area Networks--Bridges and Bridged Networks Amendment 38: 
  Configuration Enhancements for Time‐Sensitive Networking," in IEEE Std 
  802.1Qdj‐2024. DOI: https://doi.org/10.1109/IEEESTD.2024.10542670
* Perhaps, the respective YANG models are already available here:
  https://github.com/YangModels/yang
* Another part of the configuration is about managing a Linux host. We are
  not first who are trying to do this. Maybe, there is already a nice
  plugin available that does necessary things for us. Please have a look
  at https://www.sysrepo.org/plugins and particularly at
  https://github.com/telekom/sysrepo-plugins 
* Finally, there are the talker and listener applications that are used
  by AVA in the testbed. You find these in the Git repository under
  /resources/udp Please compile these and study their command line options.
  Probably, these command line arguments must be covered by a corresponding
  YANG model.
* In addition, under /resources/instructions, you find some summaries of 
  manuals and other help texts for several aspects of Linux networking.
  In addition, to the talker and listener apps, the network environment of
  the host need to be prepared as well. Start reading these and create
  (virtual) interfaces for sending and receiving TSN packets.
* There are more tasks to come...