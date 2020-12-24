# About the CoreKube Worker

Currently the CoreKube worker exists in two variants:
1. A UDP variant that will be linked with the CoreKube Frontend.
2. An SCTP variant that can be linked to the OpenAirInterface RAN for development and testing purposes.

Additionally, a simple `udp_client` is included with the UDP variant to allow for very basic testing of the UDP variant without the need for the frontent. (This sends a pre-encoded S1SetupRequest and prints the response).

# Compiling

## Dependencies

Ensure that NextEPC is [installed from source](https://nextepc.org/installation/). Setup the `$NEXTEPC_FOLDER` environment variable to point to the base of the nextepc source folder. (e.g: `NEXTEPC_FOLDER=~/nextepc/` if built in the home folder).

Symlink the three required libraries from the nextepc folder into the main library folder (`/lib/x86_64-linux-gnu/`):  
`sudo ln -s $NEXTEPC_FOLDER/install/lib/nextepc/libcore.so.1 /lib/x86_64-linux-gnu/ && sudo ln -s $NEXTEPC_FOLDER/install/lib/nextepc/libs1ap.so.1 /lib/x86_64-linux-gnu/ && sudo ln -s $NEXTEPC_FOLDER/install/lib/nextepc/libs1apasn1c.so.1 /lib/x86_64-linux-gnu/`

## Compiling the SCTP variant

Compile the required files:  
`gcc -c -I $NEXTEPC_FOLDER/lib/core/include -I $NEXTEPC_FOLDER/lib/ -I $NEXTEPC_FOLDER/lib/s1ap/asn1c/ *.c`

Link to form the `corekube_worker`:  
`rm udp_client.o udp_listener.o && ld /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o /usr/lib/x86_64-linux-gnu/crt1.o $NEXTEPC_FOLDER/install/lib/nextepc/libcore.so $NEXTEPC_FOLDER/install/lib/nextepc/libs1ap.so $NEXTEPC_FOLDER/install/lib/nextepc/libs1apasn1c.so -lsctp -lc *.o -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o corekube_worker`

To run, start `./corekube_worker` then run the OpenAirInterface RAN.

## Compiling the UDP variant

Compile the required files:  
`gcc -c -I $NEXTEPC_FOLDER/lib/core/include -I $NEXTEPC_FOLDER/lib/ -I $NEXTEPC_FOLDER/lib/s1ap/asn1c/ *.c`

Link to form the `udp_client` (useful for testing):  
`ld /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o /usr/lib/x86_64-linux-gnu/crt1.o $NEXTEPC_FOLDER/install/lib/nextepc/libcore.so $NEXTEPC_FOLDER/install/lib/nextepc/libs1ap.so $NEXTEPC_FOLDER/install/lib/nextepc/libs1apasn1c.so -lsctp -lc udp_client.o -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o udp_client`

Link to form the `corekube_worker`:  
`rm udp_client.o sctp_listener.o && ld /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o /usr/lib/x86_64-linux-gnu/crt1.o $NEXTEPC_FOLDER/install/lib/nextepc/libcore.so $NEXTEPC_FOLDER/install/lib/nextepc/libs1ap.so $NEXTEPC_FOLDER/install/lib/nextepc/libs1apasn1c.so -lsctp -lc *.o -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o corekube_worker`

To run, start `./corekube_worker` then run either the corekube frontend, or the `./udp_client`.