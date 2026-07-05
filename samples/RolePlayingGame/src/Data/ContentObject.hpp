#pragma once

// ContentObject.hpp -- C++ port of RolePlayingGameData/ContentObject.cs.

#include <string>

namespace RolePlayingGameData {

// Base type for all of the data types that load via the (XML-backed) content loader.
class ContentObject {
public:
    virtual ~ContentObject() = default;

    const std::string& AssetName() const { return assetName_; }
    void SetAssetName(const std::string& v) { assetName_ = v; }

private:
    std::string assetName_;
};

} // namespace RolePlayingGameData
