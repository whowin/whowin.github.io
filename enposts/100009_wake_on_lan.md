# Remote Boot: A simple embedded project development

> This article describes how to develop a program on an *ARM* device to power on a server remotely. It roughly introduces the development process of a simple embedded project  through this example. It will not detail the principle of wake-on-lan and *Magic Packet*.
<!--more-->

## 1. Overview
* This article introduces the development process of a simple embedded project.
* From requirements to practice, this article provides a comprehensive introduction to the entire process, and the equipment described in this article is easy to obtain and inexpensive.
* This article deals with the concepts of network programming under the C language of Linux, such as network broadcast, Magic Packet, NAT and reverse proxy.
* Readers can refer to other articles for some of the technical concepts covered in this article.
* This article may not be for beginners.

## 2. Requirement
* I have a server at home. Almost all my things are stored on this server. Whether at home or elsewhere, I need to connect to this server to do things.
* The server is on during the day and off at night (save power :)).I have to remember to press the power switch of the server after getting up and turn off the server before going to bed every day.
* If I forget to turn off the server at night, there is usually no problem. But sometimes the server is not turned on in the morning and I might be in trouble.
* Sometimes I forget to turn on the server in the morning, but I just need to log in to the server outside, and then I remember that the server is not turned on, but there is no way.
* So, I need a mechanism to switch on my server remotely to avoid embarrassment.

## 3. Brief Solution
* Let's approach this requirement as a project, which can be called **SERVER REMOTE BOOT**.
* First of all to ensure that the server motherboard support wake-on-lan. but now the vast majority of motherboards are supported (the motherboards after PCI 2.2 are generally supported, and PCI 2.2 standard is proposed in 1998), some motherboards may need BIOS Settings, please search for solutions by yourself.
* The second is that there is a small device running 24 hours a day on the LAN at home. We will use this device to broadcast Magic Packet on the LAN to wake up the server. This device should be a low-power device, the smaller the simpler the better. Let's call this device **wakener**.
* The third is to write a simple program on this **wakener**, this program can broadcast Magic Packet on the LAN to wake up the server, this program is called **wakeOnLan**.
* The fourth is that we must have a mechanism to access the wakener from the Internet. Only in this way can we control the software on the wakener. In fact, this is a problem of intranet penetration. We need a light application server(VPS is okay) connected to the Internet to complete this task.
* In this way, no matter where I am, I can log in to the wakener that is turned on 24 hours at home through the terminal (laptop, desktop, mobile phone, tablet, etc.), and run wakeOnLan to start my server.
* This 24-hour-a-day device (wakener) has the potential to do more things for you, such as NAS, remote air conditioning, etc., but the important thing is to complete the first step in front of you.

## 4. Technical Points
> The above solution is obviously very rough, but the project itself is indeed relatively simple, there is no need to do a very detailed design plan, so we only list some possible points and solutions below.

* **The basic network architecture**
  - The following is a schematic diagram showing how the various devices in this project are connected and affect each other. The Local Server is the server we want to start remotely, and the Server is a VPS connected to the Internet for intranet penetration.

    ![wake_on_lan_architecture.png][img05]
    + **Figure 1: wake-on-lan architecture diagram**
************************
* **Magic Packet**
  - **Wake-on-LAN** is actually a function of the network card. After PCI 2.2, there is an additional PME signal on the signal bus. After the computer is powered off, it enters the sleep state and can continue to supply power to the network card. After the network card receives a packet called Magic Packet, If it is detected that the data packet is sent to itself, it will send a PME signal on the PCI to control the computer to start. This function is called **wake-on-lan**.
  - Wake-on-LAN is not complicated. In a local area network, if Magic Packet is broadcast from a computer, the computer corresponding to the MAC address specified in Magic Packet will be woken up.
  - Magic Packet is a data packet with a specified format, the format is: 6 0xff, and then 16 groups of MAC addresses of the computers that need to be woken up. For example, the MAC address of the computer that need to be woken up is: 00:e0:2b:69:00:03, the Magic Packet is (hexadecimal representation):
    ```
    ff ff ff ff ff ff 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 00 e0 2b 69 00 03 
    ```
  - Usually Magic Packet broadcasts are sent using UDP, and the port number can be arbitrary, usually 7 or 9.
  - For more information about Magic Packet, please refer to [wikipedia-Wake-On-Lan](https://en.wikipedia.org/wiki/Wake-on-LAN)
  - As can be seen from Figure 1, Wakener broadcasts Magic Packet to the LAN through the router to wake up the Local Server.
  - It should be noted that Magic Packet is only valid for wired network cards, so the server to be woken up must be connected to the router using a network cable.

* **Intranet Penetration**
  - In this project, intranet penetration refers to: using certain technology to allow us to directly access a device at home from the Internet, this device is connected to the Internet through ordinary home broadband, and home broadband may not have a public IP. 
  - If the home broadband has a public IP, it is not very difficult to penetrate the intranet, and usually requires three steps:
    1. Set up a NAT on the home router so that the access from the internet is directly forwarded to a specific port of a specified device in the LAN.
    2. A device in the LAN (or the router itself) sends a heartbeat packet to a server on the Internet, so that the server can know the public IP of the home broadband;
    3. The terminal device on the Internet obtains the public IP of the home broadband through the server, and directly accesses the IP to access the designated device in the LAN;
  - If the home broadband does not have a public IP (most broadband should be like this in China), it will be more troublesome to penetrate the intranet, usually you need to use **reverse proxy** technology. This requires deploying a reverse proxy server on the Internet.
    + Server-side software that supports reverse proxy is installed on the reverse proxy server, such as *sshd*.
    + A reverse proxy client software, such as *ssh*, is installed on the device on the LAN. A reverse proxy tunnel can be established by sending reverse proxy instructions to the reverse proxy server. After *tunnel* is established, Accessing a specified port of the reverse proxy server will be mapped to a port under an IP address in the LAN, so as to achieve intranet penetration.
  - At present, there are many tools for intranet penetration. The principle is basically a reverse proxy, but it is usually much easier to use than using *ssh* directly. At least *ssh* will not automatically reconnect after an accidental disconnection. These tools will solve this problem;
  - The embedded *Linux* system used in this article will be *openwrt*. An open source project called *frp* will be used for intranet penetration, and the URL of the **frp** will be given later.

* **Other difficulties that may be faced**
  - Burn or compile a customized Linux operating system for the hardware. Fortunately, the examples in this article do not need to compile a Linux by yourself, just burn it.
  - Cross-compilation environment, which must be faced in embedded development and cannot be avoided.

## 5. Brief Embedded Development Steps
1. Requirements analysis
2. Outline design and detailed design
3. Hardware development and verification
4. Compile the operating system and required utilities for the hardware
5. Establish a cross-compilation environment for software development on the specified hardware;
6. Embedded software development;
7. Debug

> Below we will follow this development step to implement our solution.

## 6. Practice
* **Requirements analysis and design**
  > The requirements analysis and design of this project have been completed in the front. Since the project is relatively simple, no more detailed design will be made.

* **Hardware development and verification**
  - Looking for **a device that runs 24 hours a day**
    + A discarded small board is used, *CPU* is *Amlogic S805*, *ARM Cortex-A5* architecture, 4-core 1.5GHz, very low power consumption, remember the model of this CPU, remember that this CPU is 32 bit.
    + This board is taken from a product called "Money-Box" from Xunlei (a Chinese company) many years ago.

    ![money_maker.png][img01]
    + **Figure 2: Maney-Box appearance**

    ![money_maker_board.jpg][img02]
    + **Figure 3: Motherboard of Money-Box**
    ***************************
    + This board has 256M memory, 1G Flash, 100M Ethernet interface and a USB port, which is enough. In the past, some manufacturers used this CPU as a web TV set-top box, but no one should use it now.
    + After checking on Taobao, the first generation of Money-Box can not be bought, but the third generation (player cloud) is still sold second-hand, about 45-55 RMB(USD6 - 8), the CPU used is the same as the first generation, but the memory is larger. You can also buy one to play with if you want.
  - The hardware of this device obviously does not need to be verified, and Xunlei has already verified it for us.
  - Actually, there are several options for this device, if your router is OEM and usually runs openwrt on it, then you can use it directly. Or if you have an idle TV set-top box, etc., it may be useful. However, it is usually not recommended to use a tablet. One is that it consumes too much power, and the other is that it is difficult to flash Android to Linux.

* **Operating System**
  - The original Linux on the Money-Box should be compiled by Xunlei itself. It is difficult to know what Xunlei has done on this system, so it is better not to use it. A new system needs to be refreshed. Devices with CPU S805 usually support USB flashing. , so you don't need to remove the motherboard, you can refresh the system directly with a USB cable.
  - It is appropriate to burn the Openwrt system for this board, there are ready-made tutorials, and a lot of materials, which is convenient for future expansion.
  - Reburning firmware tutorial: [How to burn OpenWrt firmware for Money-Box](https://post.smzdm.com/p/axlkkz64/)；
  - Firmware and related software: [Baidu network disk](https://pan.baidu.com/s/1E4Ls05lPHHHhv0Ou8fb7GA); Code: ow2l
  - The firmware package provides two versions of openwrt, 18 and 19. It is recommended to burn openwrt 19 (because I burned this version).
  - Enjoy the process of burning the firmware by yourself.
  - The board with the re-flashed firmware should be able to log in with ssh.
    > Find the IP address of this openwrt from your router, for example: 192.168.2.100, then log in with ssh, the newly burned openwrt has no password.

    ```
    ssh root@192.168.2.100
    ```

    > The figure below is what it looks like after logging in on my openwrt. After logging in, be sure to change the root password.

    ![login_openwrt_first.png][img06]
    + **Figure 4: Log in to openwrt using ssh**
    *********************
  - openwrt has a web interface, you can enter the IP directly on the browser to enter

    ![web_openwrt_1.png][img07]
    + **Figure 5: Login to the web interface of openwrt**

    ![web_openwrt_2.png][img08]
    + **Figure 6: The web interface of openwrt**
    *******************
  - This device should use a fixed IP, not DHCP, so that it is convenient for remote login in the future, there are three ways to set a fixed IP
     1. Modify the configuration file directly
         > The network configuration file of openwrt is placed in the /etc/config/network file, you can directly modify this file and change it to the following:

        ![openwrt_fixed_ip_config.png][img09]
        + **Figure 7: Set openwrt to fixed IP**
        *************************
    2. Using the openwrt web interface
         > Log in to the openwrt web page from the browser, the password is the same as the ssh password, select: Network --> Interfaces --> Edit

        ![openwrt_fixed_ip_web_1.png][img10]
        + **Figure 8: Setting a fixed IP via openwrt's web interface**

        ![openwrt_fixed_ip_web_2.png][img11]
        + **Figure 9: Setting a fixed IP via openwrt's web interface**

        ![openwrt_fixed_ip_web_3.png][img12]
        + **Figure 10: Setting a fixed IP via openwrt's web interface**
        ********************************
    3. Bind IP with MAC address on router
         > It is to set DHCP on the router to: assign a fixed IP to the device with the specified MAC address. In this way, your openwrt device does not need to set a fixed IP. The setting method of each router is different. The interface of my router looks like this

        ![openwrt_fixed_ip_router.png][img13]
        + **Figure 11: Setting openwrt's fixed IP through router**
************************
* **Create a cross-compilation environment**
   - To write a program on Money-Box, we need to complete the programming on the local computer, and then compile it into a program that can be run on Money-Box with cross-compilation tools, and transfer it to Money-Box to run.
  - So we need a toolchain to cross compile the programs we write.
  - This toolchain is not placed on wakener, because wakener usually has poor performance, and in order to save storage space, only the runtime libraries is usually placed on it. Therefore, this toolchain should be placed on another computer running Linux. We call this computer the development computer. It is recommended to run ubuntu on the development computer.
  - The following is the process of establishing this compilation environment.
  - First log in to openwrt using ssh and check the version of your newly burned openwrt.

    ![openwrt_version.png][img03]
    + **Figure 12: Checking the version of openwrt**
    ***************************
  - Go to [openwrt official website](https://downloads.openwrt.org) to find the sdk under the version you need, and select the at91/sama5 directory, this is the toolchain of cortex-A5 architecture (CPU S805 architecture) .
    + [The download site for my toolchain](https://downloads.openwrt.org/releases/19.07.7/targets/at91/sama5/openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64.tar.xz)
  - Note that this toolchain can only run under Linux, I am running under Ubuntu, and the SDK of openwrt-19.07.7 only has a 64-bit X86 version.
  - Download the toolchain to the ~/Downloads/ directory
    ```
    wget https://downloads.openwrt.org/releases/19.07.7/targets/at91/sama5/openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64.tar.xz -C ~/Downloads/
    ```
  - Extract it
    ```
    cd ~/Downloads
    tar -xvf openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64.tar.xz
    ```
  - A new directory will be created in the ~/Downloads/ directory openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64, enter this directory, you can see the staging_dir directory.
    ```
    cd openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64/
    ls
    ```
  - Everything in this staging_dir directory is a complete toolchain, which can be copied from the sdk and used alone.
  - I copied this directory to ~/toolschain/openwrt-a5/, the reason why I put it under ~/toolschain/ is because we may have toolchains for other devices, and multiple toolchains for different devices can be put in ~/toolschain/ directory for easy management.
    ```
    mkdir -p ~/toolschain/openwrt-a5/
    cp -fr ~/Downloads/openwrt-sdk-19.07.7-at91-sama5_gcc-7.5.0_musl_eabi.Linux-x86_64/staging_dir/* ~/toolschain/openwrt-a5/
    ```
  - Now we have a complete toolchain, but this toolchain is basically useless, we need to simply configure it, that is, set some environment variables;
    + Edit a file *a5.sh* in the *HOME* directory with the following content:
      ```
      #!/bin/bash

      #A5 arm-linux-musl-eabi toolchain
      export PATH=/home/whowin/toolschain/openwrt-a5/toolchain-arm_cortex-a5+vfpv4_gcc-7.5.0_musl_eabi/bin:$PATH
      export STAGING_DIR=/home/whowin/toolschain/openwrt-a5
      ```
    + Where /home/whowin is my HOME directory, you need to change it to your HOME directory.
    + This file modifies the environment variable PATH, and then adds an environment variable STAGING_DIR. With these settings, we can use the toolchain normally.
    + Make this file executable and copy it to /bin/
      ```
      cd ~
      vi a5.sh
      (Edit and save)

      chmod 755 a5.sh
      sudo cp a5.sh /bin/
      ```
    + Putting it in the /bin/ directory is just for convenience. It has no special meaning.
    + Now we can try this toolchain.
      ```
      source /bin/a5.sh
      arm-openwrt-linux-gcc -v
      ```
    + Below is the output on my computer.

    ![toolschain_test.png][img04]
    + **Figure 13: Test Toolchain**
****************************
* **Source Code**
  - With the toolchain, programming can be done. The programming process must be completed on the development computer, not under openwrt, but the compiled executable file must be run under openwrt.
  - Below is the source code of the program wakeOnLan that needs to be run under openwrt.

    ```
    /******************************************************************************
      File Name: wakeOnLan.c
      Description:  Send a wake-on-lan command to a computer in LAN
      
      Compile:  source /bin/a5.sh
                arm-openwrt-linux-gcc -Wall wakeOnLan.c -o wakeOnLan

      Usage: wakeOnLan [boradcast IP] [MAC]
      Example: sudo ./wakeOnLan 192.168.2.255 00:e0:2b:68:00:03
      
      Date: 2022-07-26
    *******************************************************************************/
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <signal.h>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>   ////inet_ntop
    #include <netinet/in.h>  //inet_addr

    #define  BIND_PORT         7                   // Port
    #define  MSG_LEN           102                 // The lenght of magic packet
    #define  USAGE             "wakeOnLan [broadcast IP] [MAC]\n" \
                               "Example: sudo ./wakeOnLan 192.168.1.255 00:e0:2b:69:00:03\n"

    /*****************************************************
    * Function: int is_IP(char *IP)
    * Description: Check if the IP format is correct.
    * Return: 1    correct IPv4
    *         0    incorrect IPv4
    *****************************************************/
    int is_IP(char *IP) {
        int a, b, c, d;
        char e;

        if (4 == sscanf(IP, "%d.%d.%d.%d%c", &a, &b, &c, &d, &e)) {
            if (a >= 0 && a < 256 &&
                b >= 0 && b < 256 &&
                c >= 0 && c < 256 &&
                d >= 0 && d < 256) {
                return 1;
            }
        }
        return 0;
    }

    /*************************************************************************
    * Function: int is_MAC(char *mac_str, char *mac)
    * Description: Check if MAC format if correct. if so, convert it to 6 sets of numbers
    * 
    * Return: 1    correct MAC，The converted mac address is in char *mac
    *         0    incorrect MAC
    *************************************************************************/
    int is_MAC(char *mac_str, char *mac) {
        int temp[6];
        char e;
        int i;

        if (6 == sscanf(mac_str, "%x:%x:%x:%x:%x:%x%c", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &e)) {
            for (i = 0; i < 6; ++i) {
                if (temp[i] < 0 || temp[i] > 255) break;
            }
            if (i == 6) {
                for (i = 0; i < 6; ++i) {
                    mac[i] = temp[i];
                }
                return 1;
            }
        }
        return 0;
    }

    // ===================Main================================================
    int main(int argc, char *argv[]) {
        struct sockaddr_in sin;
        char *broadcast_ip;
        char wol_msg[MSG_LEN + 2];                           // magic packet
        char mac[6];
        int socket_fd;
        int i, j;
        int on = 1;
        
        // Check the number of parameters
        if (argc < 3) {
            // Wrong number of parameters.
            printf("Incorrect input parameters.\n");
            printf(USAGE);
            return -1;
        }

        // Check if the first parameter is an IP address.
        if (! is_IP(argv[1])) {
            printf("%s is an invalid IPv4.\n", argv[1]);
            printf(USAGE);
            return -1;
        }

        // Check if the second parameter is a MAC address.
        if (! is_MAC(argv[2], mac)) {
            printf("%s is an invalid MAC address.\n", argv[2]);
            printf(USAGE);
            return -1;
        }
        printf("MAC is %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]);

        // create socket
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            printf("Can not set up socket. Program exits(%d).\n", socket_fd);
            return -1;
        }

        // Allocate memory for IP addresses
        broadcast_ip = calloc(strlen(argv[1]) + 1, sizeof(char));
        if (! broadcast_ip) {
            printf("Can not allocate memory. Program exits\n");
            return -1;
        }
        // Allow sending broadcast message on socket_fd
        setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
        
        memset((void *)&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = inet_addr(broadcast_ip);       // broadcast IP
        sin.sin_port = htons(BIND_PORT);                     // port

        // six ff
        for (i = 0; i < 6; i++) {
            wol_msg[i] = 0xFF;
        }
        // 16 times mac address
        for (j = 0; j < 16; j++) {
            for (i = 0; i < 6; i++) {
                wol_msg[6 + j * 6 + i] = mac[i];
            }
        }
        // send magic packet
        sendto(socket_fd, &wol_msg, MSG_LEN, 0, (struct sockaddr *)&sin, sizeof(sin));
        close(socket_fd);
        // free allocated memory space
        free(broadcast_ip);

        printf("Magic Packet has been sended.\n");

        return 0;
    }
    ```
  - This program is really simple, the comments are complete, and there is nothing to explain.

* **cross compiling**
  - Cross-compilation is also done on the **development computer**.
  - Assuming we put the above source program in the *~/wake_on_lan/* folder, the following is the process of cross-compilation.
    ```
    cd ~/wake_on_lan
    source /bin/a5.sh
    arm-openwrt-linux-gcc -Wall wakeOnLan.c -o wakeOnLan
    ```
  - Errors are inevitable during cross-compilation, please troubleshoot by yourself.
  - The cross-compiled program cannot be run on the development computer. It needs to be copied to openwrt to run. Still assuming that the IP address of openwrt is: 192.168.2.100, you can copy the compiled program to openwrt as follows . Complete the following steps on the development computer.
    ```
    cd ~/wake_on_lan
    scp wakeOnLan root@192.168.2.100:/root/
    ```

* **Run the program on openwrt**
  - Log in to openwrt with ssh. After logging in, you should be in the /root/ directory, because /root/ is the HOME directory of the root user, and then run the program.
    ```
    ssh root@192.168.2.100

    ./wakeOnLan 192.168.2.255 00:e0:2b:69:00:03
    ```
  - The first parameter is the broadcast IP on the LAN, and the second parameter is the *MAC* address of the computer to be woken up remotely. Please modify it according to your actual situation.
  - Below is how it works on my computer.

    ![openwrt_run_wakeonlan.png][img14]
    + **Figure 14: Run wakeOnLan on openwrt**
*****************************
* **Program debugging**
  - The debugging of this program is mainly to ensure that the program can send *magic packet* correctly. It is necessary to find another computer on the LAN to monitor the data packets. This monitoring computer can run either *windows* or *Linux*. A computer that is woken up remotely is best as a monitoring computer. We use a computer running *ubuntu* as an example to demonstrate the debugging process.
  - Use the *tcpdump* tool under *ubuntu* to monitor packets. *tcpdump* must be run with *root* privileges.
  - First run *tcpdump* on the monitoring computer.
    ```
    sudo tcpdump -vv -x udp port 7
    ```
  - This line of command means to listen for packets on *udp* port 7. *-vv* means show detailed information. *-x* means display in hexadecimal. These two parameters can also be written as *-vvx*.
  - Run wakeOnLan on openwrt.
    ```
    ./wakeOnLan 192.168.2.255 00:e0:2b:69:00:03
    ```
  - Please fill in the broadcast IP and *MAC* address according to the actual situation.
  - Normally, the *Magic Packet* sent by the program can be seen on the listening computer. Take a close look to see if the packet is formatted correctly.
  - On my computer, the output is as follows:

    ![listen_magic_packet.png][img15]
    + **Figure 15: Listening to Magic Packet**

  - The part marked by the yellow underline is the IP header, which occupies 20 bytes. The part marked by the green underline is the UDP header, which is 8 bytes. The rest is *Magic Packet*.
  - If you listened to a complete and correct *Magic Packet*, congratulations, you are almost done.
  - If you use *windows* to listen to *Magic Packet*, you should usually use the well-known *Wireshark* software.

* **Wake-on-LAN test in LAN**
  - Shut down the computer that needs to be woken up. If you need to set *BIOS*, you must first set *BIOS* and then shut down the computer.
  - Use *ssh* to log in to *openwrt* on another computer in LAN, or use your mobile phone to log in to *openwrt*. If you use a mobile phone, you need to install a terminal *app* on the mobile phone. I use an Android phone, and the installed *app * Called *ConnetcBot*.
  - Run your program *wakeOnLan* on *openwrt*, and if you're lucky, your computer that was just shut down should be silently powered on.
  - However, there is usually no such good luck, so the possible problems are as follows:
    1. Is the broadcast IP in the program correct?
    2. Is the *MAC* address in the *Magic Packet* correct?
    3. Is the *Magic Packet* * formatted correctly?
    4. Is the computer that will be woken up on the same LAN as *openwrt*?
    5. Does the router restrict port 7 for *UDP*?
    6. If there is no problem with the above, I am afraid you can only suspect that the computer you want to wake up does not support Wake on LAN, or the *BIOS* settings are incorrect.

* **Intranet penetration**
  - Now that we have done most of the work, the only thing to do next is how to access this *openwrt* device from the Internet, which is the **intranet penetration** mentioned earlier.
  - Intranet penetration requires a server (*VPS*) on the Internet, which can be a very cheap VPS with weak performance, because this server just forwards data.
  - I myself use a Russian *VPS*. The price is only **US$13.04/year**. 512M RAM, 5G SSD, running *ubuntu 20.04*. Although it looks low performance, but it is still good to use. I would be happy to recommend to you:
    + [Russian VPS](https://justhost.ru/?ref=149230)
  - As mentioned earlier in the article, the intranet penetration tool I use is an open source project called *frp*, and the project's github address is as follows:
    + [Fast Reverse Proxy(frp)](https://github.com/fatedier/frp)
    + The appropriate *release* can be downloaded by following the documentation. *frps* runs on the server (VPS). The client *frpc* runs on *openwrt*.
    + Usually server-side software is 64-bit X86 architecture, which is easy to handle.
    + Must know what architecture is the device running *openwrt*? Is it 32-bit or 64-bit? For example, the device in this article, *CPU* is *S805*, which is a 32-bit *arm* architecture. Otherwise, the client software (frpc) you downloaded may not work.
    + The setting of this software is quite troublesome, please read the documentation of the project carefully and refer to its examples. Here I give my example.

      > Server side configuration file: *frps.ini*. Please set *xxx.xxx.xxx.xxx* according to the actual situation. Set *frp_log_path* to point to the directory where *frp* logs are stored.

      ```
      [common]
      bind_addr = xxx.xxx.xxx.xxx
      bind_port = 57000
      bind_udp_port = 57001
      kcp_bind_port = 57000
      proxy_bind_addr = xxx.xxx.xxx.xxx
      vhost_http_port = 58080
      vhost_https_port = 58443

      log_file = /frp_log_path/frps.log
      log_level = info
      log_max_days = 3
      disable_log_color = false

      detailed_errors_to_client = true

      authentication_method = token
      authenticate_heartbeats = false
      authenticate_new_work_conns = false
      token = skyline.admin

      oidc_client_id =
      oidc_client_secret = 
      oidc_audience = 
      oidc_token_endpoint_url = 

      allow_ports = 58000-59000,50000-53000
      max_pool_count = 15
      max_ports_per_client = 0
      tls_only = false
      subdomain_host = frps.com
      tcp_mux = true
      ```
      > Client configuration file: *frpc.ini*，Please set *xxx.xxx.xxx.xxx* according to the actual situation. *openwrt.aaa.com* is a domain name (subdomain name) whose A record points to the VPS, and should also be set according to the actual situation.

      ```
      [common]
      server_addr=xxx.xxx.xxx.xxx
      server_port=57000
      log_file=/tmp/frpc.log
      log_level=info
      log_max_days=3
      disable_log_color=false
      token=skyline.admin
      pool_count=5
      tcp_mux=true
      user=whowin
      login_fail_exit=false
      protocol=tcp
      tls_enable=falset
      dns_server=8.8.8.8
      admin_addr=127.0.0.1
      admin_port=7400
      admin_user=skyline
      admin_pwd=admin

      [ssh]
      type=tcp
      local_ip=192.168.2.100
      local_port=22
      use_encryption=false
      use_compression=false
      custom_domain=openwrt.aaa.com
      remote_port=52998
      health_check_type=tcp
      health_check_timeout_s=3
      health_check_max_failed=10
      health_check_interval_s=30

      [openwrt_web]
      type=http
      local_ip=192.168.2.100
      local_port=80
      use_encryption=false
      use_compression=true
      custom_domains=openwrt.aaa.com
      header_X-From-Where=frp           
      health_check_type=http    
      health_check_url=/        
      health_check_interval_s=90 
      health_check_max_failed=3
      health_check_timeout_s=3
      ```
  - Running *frps* on *VPS*, *path_to* points to the actual path.
    ```
    /path_to/frps -c /path_to/frps.ini &
    ```
  - running *frpc* on *openwrt*, *path_to* points to the actual path.
    ```
    /path_to/frpc -c /path_to/frpc.ini &
    ```
  - In case of problems, it is strongly recommended to carefully review *frps.log* and *frpc.log*.
  - Normally, now you can access your home *openwrt* via *frp* on the internet, like this:
    ```
    ssh root@xxx.xxx.xxx.xxx -p 52998
    ```
  - *xxx.xxx.xxx.xxx* is the IP address of the *VPS*. *52998* is the port number set in *frpc.ini*. You can set a domain name to point to the IP of *VPS*, such as setting the A record of *vps.aaa.com* to point to *VPS*, you can log in to *openwrt* like this:
    ```
    ssh root@vps.aaa.com -p 52998
    ```
  - Similarly, according to the above settings, if you want to access the *web* interface of *openwrt*, enter: *openwrt.aaa.com:58080* on the browser. *58080* is a port number set in *frps.ini*.
  - It should be noted that you must set up a firewall on the *VPS* and open the ports that may be used, otherwise *frp* will not work properly.

* **Remote boot test**
  - The computer, tablet or mobile phone used to test the remote boot cannot be connected to the *wifi* at home. It is recommended to use a mobile phone, turn off *wifi*, and open *ConnectBot*.
  - Set the connection as follows:

    ![connectbot_setting.jpg][img16]
    + **Figure 16: Setting up the connection on ConnectBot**

  - Connect to *openwrt* via *frp*

    ![connectbot_login_openwrt.jpg][img17]
    + **Figure 17: Connect openwrt on ConnectBot**

  - 在 *openwrt* 上运行 *wakeOnLan*

    ![connectbot_run_wakeonlan.jpg][img18]
    + **Figure 18: Running wakeOnLan on openwrt**

  - Normally, the computer specified by the *MAC* address should already be powered on. For convenience, you can write a *shell* script on *openwrt* to avoid having to enter IP and *MAC* address every time, like below, note that *shell* under *openwrt* is *ash*.
    ```
    $ cat wakeOnLan.sh 
    #!/bin/ash

    /home/whowin/wakeOnLan 192.168.2.255 00:e0:2b:68:00:03
    $
    ```

## 7. Afterword
> As mentioned earlier in the article, *Magic Packet* can be sent on any port, usually port 7 or 9 is used. Our example uses port 7 to send, readers can try sending from other ports, such as port 1234.

> As an option, *openwrt* has a ready-made **Wake on LAN** module. You can search for *luci-app-wol* on the *web* interface of *openwrt*. When this software is installed, the *etherwake* software is automatically installed. After installation, you can use commands like *etherwake -b AA:BB:CC:DD:EE:FF* to send *Magic Packet* to wake up the specified computer, or you can wake up the specified computer through the *web* interface.

![luci_wake_on_lan.png][img19]
+ **Figure 19: Send Magic Packet via web page.**

> So far, the project is complete. We have basically gone through the whole process of embedded development, but each stage is relatively simple. Embedded development, seemingly complicated, is actually not as difficult as imagined.

> In the design of embedded projects, in order to be able to make a more reasonable solution, we need to have enough knowledge reserves. In the design of this project, it is precisely because we have the knowledge of *Magic Packet* and reverse proxy that we can propose such a combination of software and hardware.

> There is basically no hardware development for this project. But usually, embedded software engineers need to participate in hardware development, including the selection of chip solutions, so that the hardware products designed and developed can be relatively smooth in future software development. Therefore, embedded software engineers must also have the ability to read the *datasheet* of the chip and the ability to understand the hardware schematic diagram. A good embedded software engineer can even have very good welding skills.

> The key to embedded development is software development, but it requires a lot more knowledge than ordinary software development. For example, in this project, we must know the architecture of the hardware we are using and the model of the CPU, otherwise, we will not be able to build a correct operating system for this hardware, nor can we establish a correct cross-compilation environment.

> If you like to do embedded development, I hope this article can help you.

<!-- 用于实际发布 -->
[img01]:/images/100009/money_maker.png
[img02]:/images/100009/money_maker_board.jpg
[img03]:/images/100009/openwrt_version.png
[img04]:/images/100009/toolschain_test.png
[img05]:/images/100009/wake_on_lan_architecture.png
[img06]:/images/100009/login_openwrt_first.png
[img07]:/images/100009/web_openwrt_1.png
[img08]:/images/100009/web_openwrt_2.png
[img09]:/images/100009/openwrt_fixed_ip_config.png
[img10]:/images/100009/openwrt_fixed_ip_web_1.png
[img11]:/images/100009/openwrt_fixed_ip_web_2.png
[img12]:/images/100009/openwrt_fixed_ip_web_3.png
[img13]:/images/100009/openwrt_fixed_ip_router.png
[img14]:/images/100009/openwrt_run_wakeonlan.png
[img15]:/images/100009/listen_magic_packet.png
[img16]:/images/100009/connectbot_setting.jpg
[img17]:/images/100009/connectbot_login_openwrt.jpg
[img18]:/images/100009/connectbot_run_wakeonlan.jpg
[img19]:/images/100009/luci_wake_on_lan.png
<!-- -->