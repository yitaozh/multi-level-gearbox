#include "Level.h"
#include "Flow.h"
#include <vector>
#include <string>

#include <map>

using namespace std;

class Gearbox_one_pl : public Queue {
private:
    static const int SET_NUMBER = 100;
    static const int SET_GRANULARITY = 10;       // TimeStamp Range of each queue set (level.cc)
    static const int DEFAULT_VOLUME = 1000;
    static const int DEFAULT_WEIGHT = 1;         // 01032019 Peixuan default weight
    static const int DEFAULT_BRUSTNESS = 10000;    // 01032019 Peixuan default brustness
    int volume_;                     // num of levels in scheduler
    int currentRound;           // current Round
    int pktCount;           // packet count
    //Level levels[3];
    Level levels[SET_NUMBER];        // same queue number with HCS
    //Level hundredLevel;
    //Level decadeLevel;
    //vector<Flow_pl> flows;
    //06262019 Peixuan
    vector<Packet*> pktCurRound;

    // 06262019 Peixuan
    vector<Packet*> runRound();
    //vector<Packet*> serveUpperLevel(int); // HCS -> AFQ
    void setPktCount(int);

    //12132019 Peixuan
    //typedef std::map<int, Flow_pl*> FlowTable;
    typedef std::map<string, Flow_pl*> FlowMap;
    FlowMap flowMap;

    //12132019 Peixuan
    //Flow_pl* getFlowPtr(nsaddr_t saddr, nsaddr_t daddr);
    Flow_pl* getFlowPtr(int fid);   // Peixuan 04212020
    //int getFlowPtr(ns_addr_t saddr, ns_addr_t daddr);
    //Flow_pl* insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness);
    Flow_pl* insertNewFlowPtr(int fid, int weight, int brustness);   // Peixuan 04212020

    //int updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr);
    int updateFlowPtr(int fid, Flow_pl* flowPtr);    // Peixuan 04212020

    //string convertKeyValue(nsaddr_t saddr, nsaddr_t daddr);
    string convertKeyValue(int fid);    // Peixuan 04212020

public:
    //hierarchicalQueue();
    //explicit hierarchicalQueue(int);
    Gearbox_one_pl();
    explicit Gearbox_one_pl(int);
    void enque(Packet*);
    Packet* deque();
    void setCurrentRound(int);
    int cal_theory_departure_round(hdr_ip*, int);
    int cal_insert_level(int, int);
    // Packet* serveCycle();
    // vector<Packet> serveUpperLevel(int &, int);

};
