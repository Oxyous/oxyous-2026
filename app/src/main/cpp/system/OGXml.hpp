//
// Created by Mr Steven J Baldwin on 01/07/2026.
//

#ifndef OXYOUS_2026_OGXML_HPP
#define OXYOUS_2026_OGXML_HPP

#include "../includes.hpp"
#include "OGTreeNode.hpp"

class OGXmlNode
{
public:
    friend class OGXml;
public:
    OGXmlNode() { };
    ~OGXmlNode() = default;

    void setName(const std::string& name) { m_name = name; }
    void setValue(const std::string& value) { m_value = value; }

    void addAttribute(const std::string& key, const std::string& value) { m_attributes[key] = value; }
    void addChild(std::shared_ptr<OGXmlNode> child) { m_children.push_back(child); }

    const std::string& getName() const { return m_name; }
    const std::string& getValue() const { return m_value; }
    const std::map<std::string, std::string>& getAttributes() const { return m_attributes; }
    const std::vector<std::shared_ptr<OGXmlNode>>& getChildren() const { return m_children; }
protected:
    std::string m_name;
    std::string m_value;
    std::map<std::string, std::string> m_attributes;
    std::vector<std::shared_ptr<OGXmlNode>> m_children;
    OGXmlNode* m_parent = nullptr;
};

using XMLTreeNode = OGTreeNode<OGXmlNode>;

class OGXml
{
public:
    OGXml() {

    };
    ~OGXml() = default;

    void setRoot(std::shared_ptr<OGXmlNode> root) { m_root = root; }

    static bool loadGXml(const std::string& filePath, std::vector<std::unique_ptr<OGXmlNode>>& nodes);
    static void parse(const std::string& data, XMLTreeNode* rootNode);
    static XMLTreeNode* generateTree(OGXmlNode* node, XMLTreeNode* treeNode);
    static void Dissect(const std::string& data, std::vector<std::string>&parts);

    std::shared_ptr<OGXmlNode> getRoot() { return m_root; }
    const std::shared_ptr<OGXmlNode> getRoot() const { return m_root; }

private:
    std::shared_ptr<OGXmlNode> m_root;
    static bool isSpecialCharacter(char c);
};



#endif //OXYOUS_2026_OGXML_HPP
