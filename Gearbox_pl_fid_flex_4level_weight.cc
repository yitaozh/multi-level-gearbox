#include <cmath>
#include <sstream>

#include "Gearbox_pl_fid_flex_4level.h"

static class Gearbox_pl_fid_flexClass : public TclClass {
public:
        Gearbox_pl_fid_flexClass() : TclClass("Queue/GearboxPLFL") {}
        TclObject* create(int, const char*const*) {
            fprintf(stderr, "Created new TCL HCSPL instance\n"); // Debug: Peixuan 07062019
	        return (new Gearbox_pl_fid_flex);
	}
} class_hierarchical_queue;

Gearbox_pl_fid_flex::Gearbox_pl_fid_flex():Gearbox_pl_fid_flex(DEFAULT_VOLUME) {
    fprintf(stderr, "Created new HCSPL instance\n"); // Debug: Peixuan 07062019
}

Gearbox_pl_fid_flex::Gearbox_pl_fid_flex(int volume) {
    fprintf(stderr, "Created new HCSPL instance with volumn = %d\n", volume); // Debug: Peixuan 07062019
    // 01242022 Peixuan: modifyied to int
    this->weightList[0] = 1;
    this->weightList[1] = 2;
    this->weightList[2] = 3;
    this->weightList[3] = 5;
    this->volume = volume;
    currentRound = 0;
    pktCount = 0; // 07072019 Peixuan
    //pktCurRound = new vector<Packet*>;
    //12132019 Peixuan
    typedef std::map<string, Flow_pl*> FlowMap;
    FlowMap flowMap;

    ///fprintf(stderr, "Gearbox has: %d levels, %d fifos in level 0\n", sizeof(levels)/sizeof(levels[0]), levels[0].size()); // Debug: Peixuan 01072020
    ///fprintf(stderr, "Gearbox has: %d fifos in level 1, %d fifos in level 2, %d fifos in level 3, %d fifos in level 4\n", levels[1].size(), levels[2].size(), levels[3].size(), levels[4].size()); // Debug: Peixuan 01072020
    ///fprintf(stderr, "Gearbox has: %d levels B, %d fifos in level 0 B\n", sizeof(levelsB)/sizeof(levelsB[0]), levelsB[0].size()); // Debug: Peixuan 01072020
    ///fprintf(stderr, "Gearbox has: %d fifos in level 1 B, %d fifos in level 2 B, %d fifos in level 3 B, %d fifos in level 4 B\n", levelsB[1].size(), levelsB[2].size(), levelsB[3].size(), levelsB[4].size()); // Debug: Peixuan 01072020
    ///fprintf(stderr, "Gearbox has: %d fifos in decade level, %d fifos in hundred level 2, %d fifos in third level\n", decadeLevel.size(), hundredLevel.size(), thirdLevel.size()); // Debug: Peixuan 01072020
    ///fprintf(stderr, "Gearbox has: %d fifos in decadeB level, %d fifos in hundredB level 2, %d fifos in thirdB level\n", decadeLevelB.size(), hundredLevelB.size(), thirdLevelB.size()); // Debug: Peixuan 01072020
    

    //typedef std::map<int, int> TestIntMap;
    //TestIntMap testIntMap;
}

void Gearbox_pl_fid_flex::setCurrentRound(int currentRound) {
    ///fprintf(stderr, "Set Current Round: %d\n", currentRound); // Debug: Peixuan 07062019
    this->currentRound = currentRound;

    level0ServingB = ((int)(currentRound/FIFO_PER_LEVEL)%2);
    level1ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
    level2ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

    //level3ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
    //level4ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

    ///fprintf(stderr, "Set Round: %d, level 0 serving B = %d, level 1 serving B = %d.\n", currentRound, level0ServingB, level1ServingB); // Debug: Peixuan 07062019

}

void Gearbox_pl_fid_flex::setPktCount(int pktCount) {
    ///fprintf(stderr, "Set Packet Count: %d\n", pktCount); // Debug: Peixuan 07072019
    this->pktCount = pktCount;
}

void Gearbox_pl_fid_flex::enque(Packet* packet) {   
    
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
        this->insertNewFlowPtr(iph->flowid(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020 fid
    }


    Flow_pl* currFlow = flowMap[key];
    int insertLevel = currFlow->getInsertLevel();

    departureRound = max(departureRound, currentRound);

    if ((departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL)) >= FIFO_PER_LEVEL) {
        ///fprintf(stderr, "?????Exceeds maximum round, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;   // 07072019 Peixuan: exceeds the maximum round
    }

    
    //int curFlowID = convertKeyValue(iph->saddrm, iph->daddr)   // use source IP as flow id
    int curBrustness = currFlow->getBrustness();
    if ((departureRound - currentRound) >= curBrustness) {
        ///fprintf(stderr, "?????Exceeds maximum brustness, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;   // 07102019 Peixuan: exceeds the maximum brustness
    }

    currFlow->setLastDepartureRound(departureRound);     // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped
    //this->updateFlowPtr(iph->saddr(), iph->daddr(), currFlow);  //12182019 Peixuan
    this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
    
    ///fprintf(stderr, "At Round: %d, Enqueue Packet from Flow %d with Finish time = %d.\n", currentRound, iph->saddr(), departureRound); // Debug: Peixuan 07072019

    int level0InsertingB = ((int)(departureRound/FIFO_PER_LEVEL)%2);
    int level1InsertingB = ((int)(departureRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

    int level2InsertingB = ((int)(departureRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
    //int level3InsertingB = ((int)(departureRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);

    // More levels

    ///fprintf(stderr, "Level 3 insert B: %d, Level 2 insert B: %d, Level 1 insert B: %d, Level 0 insert B: %d\n", level3InsertingB, level2InsertingB, level1InsertingB, level0InsertingB); // Debug: Peixuan 07072019

    /*if (departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) > 1 || insertLevel == 4) {
        /////fprintf(stderr, "Enqueue Level 2\n"); // Debug: Peixuan 07072019
        if (departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
            currFlow->setInsertLevel(3);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            forthLevel.enque(packet, departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
            ///fprintf(stderr, "Enqueue Level 4, forth FIFO, fifo %d\n", departureRound / (4*4*4) % 4); // Debug: Peixuan 07072019
        } else {
            currFlow->setInsertLevel(4);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[4].enque(packet, departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
            ///fprintf(stderr, "Enqueue Level 4, regular FIFO, fifo %d\n", departureRound / (4*4*4*4) % 4); // Debug: Peixuan 07072019
        }
    } else */
    
    if (departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) > 1 || insertLevel == 3) {
        if (departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                currFlow->setInsertLevel(2);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                thirdLevel.enque(packet, departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level 3, third FIFO, fifo %d\n", departureRound / (4*4) % 4); // Debug: Peixuan 07072019
            } else {
                currFlow->setInsertLevel(3);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                levels[3].enque(packet, departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level 3, regular FIFO, fifo %d\n", departureRound / (4*4*4) % 4); // Debug: Peixuan 07072019
            }

    
    } else if (departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) > 1 || insertLevel == 2) { //original
        if (!level2InsertingB) {
            /////fprintf(stderr, "Enqueue Level 1\n"); // Debug: Peixuan 07072019
            if (departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                currFlow->setInsertLevel(1);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                hundredLevel.enque(packet, departureRound / (FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level 2, hundredLevel FIFO, fifo %d\n", departureRound / (4) % 4); // Debug: Peixuan 07072019
            } else {
                currFlow->setInsertLevel(2);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                levels[2].enque(packet, departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level 2, regular FIFO, fifo %d\n", departureRound / (4*4) % 4); // Debug: Peixuan 07072019
            }
        } else {
            /////fprintf(stderr, "Enqueue Level B 1\n"); // Debug: Peixuan 07072019
            if (departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                currFlow->setInsertLevel(1);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                hundredLevelB.enque(packet, departureRound / (FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level B 2, hundredLevel FIFO, fifo %d\n", departureRound / (4) % 4); // Debug: Peixuan 07072019
            } else {
                currFlow->setInsertLevel(2);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                levelsB[2].enque(packet, departureRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level B 2, regular FIFO, fifo %d\n", departureRound / (4*4) % 4); // Debug: Peixuan 07072019
            }
        }
    } else if (departureRound / FIFO_PER_LEVEL - currentRound / FIFO_PER_LEVEL > 1 || insertLevel == 1) {
        if (!level1InsertingB) {
            /////fprintf(stderr, "Enqueue Level 1\n"); // Debug: Peixuan 07072019
            if (departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                currFlow->setInsertLevel(0);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                decadeLevel.enque(packet, departureRound  % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level 1, decede FIFO, fifo %d\n", departureRound  % 4); // Debug: Peixuan 07072019
            } else {
                currFlow->setInsertLevel(1);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                levels[1].enque(packet, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level 1, regular FIFO, fifo %d\n", departureRound / 4 % 4); // Debug: Peixuan 07072019
            }
        } else {
            /////fprintf(stderr, "Enqueue Level B 1\n"); // Debug: Peixuan 07072019
            if (departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
                currFlow->setInsertLevel(0);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                decadeLevelB.enque(packet, departureRound  % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level B 1, decede FIFO, fifo %d\n", departureRound  % 4); // Debug: Peixuan 07072019
            } else {
                currFlow->setInsertLevel(1);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                levelsB[1].enque(packet, departureRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL);
                ///fprintf(stderr, "Enqueue Level B 1, regular FIFO, fifo %d\n", departureRound / 4 % 4); // Debug: Peixuan 07072019
            }
        }

    } else {
        if (!level0InsertingB) {
            /////fprintf(stderr, "Enqueue Level 0\n"); // Debug: Peixuan 07072019
            currFlow->setInsertLevel(0);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[0].enque(packet, departureRound % FIFO_PER_LEVEL);
            ///fprintf(stderr, "Enqueue Level 0, regular FIFO, fifo %d\n", departureRound % 4); // Debug: Peixuan 07072019
        } else {
            /////fprintf(stderr, "Enqueue Level B 0\n"); // Debug: Peixuan 07072019
            currFlow->setInsertLevel(0);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levelsB[0].enque(packet, departureRound % FIFO_PER_LEVEL);
            ///fprintf(stderr, "Enqueue Level B 0, regular FIFO, fifo %d\n", departureRound % 4); // Debug: Peixuan 07072019
        }
        
    }

    setPktCount(pktCount + 1);
    ///fprintf(stderr, "Packet Count ++\n");

    ///fprintf(stderr, "Finish Enqueue\n"); // Debug: Peixuan 07062019
}

// Peixuan: This can be replaced by any other algorithms
int Gearbox_pl_fid_flex::cal_theory_departure_round(hdr_ip* iph, int pkt_size) {
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
    Flow_pl* currFlow = this->getFlowPtr(iph->flowid()); // Peixuan 04212020 fid

    int weightIndex = iph->flowid() % WEIGHT_LIST_LEN;
    //float curWeight = weightList[weightIndex];
    int curWeight = weightList[weightIndex]; // 01242022 Peixuan: modifyied to int

    //float curWeight = currFlow->getWeight();
    int curLastDepartureRound = currFlow->getLastDepartureRound();
    int curStartRound = max(currentRound, curLastDepartureRound);

    ///fprintf(stderr, "$$$$$Last Departure Round of Flow%d = %d\n",iph->saddr() , curLastDepartureRound); // Debug: Peixuan 07062019
    ///fprintf(stderr, "$$$$$Start Departure Round of Flow%d = %d\n",iph->saddr() , curStartRound); // Debug: Peixuan 07062019

    //int curDeaprtureRound = (int)(curStartRound + pkt_size/curWeight); // TODO: This line needs to take another thought

    int curDeaprtureRound = (int)(curStartRound + curWeight); // 07072019 Peixuan: basic test

    ///fprintf(stderr, "$$$$$At Round: %d, Calculated Packet From Flow:%d with Departure Round = %d\n", currentRound, iph->saddr(), curDeaprtureRound); // Debug: Peixuan 07062019
    // TODO: need packet length and bandwidh relation
    //flows[curFlowID].setLastDepartureRound(curDeaprtureRound);
    return curDeaprtureRound;
}

//06262019 Peixuan deque function of Gearbox:

//06262019 Static getting all the departure packet in this virtual clock unit (JUST FOR SIMULATION PURPUSE!)

Packet* Gearbox_pl_fid_flex::deque() {

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
        level1ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
        level2ServingB = ((int)(currentRound/(FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))%2);
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



    /*if (pktCurRound.size()) {
        // Pop out the first packet in pktCurRound until it is empty
        //Packet* pkt = pktCurRound.
        Packet *p = pktCurRound.front();
        pktCurRound.erase(pktCurRound.begin());

        hdr_ip* iph = hdr_ip::access(p);
        ///fprintf(stderr, "Dequeue Packet p with soure IP: %x\n", iph->saddr()); // Debug: Peixuan 07062019
        return p;
    } else {
        ///fprintf(stderr, "Empty Round\n"); // Debug: Peixuan 07062019
        pktCurRound = this->runRound();
        this->setCurrentRound(currentRound + 1); // Update system virtual clock
        this->deque();
    }*/

}

// Peixuan: now we only call this function to get the departure packet in the next round
vector<Packet*> Gearbox_pl_fid_flex::runRound() {

    ///fprintf(stderr, "Run Round\n"); // Debug: Peixuan 07062019

    ///fprintf(stderr, "Level 4 serving: %d \n", levels[4].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 3 serving: %d \n", levels[3].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 3 B serving: %d \n", levelsB[3].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 2 serving: %d \n", levels[2].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 2 B serving: %d \n", levelsB[2].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 1 serving: %d \n", levels[1].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 1 B serving: %d \n", levelsB[1].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 0 serving: %d \n", levels[0].getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 0 B serving: %d \n", levelsB[0].getCurrentIndex()); // Debug: Peixuan 07062019

    ///fprintf(stderr, "Forth Level serving: %d \n", forthLevel.getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Third Level serving: %d \n", thirdLevel.getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "ThirdLevel B serving: %d \n", thirdLevelB.getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Hundred Level serving: %d \n", hundredLevel.getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Hundred Level B serving: %d \n", hundredLevelB.getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Decade Level serving: %d \n", decadeLevel.getCurrentIndex()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Decade Level B serving: %d \n", decadeLevelB.getCurrentIndex()); // Debug: Peixuan 07062019

    ///fprintf(stderr, "Printing current level pkt cnt:\n");
    ///fprintf(stderr, "Level 4 pkt_cnt_: %d \n", levels[4].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 3 pkt_cnt_: %d \n", levels[3].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 3 B pkt_cnt_: %d \n", levelsB[3].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 2 pkt_cnt_: %d \n", levels[2].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 2 B pkt_cnt_: %d \n", levelsB[2].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 1 pkt_cnt_: %d \n", levels[1].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 1 B pkt_cnt_: %d \n", levelsB[1].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 0 pkt_cnt_: %d \n", levels[0].get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Level 0 B pkt_cnt_: %d \n", levelsB[0].get_level_pkt_cnt()); // Debug: Peixuan 07062019

    ///fprintf(stderr, "Forth Level pkt_cnt_: %d \n", forthLevel.get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Third Level pkt_cnt_: %d \n", thirdLevel.get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "ThirdLevel B pkt_cnt_: %d \n", thirdLevelB.get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Hundred Level pkt_cnt_: %d \n", hundredLevel.get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Hundred Level B pkt_cnt_: %d \n", hundredLevelB.get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Decade Level pkt_cnt_: %d \n", decadeLevel.get_level_pkt_cnt()); // Debug: Peixuan 07062019
    ///fprintf(stderr, "Decade Level B pkt_cnt_: %d \n", decadeLevelB.get_level_pkt_cnt()); // Debug: Peixuan 07062019

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
        bool is_level_2_update = false;
        bool is_level_3_update = false;
        //bool is_level_4_update = false;

        if (levels[0].getCurrentIndex() == 0) {
            is_level_1_update = true ;          
        }

        if (is_level_1_update && !level1ServingB) {
            levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
            if (levels[1].getCurrentIndex() == 0) {
                is_level_2_update = true;
            }
        }

        if (is_level_1_update && level1ServingB) {
            levelsB[1].getAndIncrementIndex();           // Level B 1 move to next FIFO
            if (levelsB[1].getCurrentIndex() == 0) {
                is_level_2_update = true;
            }
        }

        if (is_level_2_update && !level2ServingB) {
            levels[2].getAndIncrementIndex();           // Level 2 move to next FIFO
            if (levels[2].getCurrentIndex() == 0) {
                is_level_3_update = true;
            }
        }

        if (is_level_2_update && level2ServingB) {
            levelsB[2].getAndIncrementIndex();           // Level B 2 move to next FIFO
            if (levelsB[2].getCurrentIndex() == 0) {
                is_level_3_update = true;
            }
        }

        if (is_level_3_update) {
            levels[3].getAndIncrementIndex();            // Level 3 move to next FIFO
        }

        /*if (is_level_3_update && !level3ServingB) {
            levels[3].getAndIncrementIndex();           // Level 3 move to next FIFO
            if (levels[3].getCurrentIndex() == 0) {
                is_level_4_update = true;
            }
        }

        if (is_level_3_update && level3ServingB) {
            levelsB[3].getAndIncrementIndex();           // Level B 3 move to next FIFO
            if (levelsB[3].getCurrentIndex() == 0) {
                is_level_4_update = true;
            }
        }

        if (is_level_4_update) {
            levels[4].getAndIncrementIndex();            // Level 4 move to next FIFO
        }*/        
        
        
        /*// level cur fifo updates
        if (levels[0].getCurrentIndex() == 0) {
            if (!level1ServingB) {
                levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                ///fprintf(stderr, "<<<<<At Round:%d, Level 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levels[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    ///fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levels[2].getCurrentIndex()); // Debug: Peixuan 07212019

                    if (levels[2].getCurrentIndex() == 0)
                        levels[3].getAndIncrementIndex();       // Level 3 move to next FIFO
                        ///fprintf(stderr, "<<<<<At Round:%d, Level 3 update current FIFO as: fifo %d\n", currentRound, levels[3].getCurrentIndex()); // Debug: Peixuan 07212019

                        if (levels[3].getCurrentIndex() == 0)
                            levels[4].getAndIncrementIndex();       // Level 4 move to next FIFO
                            ///fprintf(stderr, "<<<<<At Round:%d, Level 4 update current FIFO as: fifo %d\n", currentRound, levels[4].getCurrentIndex()); // Debug: Peixuan 07212019

            } else {
                levelsB[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                ///fprintf(stderr, "<<<<<At Round:%d, Level B 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levelsB[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    ///fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levels[2].getCurrentIndex()); // Debug: Peixuan 07212019
            }
            //levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
            /////fprintf(stderr, "<<<<<At Round:%d, Level 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019
            //if (levels[1].getCurrentIndex() == 0)
            //    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
            //    ///fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levels[2].getCurrentIndex()); // Debug: Peixuan 07212019
        }*/
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
        bool is_level_2_update = false;
        bool is_level_3_update = false;
        //bool is_level_4_update = false;

        if (levelsB[0].getCurrentIndex() == 0) {
            is_level_1_update = true ;          
        }

        if (is_level_1_update && !level1ServingB) {
            levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
            if (levels[1].getCurrentIndex() == 0) {
                is_level_2_update = true;
            }
        }

        if (is_level_1_update && level1ServingB) {
            levelsB[1].getAndIncrementIndex();           // Level B 1 move to next FIFO
            if (levelsB[1].getCurrentIndex() == 0) {
                is_level_2_update = true;
            }
        }

        if (is_level_2_update && !level2ServingB) {
            levels[2].getAndIncrementIndex();           // Level 2 move to next FIFO
            if (levels[2].getCurrentIndex() == 0) {
                is_level_3_update = true;
            }
        }

        if (is_level_2_update && level2ServingB) {
            levelsB[2].getAndIncrementIndex();           // Level B 2 move to next FIFO
            if (levelsB[2].getCurrentIndex() == 0) {
                is_level_3_update = true;
            }
        }

        if (is_level_3_update) {
            levels[3].getAndIncrementIndex();            // Level 3 move to next FIFO
        }

        /*if (is_level_3_update && !level3ServingB) {
            levels[3].getAndIncrementIndex();           // Level 3 move to next FIFO
            if (levels[3].getCurrentIndex() == 0) {
                is_level_4_update = true;
            }
        }

        if (is_level_3_update && level3ServingB) {
            levelsB[3].getAndIncrementIndex();           // Level B 3 move to next FIFO
            if (levelsB[3].getCurrentIndex() == 0) {
                is_level_4_update = true;
            }
        }

        if (is_level_4_update) {
            levels[4].getAndIncrementIndex();            // Level 4 move to next FIFO
        }*/
        
        
        /*if (levelsB[0].getCurrentIndex() == 0) {

            if (!level1ServingB) {
                levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                ///fprintf(stderr, "<<<<<At Round:%d, Level 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levels[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    ///fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levelsB[2].getCurrentIndex()); // Debug: Peixuan 07212019

            } else {
                levelsB[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                ///fprintf(stderr, "<<<<<At Round:%d, Level B 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levelsB[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    ///fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levelsB[2].getCurrentIndex()); // Debug: Peixuan 07212019
            }

            //levelsB[1].getAndIncrementIndex();           // Level 1 move to next FIFO
            /////fprintf(stderr, "<<<<<At Round:%d, Level B 1 update current FIFO as: fifo %d\n", currentRound, levelsB[1].getCurrentIndex()); // Debug: Peixuan 07212019

            //if (levelsB[1].getCurrentIndex() == 0)
            //    levelsB[2].getAndIncrementIndex();       // Level 2 move to next FIFO
            //    ///fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levelsB[2].getCurrentIndex()); // Debug: Peixuan 07212019
        }*/

    }

    
    return result;
}

//Peixuan: This is also used to get the packet served in this round (VC unit)
// We need to adjust the order of serving: level0 -> level1 -> level2
vector<Packet*> Gearbox_pl_fid_flex::serveUpperLevel(int currentRound) {

    ///fprintf(stderr, "Serving Upper Level\n"); // Debug: Peixuan 07062019

    vector<Packet*> result;

    int level0size = 0;
    int level1size = 0;
    int level2size = 0;
    int level3size = 0;
    //int level4size = 0;

    int decadelevelsize = 0;
    int hundredlevelsize = 0;
    int thirdlevelsize = 0;
    //int forthlevelsize = 0;

    if (!level1ServingB) {
        level1size = levels[1].getCurrentFifoSize();
    } else {
        level1size = levelsB[1].getCurrentFifoSize();
    }

    if (!level2ServingB) {
        level2size = levels[2].getCurrentFifoSize();
    } else {
        level2size = levelsB[2].getCurrentFifoSize();
    }

    level3size = levels[3].getCurrentFifoSize();

    /*if (!level3ServingB) {
        level3size = levels[3].getCurrentFifoSize();
    } else {
        level3size = levelsB[3].getCurrentFifoSize();
    }

    level4size = levels[4].getCurrentFifoSize();*/



    if (!level1ServingB) {
        decadelevelsize = decadeLevel.getCurrentFifoSize();
    } else {
        decadelevelsize = decadeLevelB.getCurrentFifoSize();
    }

    if (!level2ServingB) {
        hundredlevelsize = hundredLevel.getCurrentFifoSize();
    } else {
        hundredlevelsize = hundredLevelB.getCurrentFifoSize();
    }

    thirdlevelsize = thirdLevel.getCurrentFifoSize();

    /*if (!level3ServingB) {
        thirdlevelsize = thirdLevel.getCurrentFifoSize();
    } else {
        thirdlevelsize = thirdLevelB.getCurrentFifoSize();
    }

    forthlevelsize = forthLevel.getCurrentFifoSize();*/

    // ToDo: swap the order of serving levels

    //Then: first level 1
    if (currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {

        if (!level1ServingB) {
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
        } else {
            // serving backup decade level
            int size = decadeLevelB.getCurrentFifoSize();
            ///fprintf(stderr, ">>>>>At Round:%d, Serve Level B 1 Convergence FIFO with fifo: %d, size: %d\n", currentRound, decadeLevelB.getCurrentIndex(), size); // Debug: Peixuan 07222019
            for (int i = 0; i < size; i++) {
                Packet* p = decadeLevelB.deque();
                if (p == 0)
                    break;
                result.push_back(p);

                hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 1 Convergence FIFO, fifo: %d\n", currentRound, iph->saddr(), decadeLevelB.getCurrentIndex()); // Debug: Peixuan 07092019

            }
            decadeLevelB.getAndIncrementIndex();

        }

        
    }
    else {
        if (!level1ServingB) {
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
        } else {
            if (!levelsB[1].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levelsB[1].getCurrentFifoSize() * 1.0 / (FIFO_PER_LEVEL - currentRound % FIFO_PER_LEVEL)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 1 serving order (fixed)
                ///fprintf(stderr, ">>>At Round:%d, Serve Level 1 B Regular FIFO with fifo: %d, size: %d\n", currentRound, levelsB[1].getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    Packet* p = levelsB[1].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                    ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 1, fifo: %d\n", currentRound, iph->saddr(), levelsB[1].getCurrentIndex()); // Debug: Peixuan 07092019
                    result.push_back(p);
                }  
            }

        }
    }



    //Then: level 2
    if (currentRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {

        int size = 0;

        /*if (!level2ServingB) {
            size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (4 - currentRound % 4)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        } else {
            size = static_cast<int>(ceil((hundredLevelB.getCurrentFifoSize() + levelsB[1].getCurrentFifoSize()) * 1.0 / (4 - currentRound % 4)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        }*/

        size = static_cast<int>(ceil((hundredlevelsize + level1size) * 1.0 / (FIFO_PER_LEVEL - currentRound % FIFO_PER_LEVEL)));  // 01132020 Peixuan


        if (!level2ServingB) {
            //int size = hundredLevel.getCurrentFifoSize();
            ///fprintf(stderr, ">>>>>At Round:%d, Serve Level 2 Convergence FIFO with fifo: %d, size: %d\n", currentRound, hundredLevel.getCurrentIndex(), size); // Debug: Peixuan 07222019
            for (int i = 0; i < size; i++) {
                Packet* p = hundredLevel.deque();
                if (p == 0)
                    break;
                result.push_back(p);

                hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 2 Convergence FIFO, fifo: %d\n", currentRound, iph->saddr(), hundredLevel.getCurrentIndex()); // Debug: Peixuan 07092019
            }
            if (currentRound % FIFO_PER_LEVEL == FIFO_PER_LEVEL - 1)
                hundredLevel.getAndIncrementIndex();
        } else {

            //int size = hundredLevelB.getCurrentFifoSize();
            ///fprintf(stderr, ">>>>>At Round:%d, Serve Level B 2 Convergence FIFO with fifo: %d, size: %d\n", currentRound, hundredLevelB.getCurrentIndex(), size); // Debug: Peixuan 07222019
            for (int i = 0; i < size; i++) {
                Packet* p = hundredLevelB.deque();
                if (p == 0)
                    break;
                result.push_back(p);

                hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 2 Convergence FIFO, fifo: %d\n", currentRound, iph->saddr(), hundredLevelB.getCurrentIndex()); // Debug: Peixuan 07092019

            }
            if (currentRound % FIFO_PER_LEVEL == FIFO_PER_LEVEL - 1)
                hundredLevelB.getAndIncrementIndex();

        }

        
    }
    else {
        if (!level2ServingB) {
            if (!levels[2].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levels[2].getCurrentFifoSize() * 1.0 / ((FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound % (FIFO_PER_LEVEL*FIFO_PER_LEVEL))));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[2].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 1 serving order (fixed)
                ///fprintf(stderr, ">>>At Round:%d, Serve Level 2 Regular FIFO with fifo: %d, size: %d\n", currentRound, levels[2].getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    Packet* p = levels[2].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                    ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 2, fifo: %d\n", currentRound, iph->saddr(), levels[2].getCurrentIndex()); // Debug: Peixuan 07092019
                    result.push_back(p);
                }  
            }
        } else {
            if (!levelsB[2].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levelsB[2].getCurrentFifoSize() * 1.0 / ((FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound % (FIFO_PER_LEVEL*FIFO_PER_LEVEL))));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[2].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 1 serving order (fixed)
                ///fprintf(stderr, ">>>At Round:%d, Serve Level 2 B Regular FIFO with fifo: %d, size: %d\n", currentRound, levelsB[2].getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    Packet* p = levelsB[2].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                    ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 2, fifo: %d\n", currentRound, iph->saddr(), levelsB[2].getCurrentIndex()); // Debug: Peixuan 07092019
                    result.push_back(p);
                }  
            }

        }
    }

    // then level 3
    if (currentRound / (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) % FIFO_PER_LEVEL == STEP_DOWN_FIFO) {
        //int size = static_cast<int>(ceil(hundredLevel.getCurrentFifoSize() * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (ori)
        //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)

        int size = 0;

        /*if (!level3ServingB) {
            size = static_cast<int>(ceil((forthLevel.getCurrentFifoSize() + levels[3].getCurrentFifoSize()) * 1.0 / ((4*4*4) - currentRound % (4*4*4))));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        } else {
            size = static_cast<int>(ceil((forthLevel.getCurrentFifoSize() + levelsB[3].getCurrentFifoSize()) * 1.0 / ((4*4*4) - currentRound % (4*4*4))));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        }*/

        size = static_cast<int>(ceil((thirdlevelsize + level2size) * 1.0 / ((FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound % (FIFO_PER_LEVEL*FIFO_PER_LEVEL))));  // 01132021 Peixuan *** Fix Level 2 serving order (fixed)

        //size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize() + levelsB[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)

        //int size = hundredLevel.getCurrentFifoSize();
        for (int i = 0; i < size; i++) {
            Packet* p = thirdLevel.deque();
            if (p == 0)
                break;
            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 4 Convergence FIFO: fifo %d\n", currentRound, iph->saddr(), forthLevel.getCurrentIndex()); // Debug: Peixuan 07092019
            result.push_back(p);
        }

        /*if (thirdLevel.getCurrentFifoSize() && currentRound / FIFO_PER_LEVEL % FIFO_PER_LEVEL != STEP_DOWN_FIFO)  // 07222019 Peixuan ***: If hundredLevel not empty, serve it until it is empty (Except Level 1 is serving Convergence FIFO (decade FIFO))
        //if (hundredLevel.getCurrentFifoSize())  // 07212019 Peixuan ***: If hundredLevel not empty, serve it until it is empty
            return result;                      // 07212019 Peixuan ***/

        if (currentRound % (FIFO_PER_LEVEL * FIFO_PER_LEVEL) == (FIFO_PER_LEVEL * FIFO_PER_LEVEL) - 1)
            thirdLevel.getAndIncrementIndex();

        

    } else if (!levels[3].isCurrentFifoEmpty()) {
        int size = static_cast<int>(ceil(levels[3].getCurrentFifoSize() * 1.0 / ((FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL) - currentRound % (FIFO_PER_LEVEL*FIFO_PER_LEVEL*FIFO_PER_LEVEL))));
        for (int i = 0; i < size; i++) {
            Packet* p = levels[3].deque();
            if (p == 0)
                break;
            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 4, fifo: %d\n", currentRound, iph->saddr(), levels[4].getCurrentIndex()); // Debug: Peixuan 07092019
            result.push_back(p);
        }
    } 

    /*//First: then level 2
    if (currentRound / (4*4) % 4 == 2) {
        //int size = static_cast<int>(ceil(hundredLevel.getCurrentFifoSize() * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (ori)
        //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)

        int size = 0;

        if (!level1ServingB) {
            size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (4 - currentRound % 4)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        } else {
            size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levelsB[1].getCurrentFifoSize()) * 1.0 / (4 - currentRound % 4)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        }

        //size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize() + levelsB[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)

        //int size = hundredLevel.getCurrentFifoSize();
        for (int i = 0; i < size; i++) {
            Packet* p = hundredLevel.deque();
            if (p == 0)
                break;
            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 2 Convergence FIFO: fifo %d\n", currentRound, iph->saddr(), hundredLevel.getCurrentIndex()); // Debug: Peixuan 07092019
            result.push_back(p);
        }

        if (hundredLevel.getCurrentFifoSize() && currentRound / 4 % 4 != 5)  // 07222019 Peixuan ***: If hundredLevel not empty, serve it until it is empty (Except Level 1 is serving Convergence FIFO (decade FIFO))
        //if (hundredLevel.getCurrentFifoSize())  // 07212019 Peixuan ***: If hundredLevel not empty, serve it until it is empty
            return result;                      // 07212019 Peixuan ***

        if (currentRound % 4 == 3)
            hundredLevel.getAndIncrementIndex();

        

    } else if (!levels[2].isCurrentFifoEmpty()) {
        int size = static_cast<int>(ceil(levels[2].getCurrentFifoSize() * 1.0 / ((4*4) - currentRound % (4*4))));
        for (int i = 0; i < size; i++) {
            Packet* p = levels[2].deque();
            if (p == 0)
                break;
            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 2, fifo: %d\n", currentRound, iph->saddr(), levels[2].getCurrentIndex()); // Debug: Peixuan 07092019
            result.push_back(p);
        }
    }

    //Then: first level 1
    if (currentRound / 4 % 4 == 2) {

        if (!level1ServingB) {
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
        } else {
            // serving backup decade level
            int size = decadeLevelB.getCurrentFifoSize();
            ///fprintf(stderr, ">>>>>At Round:%d, Serve Level B 1 Convergence FIFO with fifo: %d, size: %d\n", currentRound, decadeLevelB.getCurrentIndex(), size); // Debug: Peixuan 07222019
            for (int i = 0; i < size; i++) {
                Packet* p = decadeLevelB.deque();
                if (p == 0)
                    break;
                result.push_back(p);

                hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 1 Convergence FIFO, fifo: %d\n", currentRound, iph->saddr(), decadeLevelB.getCurrentIndex()); // Debug: Peixuan 07092019

            }
            decadeLevelB.getAndIncrementIndex();

        }

        
    }
    else {
        if (!level1ServingB) {
            if (!levels[1].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levels[1].getCurrentFifoSize() * 1.0 / (4 - currentRound % 4)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
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
        } else {
            if (!levelsB[1].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levelsB[1].getCurrentFifoSize() * 1.0 / (4 - currentRound % 4)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 1 serving order (fixed)
                ///fprintf(stderr, ">>>At Round:%d, Serve Level 1 B Regular FIFO with fifo: %d, size: %d\n", currentRound, levelsB[1].getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    Packet* p = levelsB[1].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                    ///fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 1, fifo: %d\n", currentRound, iph->saddr(), levelsB[1].getCurrentIndex()); // Debug: Peixuan 07092019
                    result.push_back(p);
                }  
            }

        }
    }*/
    
    return result;
}


// 12132019 Peixuan
//Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(nsaddr_t saddr, nsaddr_t daddr) {
Flow_pl* Gearbox_pl_fid_flex::getFlowPtr(int fid) {

    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow_pl* flow; 
    if (flowMap.find(key) == flowMap.end()) {
        //flow = this->insertNewFlowPtr(saddr, daddr, 2, 100);
        //flow = this->insertNewFlowPtr(saddr, daddr, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
        flow = this->insertNewFlowPtr(fid, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020
    }
    flow = this->flowMap[key];
    return flow;
}

//Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness) {
Flow_pl* Gearbox_pl_fid_flex::insertNewFlowPtr(int fid, int weight, int brustness) { // Peixuan 04212020
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    //string key = convertKeyValue(saddr, daddr);
    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
    //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
    this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));
    //flowMap.insert(pair(key, newFlowPtr));
    //return 0;
    return this->flowMap[key];
}

//int Gearbox_pl_fid_flex::updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr) {
int Gearbox_pl_fid_flex::updateFlowPtr(int fid, Flow_pl* flowPtr) { // Peixuan 04212020
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    //string key = convertKeyValue(saddr, daddr);
    string key = convertKeyValue(fid);  // Peixuan 04212020
    //Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
    //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
    this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));
    //flowMap.insert(pair(key, newFlowPtr));
    //return 0;
    return 0;
}

//string Gearbox_pl_fid_flex::convertKeyValue(nsaddr_t saddr, nsaddr_t daddr) {
string Gearbox_pl_fid_flex::convertKeyValue(int fid) { // Peixuan 04212020
    stringstream ss;
    //ss << saddr;
    //ss << ":";
    //ss << daddr;
    ss << fid;  // Peixuan 04212020
    string key = ss.str();
    return key; //TODO:implement this logic
}


