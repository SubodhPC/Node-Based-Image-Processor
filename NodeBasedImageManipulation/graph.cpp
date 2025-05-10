#include "Graph.h"
#include <queue>

void Graph::InitiateLinks()
{
    for (Link* link : links)
    {
        ImNodes::Link(link->id, link->from_channel->id, link->to_channel->id);
    }
}

void Graph::CreateNodesOnCanvas()
{
    for (Node* node : nodes)
    {
        node->CreateImNode();
    }
}

void Graph::TopoSort(vector<Node*>& nodes)
{
    std::unordered_map<Node*, int> indegree;
    for (Node* node : nodes) {
        indegree[node] = 0;
    }

    std::unordered_map<Node*, std::vector<Node*>> deps;

    for (Link* link : links) {
        Node* from = link->from_node;
        Node* to = link->to_node;
        deps[from].push_back(to);  // from -> to (to depends on from)
        indegree[link->to_node]++;
    }

    std::queue<Node*> q;
    for (auto& item : indegree) {
        if (item.second == 0) q.push(item.first);
    }

    std::vector<Node*> sorted;
    while (!q.empty()) {
        Node* current = q.front(); 
        q.pop();
        sorted.push_back(current);

        for (Node* dep : deps[current]) {
            indegree[dep]--;
            if (indegree[dep] == 0) q.push(dep);
        }
    }

    if (sorted.size() != nodes.size()) {
        // Cycle detected
    }
    nodes = sorted;
}

bool Graph::WouldCreateCycle(Node* from, Node* to) {
    std::unordered_set<Node*> visited;
    return HasPath(to, from, visited);  // would connecting to -> from make a cycle?
}

bool Graph::HasPath(Node* start, Node* target, std::unordered_set<Node*>& visited) {
    if (start == target) return true;
    if (visited.count(start)) return false;

    visited.insert(start);

    for (Link* link : links) {
        if (link->from_node == start) {
            if (HasPath(link->to_node, target, visited)) {
                return true;
            }
        }
    }
    return false;
}

bool Graph::Connect(int fromChannelID, int toChannelID)
{
    Channel* fromChannel = nullptr;
    Channel* toChannel = nullptr;
    Node* fromNode = GetNodeFromChannelID(fromChannelID, fromChannel);
    Node* toNode = GetNodeFromChannelID(toChannelID, toChannel);
    if (WouldCreateCycle(fromNode, toNode))
        return false;

    Link* link = new Link(GetNewId(), fromNode, toNode, fromChannel, toChannel);
    links.push_back(link);

    fromChannel->attachedLinks.insert(link);
    toChannel->attachedLinks.insert(link);

    toNode->MarkDirty();
    SetChanged(true);

    return true;
}

void Graph::DeleteNodes(vector<int>& nodeIDs)
{
    std::unordered_set<int> nodeIDSet(nodeIDs.begin(), nodeIDs.end());

    // Remove related links
    links.erase(
        std::remove_if(links.begin(), links.end(),
            [&nodeIDSet](Link* link) {
                if (nodeIDSet.contains(link->from_node->id) || nodeIDSet.contains(link->to_node->id))
                {
                    /*auto itr = link->from_channel->attachedLinks.find(link);
                    if (itr != link->from_channel->attachedLinks.end())
                        link->from_channel->attachedLinks.erase(itr);

                    itr = link->to_channel->attachedLinks.find(link);
                    if (itr != link->to_channel->attachedLinks.end())
                        link->to_channel->attachedLinks.erase(itr);

                    if (link->to_channel->data != nullptr) 
                    {
                        link->to_channel->data = nullptr;
                    }*/
                    link->to_node->MarkDirty();

                    delete link;
                    return true;
                }
                return false;
            }),
        links.end());

    // Delete nodes and remove from list
    nodes.erase(
        std::remove_if(nodes.begin(), nodes.end(),
            [&nodeIDSet](Node* node) {
                if (nodeIDSet.contains(node->id)) {
                    delete node;
                    return true;
                }
                return false;
            }),
        nodes.end());
}

void Graph::DeleteLinks(vector<int>& linkIDs)
{
    std::unordered_set<int> nodeIDSet(linkIDs.begin(), linkIDs.end());

    links.erase(
        std::remove_if(links.begin(), links.end(),
            [&nodeIDSet](Link* link) {
                if (nodeIDSet.contains(link->id)) {

                    /*auto itr = link->from_channel->attachedLinks.find(link);
                    if (itr != link->from_channel->attachedLinks.end())
                        link->from_channel->attachedLinks.erase(itr);

                    itr = link->to_channel->attachedLinks.find(link);
                    if (itr != link->to_channel->attachedLinks.end())
                        link->to_channel->attachedLinks.erase(itr);

                    if (link->to_channel->data != nullptr)
                    {
                        link->to_channel->data = nullptr;
                    }*/
                    link->to_node->MarkDirty();

                    delete link;
                    return true;
                }
                return false;
            }),
        links.end());
}

void Graph::PropagateData(Node* node)
{
    for (Channel* outPutChannel : node->outputs) {
        // Find all links starting from this output channel
        for (Link* link : links) {
            if (link->from_channel == outPutChannel) {
                // Only propagate if output has valid data
                if (outPutChannel->data != nullptr) {
                    link->to_channel->data = outPutChannel->data;
                }
            }
        }
    }
}

bool Graph::Evaluate()
{
    for (Node* n : nodes)
    {
        if (n->IsDirty())
        {
            SetChanged(true);
            break;
        }
    }
    if (!IsChanged()) return false;

    TopoSort(nodes);

    for (Node* n : nodes)
    {
        n->Evaluate();
        PropagateData(n);
    }
    SetChanged(false);
    return true;;
}

vector<int> Graph::GetSelectedNodes()
{
    int nSelNodes = ImNodes::NumSelectedNodes();
    vector<int> selectedNodeIds;
    if (nSelNodes)
    {
        selectedNodeIds.resize(nSelNodes);
        ImNodes::GetSelectedNodes(&selectedNodeIds[0]);
    }
    return selectedNodeIds;
}

vector<int> Graph::GetSelectedLinks()
{
    int nSelLinks = ImNodes::NumSelectedLinks();
    vector<int> selectedLinkIds;
    if (nSelLinks)
    {
        selectedLinkIds.resize(nSelLinks);
        ImNodes::GetSelectedLinks(&selectedLinkIds[0]);
    }
    return selectedLinkIds;
}

void Graph::ShowProperties()
{
    vector<int> nodeIds = GetSelectedNodes();
    if (!nodeIds.size())
        return;
    Node* node = GetNodeFromId(nodeIds[0]);
    if (node)
        node->CreateImNodeProperties();
}

Node* Graph::GetNodeFromChannelID(int channelId, Channel*& channel)
{
    for (Node* node : nodes) {
        for (Channel* c : node->inputs) {
            if (c->id == channelId)
            {
                channel = c;
                return node;
            }
        }
        for (Channel* c : node->outputs) {
            if (c->id == channelId)
            {
                channel = c;
                return node;
            }
        }
    }
    return nullptr;
}

Channel* Graph::findChannelFromId(int socket_id) {
    for (Node* node : nodes) {
        for (Channel* c : node->inputs)
            if (c->id == socket_id)
                return c;

        for (Channel* c : node->outputs)
            if (c->id == socket_id)
                return c;
    }
    return nullptr;
}

Node* Graph::GetNodeFromId(int nodeId)
{
    for (Node* node : nodes) {
        if (node->id == nodeId)
            return node;
    }
    return nullptr;
}

Link* Graph::GetLinkFromId(int nodeId)
{
    for (Link* link : links) {
        if (link->id == nodeId)
            return link;
    }
    return nullptr;
}

