# nonlocality
Tool for tunneling IPv4 TCP connections to hosts behind firewall. Has little to do with performance or good coding practices.

## Build
Build with cmake:
```
git clone git@github.com:szpght/nonlocality.git
cd nonlocality
cmake .
make
cp src/nonlocality-server src/nonlocality-client /somewhere/convenient
```

## Usage
```
nonlocality-server <tunneled port> <control port> <data port>
nonlocality-client <server ip> <dest ip> <tunneled port> <control port> <data port>
```

All connections to server at `<tunneled port>` will be forwarded to client 
and consecutively to `<dest ip>:<tunneled port>`. `<control port>` and `<data port>`
are used for communication between server and client.
