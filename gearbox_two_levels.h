#include "level.h"
#include "flow.h"
#include <vector>
#include <string>

#include <map>

using namespace std;

class GearboxTwoLevels : public Queue {
private:
    static const int DEFAULT_VOLUME = 2;
    static const int FIFO_PER_LEVEL = 16;

    static const int STEP_DOWN_FIFO = 8;
    static const int DEFAULT_WEIGHT = 2;    // TODO(Yitao): replace in cc and remove this
    /**
     * A new flow can send a certain amount of data (related to the value 1000) before 
     * it is considered "non-bursty" and its packets are placed in lower priority queues,
     * resulting in higher latency.
     */
    static const int DEFAULT_BRUSTNESS = 1000;
    static const int TIMEUNIT = 1;    // TODO(Yitao): remove this
    static const int WEIGHT_LIST_LEN = 4;
    int weightList[WEIGHT_LIST_LEN];
    int volume_; 
    int currentRound;
    int pktCount;

    Level levels[2];
    Level levelsB[1];

    Level decadeLevel;

    bool level0ServingB;

    vector<Packet*> pktCurRound;

    vector<Packet*> runRound();
    vector<Packet*> serveUpperLevel(int);
    void setPktCount(int);

    typedef std::map<string, Flow_pl*> FlowMap;
    FlowMap flowMap;

    Flow_pl* getFlowPtr(int fid);
    Flow_pl* insertNewFlowPtr(int fid, int weight, int brustness);

    int updateFlowPtr(int fid, Flow_pl* flowPtr);

    string convertKeyValue(int fid);


public:
    Gearbox_pl_fid_flex();
    explicit Gearbox_pl_fid_flex(int);
    void enque(Packet*);
    Packet* deque();
    void setCurrentRound(int);
    int cal_theory_departure_round(hdr_ip*, int);
    int cal_insert_level(int, int);
};
