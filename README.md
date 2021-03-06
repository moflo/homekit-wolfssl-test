# homekit-wolfssl-test
HomeKit Accessory Emulation for Particle / Arduino Development Boards, testing use of WolfSSL crypto library.

Using recently released HomeKit Non-commercial HAP Specification to create an embedded accessory client. Goal of this project is to test C/C++ based SHA-512 based SRP enabled HAP protocols for eventual porting to Particle.io or Arduino libraries.



Testing
-------

Install Wireshark app, and Bonjour Browser for protocol testing. Save UDP packets on local ethernet channel using the TCPdump command line tool and open the results in Wireshark to test the validity of MDNS packets:

    sudo tcpdump -i en0 -s 0 -w ./test.dmp
    wireshark -r test.dmp -Y mdns

Use Apple supplied "HomeKit Accessory Simulator" to generate valid MDNS HomeKit packets and use the trace methods above to compare packet & response structure. This provides "blackbox" testing, you can observe valid UDP packet structure but we still need a method to compare use of custom crypto libraries (SHA512, etc.)

Likewise, run this HomeKit Emulator MacOS app to compare MDNS packet structure & reponse protocol. This emulator uses native Cocoa MDNS (ie., NetService) function calls to register a HomeKit accessory, and can be observed within the iOS Home app. Importantly, it then uses 3rd party C/C++ libraries for SRP & Crypto functions so that those libraries can be validated before embedding them in Particle / Arduino project libraries, etc.


Protocol
--------

HTTP1.1 based pairing process. Initial request from Home app is a POST request to start pairing process...

Text Dump:

    POST /pair-setup HTTP/1.1
    Host: emulator._hap._tcp.local
    Content-Length: 6
    Content-Type: application/pairing+tlv8

Hex Dump:

    50 4F 53 54 20 2F 70 61 69 72 2D 73 65 74 75 70 20 48 54 54 50 2F 31 2E 31 0D 0A 48 6F 73 74 3A 20 65 6D 75 6C 61 74 6F 72 2E 5F 68 61 70 2E 5F 74 63 70 2E 6C 6F 63 61 6C 0D 0A 43 6F 6E 74 65 6E 74 2D 4C 65 6E 67 74 68 3A 20 36 0D 0A 43 6F 6E 74 65 6E 74 2D 54 79 70 65 3A 20 61 70 70 6C 69 63 61 74 69 6F 6E 2F 70 61 69 72 69 6E 67 2B 74 6C 76 38 0D 0A 0D 0A 00 01 00 06 01 01 00 00


TLV8 Payload:
    00 01 00 06 01 01 00 00


