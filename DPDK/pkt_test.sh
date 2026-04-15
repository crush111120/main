#!/bin/bash

#修改绑定信息
ifname="enp17s0" #绑定的网卡名
thread_file="kpktgend_0" #绑定的线程文件名

pktgen_path="/proc/net/pktgen/"

function pgset()
{
	echo $1 > ${pktgen_path}${ifname}

}
if ! `lsmod | grep -q pktgen` ;then
	modprobe pktgen 
	echo "add_device ${ifname}" > ${pktgen_path}${thread_file}
fi


if [ ! -f "/proc/net/pktgen/${ifname}" ]; then
	echo "add_device ${ifname}" > ${pktgen_path}${thread_file}
fi


#端口文件参数
pkt_size_num="1514"
count_num="1000000"
src_mac_num="aa:bb:cc:dd:00:00"
rate_num="1000"	#单位:Mbps
dst_mac_num="66:26:62:00:00:00"
pgset "pkt_size ${pkt_size_num}"
pgset "count ${count_num}"
pgset "src_mac ${src_mac_num}"
pgset "dst_mac ${dst_mac_num}"
pgset "rate ${rate_num}"

echo "start" > ${pktgen_path}pgctrl
cat ${pktgen_path}${ifname}
