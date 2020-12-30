# About the CoreKube Worker

Currently the CoreKube worker exists in two variants:
1. A UDP variant that will be linked with the CoreKube Frontend.
2. An SCTP variant that can be linked to the OpenAirInterface RAN for development and testing purposes.

Additionally, a simple `udp_client` is included with the UDP variant to allow for very basic testing of the UDP variant without the need for the frontent. (This sends a pre-encoded S1SetupRequest and prints the response).

# Compiling

The included Makefile will compile both the SCTP variant, the UDP variant, and the UDP client.
