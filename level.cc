//
// Created by Zhou Yitao on 2025-10-01.
//

#include "level.h"

Level::Level(): Level(DEFAULT_VOLUMN) {
}

Level::Level(int volumn): volumn_(volumn), current_index_(0) {

    for (int i = 0; i < volumn_; i++) {
        fifos_.push_back(new PacketQueue());
    }
}

void Level::enque(Packet* packet, int index) {
    // packet.setInsertFifo(index);
    // packet.setFifoPosition(static_cast<int>(fifos[index].size()));
    // hdr_ip* iph = hdr_ip::access(packet);

    fifos_[index]->enque(packet);
    pkt_cnt_++;
}

Packet* Level::deque() {
    if (isCurrentFifoEmpty()) {
        return 0;
    }
    Packet* packet = fifos_[current_index_]->deque();
    pkt_cnt_--;
    return packet;
}

int Level::sizeAtIndex(int index) {
    return fifos_[index]->length();
}

Packet* Level::dequeAtIndex(int index) {
    if (fifos_[index]->length() == 0) {
        return 0;
    }
    Packet* packet = fifos_[index]->deque();
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
    if (current_index_ + 1 < volumn_) {
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

int Level::getLevelPktCnt() {
    return pkt_cnt_;
}
