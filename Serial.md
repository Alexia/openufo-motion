# Serial Communication Words

Serial communication is human readable, but uses short words along with types represented as strings.

## Word Format

Short words are four characters in length. If a single word is used abbreviate the word. If multiple words are used then use the first two characters of the first two words.

Examples:

    movement => move
    state => stat
    limit switches => lisw

## Transmit

### Version (string)

Sends the motion firmware version defined in `config.h`.

The version number uses Semantic Versioning 2.0.0: https://semver.org/spec/v2.0.0.html

Example:

    vers:0.0.1

### Machine State Update (integer)

Indicates the current state of the state machine logic.

Example, STATE_PARKED_CREDITS:

    stat:1

### Current Movement Information (byte, sent as integer)

(TODO: Finish implementation.)

Byte position is left to right. For the directions 1 indicates movement is traveling in that direction. For the claw 1 indicates the claw is closed.

| Pos | Data                   |
| --- | ---------------------- |
| 7   | Forward                |
| 6   | Backward               |
| 5   | Left                   |
| 4   | Right                  |
| 3   | Up                     |
| 2   | Down                   |
| 1   | Claw (TODO: Implement) |
| 0   | (Reserved)             |

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

### Gantry Speed (byte, sent as integer)

Sent when the motor speed is changed. Multiple are sent at the same time and they can differ based on the set:sp{fb|lr|ud} command.

Example, 10011001:

    spfb:153
    splr:153
    spud:153

### Error (string)

Unrecoverable error states.

| string | Description                |
| ------ | -------------------------- |
| gapa   | The gantry failed to park. |
| clpa   | The claw failed to park.   |

Example, the gantry failed to park:

    erro:gapa

## Receive

### Set Operations

Use the `set` short word to change machine settings. These settings are temporary unless saved to EEPROM with `save`.

An acknowledgement short word `ack`(acknowledge) with be returned with `set` or `fail`.

Example, `set`:

    ack:set

Example, `fail`:

    ack:fail

#### Set Gantry Forward/Backward Speed (byte, sent as integer)

Set the forward/backward speed of the gantry. Note: The machine will enforce its minimum and maximum limits defined for the motor.

Example, 10011001:

    set:spfb:153

#### Set Gantry Left/Right Speed (byte, sent as integer)

Set the left/right speed of the gantry. Note: The machine will enforce its minimum and maximum limits defined for the motor.

Example, 10011001:

    set:splr:153

#### Set Gantry Forward/Backward Speed (byte, sent as integer)

Set the up/down speed of the gantry. Note: The machine will enforce its minimum and maximum limits defined for the motor.

Example, 10011001:

    set:spud:153

#### Set Claw Strength (byte, sent as integer)

Set the strength of the claw.

Example, 10011001:

    set:clst:153

### Save to EEPROM (null, reserved)

Send the short word `save` to save all settings to the EEPROM.
Note: In the future the null value may be replaced with a timestamp for comparison purpose.

Example:

    save:

### Move Gantry, Continuous (string, three characters {f|b|s}{l|r|s}{u|d|s})

Instruct the gantry to `move` continously in the specified direction. This is equivalent to holding the joystick in that direction. The machine will still obey limit switches and stop movement. The movement will not restart after a limit switch is triggerred.
CAUTION! Do not use with the cabinet door open.

The data format is three characters corresponding to the three directions of travel. All directions accept `s` to stop. All three positions must be sent. Invalid values will stop that direction, but this should not be relied on as it is undefined behavior and may change in the future.

| Char. | Direction |
| ----- | --------- |
| f     | forward   |
| b     | backward  |
| l     | left      |
| r     | right     |
| u     | up        |
| d     | down      |
| s     | stop      |

Example, move backward and right:

    move:brs
