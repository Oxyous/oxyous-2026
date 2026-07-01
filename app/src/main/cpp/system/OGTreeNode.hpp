//
// Created by Mr Steven J Baldwin on 01/07/2026.
//

#ifndef OXYOUS_2026_OGTREENODE_HPP
#define OXYOUS_2026_OGTREENODE_HPP

#include "../includes.hpp"

template<class T>
class OGTreeNode {
public:
    OGTreeNode(const T& data, const std::string& name = "") : m_data(data), m_parent(nullptr), m_name(name) {}

    void addChild(std::shared_ptr<OGTreeNode> child) {
        child->m_parent = this;
        m_children.push_back(child);
    }

    void removeChild(std::shared_ptr<OGTreeNode> child) {
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) {
            (*it)->m_parent = nullptr;
            m_children.erase(it);
        }
    }

    T& getData() { return m_data; }
    const T& getData() const { return m_data; }

    OGTreeNode* getParent() { return m_parent; }
    const OGTreeNode* getParent() const { return m_parent; }

    std::vector<std::shared_ptr<OGTreeNode>>& getChildren() { return m_children; }
    const std::vector<std::shared_ptr<OGTreeNode>>& getChildren() const { return m_children; }

    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
private:
    T m_data;
    OGTreeNode* m_parent;
    std::vector<std::shared_ptr<OGTreeNode>> m_children;
    std::string m_name;
};

#endif //OXYOUS_2026_OGTREENODE_HPP
