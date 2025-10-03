#ifndef QUEUE_GEARBOX_ONE_LEVEL_H
#define QUEUE_GEARBOX_ONE_LEVEL_H

#include "level.h"
#include "flow.h"
#include <deque>
#include <string>
#include <map>

static constexpr int SET_NUMBER = 100;
static constexpr int SET_GRANULARITY = 10;       // TimeStamp Range of each queue set (level.cc)
static constexpr int DEFAULT_VOLUME = 1000;
static constexpr int DEFAULT_WEIGHT = 1;         // 01032019 Peixuan default weight_
static constexpr int DEFAULT_BRUSTNESS = 10000;    // 01032019 Peixuan default brustness_

class GearboxOneLevel : public Queue {
private:
    int volume_;                     // num of levels_ in scheduler
    int current_round_;           // current Round
    int pkt_count_;           // packet count
    Level levels_[SET_NUMBER];        // same queue number with HCS
    std::deque<Packet*> pkt_cur_round_;
    typedef std::map<int, Flow*> FlowMap;
    FlowMap flowmap_;

private:
    void runRound();
    void setPktCount(int);

public:
    GearboxOneLevel();
    explicit GearboxOneLevel(int);
    void enque(Packet*);
    Packet* deque();
    int calTheoreticalDepartureRound(hdr_ip*, int);
    int calInsertLevel(int, int);
};

#endif
