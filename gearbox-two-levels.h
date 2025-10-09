#ifndef QUEUE_GEARBOX_TWO_LEVELS_H
#define QUEUE_GEARBOX_TWO_LEVELS_H

#include "level.h"
#include "flow.h"
#include <deque>
#include <map>

static const int NUM_LEVELS = 2;
static const int HIGHEST_LEVEL = NUM_LEVELS - 1;
static const int FIFO_PER_LEVEL = 16;
static const int POWERS[2] = {1, FIFO_PER_LEVEL};

// TODO(yitao): not implemented for now
static const int STEP_DOWN_FIFO = 8;
/**
 * A new flow can send a certain amount of data (related to the value 1000) before 
 * it is considered "non-bursty" and its packets are placed in lower priority queues,
 * resulting in higher latency.
 */
static const int DEFAULT_BURSTINESS = 1000;
static const int WEIGHT_LIST_LEN = 4;
static const int WEIGHT_LIST[WEIGHT_LIST_LEN] = {1, 2, 3, 5};

class GearboxTwoLevels : public Queue {
private:
    int num_levels_; 
    int current_round_;
    int pkt_count_;

    std::vector<Level> levels_;
    std::vector<Level> levelsB;

    std::deque<Packet*> pkt_cur_round_;
    typedef std::map<int, Flow*> FlowMap;
    FlowMap flowmap_;

    // Debug(yitao)
    int global_last_departure_round_;

private:
    void runRound();
    Flow* getFlowPtr(int fid);
    Flow* insertNewFlowPtr(int fid, int weight, int burstiness);
    void calInsertLevel(int, bool&, int&, int&);
    int calTheoreticalDepartureRound(hdr_ip*, int);
    void serveHighestLevel();
    void serveLowerLevels();

public:
    GearboxTwoLevels();
    explicit GearboxTwoLevels(int);
    void enque(Packet*);
    Packet* deque();
    int calInsertLevel(int, int);
};

#endif