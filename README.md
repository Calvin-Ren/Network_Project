# Network_Project
Computer Network Course Project (CST31102): Encapsulation and decapsulation of data packets from the data link layer to the application layer. 

## Structure

### Data Link Layer - ETH

```c
                                +--------------+
                                |Destnation(6B)|
                                +--------------+
                                |  Source(6B)  |
                                +--------------+
                                |   Type(2B)   | 
                                +--------------+
                                |    Payload   |
                                +--------------+
```

### Network Layer - IPv4

```c
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |Version|  IHL  |Type of Service|          Total Length         |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |         Identification        |Flags|      Fragment Offset    |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |  Time to Live |    Protocol   |         Header Checksum       |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |                       Source Address                          |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |                    Destination Address                        |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |                    Options                    |    Padding    |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Transport Layer - UDP

```c
                     +--------+--------+--------+--------+
                     |     Source      |   Destination   |
                     |      Port       |      Port       |
                     +--------+--------+--------+--------+
                     |                 |                 |
                     |     Length      |    Checksum     |
                     +--------+--------+--------+--------+
                     |              Payload              |
                     |          			                   |
                     +-----------------------------------+
```

### Application Layer - DIY

```c
 															  +---------------+
                                |Data Length(2B)|
                                +---------------+
                                |    Payload    |
                                +---------------+
                
```

