#pragma once
#include "node.h"
#include <unordered_set>
#include <unordered_map>
#include "Link.h"

class Graph
{
private:
    bool m_changed = false;
    unsigned int lastId = 0;
public:
    vector<Node*> nodes;
    vector<Link*> links;

public:
    unsigned int GetNewId() { return lastId += 5; }
    void AddNode(Node* node) 
    { 
        nodes.push_back(node); 
    }
    void InitiateLinks() {};
    void TopoSort(vector<Node*>& nodes);
    bool WouldCreateCycle(Node* from, Node* to);
    bool Connect(int from, int to);
    void Disconnect(int linkID)
    {
        vector<int> linksToDelete(1, linkID);
        DeleteLinks(linksToDelete);
        SetChanged(true);
    }

    void DeleteNodes(vector<int>& nodeIDs);
    void DeleteLinks(vector<int>& linkIDs);
    void PropagateData(Node* node);
    bool Evaluate();
    void SetChanged(bool changed) { m_changed = changed; }
    bool IsChanged() { return m_changed; }
    Node* GetNodeFromChannelID(int channelId, Channel*& channel);
    Channel* findChannelFromId(int socket_id);
    Node* GetNodeFromId(int nodeId);
    Link* GetLinkFromId(int linkId);

private:
    bool HasPath(Node* start, Node* target, std::unordered_set<Node*>& visited);
};

