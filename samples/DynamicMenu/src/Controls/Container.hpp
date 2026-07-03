#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "Control.hpp"

namespace DynamicMenu::Controls {

// Holds a set of controls in positions relative to this container control.
// Port of Controls/Container.cs.
class Container : public Control {
public:
    [[nodiscard]] const std::vector<std::shared_ptr<Control>>& Controls() const { return controls_; }

    // Adds a control to the container immediately.
    void AddControl(std::shared_ptr<Control> control) {
        control->Parent = this;
        controls_.push_back(std::move(control));
    }

    // Removes a control from the container immediately.
    void RemoveControl(const std::shared_ptr<Control>& control) {
        auto it = std::find(controls_.begin(), controls_.end(), control);
        if (it != controls_.end()) controls_.erase(it);
    }

    // Removes all controls from the container immediately (CNA addition: the
    // original manipulates the exposed `Controls` list's `Clear()` directly
    // via `Content.Load<Container>` re-population; see DynamicMenuGame's
    // LoadControls()).
    void ClearControls() { controls_.clear(); }

    // Marks a control to be added during the next container update.
    void MarkForAdd(std::shared_ptr<Control> control) { markedForAdd_.push_back(std::move(control)); }

    // Marks a control to be removed during the next container update.
    void MarkForRemove(std::shared_ptr<Control> control) { markedForRemove_.push_back(std::move(control)); }

    void Initialize() override {
        for (auto& control : controls_) {
            control->Parent = this;
            control->Initialize();
        }
    }

    void LoadContent(GraphicsDevice& graphics, ContentManager& content) override {
        Control::LoadContent(graphics, content);
        for (auto& control : controls_) {
            control->LoadContent(graphics, content);
        }
    }

    void Update(const GameTime& gameTime, const std::vector<GestureSample>& gestures) override {
        Control::Update(gameTime, gestures);

        if (Visible) {
            for (auto& control : controls_) {
                if (control->Visible) {
                    control->Update(gameTime, gestures);
                }
            }
        }

        for (auto& control : markedForRemove_) {
            RemoveControl(control);
        }
        markedForRemove_.clear();

        for (auto& control : markedForAdd_) {
            AddControl(control);
        }
        markedForAdd_.clear();
    }

    void Draw(const GameTime& gameTime, SpriteBatch& spriteBatch) override {
        Control::Draw(gameTime, spriteBatch);

        if (Visible) {
            for (auto& control : controls_) {
                if (control->Visible) {
                    control->Draw(gameTime, spriteBatch);
                }
            }
        }
    }

    // Gets a child control by the control's Name.
    [[nodiscard]] std::shared_ptr<Control> FindControlByName(const std::string& name) const {
        for (const auto& control : controls_) {
            if (control->Name == name) {
                return control;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::shared_ptr<Control>> controls_;
    std::vector<std::shared_ptr<Control>> markedForRemove_;
    std::vector<std::shared_ptr<Control>> markedForAdd_;
};

} // namespace DynamicMenu::Controls
