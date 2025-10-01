#include <cmath>
#include <sstream>

#include "gearbox_two_levels.h"

static class GearboxTwoLevels : public TclClass {
public:
        GearboxTwoLevels() : TclClass("Queue/GEARBOXTWOLEVELS") {}
        TclObject* create(int, const char*const*) {
            fprintf(stderr, "Created new TCL gearbox two levels instance\n");
	        return (new GearboxTwoLevels);
	}
} class_hierarchical_queue;

GearboxTwoLevels::GearboxTwoLevels():GearboxTwoLevels(DEFAULT_VOLUME) {
}

GearboxTwoLevels::GearboxTwoLevels(int volume_) {
    fprintf(stderr, "Created new gearbox two levels instance with volumn = %d\n", volume_);
    this->weightList[0] = 1;
    this->weightList[1] = 2;
    this->weightList[2] = 3;
    this->weightList[3] = 5;
    this->volume_ = volume_;
    currentRound = 0;
    pktCount = 0;
    typedef std::map<string, Flow*> FlowMap;
    FlowMap flowMap;

}

void GearboxTwoLevels::setCurrentRound(int currentRound) {
    this->currentRound = currentRound;

    level0ServingB = ((int)(currentRound/FIFO_PER_LEVEL)%2);

}

void GearboxTwoLevels::setPktCount(int pktCount) {
    this->pktCount = pktCount;
}

void GearboxTwoLevels::enque(Packet* packet) {   
    
    hdr_ip* iph = hdr_ip::access(packet);
    int pkt_size = packet->hdrlen_ + packet->datalen();


    ///////////////////////////////////////////////////
    // TODO: get theory departure Round
    // You can get flowId from iph, then get
    // "lastDepartureRound" -- departure round of last packet of this flow
    int departureRound = cal_theory_departure_round(iph, pkt_size);

    // 20190626 Yitao
    /* With departureRound and currentRound, we can get the insertLevel, insertLevel is a parameter of flow and we can set and read this variable.
    */

    string key = convertKeyValue(iph->flowid());
    // Not find the current key
    if (flowMap.find(key) == flowMap.end()) {
        this->insertNewFlowPtr(iph->flowid(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
    }


    Flow* currFlow = flowMap[key];
    int insertLevel = currFlow->getInsertLevel();

    departureRound = max(departureRound, currentRound);


    if ((departureRound / (FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL)) >= FIFO_PER_LEVEL) {
        drop(packet);
        return;
    }

    
    //int curFlowID = convertKeyValue(iph->saddrm, iph->daddr)   // use source IP as flow id
    int curBrustness = currFlow->getBrustness();
    if ((departureRound - currentRound) >= curBrustness) {
        drop(packet);
        return; 
    }

    currFlow->setLastDepartureRound(departureRound);     // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped
    this->updateFlowPtr(iph->flowid(), currFlow);
    

    int level0InsertingB = ((int)(departureRound / FIFO_PER_LEVEL) % 2);


    if (departureRound / FIFO_PER_LEVEL - currentRound / FIFO_PER_LEVEL > 1 || insertLevel == 1) {
        if (departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
            currFlow->setInsertLevel(0);
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            decadeLevel.enque(packet, departureRound  % FIFO_PER_LEVEL);
        } else {
            currFlow->setInsertLevel(1);
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[1].enque(packet, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
        }

    } else {
        if (!level0InsertingB) {
            currFlow->setInsertLevel(0);
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[0].enque(packet, departureRound % FIFO_PER_LEVEL);
        } else {
            currFlow->setInsertLevel(0);
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levelsB[0].enque(packet, departureRound % FIFO_PER_LEVEL);
        }
        
    }

    setPktCount(pktCount + 1);
}

// Peixuan: This can be replaced by any other algorithms
int GearboxTwoLevels::cal_theory_departure_round(hdr_ip* iph, int pkt_size) {
    //int		fid_;	/* flow id */
    //int		prio_;
    // parameters in iph
    // TODO

    // Peixuan 06242019
    // For simplicity, we assume flow id = the index of array 'flows'

    string key = convertKeyValue(iph->flowid());    // Peixuan 04212020 fid
    Flow* currFlow = this->getFlowPtr(iph->flowid()); // Peixuan 04212020 fid

    int weightIndex = iph->flowid() % WEIGHT_LIST_LEN;
    int curWeight = weightList[weightIndex]; // 01242022 Peixuan: modifyied to int

    int curLastDepartureRound = currFlow->getLastDepartureRound();
    int curStartRound = max(currentRound, curLastDepartureRound);

    int curDeaprtureRound = (int)(curStartRound + curWeight); // 07072019 Peixuan: basic test

    return curDeaprtureRound;
}

Packet* GearboxTwoLevels::deque() {

    if (pktCount == 0) {
        return 0;
    }

    while (!pktCurRound.size()) { 
        pktCurRound = this->runRound();

        this->setCurrentRound(currentRound + 1); // Update system virtual clock

        level0ServingB = ((int)(currentRound/FIFO_PER_LEVEL)%2);
    }

    Packet *p = pktCurRound.front();
    pktCurRound.erase(pktCurRound.begin());

    setPktCount(pktCount - 1);

    hdr_ip* iph = hdr_ip::access(p);

    return p;
}

// Peixuan: now we only call this function to get the departure packet in the next round
vector<Packet*> GearboxTwoLevels::runRound() {

    vector<Packet*> result;

    // Debug: Peixuan 07062019: Bug Founded: What if the queue is empty at the moment? Check Size!

    //current round done

    vector<Packet*> upperLevelPackets = serveUpperLevel(currentRound);

    // Peixuan
    /*for (int i = 0; i < upperLevelPackets.size(); i++) {
        packetNumRecord.push_back(packetNum);
        packetNum--;
    }*/

    result.insert(result.end(), upperLevelPackets.begin(), upperLevelPackets.end());
    
    /////fprintf(stderr, "Extracting packet\n"); // Debug: Peixuan 07062019


    if (!level0ServingB) {
        Packet* p = levels[0].deque();

        /////fprintf(stderr, "Get packet pointer\n"); // Debug: Peixuan 07062019

        if (!p) {
            ///fprintf(stderr, "No packet\n"); // Debug: Peixuan 07062019
        }

        while (p) {

            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 0: fifo %d\n", currentRound, iph->saddr(), levels[0].getCurrentIndex()); // Debug: Peixuan 07092019

            result.push_back(p);
            p = levels[0].deque();
        }

        levels[0].getAndIncrementIndex();               // Level 0 move to next FIFO
        ///fprintf(stderr, "<<<<<At Round:%d, Level 0 update current FIFO as: fifo %d\n", currentRound, levels[0].getCurrentIndex()); // Debug: Peixuan 07212019

        // 01052020 Peixuan
        bool is_level_1_update = false;

        if (levels[0].getCurrentIndex() == 0) {
            is_level_1_update = true ;          
        }

        if (is_level_1_update) {
            levels[1].getAndIncrementIndex();            // Level 3 move to next FIFO
        }

    } else {
        Packet* p = levelsB[0].deque();

        /////fprintf(stderr, "Get packet pointer\n"); // Debug: Peixuan 07062019

        if (!p) {
            ///fprintf(stderr, "No packet\n"); // Debug: Peixuan 07062019
        }

        while (p) {

            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 0: fifo %d\n", currentRound, iph->saddr(), levelsB[0].getCurrentIndex()); // Debug: Peixuan 07092019

            result.push_back(p);
            p = levelsB[0].deque();
        }

        levelsB[0].getAndIncrementIndex();               // Level 0 move to next FIFO
        ///fprintf(stderr, "<<<<<At Round:%d, Level B 0 update current FIFO as: fifo %d\n", currentRound, levelsB[0].getCurrentIndex()); // Debug: Peixuan 07212019

        
        // 01052020 Peixuan
        bool is_level_1_update = false;

        if (levelsB[0].getCurrentIndex() == 0) {
            is_level_1_update = true ;          
        }

        if (is_level_1_update) {
            levels[1].getAndIncrementIndex();            // Level 3 move to next FIFO
        }
    }

    
    return result;
}

// Peixuan: This is also used to get the packet served in this round (VC unit)
// We need to adjust the order of serving: level0 -> level1 -> level2
vector<Packet*> GearboxTwoLevels::serveUpperLevel(int currentRound) {

    vector<Packet*> result;

    int level0size = 0;
    int level1size = 0;

    int decadelevelsize = 0;

    level1size = levels[1].getCurrentFifoSize();

    decadelevelsize = decadeLevel.getCurrentFifoSize();

    // ToDo: swap the order of serving levels

    // Then: first level 1
    if (currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {

        int size = decadeLevel.getCurrentFifoSize();
        for (int i = 0; i < size; i++) {
            Packet* p = decadeLevel.deque();
            if (p == 0)
                break;
            result.push_back(p);

            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug
        }
        decadeLevel.getAndIncrementIndex();
        
    }
    else {

        if (!levels[1].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levels[1].getCurrentFifoSize() * 1.0 / (FIFO_PER_LEVEL - currentRound % FIFO_PER_LEVEL)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                for (int i = 0; i < size; i++) {
                    Packet* p = levels[1].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug
                    result.push_back(p);
                } 
        }

    }
    
    return result;
}

Flow* GearboxTwoLevels::getFlowPtr(int fid) {

    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow* flow; 
    if (flowMap.find(key) == flowMap.end()) {
        flow = this->insertNewFlowPtr(fid, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020
    }
    flow = this->flowMap[key];
    return flow;
}

Flow* GearboxTwoLevels::insertNewFlowPtr(int fid, int weight, int brustness) { // Peixuan 04212020
    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow* newFlowPtr = new Flow(1, weight, brustness);
    this->flowMap.insert(pair<string, Flow*>(key, newFlowPtr));
    return this->flowMap[key];
}

int GearboxTwoLevels::updateFlowPtr(int fid, Flow* flowPtr) { // Peixuan 04212020
    string key = convertKeyValue(fid);  // Peixuan 04212020
    this->flowMap.insert(pair<string, Flow*>(key, flowPtr));
    return 0;
}

string GearboxTwoLevels::convertKeyValue(int fid) { // Peixuan 04212020
    stringstream ss;
    ss << fid;  // Peixuan 04212020
    string key = ss.str();
    return key; //TODO:implement this logic
}


