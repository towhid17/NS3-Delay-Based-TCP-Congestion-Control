from xml.etree import ElementTree as ET 
import sys
et=ET.parse(sys.argv[1])

sum_throuput_NewReno = 0.0
square_sum_Chd = 0.0
sum_throuput_Chd = 0.0
square_sum_NewReno = 0.0
NewReno_flow_count = 0
Chd_flow_count = 0
totalFlows = 10

for flow in et.findall("FlowStats/Flow"):
    
    sim_start_time = float(flow.get('timeFirstRxPacket')[:-2])
    sim_stop_time = float(flow.get('timeLastRxPacket')[:-2])
    flowId = int(flow.get('flowId'))
        
    duration = sim_stop_time - sim_start_time
    THP = ((int(flow.get('rxBytes'))*8.0)/duration)/1024
    if flowId<=totalFlows:
    	if (flowId%2)==0:
    		Chd_flow_count+=1
    		sum_throuput_Chd += THP
    		square_sum_NewReno += THP * THP
    	else:
    		NewReno_flow_count+=1
    		sum_throuput_NewReno += THP
    		square_sum_Chd += THP * THP
   	

NewReno_fairness = (sum_throuput_NewReno * sum_throuput_NewReno)/ (NewReno_flow_count * square_sum_Chd)
Chd_fairness = (sum_throuput_Chd * sum_throuput_Chd)/ (Chd_flow_count * square_sum_NewReno)

print("Fairness")
print("--------------")
print("TcpChd: "+str(Chd_fairness))
print("TcpNewReno: "+str(NewReno_fairness))






