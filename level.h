//
// Created by Zhou Yitao on 2018-12-04.
//

#ifndef LEVEL_H
#define LEVEL_H

#include "queue.h"
using namespace std;

class Level {
private:
    static const int DEFAULT_VOLUME = 10;
    int volume_;                         // num of fifos in one level
    int current_index_;                   // current serve index
    PacketQueue *fifos_[DEFAULT_VOLUME];

    int pkt_cnt_;

public:
    Level();
    void enque(Packet* packet, int index);
    Packet* deque();
    int getCurrentIndex();
    void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)
    void getAndIncrementIndex();
    int getCurrentFifoSize();
    bool isCurrentFifoEmpty();
    int size();
    int get_level_pkt_cnt();
};


#endif //LEVEL_H
