//
// Created by Zhou Yitao on 2018-12-04.
//

#ifndef HIERARCHICALSCHEDULING_TASK_H
#define HIERARCHICALSCHEDULING_TASK_H

// will be used in package-send function
#include "queue.h"
using namespace std;

class Flow {
private:
    int flowId;
    float weight;
    int brustness; // 07102019 Peixuan: control flow brustness level
    static const int DEFAULT_BRUSTNESS = 1000;  // 07102019 Peixuan: control flow brustness level

    int lastDepartureRound;
    int insertLevel;
public:
    Flow(int id, float weight);
    Flow(int id, float weight, int brustness); // 07102019 Peixuan: control flow brustness level

    float getWeight() const;
    int getBrustness() const; // 07102019 Peixuan: control flow brustness level
    void setBrustness(int brustness); // 07102019 Peixuan: control flow brustness level

    int getLastDepartureRound() const;
    void setLastDPepartureRound(int lastDepartureRound);

    void setWeight(float weight);

    int getInsertLevel() const;

    void setInsertLevel(int insertLevel);
};


#endif //HIERARCHICALSCHEDULING_TASK_H
