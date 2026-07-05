#pragma once

// ContentEntry.hpp -- C++ port of RolePlayingGameData/ContentEntry.cs.

#include <memory>
#include <string>

namespace RolePlayingGameData {

// A description of a piece of content and quantity for various purposes.
// T's content is not automatically loaded, as the content path may be incomplete
// (matches the original's remark) -- callers populate Content after construction.
template <typename T>
class ContentEntry {
public:
    std::string ContentName;
    std::shared_ptr<T> Content;
    int Count = 1;

    virtual ~ContentEntry() = default;
};

} // namespace RolePlayingGameData
