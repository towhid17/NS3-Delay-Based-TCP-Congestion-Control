from xml.etree import ElementTree as ET 
import sys
et=ET.parse(sys.argv[1])

rx_bits = 0
rx_packets = 0
tx_packets = 0
total_delay = 0.0

for flow in et.findall("FlowStats/Flow"):
    rx_bits += int(flow.get('rxBytes'))
    rx_packets += int(flow.get('rxPackets'))
    tx_packets += int(flow.get('txPackets'))
    total_delay += float(flow.get('delaySum')[:-2])

    sim_start_time = float(flow.get('timeFirstRxPacket')[:-2])
    sim_stop_time = float(flow.get('timeLastRxPacket')[:-2])
        
duration = (sim_stop_time-sim_start_time)*1e-9
print("Throughput = %.3f Mbps" % (rx_bits*8/(duration*1e6),))

total_dropped_packets=tx_packets-rx_packets

print("Delivery Ratio = %.3f " % (rx_packets*1.0/tx_packets,))
print("Drop Ratio = %.3f " % (total_dropped_packets*1.0/tx_packets,))

print("End to End Delay = %.3f ms" % (total_delay*1e-6/rx_packets,))





