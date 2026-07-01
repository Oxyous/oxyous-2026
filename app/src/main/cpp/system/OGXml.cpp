//
// Created by Mr Steven J Baldwin on 01/07/2026.
//

#include "OGXml.hpp"
#include "../resources/ResourceManager.hpp"


bool OGXml::loadGXml(const std::string &filePath, std::vector<std::unique_ptr<OGXmlNode>> &nodes) {
    std::string buffer;

    if (!RESOURCE_MANAGER->readFileFromAssets(filePath, buffer)) {
        return false;
    }

    nodes.clear();
    std::vector<OGXmlNode *> parentStack;

    auto trim = [](std::string value) {
        size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
            begin++;
        }

        size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            end--;
        }

        return value.substr(begin, end - begin);
    };

    size_t cursor = 0;
    while (cursor < buffer.size()) {
        size_t openTag = buffer.find('<', cursor);
        if (openTag == std::string::npos) {
            break;
        }

        if (openTag > cursor && !parentStack.empty()) {
            std::string value = trim(buffer.substr(cursor, openTag - cursor));
            if (!value.empty()) {
                parentStack.back()->setValue(value);
            }
        }

        size_t closeTag = buffer.find('>', openTag + 1);
        if (closeTag == std::string::npos) {
            break;
        }

        std::string tag = trim(buffer.substr(openTag + 1, closeTag - openTag - 1));
        cursor = closeTag + 1;

        if (tag.empty() || tag[0] == '?' || tag.rfind("!--", 0) == 0) {
            continue;
        }

        if (tag[0] == '/') {
            if (!parentStack.empty()) {
                parentStack.pop_back();
            }
            continue;
        }

        bool selfClosing = false;
        if (!tag.empty() && tag.back() == '/') {
            selfClosing = true;
            tag.pop_back();
            tag = trim(tag);
        }

        auto node = std::make_unique<OGXmlNode>();

        size_t nameEnd = 0;
        while (nameEnd < tag.size() && !std::isspace(static_cast<unsigned char>(tag[nameEnd]))) {
            nameEnd++;
        }
        node->setName(tag.substr(0, nameEnd));

        size_t index = nameEnd;
        while (index < tag.size()) {
            while (index < tag.size() && std::isspace(static_cast<unsigned char>(tag[index]))) {
                index++;
            }

            if (index >= tag.size()) {
                break;
            }

            size_t keyStart = index;
            while (index < tag.size() && tag[index] != '=' && !std::isspace(static_cast<unsigned char>(tag[index]))) {
                index++;
            }
            std::string key = tag.substr(keyStart, index - keyStart);
            if (key.empty()) {
                break;
            }

            while (index < tag.size() && std::isspace(static_cast<unsigned char>(tag[index]))) {
                index++;
            }
            if (index < tag.size() && tag[index] == '=') {
                index++;
            }
            while (index < tag.size() && std::isspace(static_cast<unsigned char>(tag[index]))) {
                index++;
            }

            std::string value;
            if (index < tag.size() && (tag[index] == '"' || tag[index] == '\'')) {
                char quote = tag[index++];
                size_t valueStart = index;
                while (index < tag.size() && tag[index] != quote) {
                    index++;
                }
                value = tag.substr(valueStart, index - valueStart);
                if (index < tag.size()) {
                    index++;
                }
            } else {
                size_t valueStart = index;
                while (index < tag.size() && !std::isspace(static_cast<unsigned char>(tag[index]))) {
                    index++;
                }
                value = tag.substr(valueStart, index - valueStart);
            }

            node->addAttribute(key, value);
        }

        OGXmlNode *rawNode = node.get();
        if (!parentStack.empty()) {
            rawNode->m_parent = parentStack.back();
            parentStack.back()->addChild(std::shared_ptr<OGXmlNode>(rawNode, [](OGXmlNode *) {}));
        }

        nodes.push_back(std::move(node));

        if (!selfClosing) {
            parentStack.push_back(rawNode);
        }
    }

    return true;
}

void OGXml::parse(const std::string &data, XMLTreeNode *rootNode) {

}

XMLTreeNode *OGXml::generateTree(OGXmlNode *node, XMLTreeNode *treeNode) {
    return nullptr;
}

void OGXml::Dissect(const std::string &data, std::vector<std::string> &parts) {

}

bool OGXml::isSpecialCharacter(char c) {
    if (c == ' ' || c == '<' || c == '>' || c == '/') {
        return true;
    }

    return false;
}
