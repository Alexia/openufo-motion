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

(TODO: Implement)

Byte position is left to right. For the directions 1 indicates movement is traveling in that direction. For the claw 1 indicates the claw is closed.

| Pos | Data       |
| --- | ---------- |
| 7   | Forward    |
| 6   | Backward   |
| 5   | Left       |
| 4   | Right      |
| 3   | Up         |
| 2   | Down       |
| 1   | Claw       |
| 0   | (Reserved) |

Example, moving backward and right:

    move:80

### Limit Switch Information (byte, sent as integer)

Byte position is leftmost to rightmost. 1 indicates the switch is engaged.

| Pos | Data       |
| --- | ---------- |
| 7   | Forward    |
| 6   | Backward   |
| 5   | Left       |
| 4   | Right      |
| 3   | Up         |
| 2   | Down       |
| 1   | Claw Open  |
| 0   | Claw Close |

Example, limit switches backward and right engaged:

    lisw:80

### Player Switch Information (byte, sent as integer)

Byte position is leftmost to rightmost. 1 indicates the switch is engaged.

| Pos | Data       |
| --- | ---------- |
| 7   | Forward    |
| 6   | Backward   |
| 5   | Left       |
| 4   | Right      |
| 3   | (Reserved) |
| 2   | Down       |
| 1   | (Reserved) |
| 0   | (Reserved) |

Example, limit switches backward and right engaged:

    plsw:80

### Internal Switch Information (byte, sent as integer)

Byte position is leftmost to rightmost. 1 indicates the switch is engaged.

| Pos | Data           |
| --- | -------------- |
| 7   | Token Credit   |
| 6   | Service Credit |
| 5   | Program        |
| 4   | Tilt           |
| 3   | Prize Detect   |
| 2   | (Reserved)     |
| 1   | (Reserved)     |
| 0   | (Reserved)     |

Example, limit switches backward and right engaged:

    insw:80

### Prize Detection (bool, sent as integer)

Prize detection after parking and opening the claw.

Example, prize detected:

    prde:1

### Gantry Park Started (null)

Indicates that the machine is doing an automated gantry park.

Example, gantry parking:

    gapa:

### Claw Park Started (null)

Indicates that the machine is doing an automated claw park.

Example, claw parking:

    clpa:

### Credit Update (integer)

Sent when the total credits changes.

Example, five credits:

    cred:5

## Receive
