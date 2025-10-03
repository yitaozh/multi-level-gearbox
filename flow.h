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
    int brustness_; // 07102019 Peixuan: control flow brustness level
    static const int DEFAULT_BRUSTNESS = 1000;  // 07102019 Peixuan: control flow brustness level

    int last_departure_round_;
    int insert_level_;
public:
    Flow(int id, float weight);
    Flow(int id, float weight, int brustness); // 07102019 Peixuan: control flow brustness level

    float getWeight() const;
    int getBrustness() const; // 07102019 Peixuan: control flow brustness level
    void setBrustness(int brustness); // 07102019 Peixuan: control flow brustness level
    int getLastDepartureRound() const;
    void setLastDepartureRound(int last_departure_round_);
    void setWeight(float weight);
    int getInsertLevel() const;
    void setInsertLevel(int insert_level_);
};


#endif //FLOW_H
