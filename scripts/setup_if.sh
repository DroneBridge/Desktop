#!/bin/sh
INTERFACE="wlx18a6f716a511"
FREQU=2472
iw dev $INTERFACE set bitrates legacy-2.4 54
ifconfig $INTERFACE down
iw dev $INTERFACE set monitor none
ifconfig $INTERFACE up
iw dev $INTERFACE set freq $FREQU
