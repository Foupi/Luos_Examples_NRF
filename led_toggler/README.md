# LED Toggler

Luos container managing the state of the LED1 of the board hosting it.

## Description

The LED Toggler container is of type `STATE_MOD`.

It can manage the following messages:

* `IO_STATE`: Depending on the message payload, turns on _(if_ `0x01`_)_
or off _(if_ `0x00`_)_ the LED1.

* `ASK_PUB_CMD`: Answers with the current state of the LED1.

The LED Toggler container can send the following message:

* `IO_STATE`: Sent as an answer to an `ASK_PUB_CMD` message. The payload
describes the state of the LED1: `0x01` if it is on, `0x00` if it is
off.
