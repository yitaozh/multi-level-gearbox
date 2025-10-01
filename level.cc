//
// Created by Zhou Yitao on 2018-12-04.
//

#include "level.h"

Level::Level(): volume_(DEFAULT_VOLUME), current_index_(0) {

    for (int i = 0; i < volume_; i++) {
        fifos_[i] = new PacketQueue;
    }
}

void Level::enque(Packet* packet, int index) {
    // packet.setInsertFifo(index);
    // packet.setFifoPosition(static_cast<int>(fifos[index].size()));
    // hdr_ip* iph = hdr_ip::access(packet);

    fifos_[index]->enque(packet);
    pkt_cnt_++:
}

Packet* Level::deque() {
    Packet *packet;


    if (isCurrentFifoEmpty()) {
        return 0;
    }
    packet = fifos_[current_index_]->deque();
    pkt_cnt_--;
    return packet;
}

int Level::getCurrentIndex() {
    return current_index_;
}

void Level::setCurrentIndex(int index) {
    current_index_ = index;
}

void Level::getAndIncrementIndex() {
    if (current_index_ + 1 < volume_) {
        current_index_++;
    } else {
        current_index_ = 0;
    }
}

bool Level::isCurrentFifoEmpty() {
    return fifos_[current_index_]->length() == 0;
}

int Level::getCurrentFifoSize() {
    return fifos_[current_index_]->length();
}

int Level::size() {
    // get real fifo number
    return sizeof(fifos_) / sizeof(fifos_[0]);
}

int Level::get_level_pkt_cnt() {
    return pkt_cnt_;
}
