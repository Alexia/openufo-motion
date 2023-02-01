# Serial Communication Words

Serial communication is human readable, but uses short words along with types represented as strings.

## Word Format

Short words are four characters in length. If a single word is used abbreviate the word. If multiple words are used then use the first two characters of the first two words.

Examples:

    movement => move
    state => stat
    limit switches => lisw

## Transmit

### Machine State Update (integer)

Example, STATE_PARKED_CREDITS:

    stat:1

### Current Movement Information (byte, sent as integer)

Byte position is left to right. For the directions 1 indicates movement is traveling in that direction. For the claw 1 indicates the claw is closed.

| Pos | Data       |
| --- | ---------- |
| 0   | Forward    |
| 1   | Backward   |
| 2   | Left       |
| 3   | Right      |
| 4   | Up         |
| 5   | Down       |
| 6   | Claw       |
| 7   | (Reserved) |

Example, moving backward and right:

    move:80

### Limit Switch Information (byte, sent as integer)

Byte position is left to right. 1 indicates the switch is engaged.

| Pos | Data        |
| --- | ----------- |
| 0   | Forward     |
| 1   | Backward    |
| 2   | Left        |
| 3   | Right       |
| 4   | Up          |
| 5   | Down        |
| 6   | Claw Open   |
| 7   | Claw Closed |

Example, limit switches backward and right engaged:

    lisw:80

### Player Switch Information (byte, sent as integer)

Byte position is left to right. 1 indicates the switch is engaged.

| Pos | Data       |
| --- | ---------- |
| 0   | Forward    |
| 1   | Backward   |
| 2   | Left       |
| 3   | Right      |
| 4   | (Reserved) |
| 5   | Down       |
| 6   | (Reserved) |
| 7   | (Reserved) |

Example, limit switches backward and right engaged:

    plsw:80

### Internal Switch Information (byte, sent as integer)

Byte position is left to right. 1 indicates the switch is engaged.

| Pos | Data           |
| --- | -------------- |
| 0   | Token Credit   |
| 1   | Service Credit |
| 2   | Program        |
| 3   | Tilt           |
| 4   | Prize Detect   |
| 5   | (Reserved)     |
| 6   | (Reserved)     |
| 7   | (Reserved)     |

Example, limit switches backward and right engaged:

    insw:80

### Prize Detection (bool, sent as integer)

Prize detection after parking and opening the claw.

Example, prize detected:

    prde:1

## Receive
