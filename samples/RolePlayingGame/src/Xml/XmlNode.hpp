#pragma once

// XmlNode.hpp -- Minimal, sample-local XML DOM parser used by RolePlayingGame's
// content loader (see ../ContentLoader.hpp and missing.md's "XML data loader"
// section). RolePlayingGame ships 281 small XML data files (armor, weapons,
// monsters, maps, quests, etc.) in the stock XNA "IntermediateSerializer"
// format: <Asset Type="Namespace.ClassName"><PropertyName>value</PropertyName>...
// Element names match C# property names 1:1, nested objects are nested
// elements, List<T> is a parent element with repeated <Item> children, and
// Point/Vector2 are two/three space-separated numbers as element text.
// This parser only needs to handle that one well-formed, attribute-light
// dialect (plus the single "Type" attribute on <Asset>) -- not general XML.

#include <cctype>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace RolePlaying::Xml {

class XmlNode {
public:
    std::string tag;
    std::string text;
    std::map<std::string, std::string> attributes;
    std::vector<std::shared_ptr<XmlNode>> children;

    // Returns the first child with the given tag name, or nullptr.
    XmlNode* Child(const std::string& name) const {
        for (auto& c : children) {
            if (c->tag == name) return c.get();
        }
        return nullptr;
    }

    // Returns all children with the given tag name (used for <Item> lists).
    std::vector<XmlNode*> Children(const std::string& name) const {
        std::vector<XmlNode*> result;
        for (auto& c : children) {
            if (c->tag == name) result.push_back(c.get());
        }
        return result;
    }

    // Concatenated text of all direct text nodes (trimmed).
    std::string Text() const {
        std::string t = text;
        size_t start = t.find_first_not_of(" \t\r\n");
        size_t end = t.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return t.substr(start, end - start + 1);
    }
};

namespace detail {

inline void SkipWhitespaceAndComments(const std::string& s, size_t& i) {
    for (;;) {
        while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
        if (i + 3 < s.size() && s.compare(i, 4, "<!--") == 0) {
            size_t end = s.find("-->", i + 4);
            i = (end == std::string::npos) ? s.size() : end + 3;
            continue;
        }
        break;
    }
}

// Parses one element starting at '<' (i points at '<'). Advances i past '>'.
inline std::shared_ptr<XmlNode> ParseElement(const std::string& s, size_t& i) {
    if (s[i] != '<') throw std::runtime_error("XmlNode: expected '<'");
    i++;
    auto node = std::make_shared<XmlNode>();

    size_t tagStart = i;
    while (i < s.size() && s[i] != ' ' && s[i] != '>' && s[i] != '/' && s[i] != '\t' &&
           s[i] != '\n' && s[i] != '\r')
        i++;
    node->tag = s.substr(tagStart, i - tagStart);

    // attributes
    for (;;) {
        while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
        if (i >= s.size()) break;
        if (s[i] == '/' || s[i] == '>') break;
        size_t nameStart = i;
        while (i < s.size() && s[i] != '=' && !std::isspace((unsigned char)s[i])) i++;
        std::string attrName = s.substr(nameStart, i - nameStart);
        while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
        if (i < s.size() && s[i] == '=') {
            i++;
            while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
            char quote = s[i];
            i++;
            size_t valStart = i;
            while (i < s.size() && s[i] != quote) i++;
            node->attributes[attrName] = s.substr(valStart, i - valStart);
            i++;
        }
    }

    if (i < s.size() && s[i] == '/') {
        // self-closing
        i += 2; // skip "/>"
        return node;
    }
    i++; // skip '>'

    // content: text and/or child elements, until matching close tag
    std::string textAccum;
    for (;;) {
        if (i >= s.size()) break;
        if (s[i] == '<') {
            if (i + 1 < s.size() && s[i + 1] == '/') {
                // closing tag
                i += 2;
                while (i < s.size() && s[i] != '>') i++;
                i++; // skip '>'
                break;
            }
            if (i + 3 < s.size() && s.compare(i, 4, "<!--") == 0) {
                size_t end = s.find("-->", i + 4);
                i = (end == std::string::npos) ? s.size() : end + 3;
                continue;
            }
            node->children.push_back(ParseElement(s, i));
        } else {
            textAccum += s[i];
            i++;
        }
    }
    node->text = textAccum;
    return node;
}

} // namespace detail

// Parses a full XML document and returns its root element (skipping the
// <?xml ...?> declaration and any BOM).
inline std::shared_ptr<XmlNode> ParseDocument(const std::string& contents) {
    size_t i = 0;
    // skip UTF-8 BOM
    if (contents.size() >= 3 &&
        (unsigned char)contents[0] == 0xEF &&
        (unsigned char)contents[1] == 0xBB &&
        (unsigned char)contents[2] == 0xBF) {
        i = 3;
    }
    detail::SkipWhitespaceAndComments(contents, i);
    if (i + 1 < contents.size() && contents[i] == '<' && contents[i + 1] == '?') {
        size_t end = contents.find("?>", i);
        i = (end == std::string::npos) ? contents.size() : end + 2;
    }
    detail::SkipWhitespaceAndComments(contents, i);
    return detail::ParseElement(contents, i);
}

} // namespace RolePlaying::Xml
