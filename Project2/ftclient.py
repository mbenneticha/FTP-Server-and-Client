###########################################################
#Program: 2
#File Name: ftserve.py
#Course: CS372 INTRO to Computer Networks
#Programmer: Mariam Ben-Neticha
#Date: 3.12.2017
#Description: Implements the server side of a 
#             2-connection client-server file-transfer network application.
#             The server will bind to the port and listen
#             for a TCP connection request from the client.
#             A TCP control connection, P, will be established.
#             Client will send command to server and server will initiate a
#             TCP data connection, Q, to send info or transfer data.
#             Connection Q is terminated immediately after data sent.
#             Server will continue to listen until SIGINT
#             is received and client has terminated Connection P.
#Resources: - Internet Protocol and Support Python
#          Documentation: 
#http://docs.python.org/release/2.6.5/library/internet.html
#           - Lecture slides on FTP connections and sockets
#           - Stackoverflow.com for implementation references
#           and error/debugging solutions
#
###########################################################

#!/usr/bin/env python

import socket
import sys
import struct
import time
import clienthelper
import SocketServer

#validate the correct number of arguments
if (not clienthelper.checkArgs(sys.argv)):
    sys.exit()

#Setup command/control socket connection, P
command_socket = clienthelper.initiateContact(sys.argv)

#Setup data socket connection, Q
data_socket = clienthelper.setupDataConnection(sys.argv)

try:
    clienthelper.makeRequest(command_socket, sys.argv)
    data_socket.handle_request()
    
    # Clean up sockets
    command_socket.close()
    data_socket.server_close()
    sys.exit()
    
except KeyboardInterrupt:
    # Clean up sockets
    command_socket.close()
    data_socket.server_close()
    sys.exit()