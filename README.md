### kSerial packet format  

| byte 1 | byte 2 | byte 3 | byte 4 | byte 5 | byte 6 | byte 7 |  ...   | byte L + 8 |
| :----: | :----: | :----: | :----: | :----: | :----: | :----: | :----: | :--------: |
|   HK   |   HS   |   TP   |   LN   |   P1   |   P2   |   CK   |   DN   |     ER     |

| name | information                    |
| :--: | ------------------------------ |
|  HK  | header 'K' (75)                |
|  HS  | header 'S' (83)                |
|  TP  | data type (4-bit)              |
|  LN  | data length (12-bit, 0~4095)   |
|  P1  | parameter 1                    |
|  P2  | parameter 2                    |
|  CK  | checksum                       |
|  ... | ...                            |
|  DN  | data                           |
|  ER  | finish '\r' (13)               |

|   type   |   binrary    |
| :------: | ------------ |
|  uint8   | 0x0, 4'b0000 |
|  uint16  | 0x1, 4'b0001 |
|  uint32  | 0x2, 4'b0010 |
|  uint64  | 0x3, 4'b0011 |
|  int8    | 0x4, 4'b0100 |
|  int16   | 0x5, 4'b0101 |
|  int32   | 0x6, 4'b0110 |
|  int64   | 0x7, 4'b0111 |
|  half    | 0x9, 4'b1001 |
|  float   | 0xA, 4'b1010 |
|  double  | 0xB, 4'b1011 |
|    R0    | 0x8, 4'b1000 |
|    R1    | 0xC, 4'b1100 |
|    R2    | 0xD, 4'b1101 |
|    R3    | 0xE, 4'b1110 |
|    R4    | 0xF, 4'b1111 |

more information  
http://kitsprout.logdown.com/posts/883899

### Youtube DEMO
https://youtu.be/MjcuTlffRoM
