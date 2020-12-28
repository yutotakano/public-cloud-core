# About the CoreKube Worker

Currently the CoreKube worker exists in two variants:
1. A UDP variant that will be linked with the CoreKube Frontend.
2. An SCTP variant that can be linked to the OpenAirInterface RAN for development and testing purposes.

Additionally, a simple `udp_client` is included with the UDP variant to allow for very basic testing of the UDP variant without the need for the frontent. (This sends a pre-encoded S1SetupRequest and prints the response).

# Compiling

## Compiling the SCTP variant

Compile the required files:  
`gcc -c -I lib/core/include -I lib/ -I lib/s1ap/asn1c/ *.c`

Link to form the `corekube_worker`:  
`rm udp_client.o udp_listener.o && ld /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o /usr/lib/x86_64-linux-gnu/crt1.o bin/libcore.so.1 bin/libs1ap.so.1 bin/libs1apasn1c.so.1 -rpath=bin/ -lsctp -lc *.o -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o corekube_worker`

To run, start `./corekube_worker` then run the OpenAirInterface RAN.

## Compiling the UDP variant

Compile the required files:  
`gcc -c -I lib/core/include -I lib/ -I lib/s1ap/asn1c/ *.c`

Link to form the `udp_client` (useful for testing):  
`ld /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o /usr/lib/x86_64-linux-gnu/crt1.o bin/libcore.so.1 bin/libs1ap.so.1 bin/libs1apasn1c.so.1 -rpath=bin/ -lsctp -lc udp_client.o -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o udp_client`

Link to form the `corekube_worker`:  
`rm udp_client.o sctp_listener.o && ld /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o /usr/lib/x86_64-linux-gnu/crt1.o bin/libcore.so.1 bin/libs1ap.so.1 bin/libs1apasn1c.so.1 -rpath=bin/ -lsctp -lc *.o -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o corekube_worker`

To run, start `./corekube_worker` then run either the corekube frontend, or the `./udp_client`.
