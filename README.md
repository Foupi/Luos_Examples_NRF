# Luos containers for Nordic PCA10040 boards

This repository contains several containers implemented for the Nordic
PCA10040 boards in the scope of the Luos GATT and Luos Mesh Bridge
projects:

* `gate`: Luos Gate container adapted to the PCA10040 boards and needs
of the Luos GATT and Luos Mesh Bridge projects.

* `led_toggler`: IO state container allowing manipulation of the LED1
of the board hosting it.

* `mesh_bridge`: Container establishing a bridge between two or more
Luos networks over a Bluetooth Mesh network, allowing them to share
routing table entries and send messages to each other.

* `mesh_provisioner`: Container responsible for managing the Bluetooth
Mesh network the Mesh Bridge containers operate in, both by provisioning
and by configuring them.

* `rtb_builder`: Container designed for test purposes, running a
detection when asked to.
