# c-othello-net

## Protocol

### Client query

```
| query code (1 byte) | data length (1 byte) | serialized data (Bencode, JSON, XML...) |
```

#### Query mapping

0. Connect: protocol version + username => null
1. List room: null => room list
2. Create room: null => null
3. Join room: room => null
4. Leave room: null => null
5. Send message: message => null
6. Start game: null => null
7. Play turn: stroke => null (next stroke ?)

#### Client states

* Connected: 1 2 3
* In room: 4 5 6
* In game: 5 7

### Server reply

```
| reply code - status (1 byte) | data length (1 byte) | serialized data (Bencode, JSON, XML...) |
```

#### Reply mapping

0. Success
1. Failure: server is full
2. Failure: room is full
3. Failure: bad stroke
