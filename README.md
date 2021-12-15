# Traceroute
Traceroute program made from scratch using ICMP and RAW sockets

How to run:
1. Compile with: gcc traceroute.c -o traceroute
2. To run use: sudo ./traceroute "website'. For example if we want to visit yahoo.com we would use: sudo ./traceroute www.yahoo.com
**MUST RUN WITH SUDO SINCE USING RAW SOCKETS**

Output:

For each stop in the route the program will display 
1. How many hops it took to get there
2. ICMP message type
3. Time it took to get there
4. IP adrress of the host at that stop
5. Name of the host at that stop if it's available
