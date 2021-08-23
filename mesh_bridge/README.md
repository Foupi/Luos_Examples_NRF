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
`MESH_BRIDGE_MSG_END`, signals the end of Mesh Bridge command values.

## Container tables

In order to keep track of whether each container entry of the routing
table represents a container hosted by the local or a remote Luos
network, the Mesh Bridge container implements two data structures
containing data about local and remote containers.

The **local container table** stores information about local containers
exposed to other Luos networks:
* The _routing table entry_ exposed to other networks, as a
`routing_table_t`.
* The _id of the container_ on the local network, as an `uint16_t`.

Some kinds of containers can be excluded from this table when it is
filled: for instance, exposing the Mesh Bridge container would be
redundant.

The **remote container table** stores information about remote
containers exposed by other Luos networks:
* The _routing table entry_ exposed by the remote network, as a
`routing_table_t`.
* The _Bluetooth Mesh unicast address_ of the Bluetooth Mesh node
hosting the Mesh Bridge container instance in the remote network, as an
`uint16_t`.
* The _id of the container_ on the local network, as an `uint16_t`.
* The _local container instance_, as a `container_t*`.

Good management of these tables through Luos messages is crucial to the
behaviour of an application:

* Before engaging the routing table extension procedure, the local
container table **must** have been filled using the
`MESH_BRIDGE_FILL_LOCAL_CONTAINER_TABLE` message; else, local containers
could not be exposed and message exchanges would not be possible.

* When a container is to run a detection, the container tables **must**
be updated beforehand using a `MESH_BRIDGE_UPDATE_INTERNAL_TABLES`
message containing the ID of the container running the detection; else,
the local IDs stored in the internal tables may become wrong, which
will trigger malfunctions when trying to send messages to remote
containers.

* When new containers or nodes are added to the Luos network:
  * The container tables **must** be cleared using the
`MESH_BRIDGE_CLEAR_INTERNAL_TABLES` message _before_ a detection is run;
else, the behaviour of the remote container table could be severaly
affected.
  * The local container table **must** be filled using the
`MESH_BRIDGE_FILL_LOCAL_CONTAINER_TABLE` message _after_ the internal
tables are cleared and the detection is run; else, as the routing table
does not differentiate local and remote containers, remote containers
might be exposed as local ones, which would result in undefined
behaviour.

## Routing table extension

FIXME RTB extension description

## Messages exchange

FIXME MSG exchange description
