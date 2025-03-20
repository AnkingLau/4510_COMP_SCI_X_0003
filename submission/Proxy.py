import socket
import sys
import os
import argparse
import re

BUFFER_SIZE = 1000000

# Argument parser for command-line inputs
parser = argparse.ArgumentParser()
parser.add_argument('hostname', help='The IP Address of the Proxy Server')
parser.add_argument('port', type=int, help='The port number of the proxy server')
args = parser.parse_args()
proxyHost = args.hostname
proxyPort = args.port

# Create a server socket with error handling
try:
    serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    print('Socket created successfully')
except socket.error as e:
    print('Failed to create socket:', e)
    sys.exit(1)

# Bind the server socket to a host and port
try:
    serverSocket.bind((proxyHost, proxyPort))
    print(f'Port {proxyPort} is bound successfully')
except socket.error as e:
    print('Port is already in use:', e)
    sys.exit(1)

# Listen for incoming connections
serverSocket.listen(5)
print(f'Listening on {proxyHost}:{proxyPort}')

while True:
    print('Waiting for a connection...')
    try:
        clientSocket, clientAddress = serverSocket.accept()
        print(f'Connection received from {clientAddress}')
    except socket.error as e:
        print('Failed to accept connection:', e)
        continue

    try:
        message_bytes = clientSocket.recv(BUFFER_SIZE)
        message = message_bytes.decode('utf-8')
        print('Received request:\n' + message)
    except socket.error as e:
        print('Failed to receive request:', e)
        clientSocket.close()
        continue

    requestParts = message.split('\r\n')[0].split()
    if len(requestParts) < 3:
        clientSocket.close()
        continue

    method, URI, version = requestParts[:3]
    print(f'Method: {method}\nURI: {URI}\nVersion: {version}\n')

    # Normalize and sanitize the URI
    URI = re.sub(r'^(?:/?)http(?:s?)://', '', URI, count=1)
    URI = URI.replace('/..', '')  # Prevent directory traversal attack
    resourceParts = URI.split('/', 1)
    hostname = resourceParts[0]
    resource = '/' + resourceParts[1] if len(resourceParts) == 2 else '/'
    print(f'Requested Resource: {resource}')

    # Define cache location
    cacheLocation = f'./cache/{hostname}{resource}'.rstrip('/')
    if cacheLocation.endswith('/'):
        cacheLocation += 'default'
    print(f'Cache location: {cacheLocation}')

    # Serve from cache if available
    if os.path.isfile(cacheLocation):
        with open(cacheLocation, 'rb') as cacheFile:
            cacheData = cacheFile.read()
            clientSocket.sendall(cacheData)
            print('Cache hit! Sent cached response to client')
    else:
        try:
            # Connect to the origin server
            with socket.create_connection((hostname, 80)) as originServerSocket:
                print(f'Connected to origin server: {hostname}')
                
                # Construct and forward request to origin server
                originServerRequest = f'{method} {resource} {version}\r\n'
                originServerRequestHeader = f'Host: {hostname}\r\nConnection: close\r\n\r\n'
                request = originServerRequest + originServerRequestHeader
                print(f'Forwarding request:\n{request}')
                
                originServerSocket.sendall(request.encode())
                response = b""
                
                # Receive response from origin server
                while True:
                    part = originServerSocket.recv(BUFFER_SIZE)
                    if not part:
                        break
                    response += part
                
                # Send response to client
                clientSocket.sendall(response)
                print('Response forwarded to client')
                
                # Cache the response
                os.makedirs(os.path.dirname(cacheLocation), exist_ok=True)
                with open(cacheLocation, 'wb') as cacheFile:
                    cacheFile.write(response)
                    print('Response cached successfully')
                
        except Exception as e:
            print('Error processing request:', e)
    
    clientSocket.close()
