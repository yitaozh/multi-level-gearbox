set enableMultiPath 1
set perflowMP 1

############## Multipathing ###########################

if {$enableMultiPath == 1} {
    $ns rtproto DV
    Agent/rtProto/DV set advertInterval	[expr 2*$sim_end]  
    Node set multiPath_ 1 
    if {$perflowMP != 0} {
	Classifier/MultiPath set perflow_ 1
    }
}
if {$perflowMP == 0} {  
    Agent/TCP/FullTcp set dynamic_dupack_ 0.75
}
# Peixuan 12102019: We probably don't need to care about the multipath
# Peixuan 12102019: Q1, What is this connection_per_pair, what is connection?
set connections_per_pair 1

# Peixuan 12102019: Modified this topology, decrese port# from 8 to 4
set num_ports		2
set num_pod		$num_ports
set num_agr_per_pod	[expr $num_ports/2]
set num_edg_per_pod	[expr $num_ports/2]
set num_cor_per_agg	[expr $num_ports/2]
#set num_host_per_sw 	[expr $num_ports/2]
set num_host_per_sw 	8
set num_cores 		[expr ($num_agr_per_pod)*($num_ports/2)]
set num_host_per_pod 	[expr ($num_host_per_sw)*($num_edg_per_pod)]
set num_hosts		[expr ($num_host_per_sw)*($num_edg_per_pod)*($num_pod)]
set num_edgs		[expr ($num_edg_per_pod)*($num_pod)]
set num_agrs		[expr ($num_agr_per_pod)*($num_pod)]
puts "num_ports:$num_ports num_pod:$num_pod num_agr_per_pod:$num_agr_per_pod num_edg_per_pod:$num_edg_per_pod num_cor_per_agg:$num_cor_per_agg num_host_per_sw:$num_host_per_sw num_cores:$num_cores num_host_per_pod :$num_host_per_pod num_hosts:$num_hosts num_edgs:$num_edgs num_agrs:$num_agrs"
############# Topology #########################

# Peixuan 12102019: Q2, What is this $ctr, I cannot find its definition
#SetTop(int NumHostPerEdge, int NumEdgPerPod, int NumAggPerPod, int NumCorPerAgg,int NumPod, int NumCore)
$ctr settop $num_host_per_sw $num_edg_per_pod $num_agr_per_pod $num_cor_per_agg $num_pod $num_cores

#set hostedg_rate [expr $link_rate]
set hostedg_rate 10
set edgeagg_rate [expr $link_rate];#[expr $link_rate*$num_host_per_sw/$num_agr_per_pod]
set aggcore_rate $edgeagg_rate;#[expr $link_rate*$num_host_per_pod/$num_cores]
puts "hostedg_rate:$hostedg_rate edgeagg_rate:$edgeagg_rate aggcore_rate:$aggcore_rate"

#SetBW(int HostEdgeBW, int EdgeAggBW, int AggCoreBW)
$ctr setbw $hostedg_rate $edgeagg_rate $aggcore_rate

# 02262020 Peixuan: Set bottleneck link rate and delay
set bottoneck_rate 10Gb
set bottoneck_delay 10ns
set other_link_delay 1us

# 20251012 Yitao: turn off debug
set debug 0   ;# 默认关闭

#Define Agg-Core and then Edge and hosts!
#the Node ID matters for the controller :)

#Creating Aggregation Switches
for { set pod 0 } { $pod < $num_pod } { incr pod } {
	for { set index 0 } { $index < $num_agr_per_pod } { incr index } {
		set agr($pod,$index) [$ns node]
	}
}
#Creating Core Switches
for { set index 0 } { $index < $num_cores } { incr index } {
	set core($index) [$ns node]
}

#Creating Edge Switches
for { set pod 0 } { $pod < $num_pod } { incr pod } {
	for { set index 0 } { $index < $num_edg_per_pod } { incr index } {
		set edg($pod,$index) [$ns node]
	}
}

#Creating Hosts
for { set pod 0 } { $pod < $num_pod } { incr pod } {
	for { set edge 0 } { $edge < $num_edg_per_pod } { incr edge } {
		for { set index 0 } { $index < $num_host_per_sw } { incr index } {
			set host($pod,$edge,$index) [$ns node]
		}
	}
}

Queue/RPQ set pfc_threshold_1 $pfc_thr1_host_edg
Queue/RPQ set margin $margin_

# Peixuan12102019: Q3, where is this $switchAlg defined?
#Creating Likns b.w Edges & Hosts
for { set pod 0 } { $pod < $num_pod } { incr pod } {
	for { set edge 0 } { $edge < $num_edg_per_pod } { incr edge } {
		for { set index 0 } { $index < $num_host_per_sw } { incr index } {
			#$ns duplex-link $host($pod,$edge,$index) $edg($pod,$edge) $bottoneck_rate $bottoneck_delay $bottleneckAlg 
			$ns duplex-link $host($pod,$edge,$index) $edg($pod,$edge) [expr $hostedg_rate]Gb $bottoneck_delay $bottleneckAlg 
		}
	}
}

Queue/RPQ set pfc_threshold_1 $pfc_thr1_edg_agg
Queue/RPQ set margin $margin_

#Creating Likns b.w Edges & Agr
for { set pod 0 } { $pod < $num_pod } { incr pod } {
	for { set edge 0 } { $edge < $num_edg_per_pod } { incr edge } {
		for { set index 0 } { $index < $num_agr_per_pod } { incr index } {
			#$ns duplex-link $edg($pod,$edge) $agr($pod,$index) [expr $edgeagg_rate]Gb $other_link_delay $switchAlg
			$ns duplex-link $edg($pod,$edge) $agr($pod,$index) [expr $edgeagg_rate]Gb $other_link_delay $bottleneckAlg  
		}
	}
}
#Creating Likns b.w Cores & Agr
for { set pod 0 } { $pod < $num_pod } { incr pod } {
	for { set index 0 } { $index < $num_agr_per_pod } { incr index } {
		for { set port 0 } { $port < [expr $num_ports/2] } { incr port } {
			set core_index [expr ($index*$num_ports/2)+$port]
			#$ns duplex-link $core($core_index) $agr($pod,$index) [expr $aggcore_rate]Gb $other_link_delay $switchAlg
			$ns duplex-link $core($core_index) $agr($pod,$index) [expr $aggcore_rate]Gb $other_link_delay $bottleneckAlg  
		}
	}
}
set dlink_rate [expr $link_rate*2]
#Set RDQ queue size to 20
#$ns queue-limit $node_(r1) $node_(r2) 500

#############  Agents          #########################
set lambda [expr ($link_rate*$load*1000000000)/($meanFlowSize*8.000/1460.000*1500.00000)]
puts "Arrival: Poisson with inter-arrival [expr 1/$lambda * 1000] ms"
puts "FlowSize: Web Search (DCTCP) with mean = $meanFlowSize"

puts "Setting up connections ..."; flush stdout

set flow_gen 0
set flow_fin 0

set init_fid 0
set tbf 0

for { set pod 0 } { $pod < $num_pod } { incr pod } {
	for { set pod2 0 } { $pod2 < $num_pod } { incr pod2 } {
		if { $pod!=$pod2} {
			for { set edge 0 } { $edge < $num_edg_per_pod } { incr edge } {
				for { set index 0 } { $index < $num_host_per_sw } { incr index } {
					for { set edge2 0 } { $edge2 < $num_edg_per_pod } { incr edge2 } {
						for { set index2 0 } { $index2 < $num_host_per_sw } { incr index2 } {
							set agtagr($pod,$edge,$index,$pod2,$edge2,$index2) [new Agent_Aggr_pair]
							$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) setup $ctr $host($pod,$edge,$index) $host($pod2,$edge2,$index2) [array get tbf] 0 "$pod $edge $index $pod2 $edge2 $index2" $connections_per_pair $init_fid "TCP_pair" $pod $edge $index $pod2 $edge2 $index2
							$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) attach-logfile $flowlog
							if ($debug) {
								puts -nonewline "($pod,$edge,$index,$pod2,$edge2,$index2) "
							}
							set w1 [expr $pod*$num_edg_per_pod*$num_host_per_sw+$edge*$num_host_per_sw+$index]
							set w2 [expr $pod2*$num_edg_per_pod*$num_host_per_sw+$edge2*$num_host_per_sw+$index2]
							$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) set_PCarrival_process  [expr $lambda/($num_hosts - 1)] "$env(HOME)/eval/common/CDF_search.tcl" [expr 17*$w1+1244*$w2] [expr 33*$w1+4369*$w2]
#							$ns at 0.1 "$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) warmup 0.5 5"
							$ns at 1 "$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) init_schedule"
							set init_fid [expr $init_fid + $connections_per_pair];
						}
					}		
				}			
			}
		}
	}
}

# Peixuan12102019: Incast connection by Peixuan
# Peixuan12102019: All the connection have the same receiver as the last host of the last edge switch in the last pod
##set pod2 [expr $num_pod - 1]
##set edge2 [expr $num_edg_per_pod - 1]
##set index2 [expr $num_host_per_sw - 1]

#set pod2 0
#set edge2 0
#set index2 0

#for { set pod 0 } { $pod < $num_pod } { incr pod } {
#	if { $pod!=$pod2 } {
#		for { set edge 0 } { $edge < $num_edg_per_pod } { incr edge } {
#			for { set index 0 } { $index < $num_host_per_sw } { incr index } {

#				set agtagr($pod,$edge,$index,$pod2,$edge2,$index2) [new Agent_Aggr_pair]
#				$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) setup $ctr $host($pod,$edge,$index) $host($pod2,$edge2,$index2) [array get tbf] 0 "$pod $edge $index $pod2 $edge2 $index2" $connections_per_pair $init_fid "TCP_pair" $pod $edge $index $pod2 $edge2 $index2
#				$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) attach-logfile $flowlog
#				puts -nonewline "($pod,$edge,$index,$pod2,$edge2,$index2) "
#				set w1 [expr $pod*$num_edg_per_pod*$num_host_per_sw+$edge*$num_host_per_sw+$index]
#				set w2 [expr $pod2*$num_edg_per_pod*$num_host_per_sw+$edge2*$num_host_per_sw+$index2]

#				$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) set_PCarrival_process  [expr $lambda/($num_hosts - 1)] "$env(HOME)/eval/common/CDF_search.tcl" [expr 17*$w1+1244*$w2] [expr 33*$w1+4369*$w2]
##				$ns at 0.1 "$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) warmup 0.5 5"
#				$ns at 1 "$agtagr($pod,$edge,$index,$pod2,$edge2,$index2) init_schedule"
#				set init_fid [expr $init_fid + $connections_per_pair];							
#			}			
#		}
#	}	
#}



# Peixuan12102019: Q4: All the traffic are across the pods, right?
# Peixuan12102019: $agtagr as the connection, agent pair
# Peixuan12102019: Q5: line 136, what is this for? Set up connection between hosts?
# Peixuan12102019: Q6: What is w1, w2 for? Pod# * edge# * host# + edge index1/2 * host# + index index 1/2? What is this for? An index?





proc time_ {} {
	puts "Date: [clock format [clock seconds]]"	
}

$ns at 0.9 "time_"
