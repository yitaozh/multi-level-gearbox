#include <cmath>
#include <sstream>

#include "gearbox-one-level.h"

static class GearboxOneLevelClass : public TclClass {
public:
        GearboxOneLevelClass() : TclClass("Queue/GearboxOneLevel") {}
        TclObject* create(int, const char*const*) {
            // fprintf(stderr, "Created new TCL HCSPL instance\n"); // Debug: Peixuan 07062019
	        return (new GearboxOneLevel);
	}
} class_hierarchical_queue;

GearboxOneLevel::GearboxOneLevel():GearboxOneLevel(NUM_LEVELS) {
}

GearboxOneLevel::GearboxOneLevel(int num_levels) : num_levels_(num_levels) {
    // fprintf(stderr, "Created new gearbox two levels_ instance with volumn = %d\n", num_levels_);
    for (int i = 0; i < num_levels_; i++) {
        levels_.push_back(Level(FIFO_PER_LEVEL));
    }
    current_round_ = 0;
    pkt_count_ = 0;
    global_last_departure_round_ = 0;
}

void GearboxOneLevel::enque(Packet* packet) {   
    
    hdr_ip* iph = hdr_ip::access(packet);
    int pkt_size = packet->hdrlen_ + packet->datalen();

    int departure_round = calTheoreticalDepartureRound(iph, pkt_size);
    global_last_departure_round_ = max(global_last_departure_round_, departure_round);

    Flow* flow = getFlowPtr(iph->flowid());
    int burstiness = flow->getBurstiness();
    if ((departure_round - current_round_) >= burstiness) {
        // fprintf(stderr, "Exceeds maximum brustness, drop the packet from Flow %d\n", iph->saddr()); //
        drop(packet);
        return; 
    }

    int insert_index = departure_round % FIFO_PER_LEVEL;
    flow->setLastDepartureRound(departure_round);

    levels_[HIGHEST_LEVEL].enque(packet, insert_index);
    // fprintf(stderr, "[Q=%p] Enqueue Flow %d Packet at Level %d, Index %d, departure_round: %d at round: %d\n", 
    //     this, iph->flowid(), HIGHEST_LEVEL, insert_index, departure_round, current_round_);
    pkt_count_++;
}

int GearboxOneLevel::calTheoreticalDepartureRound(hdr_ip* iph, int pkt_size) {
    Flow* flow = getFlowPtr(iph->flowid());

    int last_departure_round = flow->getLastDepartureRound();
    last_departure_round = max(current_round_, last_departure_round);

    int departure_round = last_departure_round + flow->getWeight();

    return departure_round;
}

Packet* GearboxOneLevel::deque() {
    if (pkt_count_ == 0) {
        // fprintf(stderr, "Scheduler Empty\n");
        return 0;
    }

    if (pkt_cur_round_.empty()) {
        while (pkt_cur_round_.empty()) {
            runRound();
            // fprintf(stderr, "[Q=%p] Round %d passed with packet number: %d, current queue size: %d, last enqued packet: %d\n", 
            //     this, current_round_, pkt_cur_round_.size(), pkt_count_, global_last_departure_round_);
            current_round_++;
            // if (current_round_ > 400) {
            //     exit(1);
            // }
        }
        current_round_--;
    }

    Packet *p = pkt_cur_round_.front();
    pkt_cur_round_.pop_front();

    pkt_count_--;
    return p;
}

// Peixuan: now we only call this function to get the departure packet in the next round
void GearboxOneLevel::runRound() {
    serveHighestLevel();
}

void GearboxOneLevel::serveHighestLevel() {
    int index = current_round_ % FIFO_PER_LEVEL;

    while (levels_[HIGHEST_LEVEL].sizeAtIndex(index) > 0) {
        Packet* p = levels_[HIGHEST_LEVEL].dequeAtIndex(index);
        if (p == 0)
            break;
        hdr_ip* iph = hdr_ip::access(p);
        pkt_cur_round_.push_back(p);
    }
}

Flow* GearboxOneLevel::getFlowPtr(int fid) {
    if (flowmap_.find(fid) == flowmap_.end()) {
        return insertNewFlowPtr(fid, WEIGHT_LIST[fid % WEIGHT_LIST_LEN], DEFAULT_BURSTINESS);
    }
    return flowmap_[fid];
}

Flow* GearboxOneLevel::insertNewFlowPtr(int fid, int weight, int burstiness) {
    Flow* flow = new Flow(fid, weight, burstiness);
    flowmap_[fid] = flow;
    return flow;
}
