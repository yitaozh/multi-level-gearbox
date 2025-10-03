//
// Created by Zhou Yitao on 2025-10-01.
//

#include "flow.h"

Flow::Flow(int id, float weight) {
    this->flowid_ = id;
    this->weight_ = weight;
    this->brustness_ = DEFAULT_BRUSTNESS;
    this->insert_level_ = 0;
    this->last_departure_round_ = 0;
}

Flow::Flow(int id, float weight, int brustness) {
    this->flowid_ = id;
    this->weight_ = weight;
    this->brustness_ = brustness;
    this->insert_level_ = 0;
    this->last_departure_round_ = 0;
}

int Flow::getLastDepartureRound() const {
    return last_departure_round_;
}

void Flow::setLastDepartureRound(int last_departure_round) {
    Flow::last_departure_round_ = last_departure_round;
}

float Flow::getWeight() const {
    return weight_;
}

void Flow::setWeight(float weight) {
    Flow::weight_ = weight;
}

int Flow::getInsertLevel() const {
    return insert_level_;
}

void Flow::setInsertLevel(int insert_level) {
    Flow::insert_level_ = insert_level;
}

int Flow::getBrustness() const {
    return brustness_;
}


void Flow::setBrustness(int brustness) {
    Flow::brustness_ = brustness;
}
