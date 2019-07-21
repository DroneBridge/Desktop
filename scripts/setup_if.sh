#!/bin/bash
interface="wlx18a6f716a511"
frequ=2472

ip link set $interface down
iw dev $interface set type managed
ip link set $interface up
iw dev $interface set bitrates legacy-2.4 54
ip link set $interface down
iw dev $interface set type monitor
ip link set $interface up
iw dev $interface set freq $frequ
