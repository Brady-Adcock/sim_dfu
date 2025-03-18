.. _bluetooth-hci-ipc-sample:

Bluetooth: HCI IPC
##################

Overview
********

This network core image exposes a simple HCI IPC interface over Bluetooth LE. The interface
allows a host to send HCI commands to a remote device and receive HCI events. This image
also synchronizes multiple devices together to allow for the application cores to time-stamp 
their packets with a common time base.

Requirements
************

* A Candi or Trixie board (nrf5340 SoC) with IPC subsystem and Bluetooth LE support
* A second board 

Building and Running
********************

Set your working directory to this file's location and run the following commands:

TODO: update this with correct commands

west build -b <board> -p auto
