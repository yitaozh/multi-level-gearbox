#include <cmath>
#include <sstream>

#include "gearbox-two-levels.h"

static class GearboxTwoLevelsClass : public TclClass {
public:
        GearboxTwoLevelsClass() : TclClass("Queue/GearboxTwoLevels") {}
        TclObject* create(int, const char*const*) {
            // fprintf(stderr, "Created new TCL gearbox two levels_ instance\n");
	        return (new GearboxTwoLevels);
	}
} class_hierarchical_queue;

GearboxTwoLevels::GearboxTwoLevels():GearboxTwoLevels(NUM_LEVELS) {
}

GearboxTwoLevels::GearboxTwoLevels(int num_levels) : num_levels_(num_levels) {
    // fprintf(stderr, "Created new gearbox two levels_ instance with volumn = %d\n", num_levels_);
    for (int i = 0; i < num_levels_; i++) {
        levels_.push_back(Level(FIFO_PER_LEVEL));
        levelsB.push_back(Level(FIFO_PER_LEVEL));
    }
    current_round_ = 0;
    pkt_count_ = 0;
}

void GearboxTwoLevels::enque(Packet* packet) {   
    
    hdr_ip* iph = hdr_ip::access(packet);
    int pkt_size = packet->hdrlen_ + packet->datalen();

    int departure_round = calTheoreticalDepartureRound(iph, pkt_size);

    Flow* flow = getFlowPtr(iph->flowid());
    int burstiness = flow->getBurstiness();
    if ((departure_round - current_round_) >= burstiness) {
        // fprintf(stderr, "Exceeds maximum brustness, drop the packet from Flow %d\n", iph->saddr()); //
        drop(packet);
        return; 
    }

    bool inserting_backup = false;
    int insert_level = 0, insert_index = 0;
    calInsertLevel(departure_round, inserting_backup, insert_level, insert_index);

    if (NUM_LEVELS == insert_level) {
        // fprintf(stderr, "Exceeds maximum level, drop the packet from Flow %d\n", iph->saddr()); //
        drop(packet);
        return;
    }

    flow->setLastDepartureRound(departure_round);
    insert_level = max(flow->getInsertLevel(), insert_level);
    flow->setInsertLevel(insert_level);

    if (insert_level == NUM_LEVELS - 1) {
        levels_[insert_level].enque(packet, insert_index);
    } else {
        if (!inserting_backup) {
            levels_[insert_level].enque(packet, insert_index);
        } else {
            levelsB[insert_level].enque(packet, insert_index);
        }
    }
    // fprintf(stderr, "[Q=%p] Enqueue Flow %d Packet at Level %d, Index %d, backup: %d, departure_round: %d\n", 
    //     this, iph->flowid(), insert_level, insert_index, inserting_backup, departure_round);
    pkt_count_++;
}

int GearboxTwoLevels::calTheoreticalDepartureRound(hdr_ip* iph, int pkt_size) {
    Flow* flow = getFlowPtr(iph->flowid());

    int last_departure_round = flow->getLastDepartureRound();
    last_departure_round = max(current_round_, last_departure_round);

    int departure_round = last_departure_round + flow->getWeight();

    return departure_round;
}

void GearboxTwoLevels::calInsertLevel(int departure_round, bool &inserting_backup, int &insert_level, int& insert_index) {
    int current_round = current_round_;
    int diff = insert_level == HIGHER_LEVEL ? 0 : 1;
    while (departure_round / FIFO_PER_LEVEL - current_round / FIFO_PER_LEVEL > diff) {
        insert_level++;
        if (insert_level >= NUM_LEVELS) {
            break;
        }
        departure_round = departure_round / FIFO_PER_LEVEL;
        current_round = current_round / FIFO_PER_LEVEL;
    }
    inserting_backup = departure_round / FIFO_PER_LEVEL % 2;
    insert_index = departure_round % FIFO_PER_LEVEL;
}

Packet* GearboxTwoLevels::deque() {
    if (pkt_count_ == 0) {
        // fprintf(stderr, "Scheduler Empty\n");
        return 0;
    }

    while (pkt_cur_round_.empty()) {
        runRound();
        // fprintf(stderr, "[Q=%p] Round %d passed with packet number: %d\n", 
        //     this, current_round_, pkt_cur_round_.size());
        current_round_++;

        // if (current_round_ > 200) {
        //     exit(1);
        // }
    }

    current_round_--;

    Packet *p = pkt_cur_round_.front();
    pkt_cur_round_.pop_front();

    pkt_count_--;
    return p;
}

// Peixuan: now we only call this function to get the departure packet in the next round
void GearboxTwoLevels::runRound() {

    serveHighestLevel();

    serveLowerLevels();
}

void GearboxTwoLevels::serveHighestLevel() {
    int power = POWERS[HIGHER_LEVEL];
    int index = current_round_ / power % FIFO_PER_LEVEL;
    int divider = power - current_round_ % power;

    int size = int(ceil(levels_[HIGHER_LEVEL].sizeAtIndex(index) * 1.0 / divider));
    while (levels_[HIGHER_LEVEL].sizeAtIndex(index) > 0 && size > 0) {
        Packet* p = levels_[HIGHER_LEVEL].dequeAtIndex(index);
        if (p == 0)
            break;
        hdr_ip* iph = hdr_ip::access(p);
        pkt_cur_round_.push_back(p);
        size--;
    }
}

void GearboxTwoLevels::serveLowerLevels() {
    int level = NUM_LEVELS - 2;
    while (level >= 0) {
        int power = POWERS[level];
        int index = current_round_ / power % FIFO_PER_LEVEL;
        int divider = power - current_round_ % power;

        Level* serving_level = current_round_ / FIFO_PER_LEVEL % 2 ? &levelsB[level] : &levels_[level];
        int size = int(ceil(serving_level->sizeAtIndex(index) * 1.0 / divider));
        while (serving_level->sizeAtIndex(index) > 0 && size > 0) {
            Packet* p = serving_level->dequeAtIndex(index);
            if (p == 0)
                break;
            hdr_ip* iph = hdr_ip::access(p);
            pkt_cur_round_.push_back(p);
            size--;
        }

        level--;
    }
}


Flow* GearboxTwoLevels::getFlowPtr(int fid) {
    if (flowmap_.find(fid) == flowmap_.end()) {
        return insertNewFlowPtr(fid, WEIGHT_LIST[fid % WEIGHT_LIST_LEN], DEFAULT_BURSTINESS);
    }
    return flowmap_[fid];
}

Flow* GearboxTwoLevels::insertNewFlowPtr(int fid, int weight, int burstiness) {
    Flow* flow = new Flow(fid, weight, burstiness);
    flowmap_[fid] = flow;
    return flow;
}
