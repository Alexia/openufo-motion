# Serial Communication Words

Serial communication is human readable, but uses short words along with types represented as strings.

## Word Format

Short words are four characters in length.  If a single word is used abbreviate the word.  If multiple words are used then use the first two characters of the first two words.

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

Byte position is left to right.  For the directions 1 indicates movement is traveling in that direction.  For the claw 1 indicates the claw is closed.

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

Byte position is leftmost to rightmost.  1 indicates the switch is engaged.

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

Byte position is leftmost to rightmost.  1 indicates the switch is engaged.

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

Byte position is leftmost to rightmost.  1 indicates the switch is engaged.

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

Sent when the motor speed is changed.  Multiple are sent at the same time and they can differ based on the set:sp{fb|lr|ud} command.

Example, 10011001:

    spfb:153
    splr:153
    spud:153

#### Play Time (integer)

Indicates the remaining play time.  Current only sends when the timer starts counting down.

Example, 10:

    time:10

### Error (string)

Unrecoverable error states.

| string | Description                |
| ------ | -------------------------- |
| gapa   | The gantry failed to park. |
| clpa   | The claw failed to park.   |

Example, the gantry failed to park:

    erro:gapa

## Receive

An acknowledgement short word `ack`(acknowledge) with be returned with the original short word or `fail`.

### Set Operations

Use the `set` short word to change machine settings.  These settings are temporary unless saved to EEPROM with `save`.

Example, `set`:

    ack:set

Example, `fail`:

    ack:fail

#### Set Gantry Forward/Backward Speed (byte, sent as integer)

Set the forward/backward speed of the gantry.  Note: The machine will enforce its minimum and maximum limits defined for the motor.

Example, 10011001:

    set:spfb:153

#### Set Gantry Left/Right Speed (byte, sent as integer)

Set the left/right speed of the gantry.  Note: The machine will enforce its minimum and maximum limits defined for the motor.

Example, 10011001:

    set:splr:153

#### Set Gantry Forward/Backward Speed (byte, sent as integer)

Set the up/down speed of the gantry.  Note: The machine will enforce its minimum and maximum limits defined for the motor.

Example, 10011001:

    set:spud:153

#### Set Claw Strength (byte, sent as integer)

Set the strength of the claw.

Example, 10011001:

    set:clst:153

#### Set Play Time (integer)

Set the maximum play time in seconds.

Example, 10:

    set:time:10

### Save to EEPROM (null, reserved)

(TODO: Implement)

Send the short word `save` to save all settings to the EEPROM.
Note: In the future the null value may be replaced with a timestamp for comparison purpose.

Example:

    save:

### Read EEPROM Settings (byte(s))

(TODO: Implement)

Reads the settings from EEPROM and sends them back.

Example:

    read:

### Move Gantry, Continuous (string, three characters {f|b|s|n}{l|r|s|n}{u|d|s|n})

Instruct the gantry to `move` continously in the specified direction.  This is equivalent to holding the joystick in that direction.  The machine will still obey limit switches and stop movement.  The movement will not restart after a limit switch is triggerred.
CAUTION! Do not use with the cabinet door open.

The data format is three characters corresponding to the three directions of travel.  All directions accept `s` to stop.  All three positions must be sent.  Invalid values will stop that direction, but this should not be relied on as it is undefined behavior and may change in the future.

| Char. | Direction      |
| ----- | -------------- |
| f     | forward        |
| b     | backward       |
| l     | left           |
| r     | right          |
| u     | up             |
| d     | down           |
| s     | stop           |
| n     | null/no change |

Example, move backward, right, and stop up/down:

    move:brs

### Move Gantry, Timed (string, three characters {f|b|s|n}{l|r|s|n}{u|d|s|n}, integer milliseconds)

Instruct the gantry to `move` in the specified direction for a specified about of milliseconds.  The machine will still obey limit switches and stop movement.  The movement will not restart after a limit switch is triggerred.
CAUTION! Do not use with the cabinet door open.

The data format is three characters corresponding to the three directions of travel.  All directions accept `s` to stop.  All three positions must be sent.  Invalid values will stop that direction, but this should not be relied on as it is undefined behavior and may change in the future.

| Char. | Direction      |
| ----- | -------------- |
| f     | forward        |
| b     | backward       |
| l     | left           |
| r     | right          |
| u     | up             |
| d     | down           |
| s     | stop           |
| n     | null/no change |

The movement duration is specified in milliseconds as the third part.  The current maximum allowed is 1000 milliseconds.

Example, move backward, right, and stop up/down for 100 milliseconds

    move:brs:100

### Claw Open/Close (integer)

Close or open the claw.

Example, close the claw:

    claw:1

Example, open the claw:

    claw:0

### Credit Add/Subtract (string)

Add or subtract a credit from the current total, 100 maximum at a time.  Use a + or - sign with the number of credits to add or subtract.  Attempting to send an amount greater than `UINT16_MAX` will result in overflow math.(Don't do this.) The total credits that can be counted is UINT16_MAX or 65,535.

Examples:

    cred:+1
    cred:-1

### Emergency Stop (null)

Calls emergencyStop() which immediately releases all motors and the claw.  The state machine is moved to STATE_ERROR.

Examples:

    emst:

### Clear Error (null)

Clear the state machine and reset to STATE_BOOT.  This is mainly used to clear STATE_ERROR.  Behavior when clearing from other states may have unexpected behavior.

Examples:

    cler:

### Gantry Park (null)

Park the gantry.

Example:

    gapa:

### Claw Park (null)

Park the claw.

Example:

    clpa:

### Status Report

Instructs to return all status information including, but not limited to: `vers`, `stat`, `move`, `insw`, `lisw`, `plsw`, `cred`, `time`
This short word should be used sparingly and mainly used to synchronize the controller state machine to the motion state machine on load.  This operation can cause blocking interrupts.

Example:

    stre:

### Play Start

Transitions the state from STATE_PARKED_CREDITS to STATE_PLAYER_CONTROL to begin the play session.  This is used for remote control from the controller issuing move commands.  This command only works if the current state is STATE_PARKED_CREDITS.
Internally the state machine normally transitions from STATE_PARKED_CREDITS to STATE_PLAYER_CONTROL only when input is detected from the player switches.

Example:

    plst:

### Play End

Transitions the state from STATE_PLAYER_CONTROL to STATE_GRAB to end the play seesion.  This is used for remote control from the controller issuing move commands.  This command only works if the current state is STATE_PLAYER_CONTROL.
Internally the state machine normally transitions from STATE_PLAYER_CONTROL to STATE_GRAB only when input is detected from the player drop switch.

Example:

    plen: