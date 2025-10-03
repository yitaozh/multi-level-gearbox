//
// Created by Zhou Yitao on 2025-10-01.
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
    int sizeAtIndex(int index);
    Packet* dequeAtIndex(int index);
    int getCurrentIndex();
    void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)
    void getAndIncrementIndex();
    int getCurrentFifoSize();
    bool isCurrentFifoEmpty();
    int size();
    int getLevelPktCnt();
};


#endif //LEVEL_H
