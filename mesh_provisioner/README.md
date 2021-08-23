# Mesh Provisioner

Luos container managing the Bluetooth Mesh network linking Mesh Bridge
containers.

## Description

The Mesh Provisioner container is of type `STATE_MOD`.

It can manage the following message:

* `IO_STATE`: Depending on the message payload, starts _(if_ `0x01`_)_
or stops _(if_ `0x00`_)_ scanning for unprovisioned devices.

The Mesh Provisioner container can send the following message:

* `IO_STATE`: Always with a payload value of `0x00`, signals that the
maximum number of nodes accepted in the Bluetooth Mesh network has been
reached. It can be sent in response to an `IO_STATE` message asking to
start scanning for unprovisioned devices, or in broadcast to the whole
Luos network when after configuring the last possible node the Mesh
Provisioner container attempts to start scanning again.

## Device provisioning

The Mesh Provisioner container will provision into a Bluetooth Mesh node
any device exposing the authentication data defined in the
`common/include/luos_mesh_common.h` file.

It uses a static authentication process, which does not need any user
interaction.

## Node configuration

The Mesh Provisioner container configures the "Luos RTB" and "Luos MSG"
models defined in the `common/mesh_models` folder.

It defines their publication and suscribe address as the Luos Bluetooth
Mesh group address, defined in the `common/include/luos_mesh_common.h`
file.
