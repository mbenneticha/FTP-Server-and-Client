README.txt
Program 2
File Name: README.txt
Course: CS372 INTRO to Computer Networks
Programmer: Mariam Ben-Neticha
Date: 3.12.17

CLIENT:
** ftclient is implemented in Python **
** ftclient works with ftclienthelper.py **
* This program can run on any flip server *
* Client and Server must be run on the SAME flip server [i.e. flip1.engr.oregonstate.edu] *
* No specified testing machine *
* References for ftclient can be found in header of ftclient.py *

How to Run the Client:
1. Compile and run the chatserve server by entering the following into the command line:
		'python ./ftclient.py <HOST_NAME> <SERVER_PORT> <COMMAND> [FILENAME] <DATA_PORT>'
		Example: 'python ./ftclient.py flip1.engr.oregonstate.edu 30333 -l 30336'
2. Continue making requests to server as desired. Requested content will be displayed in console
	or a message will be displayed conveying the success of the request.
3. Multiple clients can send requests to the same server on different ports. [Multi-threading allowed]


SERVER:
** ftserve is implemented in C **
* This program can run on any flip server *
* Client and Server must be run on the SAME flip server [i.e. flip1.engr.oregonstate.edu] *
* No specified testing machine *
* References for ftserve can be found in header of ftserve.c *

How to Run the Server:
1. Compile the ftserver by running the following into the command line:
		gcc -o ftserver ftserver.c
2. Run the server by entering the following:
		ftserver <SERVER_PORT>
		Example: ftserver 30333
3. Server will wait for connections and display connections established.


Extra Credit:
	-	The program runs on ANY OSU flip server.
	- 	Allows for multi-threading