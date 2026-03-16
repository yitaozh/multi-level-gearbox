#include <cmath>
#include <sstream>
#include <map>
#include <algorithm>

#include "Gearbox_pl_fid_flex_2level.h"

extern std::map<int, int> flow_max_level;

static class Gearbox_pl_fid_flex_2levelsClass : public TclClass {
public:
        Gearbox_pl_fid_flex_2levelClass() : TclClass("Queue/GearboxPLFL2Levels") {}
        TclObject* create(int, const char*const*) {
            // fprintf(stderr, "Created new TCL HCSPL instance\n"); // Debug: Peixuan 07062019
	        return (new Gearbox_pl_fid_2level);
	}
} class_hierarchical_queue;

Gearbox_pl_fid_2level::Gearbox_pl_fid_2level():Gearbox_pl_fid_2level(DEFAULT_VOLUME) {
    // fprintf(stderr, "Created new HCSPL instance\n"); // Debug: Peixuan 07062019
}

Gearbox_pl_fid_2level::Gearbox_pl_fid_2level(int volume) {
    // fprintf(stderr, "Created new HCSPL instance with volumn = %d\n", volume); // Debug: Peixuan 07062019
    this->volume = volume;
    currentRound = 0;
    pktCount = 0; // 07072019 Peixuan
    for (int i = 0; i < DEFAULT_VOLUME; i++) {
        levels.push_back(Level(FIFO_PER_LEVEL));
        if (i > 0) {
            levelsB.push_back(Level(FIFO_PER_LEVEL));
        }
    }
    //pktCurRound = new vector<Packet*>;
    //12132019 Peixuan
    typedef std::map<string, Flow*> FlowMap;
    FlowMap flowMap;

}

void Gearbox_pl_fid_2level::setCurrentRound(int currentRound) {
    ///fprintf(stderr, "Set Current Round: %d\n", currentRound); // Debug: Peixuan 07062019
    this->currentRound = currentRound;

    level0ServingB = ((int)(currentRound/FIFO_PER_LEVEL)%2);

}

void Gearbox_pl_fid_2level::setPktCount(int pktCount) {
    ///fprintf(stderr, "Set Packet Count: %d\n", pktCount); // Debug: Peixuan 07072019
    this->pktCount = pktCount;
}

void Gearbox_pl_fid_2level::enque(Packet* packet) {

    hdr_ip* iph = hdr_ip::access(packet);
    int pkt_size = packet->hdrlen_ + packet->datalen();

    ///fprintf(stderr, "AAAAA Start Enqueue Flow %d Packet\n", iph->saddr()); // Debug: Peixuan 07062019

    ///////////////////////////////////////////////////
    // TODO: get theory departure Round
    // You can get flowId from iph, then get
    // "lastDepartureRound" -- departure round of last packet of this flow
    int departureRound = cal_theory_departure_round(iph, pkt_size);
    /////fprintf(stderr, "Calculated departure round of this Flow %d Packet, finish time = %d\n", iph->saddr(), departureRound); // Debug: Peixuan 12142019

    ///////////////////////////////////////////////////

    // 20190626 Yitao
    /* With departureRound and currentRound, we can get the insertLevel, insertLevel is a parameter of flow and we can set and read this variable.
    */

    //int flowId = iph->flowid();
    //string key = convertKeyValue(iph->saddr(), iph->daddr());
    string key = convertKeyValue(iph->flowid()); // Peixuan 04212020 fid
    // Not find the current key
    if (flowMap.find(key) == flowMap.end()) {
        //flowMap[key] = Flow_pl(iph->saddr, iph->daddr, 2, 100);
        //insertNewFlowPtr(iph->saddr(), iph->daddr(), 2, 100);
        //this->insertNewFlowPtr(iph->saddr(), iph->daddr(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
        int weight = WEIGHT_LIST[iph->flowid() % WEIGHT_LIST_LEN];
        this->insertNewFlowPtr(iph->flowid(), weight, DEFAULT_BRUSTNESS); // Peixuan 04212020 fid
    }


    Flow* currFlow = flowMap[key];
    int insertLevel = currFlow->getInsertLevel();

    // Update global flow max level
    if (flow_max_level.find(iph->flowid()) == flow_max_level.end()) {
        flow_max_level[iph->flowid()] = insertLevel;
    } else {
        flow_max_level[iph->flowid()] = std::max(flow_max_level[iph->flowid()], insertLevel);
    }

    departureRound = max(departureRound, currentRound);


    if ((departureRound / (FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL)) >= FIFO_PER_LEVEL) {
        int smallestDepartureRound = *lastDepartureRound.begin();
        if (pktCount == 0) {
            currentRound = departureRound;
        };
    }

    if ((departureRound / (FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL)) >= FIFO_PER_LEVEL) {
        drop(packet);
        return;   // 07072019 Peixuan: exceeds the maximum round
    }


    //int curFlowID = convertKeyValue(iph->saddrm, iph->daddr)   // use source IP as flow id
    int curBrustness = currFlow->getBurstiness();
    if ((departureRound - currentRound) >= curBrustness) {
        ///fprintf(stderr, "?????Exceeds maximum brustness, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;   // 07102019 Peixuan: exceeds the maximum brustness
    }

    currFlow->setLastDepartureRound(departureRound + currFlow->getWeight());     // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped
    if (lastDepartureRound.find(departureRound) != lastDepartureRound.end()) {
        lastDepartureRound.erase(lastDepartureRound.find(departureRound)); // Remove the old departure round value
    }
    lastDepartureRound.insert(departureRound + currFlow->getWeight());
    this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid

    ///fprintf(stderr, "At Round: %d, Enqueue Packet from Flow %d with Finish time = %d.\n", currentRound, iph->saddr(), departureRound); // Debug: Peixuan 07072019

    int level0InsertingB = ((int)(departureRound/FIFO_PER_LEVEL)%2);
    //int level1InsertingB = ((int)(departureRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

    //int level2InsertingB = ((int)(departureRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
    //int level3InsertingB = ((int)(departureRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

    // More levels

    ///fprintf(stderr, "Level 3 insert B: %d, Level 2 insert B: %d, Level 1 insert B: %d, Level 0 insert B: %d\n", level3InsertingB, level2InsertingB, level1InsertingB, level0InsertingB); // Debug: Peixuan 07072019

    if (departureRound / FIFO_PER_LEVEL - currentRound / FIFO_PER_LEVEL > 1 || insertLevel == 1) {
        if (departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
            currFlow->setInsertLevel(0);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            decadeLevel.enque(packet, departureRound  % FIFO_PER_LEVEL);
            iph->prio() = 0; // Record insert level
            ///fprintf(stderr, "Enqueue Level 1, decede FIFO, fifo %d\n", departureRound  % 4); // Debug: Peixuan 07072019
        } else {
            currFlow->setInsertLevel(1);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[1].enque(packet, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
            iph->prio() = 1; // Record insert level
            ///fprintf(stderr, "Enqueue Level 1, regular FIFO, fifo %d\n", departureRound / 4 % 4); // Debug: Peixuan 07072019
        }

    } else {
        if (!level0InsertingB) {
            /////fprintf(stderr, "Enqueue Level 0\n"); // Debug: Peixuan 07072019
            currFlow->setInsertLevel(0);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[0].enque(packet, departureRound % FIFO_PER_LEVEL);
            iph->prio() = 0; // Record insert level
            ///fprintf(stderr, "Enqueue Level 0, regular FIFO, fifo %d\n", departureRound % 4); // Debug: Peixuan 07072019
        } else {
            /////fprintf(stderr, "Enqueue Level B 0\n"); // Debug: Peixuan 07072019
            currFlow->setInsertLevel(0);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levelsB[0].enque(packet, departureRound % FIFO_PER_LEVEL);
            iph->prio() = 0; // Record insert level
            ///fprintf(stderr, "Enqueue Level B 0, regular FIFO, fifo %d\n", departureRound % 4); // Debug: Peixuan 07072019
        }

    }

    setPktCount(pktCount + 1);
    ///fprintf(stderr, "Packet Count ++\n");

    ///fprintf(stderr, "Finish Enqueue\n"); // Debug: Peixuan 07062019
}

// Peixuan: This can be replaced by any other algorithms
int Gearbox_pl_fid_2level::cal_theory_departure_round(hdr_ip* iph, int pkt_size) {
    //int		fid_;	/* flow id */
    //int		prio_;
    // parameters in iph
    // TODO

    // Peixuan 06242019
    // For simplicity, we assume flow id = the index of array 'flows'

    ///fprintf(stderr, "$$$$$Calculate Departure Round at VC = %d\n", currentRound); // Debug: Peixuan 07062019

    //int curFlowID = iph->saddr();   // use source IP as flow id
    //int curFlowID = iph->flowid();   // use flow id as flow id
    //string key = convertKeyValue(iph->saddr(), iph->daddr());
    string key = convertKeyValue(iph->flowid());    // Peixuan 04212020 fid
    //Flow_pl currFlow = *flowMap[key];   // 12142019 Peixuan: We have problem here.
    //Flow_pl currFlow = Flow_pl(1, 2, 100);   // 12142019 Peixuan: Debug
    //Flow_pl* currFlow = this->getFlowPtr(iph->saddr(), iph->daddr()); //12142019 Peixuan fixed
    Flow* currFlow = this->getFlowPtr(iph->flowid()); // Peixuan 04212020 fid



    float curWeight = currFlow->getWeight();
    int curLastDepartureRound = currFlow->getLastDepartureRound();
    int curStartRound = max(currentRound, curLastDepartureRound);

    ///fprintf(stderr, "$$$$$Last Departure Round of Flow%d = %d\n",iph->saddr() , curLastDepartureRound); // Debug: Peixuan 07062019
    ///fprintf(stderr, "$$$$$Start Departure Round of Flow%d = %d\n",iph->saddr() , curStartRound); // Debug: Peixuan 07062019

    //int curDeaprtureRound = (int)(curStartRound + pkt_size/curWeight); // TODO: This line needs to take another thought

    // int curDeaprtureRound = (int)(curStartRound + curWeight); // 07072019 Peixuan: basic test

    ///fprintf(stderr, "$$$$$At Round: %d, Calculated Packet From Flow:%d with Departure Round = %d\n", currentRound, iph->saddr(), curDeaprtureRound); // Debug: Peixuan 07062019
    // TODO: need packet length and bandwidh relation
    //flows[curFlowID].setLastDepartureRound(curDeaprtureRound);
    return curStartRound;
}

//06262019 Peixuan deque function of Gearbox:

//06262019 Static getting all the departure packet in this virtual clock unit (JUST FOR SIMULATION PURPUSE!)

Packet* Gearbox_pl_fid_2level::deque() {

    ///fprintf(stderr, "Start Dequeue\n"); // Debug: Peixuan 07062019

    /////fprintf(stderr, "Queue size: %d\n",pktCurRound.size()); // Debug: Peixuan 07062019

    if (pktCount == 0) {
        ///fprintf(stderr, "Scheduler Empty\n"); // Debug: Peixuan 07062019
        return 0;
    }

    //int i = 0; // 07252019 loop debug

    while (!pktCurRound.size()) {   // 07252019 loop debug
    //while (!pktCurRound.size() && i < 1000) {   // 07252019 loop debug
        ///fprintf(stderr, "Empty Round\n"); // Debug: Peixuan 07062019
        pktCurRound = this->runRound();
        ///fprintf(stderr, "Current Round: %d, pkts: %d\n", currentRound, pktCurRound); // Peixuan 01072020
        ///fprintf(stderr, "Current Round: %d, pkts num: %d\n", currentRound, pktCurRound.size()); // Peixuan 01072020

        this->setCurrentRound(currentRound + 1); // Update system virtual clock

        level0ServingB = ((int)(currentRound/FIFO_PER_LEVEL)%2);
        //level1ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
        //level2ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
        //level3ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

        ///fprintf(stderr, "Now update Round: %d, level 0 serving B = %d, level 1 serving B = %d, level 2 serving B = %d, level 3 serving B = %d.\n", currentRound, level0ServingB, level1ServingB, level2ServingB, level3ServingB); // Debug: Peixuan 07062019
        //this->deque();
        //i++; // 07252019 loop debug
        //if (i > 1000) { // 07252019 loop debug
        //    return 0;   // 07252019 loop debug
        //}   // 07252019 loop debug
    }

    Packet *p = pktCurRound.front();
    pktCurRound.erase(pktCurRound.begin());

    setPktCount(pktCount - 1);
    ///fprintf(stderr, "Packet Count --\n");

    hdr_ip* iph = hdr_ip::access(p);
    ///fprintf(stderr, "*****Dequeue Packet p with soure IP: %x\n", iph->saddr()); // Debug: Peixuan 07062019

    // Printing sequence test
    // hdr_tcp* tcph = hdr_tcp::access(p);

    return p;

}

// Peixuan: now we only call this function to get the departure packet in the next round
vector<Packet*> Gearbox_pl_fid_2level::runRound() {

    ///fprintf(stderr, "Run Round\n"); // Debug: Peixuan 07062019

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

//Peixuan: This is also used to get the packet served in this round (VC unit)
// We need to adjust the order of serving: level0 -> level1 -> level2
vector<Packet*> Gearbox_pl_fid_2level::serveUpperLevel(int currentRound) {

    ///fprintf(stderr, "Serving Upper Level\n"); // Debug: Peixuan 07062019

    vector<Packet*> result;

    int level0size = 0;
    int level1size = 0;
    //int level2size = 0;
    //int level3size = 0;
    //int level4size = 0;

    int decadelevelsize = 0;
    //int hundredlevelsize = 0;
    //int thirdlevelsize = 0;
    //int forthlevelsize = 0;

    level1size = levels[1].getCurrentFifoSize();

    decadelevelsize = decadeLevel.getCurrentFifoSize();

    // ToDo: swap the order of serving levels

    //Then: first level 1
    if (currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {

        int size = decadeLevel.getCurrentFifoSize();
            ///fprintf(stderr, ">>>>>At Round:%d, Serve Level 1 Convergence FIFO with fifo: %d, size: %d\n", currentRound, decadeLevel.getCurrentIndex(), size); // Debug: Peixuan 07222019
            for (int i = 0; i < size; i++) {
                Packet* p = decadeLevel.deque();
                if (p == 0)
                    break;
                result.push_back(p);

                hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 1 Convergence FIFO, fifo: %d\n", currentRound, iph->saddr(), decadeLevel.getCurrentIndex()); // Debug: Peixuan 07092019
            }
        decadeLevel.getAndIncrementIndex();

    }
    else {

        if (!levels[1].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levels[1].getCurrentFifoSize() * 1.0 / (FIFO_PER_LEVEL - currentRound % FIFO_PER_LEVEL)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 1 serving order (fixed)
                ///fprintf(stderr, ">>>At Round:%d, Serve Level 1 Regular FIFO with fifo: %d, size: %d\n", currentRound, levels[1].getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    Packet* p = levels[1].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                    ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 1, fifo: %d\n", currentRound, iph->saddr(), levels[1].getCurrentIndex()); // Debug: Peixuan 07092019
                    result.push_back(p);
                }
        }

    }

    return result;
}


// 12132019 Peixuan
//Flow_pl* Gearbox_pl_fid_2level::getFlowPtr(nsaddr_t saddr, nsaddr_t daddr) {
Flow* Gearbox_pl_fid_2level::getFlowPtr(int fid) {

    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow* flow;
    if (flowMap.find(key) == flowMap.end()) {
        //flow = this->insertNewFlowPtr(saddr, daddr, 2, 100);
        //flow = this->insertNewFlowPtr(saddr, daddr, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
        int weight = WEIGHT_LIST[fid % WEIGHT_LIST_LEN];
        flow = this->insertNewFlowPtr(fid, weight, DEFAULT_BRUSTNESS); // Peixuan 04212020
    }
    flow = this->flowMap[key];
    return flow;
}

//Flow_pl* Gearbox_pl_fid_2level::insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness) {
Flow* Gearbox_pl_fid_2level::insertNewFlowPtr(int fid, int weight, int brustness) { // Peixuan 04212020
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    //string key = convertKeyValue(saddr, daddr);
    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow* newFlowPtr = new Flow(1, weight, brustness);
    //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
    this->flowMap.insert(pair<string, Flow*>(key, newFlowPtr));
    //flowMap.insert(pair(key, newFlowPtr));
    //return 0;
    return this->flowMap[key];
}

//int Gearbox_pl_fid_2level::updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr) {
int Gearbox_pl_fid_2level::updateFlowPtr(int fid, Flow* flowPtr) { // Peixuan 04212020
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    //string key = convertKeyValue(saddr, daddr);
    string key = convertKeyValue(fid);  // Peixuan 04212020
    //Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
    //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
    this->flowMap.insert(pair<string, Flow*>(key, flowPtr));
    //flowMap.insert(pair(key, newFlowPtr));
    //return 0;
    return 0;
}

//string Gearbox_pl_fid_2level::convertKeyValue(nsaddr_t saddr, nsaddr_t daddr) {
string Gearbox_pl_fid_2level::convertKeyValue(int fid) { // Peixuan 04212020
    stringstream ss;
    //ss << saddr;
    //ss << ":";
    //ss << daddr;
    ss << fid;  // Peixuan 04212020
    string key = ss.str();
    return key; //TODO:implement this logic
}


