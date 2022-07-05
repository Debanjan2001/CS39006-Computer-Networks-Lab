# CS39006-Computer-Networks-Lab

## Authors

- Group Number: 25 (CS39006-Spring-2022)
- Members:
  - Pritkumar Godhani (19CS10048)
  - Debanjan Saha (19CS30014)

## Description

This repository contains all the implementations of the networks/socket programming concepts learnt in the Computer Networks course.

The directories present in this repository are as follows:

- `a1-tcp-client-server` : Implementatation of an iterative TCP Server-Client and an iterative UDP Server-Client. Client send a text file to the server. Server counts the number of sentences,  words, and characters in the file and sends them back to the client.

- `a2-dns-client-server` : Implementation of DNS Name Resolution Client-Server using concurrent DNS Server which multiplexes TCP and UDP Sockets using select() call.

- `a3-file-transfer-protocol` : Implementation of a Minimalistic File Transfer Protocol using TCP Connection on custom IP Address and Port  having features of password protected authentication, directory content listing,single/multiple files upload/download etc. 

- `a4-my-reliable-protocol` : Implementation of a MRP Socket to ensure reliable communication using unreliable UDP sockets using a novel message acknowledgement scheme and concurrent threads for handling incoming messages, timeouts, and retransmission of unacknowledged messages.

- `a5-mytraceroute` : Implementation of our version of the linux command 'traceroute' using Raw Sockets and manually handcrafting a UDP packet