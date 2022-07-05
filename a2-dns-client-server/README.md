# Instructions to run
### 2a.

- UDP Iterative Server
```
gcc dnsserver.c -o dnsserver && ./dnsserver
```

- UDP Client
```
gcc dnsclient.c -o dnsclient && ./dnsclient
```
### 2b.

- TCP Concurrent+UDP Server
```
gcc new_dnsserver.c -o new_dnsserver && ./new_dnsserver
```

- UDP Client
```
gcc dnsclient.c -o dnsclient && ./dnsclient
```

- TCP Client
```
gcc new_dnsclient.c -o new_dnsclient && ./new_dnsclient
```
