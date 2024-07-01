#ifndef NET
#define NET

#include <string.h>
#include <omnetpp.h>
#include <map>
#include <queue>
#include <packet_m.h>

using namespace omnetpp;
using namespace std;

class Net: public cSimpleModule {
private:

public:
    Net();
    virtual ~Net();
    static const int MAX_NEIGHBORS = 4;
    static const int MAX_NODES = 60;
    static const int HELLO_KIND = 1;
    static const int DATA_KIND = 2;
    static const int NORMAL_KIND = 3;
    // map of nodes from which we have their data
    map<int, bool> recvData;
    map<int, int> gatesToNeighbors;
    int neighbors[MAX_NEIGHBORS];
    int usedNeighbors;
    int neighborTable[MAX_NODES][MAX_NEIGHBORS];
    int path[MAX_NODES];
    cMessage *floodEvent;
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
    virtual int shortestPath(int dest);
};

Define_Module(Net);

#endif /* NET */

Net::Net() {
}

Net::~Net() {
}

/*
 * Use BFS to find the shortest path to the destination considering the neighbor table
 * and return the next node in the path in each iteration and visit it until we reach the destination
 */
int Net::shortestPath(int dest) {
    // queue of nodes to visit
    queue<int> q;
    // map of visited nodes
    map<int, bool> visited;
    // map of previous nodes
    map<int, int> prev;
    // map of distances
    map<int, int> dist;
    // initialize maps
    for (int i = 0; i < MAX_NODES; i++) {
        visited[i] = false;
        prev[i] = -1;
        dist[i] = 0;
    }
    // start from the source node
    int source = this->getParentModule()->getIndex();
    visited[source] = true;
    dist[source] = 0;
    q.push(source);
    // BFS
    while (!q.empty()) {
        int u = q.front();
        if (u == dest) {
            break;
        }
        q.pop();
        // for each neighbor of u
        for (int i = 0; i < MAX_NEIGHBORS; i++) {
            int v = neighborTable[u][i];
            // if v is a neighbor of u and v is not visited
            if (v != -1 && !visited[v]) {
                visited[v] = true;
                prev[v] = u;
                dist[v] = dist[u] + 1;
                q.push(v);
            }
        }
    }
    int u = dest;
    int i = 0;
    // get the path from the destination to the source
    while (prev[u] != -1 && u != source) {
        path[i] = prev[u];
        u = prev[u];
        i++;
    }
    if (u == source && i == 1) {
        return dest;
    } else if (u == source && i > 1) {
        return path[i - 2];
    } else {
        return -1;
    }
}


void Net::initialize() {
    // Neighbors initial values
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        neighbors[i] = -1;
        for (int j = 0; j < MAX_NODES; j++) {
            neighborTable[j][i] = -1;
        }
    }
    usedNeighbors = 0;
    // Send hello packet to all neighbors
    Packet *pkt = new Packet("Hello packet", HELLO_KIND);
    pkt->setSource(this->getParentModule()->getIndex());
    pkt->setKind(HELLO_KIND);

    // Send packet to all neighbors
    for (int i = 0; i < gateSize("toLnk"); i++) {
        send(pkt->dup(), "toLnk$o", i);
    }
    // Delete packet
    delete pkt;
    recvData[this->getParentModule()->getIndex()] = true;

    // wait to receive hello packets from neighbors
    floodEvent = new cMessage("floodEvent");
    scheduleAt(simTime() + 0.0003, floodEvent);
}

void Net::finish() {
    // Cancel the floodEvent if it is scheduled
    if (floodEvent && floodEvent->isScheduled()) {
        cancelAndDelete(floodEvent);
    }
}

void Net::handleMessage(cMessage *msg) {
    if (msg == floodEvent){
        // Copy neighbors in neighbor table
        for (int i = 0; i < MAX_NEIGHBORS; i++) {
            neighborTable[this->getParentModule()->getIndex()][i] = neighbors[i];
        }
        // All neighbors have sent hello packets
        // Send data packet to all neighbors
        Packet *pkt = new Packet("Data packet", DATA_KIND);
        pkt->setSource(this->getParentModule()->getIndex());
        pkt->setKind(DATA_KIND);
        // put neighbors array in the message
        for (int i = 0; i < MAX_NEIGHBORS; i++) {
            pkt->setNeighbors(i, neighbors[i]);
        }
        // Send packet for each neighbor
        for (int i = 0; i < gateSize("toLnk"); i++) {
            send(pkt->dup(), "toLnk$o", i);
        }
        // Delete packet
        delete(pkt);
    } else {

        // All msg (events) on net are packets
        Packet *pkt = (Packet *) msg;

        // If hello packet, write neighbour ID
        if(pkt->getKind() == HELLO_KIND){
            // If there is a free neighbour slot
            if(usedNeighbors < MAX_NEIGHBORS){
                // Write neighbour ID
                neighbors[usedNeighbors] = pkt->getSource();
                gatesToNeighbors[pkt->getSource()] = pkt->getArrivalGate()->getIndex();
                // Increment used neighbors
                usedNeighbors++;
            } else {
                // Error
                EV << "ERROR: Too many neighbors" << endl;
            }
        } else if(pkt->getKind() == DATA_KIND){

            // get neighbors array from the message and store it in the table
            for (int i = 0; i < MAX_NEIGHBORS; i++) {
                neighborTable[pkt->getSource()][i] = pkt->getNeighbors(i);
                // add the sender neighbors to resend map
                if(pkt->getNeighbors(i) != -1){
                    // if neighbor node is not in the map of received data
                    if(recvData.find(pkt->getNeighbors(i)) == recvData.end()){
                        // add it to the map
                        recvData[pkt->getNeighbors(i)] = false;
                    }
                }
            }

            // Send packet to all neighbors except the sender if
            // the packet source was not recvData
            for (int i = 0; i < gateSize("toLnk"); i++) {
                // If neighbour is not the sender and the packet source was not in resend map
                if(i != pkt->getArrivalGate()->getIndex() && !recvData[pkt->getSource()]){
                    send(pkt->dup(), "toLnk$o", i);
                }
            }
            // Mark packet source as received
            recvData[pkt->getSource()] = true;
            delete pkt;

        } else if(pkt->getKind() == NORMAL_KIND){
            // If packet is for me
            if(pkt->getDestination() == this->getParentModule()->getIndex()){
                // Send packet to application
                send(pkt, "toApp$o");
            } else {
                // Search shortest path to destination and send packet via
                // the corresponding link
                int nextNode = shortestPath(pkt->getDestination());
                if(gatesToNeighbors.find(nextNode) != gatesToNeighbors.end()){
                    pkt->setHopCount(pkt->getHopCount() + 1);
                    send(pkt, "toLnk$o", gatesToNeighbors[nextNode]);
                } else {
                    // Error
                    EV << "ERROR: Next node " << nextNode << " not found" << endl;
                }
            }
        } else {
            // Error
            EV << "ERROR: Unknown packet kind" << endl;
            delete(pkt);
        }
    }
}
