//
// Created by Zhou Yitao on 2025-10-01.
//

#ifndef FLOW_H
#define FLOW_H

// will be used in package-send function
#include "queue.h"
using namespace std;

class Flow {
private:
    int flowid_;
    float weight_;
    int burstiness_;
    static const int DEFAULT_BURSTINESS = 1000;

    int last_departure_round_;
    int insert_level_;
public:
    Flow(int id, float weight);
    Flow(int id, float weight, int burstiness); // 07102019 Peixuan: control flow burstiness level

    float getWeight() const;
    int getBurstiness() const; // 07102019 Peixuan: control flow burstiness level
    void setBurstiness(int burstiness); // 07102019 Peixuan: control flow burstiness level
    int getLastDepartureRound() const;
    void setLastDepartureRound(int last_departure_round_);
    void setWeight(float weight);
    int getInsertLevel() const;
    void setInsertLevel(int insert_level_);
};


#endif //FLOW_H
