# Operation systems and networks
Assignment was to create system with two synchronized servers and multiple clients, where clients can write and read data. In the case of inconsistency, fresher data will be used. When client is trying to connect to unavailable server, it then tries second server.

Done with classmate @Hanzik815

Client is reading from stdin two types of commands: set, sends data to server, and get, reads data from server.

## Syntax
	set <ID> <DATA>
	get <ID>

Where <ID> is integer from 1 to 32 and <DATA> is arbitrary string. Commands are divided by EOL (0x0A).

Command set sends data to server, where they are stored under given ID. Command get prints the data from server to stdout.
<DATA> can have up to 255 characters, everything above 255 is trashed.

User should be warned, whenever data cannot be stored. E.g. Server is unexpectedly unavailable. 

Client side of application reads commands from stdin, until stream of input (or file) ends, then client app ends. When server is closed, clients connected to this server should try to connect to other server (and finish unfinished operations with the first server). 

If none server is available, then client ends with error.

Servers must save data on hard-drive, so they can be restored when everything is disconnected. Stored conflicted data never can be merged, so the older versions are kept.

Communication between client and servers are build on TCP sockets. Communication protocols are designed by you. You can assume synchronized system time on servers.

### Running server
	server <PORT> <SECOND-SERVER-PORT≥ <SECOND-SERVER-ADDRESS>

### Running client
	client <SERVER-PORT> <SERVER-ADDRESS> <BACKUP-SERVER-PORT> <BACKUP-SERVER-ADDRESS>
