### kSerial packet format  

| byte 1 | byte 2 | byte 3 | byte 4 | byte 5 | byte 6 |  ...   | byte L - 2 | byte L - 1 |   byte L   |
| :----: | :----: | :----: | :----: | :----: | :----: | :----: | :--------: | :--------: | :--------: |
|   HK   |   HS   |   L    |   T    |   P1   |   P2   |  ...   |     DN     |     ER     |     EN     |

| name | information |
| :--: | ----------- |
|  HK  | header 'K' (75) |
|  HS  | header 'S' (83) |
|  L   | total length (minimum 8) |
|  T   | data type |
|  P1  | parameter 1 |
|  P2  | parameter 2 |
|  ... | ... |
|  DN  | data |
|  ER  | finish '\r' (13) |
|  EN  | finish '\n' (10) |

more information  
http://kitsprout.logdown.com/posts/883899
