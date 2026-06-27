#pragma once
#include <deque>
#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace PathDrawing {

using namespace Microsoft::Xna::Framework;

class WaypointList {
    std::deque<Vector2> data_;
public:
    void Clear()                  { data_.clear(); }
    void Enqueue(const Vector2& v){ data_.push_back(v); }
    void Dequeue()                { data_.pop_front(); }
    Vector2 Peek() const          { return data_.front(); }
    int Count() const             { return (int)data_.size(); }
    Vector2 operator[](int i) const { return data_[i]; }
};

} // namespace PathDrawing
