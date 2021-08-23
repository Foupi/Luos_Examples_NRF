# Luos Gate container - Luos GATT / Luos Mesh Bridge

This container is based on the Gate container available on the Luos
examples repository:
<https://github.com/Luos-io/Examples/tree/master/Apps/Gate>
_(at commit feb3f42)_.

A few modifications have been made to its behaviour to match Bluetooth
capabilities of the Nordic PCA10040 boards.

## Physical setup

FIXME Describe physical setup.

## Behavioural modifications

* As establishing a HAL for PCA10040 boards was not an absolute priority
in the Luos GATT and Luos Mesh Bridge projects, the serial module
connecting the Gate container to the pilot PC makes no use of the DMA
module implemented in the boards. Besides, the UART manipulation code
was moved to a separate module.

* FIXME Debug mode.
* FIXME Refresh loop and main loop.

## Luos Mesh Bridge

FIXME New messages implemented.
