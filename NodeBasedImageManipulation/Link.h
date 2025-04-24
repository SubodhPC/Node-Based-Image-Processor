#pragma once
class Node;
struct Channel;
struct Link {
    int id = 0;
    Node* from_node = 0;
    Node* to_node = 0;
    Channel* from_channel = 0;
    Channel* to_channel = 0;

    Link(unsigned int id, Node* fromNode, Node* toNode, Channel* fromChannel, Channel* toChannel)
        : id(id), from_node(fromNode), to_node(toNode), from_channel(fromChannel), to_channel(toChannel) {
    }

    void* GetPropogatedData()
    {
        if (from_channel->data)
            return from_channel->data;

        return nullptr;
    }

    ~Link()
    {
        auto itr = from_channel->attachedLinks.find(this);
        if (itr != from_channel->attachedLinks.end())
            from_channel->attachedLinks.erase(itr);

        itr = to_channel->attachedLinks.find(this);
        if (itr != to_channel->attachedLinks.end())
            to_channel->attachedLinks.erase(itr);

        if (to_channel->data != nullptr)
        {
            to_channel->data = nullptr;
        }

        from_node = nullptr;
        to_node = nullptr;
        from_channel = nullptr;
        to_channel = nullptr;
    }
};