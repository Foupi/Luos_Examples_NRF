# Luos Gate container - Luos GATT / Luos Mesh Bridge

This container is based on the Gate container available on the Luos
examples repository:
<https://github.com/Luos-io/Examples/tree/master/Apps/Gate>
_(at commit feb3f42)_.

A few modifications have been made to its behaviour to match Bluetooth
capabilities of the Nordic PCA10040 boards.

## Physical setup

To ensure correct communication between the board hosting the Gate
container and the pilot PC, they shall be connected with the following
links:

| **PC FUNCTIONALITY** | **BOARD FUNCTIONALITY** | **BOARD PIN** |
| -------------------- | ----------------------- | ------------- |
| GND | GND | GND |
| RX | TX | P0.06 |
| TX | RX | P0.08 |

The pilot PC program interacting with the Gate shall have the following
serial configuration:

| **VARIABLE** | **VALUE** |
| ------------ | --------- |
| Baudrate | 115 200 |
| Word length | 8 |
| Stop bits | 1 |
| Parity | None |
| Hardware Flow Control | None |

## Behavioural modifications

* As establishing a HAL for PCA10040 boards was not an absolute priority
in the Luos GATT and Luos Mesh Bridge projects, the serial module
connecting the Gate container to the pilot PC makes no use of the DMA
module implemented in the boards. Besides, the UART manipulation code
was moved to a separate module.

* In order to ensure easy debugging using a terminal emulator, a DEBUG
mode was implemented. This mode prints additional messages on the serial
link, and removes the JSON printed at each round of the refresh loop.
This mode should only be used with a terminal emulator, and not with
Pyluos.

* As the Bluetooth stacks _(both Low Energy and Mesh)_ implemented by
Nordic for the PCA10040 boards could not handle sending `ASK_PUB_CMD`
messages to all remote containers at a quick _(< 200ms)_ pace, it was
first decided to significantly increase the delay between each round of
the main loop; however, this solution created a delay between sending
a command to the Gate through the terminal emulator and seeing this
command be executed. In order to solve this issue, the state refresh
messages were removed from the Gate loop and placed in a timer callback,
which allowed fast-paced command management without overflowing the
Bluetooth stacks with `ASK_PUB_CMD` messages.

## Luos Mesh Bridge

As the Luos Mesh Bridge project introduced a new container type, new
messages had to be implemented:

* `ext_rtb`: This message sends a routing table extension request to the
Mesh Bridge container. It does not need a payload.

In order to allow the Gate to manage these messages, the
`LUOS_MESH_BRIDGE` macro shall be defined in the configuration.
