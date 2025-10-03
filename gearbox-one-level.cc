#include <cmath>
#include <sstream>

#include "gearbox-one-level.h"

static class GearboxOneLevelClass : public TclClass {
public:
        GearboxOneLevelClass() : TclClass("Queue/GearboxOneLevel") {}
        TclObject* create(int, const char*const*) {
            // fprintf(stderr, "Created new TCL GearboxOneLevel instance\n"); // Debug: Peixuan 07062019
	        return (new GearboxOneLevel);
	}
} class_Gearbox_one;

GearboxOneLevel::GearboxOneLevel():GearboxOneLevel(DEFAULT_VOLUME) { }

GearboxOneLevel::GearboxOneLevel(int volume_) {
    // fprintf(stderr, "Created new Gearbox_one instance with volumn = %d\n", volume_); // Debug: Peixuan 07062019
    this->volume_ = volume_;

    current_round_ = 0;
    pkt_count_ = 0;
}

void GearboxOneLevel::enque(Packet* packet) {   
    
    hdr_ip* iph = hdr_ip::access(packet);
    int pkt_size = packet->hdrlen_ + packet->datalen();

    int departure_round = calTheoreticalDepartureRound(iph, pkt_size);

    Flow* flow = flowmap_[iph->flowid()];
    int insert_level = 0;

    // one level gearbox, each level only handles 10 virtual clock
    if ((departure_round - current_round_) >= SET_GRANULARITY * SET_NUMBER) {
        fprintf(stderr, "Exceeds maximum round, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;
    }
   
    int brustness = flow->getBrustness();
    if ((departure_round - current_round_) >= brustness) {
        fprintf(stderr, "Exceeds maximum brustness, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;
    }

    flow->setLastDepartureRound(departure_round);

    int set_id = (departure_round / SET_GRANULARITY) % SET_NUMBER;
    // fprintf(stderr, "departure_round/SET_GRANULARITY = %d/%d = %d\n", departure_round, SET_GRANULARITY, (departure_round/SET_GRANULARITY)); // Debug: Peixuan 07072019
    // fprintf(stderr, "Enqueue Set %d\n", set_id); // Debug: Peixuan 07072019
    int fifo_granularity = SET_GRANULARITY / 10;
    levels_[set_id].enque(packet, (departure_round / fifo_granularity) % SET_GRANULARITY);
    pkt_count_++;
}

// Peixuan: This can be replaced by any other algorithms
int GearboxOneLevel::calTheoreticalDepartureRound(hdr_ip* iph, int pkt_size) {
    // fprintf(stderr, "$$$$$Calculate Departure Round at VC = %d\n", current_round_); // Debug: Peixuan 07062019

    Flow* flow = this->getFlowPtr(iph->flowid()); // Peixuan 04212020 fid

    float weight = flow->getWeight();
    int last_departure_round = flow->getLastDepartureRound();
    last_departure_round = max(current_round_, last_departure_round);

    int departure_round = (int)(last_departure_round + weight); // 07072019 Peixuan: basic test

    return departure_round;
}

Packet* GearboxOneLevel::deque() {

    // fprintf(stderr, "Start Dequeue\n"); // Debug: Peixuan 07062019

    if (pkt_count_ == 0) {
        // fprintf(stderr, "Scheduler Empty\n");
        return 0;
    }

    while (pkt_cur_round_.empty()) {
        runRound();
        current_round_++;
    }

    Packet *p = pkt_cur_round_.front();
    pkt_cur_round_.pop_front();

    pkt_count_--;
    return p;

}

// Peixuan: now we only call this function to get the departure packet in the next round
void GearboxOneLevel::runRound() {

    int cur_set = (current_round_ / SET_GRANULARITY) % SET_NUMBER;    // Find the current serving set
    int index = current_round_ % SET_GRANULARITY;
    // fprintf(stderr, "Serving Set %d\n", cur_set); // Debug: Peixuan 08022019

    if (levels_[cur_set].sizeAtIndex(index) == 0) {
        // fprintf(stderr, "No packet at round %d\n", current_round_); // Debug: Peixuan 07062019
        return;
    }

    while (levels_[cur_set].sizeAtIndex(index)) {
        Packet* p = levels_[cur_set].dequeAtIndex(index);
        pkt_cur_round_.push_back(p);
    }
}

Flow* GearboxOneLevel::getFlowPtr(int fid) { // Peixuan 04212020
    if (flowmap_.find(fid) == flowmap_.end()) {
        return insertNewFlowPtr(fid, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020
    }
    return flowmap_[fid];
}

Flow* GearboxOneLevel::insertNewFlowPtr(int fid, int weight, int brustness) { // Peixuan 04212020
    Flow* flow = new Flow(fid, weight, brustness);
    this->flowmap_[fid] = flow;
    return flow;
}
