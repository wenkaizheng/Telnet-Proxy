# Telnet-Proxy
Server proxy and client proxy for telnet, and it can resume the data even you get lost of connection from wifi

Implement TCP proxy programs:

First get familiar with telnet, which will be used to test your programs. Run telnet on ClientVM to log into ServerVM: 

	telnet sip

where sip is ServerVM’s eth1 IP address. After successful login, you should get a normal Unix shell session. To log out, type “logout”, “exit”, or “ctrl d”.

For this milestone, you will implement a client-side proxy (“cproxy) and a server-side proxy (“sproxy) that will relay traffic between the telnet program and telnet server/daemon. The specific actions to implement are as follows.



	ClientVM	ServerVM









1.	Start sproxy on ServerVM. It should listen on TCP port 6200.

sproxy 6200

2.	Start the cproxy on ClientVM to listen on TCP port 5200, and specify the sproxy:

	cproxy 5200 sip 6200

3.	On ClientVM, telnet to cproxy:
	
telnet localhost 5200

The above command will have telnet open a TCP connection with cproxy. After accepting the connection, cproxy should open a TCP connection to sproxy. Upon accepting this connection, sproxy should open a TCP connection to the telnet daemon on localhost (address 127.0.0.1, port 23). Now, as shown in the above figure, cproxy and sproxy connect telnet and telnet daemon via three TCP connections. 

In order for telnet to work, cproxy and sproxy should relay traffic received from one side to the other side. For example, cproxy has two connected sockets. It should wait for incoming data on both sockets. If either one receives some data, cproxy should read them from the socket, and send them to the other socket. Similarly, sproxy should operate the same way. Since both proxies merely relay data between their sockets without any modification, from user’s point of view, telnet should just work the same way as if it directly connects to the daemon, e.g., type a Unix command and get results displayed on the terminal. 

To be able to wait for input from multiple sockets at the same time, you need to use select( ). A tutorial of select( ) is posted on D2L.

Another required feature is the closing behavior. When the user closes the telnet session, e.g., typing “exit”, “logout”, or ctrl-d, cproxy should close both of its connections, and sproxy should do so too. When recv( ) returns 0 or -1, it means the socket has been closed by the peer or it has an error. In either case the socket should be closed. After closing the sockets, both proxies will go back to the listening mode, waiting for new telnet sessions.

To isolate problems when debugging your proxy programs, you can run telnet---cproxy---daemon first to debug cproxy, then telnet---sproxy---daemon to debug sproxy, and finally put them together as telnet---cproxy---sproxy---daemon and test.

Deliverables: 
•	Source files cproxy.c, sproxy.c, and Makefile that will produce binaries “cproxy” and “sproxy” after typing “make”.

Detecting and handling address changes:

Let’s first establish a telnet session, change IP address, and see what happens.

Telnet from ClientVM directly to ServerVM, which gives you a Unix shell session. Within this session, run a ping command:

	ping localhost

This command runs ping on ServerVM and is pinging ServerVM itself, but the results are transferred and displayed on the ClientVM terminal. This is just a simple way to generate some continuous traffic between ServerVM to ClientVM. As long as the telnet session is working, it will keep displaying the result of each ping at one-second interval with increasing icmp_seq number without any gap, i.e., data loss.

Open another terminal on ClientVM, remove the Client’s eth1 address, say, 1.2.3.4

	sudo ip addr del 1.2.3.4/24 dev eth1

This removes the current IP address from the eth1 interface. You can use the following command to show the interface information before and after the change:

	ip addr show eth1

Once the current address is removed, the ping results in the telnet session will stop immediately. Actually the ping program is still running on ServerVM, but because the TCP connection has been severed by the address removal, the results cannot be transferred to ClientVM. Similarly, no input from the ClientVM can be transferred to ServerVM, so the telnet session appears hanging there with no response.

Give a new, different address to Client’s eth1, say 1.2.3.5:

	sudo ip addr add 1.2.3.5/24 dev eth1

In this project, the new address must have the same first 3 numbers as the old address. The last number of the new address can be any number between 1 and 254, excluding the numbers already used by the ServerVM and the old ClientVM addresses.

Because the existing telnet session was established using the old address, adding a new address doesn’t resume the session, which will still be hanging, not responding to any key stroke. That’s the problem we want to fix. 

Remember to clean up:
•	To terminate the hanging telnet session, type “ctrl ]” then “ctrl d”. 
•	Also log into ServerVM again and kill the ping process.

Although in this project we created the problem by manually removing/changing IP address, this address change actually happens in the real world automatically when you take your laptop to a different network, say, from UAWiFi to Starbucks WiFi, and when it happens, all ongoing TCP connections will stop working.

Our goal is to resume the telnet session after the new address has been added without any modification to the telnet application or the OS. The idea is to use your cproxy, sproxy, and have a protocol between these proxies to detect and handle address changes. With cproxy and sproxy, the telnet session consists of three TCP connections. When the ClientVM’s address changes, it only breaks the connection between cproxy and sproxy; the other two connections are within a host and use address 127.0.0.1 (meaning localhost), which doesn’t change no matter how a host moves. Therefore, the problem is limited to the cproxy---sproxy connection.

1.	In order to detect the address removal, cproxy and sproxy send each other a heartbeat message every second. Both proxy programs treat the missing of three consecutive heartbeats as the indication of connection loss. They should close the disconnected sockets. Use select( ) to implement the one-second timer.

2.	cproxy should try connecting to sproxy again. When the new address is added, such a connection request will go through and the connection between cproxy and sproxy is reestablished using new sockets. Application data should now flow again. In our example, ping results will show up again on ClientVM’s terminal. 

To ensure the new address is available before calling connect( ), you can put the address removal command and address add command in one line separated by a semicolon (;) and run them together.

sudo ip addr del 1.2.3.4/24 dev eth1; sudo ip addr add 1.2.3.5/24 dev eth1

3.	From sproxy’s point of view, when a new connection with cproxy is established, there are two possibilities: (i) it is a brand new telnet session, or (ii) it is the continuation of a previous session after the ClientVM has got a new address. In the former case, sproxy should close the existing connection to the telnet daemon, and open a new connection to have a new telnet session. In the latter case, sproxy should keep using the existing connection to the daemon. To differentiate these two cases, cproxy needs to tell sproxy whether it is a brand new session or not. To achieve this, you can use a random number generated by cproxy as the ID of a session, and include this ID in the heartbeat message. 


To address all these issues, you need to define a packet format for messages between cproxy and sproxy. 
•	There are two types of messages: heartbeat and data. 
•	Every message should have a header and a payload. The header should identify the type of the message, and the length of the payload. 
•	The payload of heartbeat messages should include the session ID (i.e., a random number generated by cproxy at the beginning of a session). Upon the connection between cproxy and sproxy, cproxy should immediately send a heartbeat message. The ID included in this first heartbeat will be used in all the following heartbeats between cproxy and sproxy.
•	The payload of data messages is the telnet application traffic.

A successful project should make a telnet session survive host mobility, i.e., resume it after the new address is added.

Reliable Transfer:
If you wait some time, e.g., 5-10 seconds, before adding the new address, some ping output will be lost because they are sent to the old address that no long exists. This will be shown as a gap in icmp_seq when the session is resumed. This doesn’t affect much in our telnet/ping example, but may break other applications if the lost data is critical. You’re encouraged to fix this problem as follows to get extra credit.

To identify which data have been lost and be able to retransmit them once the session is resumed, we will need a sequence number (SeqN) for every data message, and include an acknowledgement number (AckN)as well, pretty much the same mechanism used by TCP to ensure reliable transfer.

More specifically, in the message header, include two more fields: SeqN and AckN. SeqN counts the number of data messages. AckN is the next data message expected to be received; it effectively acknowledges the receipt of all data messages SeqN < AckN.

Upon a connection establishment between the proxies, cproxy will send a heartbeat to sproxy, then sproxy sends a heartbeat to cproxy. After this initial exchange, both ends know whether there has been any lost data message, and retransmit them if so.

To be able to retransmit, each proxy should maintain a queue (e.g., linked list) of data messages that have been sent out but not acknowledged yet. A message is added to this queue when it is sent out, and removed when it is acknowledged by the other proxy.

Once this functionality is implemented, you should see no gap in icmp_seq even when there is a significant time wait between address removal (e.g., a laptop leaving office) and address addition (e.g., the laptop arrives at home).

