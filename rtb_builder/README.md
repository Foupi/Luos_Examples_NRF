# RTB Builder

Luos container running a detection when asked to.

## Description

The RTB Builder container is of a custom type, named `RTB_MOD`, which if
not defined to a precise values corresponds to `LUOS_LAST_TYPE`.

It can manage the following messages:

* `RTB_BUILD`: Runs a detection.

* `RTB_PRINT`: Prints the routing table on the debug channel of the
board.

The RTB Builder container can send the following messages:

* `RTB_COMPLETE`: Sent after the detection is complete.

The first RTB Builder message is indexed at a value named
`RTB_MSG_BEGIN` which, if not defined, is equal to `LUOS_PROTOCOL_NB`; a
last entry, named `RTB_MSG_END`, signals the end of RTB Builder command
values.
