#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Microsoft/Xna/Framework/Point.hpp"

namespace Pathfinding {

using namespace Microsoft::Xna::Framework;

struct MapData {
    int NumberRows    = 0;
    int NumberColumns = 0;
    Point Start;
    Point End;
    std::vector<Point> Barriers;
};

// Extract text between <tag>...</tag>
static std::string extractTag(const std::string& xml, const std::string& tag) {
    std::string open  = "<" + tag + ">";
    std::string close = "</" + tag + ">";
    auto s = xml.find(open);
    auto e = xml.find(close);
    if (s == std::string::npos || e == std::string::npos) return "";
    return xml.substr(s + open.size(), e - s - open.size());
}

inline MapData ParseMapXML(const std::string& path) {
    std::ifstream f(path);
    std::string xml((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    MapData m;
    m.NumberRows    = std::stoi(extractTag(xml, "NumberRows"));
    m.NumberColumns = std::stoi(extractTag(xml, "NumberColumns"));

    {
        std::istringstream ss(extractTag(xml, "Start"));
        ss >> m.Start.X >> m.Start.Y;
    }
    {
        std::istringstream ss(extractTag(xml, "End"));
        ss >> m.End.X >> m.End.Y;
    }
    {
        std::istringstream ss(extractTag(xml, "Barriers"));
        int x, y;
        while (ss >> x >> y)
            m.Barriers.push_back(Point(x, y));
    }
    return m;
}

} // namespace Pathfinding
