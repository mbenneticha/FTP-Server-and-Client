###########################################################
#Program: 2
#File Name: ftclienthelper.py
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

import socket
import sys
import struct
import time
import SocketServer
import os.path

#method to set up socket connection, P, with server
def initiateContact(argv):
    tcpIp = sys.argv[1]
    tcpPort = int(sys.argv[2]) 
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        s.connect((tcpIp, tcpPort))
    except socket.error:
        print "Error: Failed to connect. Either host name or command port number is incorrect."
        sys.exit()
    return s


# class definition for handler to get server info via command port
class MyTCPHandler(SocketServer.BaseRequestHandler):
    action = ""
    dataPort = ""
    trans_file_name = ""
    server_host_name = ""
    
    #function that overrides the base-class request handler
    def handle(self):
        self.server_host_name = socket.gethostbyaddr(self.client_address[0])[0]
        
        #self.request is the TCP socket connected to the client
        if self.action == "-l":
            # Get dir list from server
            self.data = self.request.recv(1024).strip()
            self.outputDirList()
        else:
            # Get file from server
            #status == e means error: file not found
            status = self.request.recv(1).strip()
            if (status == "e"):
                print "%s:%d says FILE NOT FOUND" % (self.server_host_name, self.dataPort)
            else:
                if (os.path.isfile(self.trans_file_name)):
                    print "Warning: File already exists. Overwriting anyways."
                    
                try:
                    self.data = self.receiveFile()
                    self.saveTransferedFile()
                    print "File transfer complete."
                except Exception, e:
                    print "Error: Something is wrong with receiving the file. Please try again in 5 seconds."
    
    #function that displays the directory list to the console
    def outputDirList(self):
        strLen = len(self.data)
        cleanDirList = ""
        
        #removes the '\x00' char from payload.
        for char in self.data:
            if char != '\x00':
                cleanDirList += char
        
        dirList = cleanDirList.split(",")
        
        print "Receiving directory structure from %s:%d" % (dirList[0], self.dataPort)
        
        for i in range(1, len(dirList)):
            print dirList[i]
    
    #function that keeps reading data from the server-stream until null found
    def receiveFile(self):
        print "Receiving \"%s\" from %s:%d" % (self.trans_file_name, self.server_host_name, self.dataPort)
        
        data = ''
        foundNull = False
        total_data=[]
    
        data = self.request.recv(100)
        while data:
            total_data.append(data)
            data = self.request.recv(100)
    
        # Return string constructed from data array 
        return ''.join(total_data)
    
    #function that save the requested file contents to file in client folder
    def saveTransferedFile(self):
        saveFile = open(self.trans_file_name, "w")
        saveFile.write(self.data)
        saveFile.close()
        
#function that sets up the TCP data connection
def setupDataConnection(argv):
    MyTCPHandler.action = argv[3]
    MyTCPHandler.dataPort = GetDataPort(argv)
    MyTCPHandler.trans_file_name = argv[4]
    
    HOST, PORT = argv[1], GetDataPort(argv)
    data_socket = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    return data_socket

#function that sends client request to the server
def makeRequest(s, argv):
    sendString = GenerateServerCommand(argv);
    
    try:
        s.send(sendString)
    except:
        print "Error: Server disconnected..."
        sys.exit()

#function that uses user-entered args to create client command request
def GenerateServerCommand(argv):
    if len(argv) == 5:
        return argv[3] + "," + argv[4] + "," + socket.gethostname() + "\n";
    elif len(argv) == 6:
        return argv[3] + "," + argv[5] + "," + argv[4] + "," + socket.gethostname() + "\n";
    else:
        return "";
        
#function that validates arguments user passes into program/request
def checkArgs(argv):
    #ensure correct number of arguments
    if len(argv) != 5 and len(argv) != 6:
        print 'ERROR: Enter one of two valid commands:'
        print 'ftclient <hostname> <server port> <command> <data port>'
        print 'ftclient <hostname> <server port> <command> <file name> <data port>'
        return False

    dataPort = 0
    if len(argv) == 5:
        dataPort = argv[4]

    if len(argv) == 6:
        dataPort = argv[5]

    serverPort = 0
    try:
        # Convert port param into a number
        serverPort = int(argv[2])
        dataPortNumber = int(dataPort)
    except:
        print 'Error: One or both ports are not valid numbers'

    # Check if the server port is within an acceptable numeric range
    if serverPort < 1024 or 65535 < serverPort:
        print 'Error: Server Port has to be in the range 1024 to 65525'
        return False

    # Check if the data port is within an acceptable numeric range
    if dataPortNumber < 1024 or 65535 < dataPortNumber:
        print 'Error: Data Port has to be in the range 1024 to 65525'
        return False

    # Check if supported commands are sent in
    if (argv[3] != '-l') and (argv[3] != '-g'):
        print 'Error: Only -l and -g commands are supported'
        return False

    return True
    
#function that gets the dataPort number from user-entered arguments
def GetDataPort(argv):
    if len(argv) == 5:
        return int(argv[4]);
    elif len(argv) == 6:
        return int(argv[5]);
    else:
        return -1;