
ICMP CLIENT-SERVER TESTING
==========================

1. SERVER MACHINE (Terminal 1)
------------------------------

Compile:
gcc icmp_reply.c -o icmp_reply

Run:
sudo ./icmp_reply

Expected Output:

ICMP Reply Server Started
Waiting for Echo Requests...

Echo Request Received
From: 192.168.1.20
Echo Reply Sent Successfully


2. CLIENT MACHINE (Terminal 2)
------------------------------

Find the Server IP Address:
hostname -I

Compile:
gcc icmp_request.c -o icmp_request

Run:
sudo ./icmp_request 192.168.1.10

Note:
Replace 192.168.1.10 with the actual IP address
of the server machine.


3. EXPECTED CLIENT OUTPUT
-------------------------

Raw ICMP Socket Created Successfully
ICMP Packet Created
Type       : 8 (Echo Request)
Code       : 0

ICMP Echo Request Sent Successfully
Destination IP: 192.168.1.10

Echo Reply Received Successfully!
From IP : 192.168.1.10
Type    : 0 (Echo Reply)
Code    : 0
Sequence: 1


4. COMMUNICATION FLOW
---------------------

CLIENT
  |
  | ICMP Echo Request (Type 8)
  |
  v
SERVER
  |
  | Receives Echo Request
  | Generates ICMP Echo Reply
  |
  v
CLIENT
  |
  | Receives ICMP Echo Reply (Type 0)
  |
  v
SUCCESSFUL COMMUNICATION


5. REQUIREMENTS
---------------

- Both machines must be reachable over the network.
- Both programs must be executed with sudo.
- Allow incoming ICMP traffic through the server firewall.
- Both machines must support IPv4 raw sockets.
- Use the actual server IP address when running the client.

NOTE:
For testing on the same Linux machine, use 127.0.0.1
as the server IP address.
