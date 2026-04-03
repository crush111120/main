在终端执行： gcc eth_pkt_send.c -o eth_pkt_send
然后在终端运行: sudo ./eth_pkt_send enp60s0 <src_ip> <dst_ip> <src_port> <dst_port> <num>　
# gcc eth_recv_pkt.c -o eth_recv_pkt
在终端执行： sudo ./eth_recv_pkt enp60s0 <dst_port>