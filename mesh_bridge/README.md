# Mesh Bridge

Luos container linking several Luos networks through a Bluetooth Mesh
network.

## Description

The Mesh Bridge container is of a custom type, named `MESH_BRIDGE_MOD`,
which if not defined to a precise value corresponds to `LUOS_LAST_TYPE`.

It can manage the following messages:

* `MESH_BRIDGE_EXT_RTB_CMD`: Engages a routing table extension
procedure.

* `MESH_BRIDGE_PRINT_INTERNAL_TABLES`: Prints the internal tables on
the debug channel of the host board _(for debugging purposes)_.

* `MESH_BRIDGE_CLEAR_INTERNAL_TABLES`: Clears the local and remote
container tables _(called if new containers or nodes have been added to
the local Luos network)_.

* `MESH_BRIDGE_FILL_LOCAL_CONTAINER_TABLE`: Fills the local container
table with relevant entries of the local routing table _(necessary for
extending the routing table)_.

* `MESH_BRIDGE_UPDATE_INTERNAL_TABLES`: Updates the local IDs of the
entries from the local and remote container tables in prediction of a
detection run by the container with the given ID _(necessary if another
container runs a detection; payload is a container ID as an_
`uint16_t`_)_.

The Mesh Bridge container will send the following messages:

* `MESH_BRIDGE_EXT_RTB_COMPLETE`: Sent after the routing table extension
procedure is complete. If this procedure had been requested by another
container, then this message is sent to the source container as a
response; else, if it was triggered by another Luos network extending
its own routing table, it is sent as a broadcast message to the whole
Luos network.

* `MESH_BRIDGE_INTERNAL_TABLES_CLEARED`: Sent after local and remote
container tables have been cleared.

* `MESH_BRIDGE_LOCAL_CONTAINER_TABLE_FILLED`: Sent after filling the
local container table, contains the number of entries stored _(payload
is the number of entries as an_ `uint16_t`_)_.

* `MESH_BRIDGE_INTERNAL_TABLES_UPDATED`: Sent after the local IDs of
entries from the local and remote container tables have been updated.

The first Mesh Bridge message is indexed at a value named
`MESH_BRIDGE_MSG_BEGIN` which, if not defined, is equal to
`LUOS_PROTOCOL_NB`; a last entry in the command enum, named
`MESH_BRIDDGE_MSG_END`, signals the end of Mesh Bridge command values.

## Container tables

FIXME Local container table
FIXME Remote container table

## Routing table extension

FIXME RTB extension description

## Messages exchange

FIXME MSG exchange description
