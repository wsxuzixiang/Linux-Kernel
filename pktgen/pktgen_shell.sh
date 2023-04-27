#!/bin/bash
# pktgen.conf -- Sample configuration for send on two devices on a UP system

#modprobe pktgen

if [[ `lsmod | grep pktgen` == "" ]];then
   modprobe pktgen
fi

if [[ $1 == "" ]];then
   pktsize=550
else
   pktsize=$1
fi

function pgset() {
    local result

    echo $1 > $PGDEV

    result=`cat $PGDEV | fgrep "Result: OK:"`
    if [ "$result" = "" ]; then
         cat $PGDEV | fgrep Result:
    fi
}

function pg() {
    echo inject > $PGDEV
    cat $PGDEV
}

# On UP systems only one thread exists -- so just add devices
# We use eth1, eth1

echo "Adding devices to run".

PGDEV=/proc/net/pktgen/kpktgend_0
pgset "rem_device_all"
pgset "add_device enp0s5"
pgset "max_before_softirq 1"

# Configure the individual devices
echo "Configuring devices"

PGDEV=/proc/net/pktgen/enp0s5

pgset "clone_skb 1000"
pgset "pkt_size $pktsize"
pgset "src_mac 00:1c:42:4e:10:01"
pgset "flag IPSRC_RND"
pgset "src_min 192.168.1.253"
pgset "src_max 192.168.1.255"
pgset "dst 192.168.1.101"
pgset "dst_mac  1c:57:dc:49:ae:23"
pgset "count 0"

# Time to run

PGDEV=/proc/net/pktgen/pgctrl
echo "pkgsize:$pktsize"
echo "Running... ctrl^C to stop"

pgset "start"

echo "Done"
