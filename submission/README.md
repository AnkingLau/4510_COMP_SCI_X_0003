HTTP Web Proxy Server Programming Assignment

Overview

This Assignment is a simple web proxy server that handles HTTP/1.1 GET requests. The proxy is capable of caching web pages and other objects such as images, improving response times and reducing bandwidth usage.

Features

Supports HTTP/1.1 GET requests.

Caches web pages and all types of objects (HTML, images, etc.).

Retrieves objects from the server if not present in the cache.

Serves cached objects to reduce redundant network requests.

Enhances performance by reducing page load times.

Requirements

Python 3.13.

Required libraries:

socket

threading

os

time

Installation & Setup

Clone the repository:

git clone https://github.com/AnkingLau/Assignment_1.git
cd Assignment_1

Run the proxy server:

python Proxy.py

Configure your web browser or network settings to use the proxy server.

How It Works

The client (browser) sends an HTTP GET request to the proxy server.

The proxy server checks if the requested object is in its cache:

If cached, it serves the object directly from the cache.

If not cached, it fetches the object from the original web server, stores it in the cache, and forwards it to the client.

The caching mechanism reduces redundant requests, improving performance.

Usage

Ensure the proxy server is running before making HTTP requests.

Set your browser’s proxy settings to point to the proxy server.

Open a web page; subsequent requests for the same resource will be served from the cache.

Limitations

Only handles HTTP/1.1 GET requests.

Does not support HTTPS (SSL/TLS) requests.

Cache expiration policies are not implemented.

Future Improvements

Support for HTTPS (SSL/TLS) requests.

Implement cache expiration and eviction policies.

Multi-threading for handling multiple client requests efficiently.

