#pragma once

// Direct port of PeerToPeerGame.cs. Sample showing how to implement a simple
// multiplayer network session using a peer-to-peer network topology: unlike
// ClientServerSample's single-authority model (one host machine simulates every tank)
// or NetworkPrediction's client-side prediction/smoothing on top of that same
// authority split, this sample has *no* host/client distinction for simulation
// purposes at all -- every machine fully simulates and owns its own locally
// controlled tanks (UpdateLocalGamer() below runs the identical Tank::Update() on
// every peer) and simply broadcasts its own resulting state to everyone in the
// session each frame (LocalNetworkGamer::SendData() with no explicit recipient).
// Remote tanks are wholesale-overwritten from whatever the last packet said, with no
// throttled send rate, prediction, or smoothing (contrast NetworkPrediction, which
// adds all of that on top of this same basic session/packet plumbing). gamer.IsHost
// is used only for the "(host)" label -- it plays no role in who updates what.
//
// See missing.md: this sample needed **no** networking workarounds beyond what
// ClientServerSample/NetworkPrediction already established as resolved (DEFERRED.md
// items #19/#20/#21) -- and, notably, it doesn't touch NetworkSession::SessionProperties
// at all, so DEFERRED.md item #27 (found while porting NetworkPrediction) simply never
// comes up here either.

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/GameComponentCollection.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/GamerServices/GamerServicesComponent.hpp>
#include <Microsoft/Xna/Framework/GamerServices/Gamer.hpp>
#include <Microsoft/Xna/Framework/GamerServices/SignedInGamer.hpp>
#include <Microsoft/Xna/Framework/GamerServices/SignedInGamerCollection.hpp>
#include <Microsoft/Xna/Framework/GamerServices/Guide.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSession.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSessionType.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSessionProperties.hpp>
#include <Microsoft/Xna/Framework/Net/AvailableNetworkSessionCollection.hpp>
#include <Microsoft/Xna/Framework/Net/LocalNetworkGamer.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkGamer.hpp>
#include <Microsoft/Xna/Framework/Net/PacketWriter.hpp>
#include <Microsoft/Xna/Framework/Net/PacketReader.hpp>
#include <Microsoft/Xna/Framework/Net/SendDataOptions.hpp>
#include <Microsoft/Xna/Framework/Net/GamerJoinedEventArgs.hpp>
#include <Microsoft/Xna/Framework/Net/NetworkSessionEndedEventArgs.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>

#include "Tank.hpp"

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Xna::Framework::GamerServices;
using namespace Microsoft::Xna::Framework::Net;

namespace PeerToPeer {

class PeerToPeerGame : public Game {
public:
    PeerToPeerGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(kScreenWidth);
        graphics_->setPreferredBackBufferHeightProperty(kScreenHeight);

        getContentProperty().setRootDirectoryProperty("Content");

        // Matches the C# original's own `Components.Add(new GamerServicesComponent(this));`.
        // A real GamerServicesComponent is safe to construct and add here directly --
        // ClientServerSample's three original workarounds (DEFERRED.md items #19/#20/#21)
        // are all resolved upstream now, reconfirmed a second time by NetworkPrediction and
        // now a third time by this sample.
        gamerServices_ = std::make_unique<GamerServicesComponent>(*this);
        getComponentsProperty().Add(gamerServices_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "PeerToPeerGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        font_.emplace(getContentProperty().Load<SpriteFont>("font"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        if (networkSession_ == nullptr) {
            // If we are not in a network session, update the menu screen that will let
            // us create or join one.
            UpdateMenuScreen();
        } else {
            // If we are in a network session, update it.
            UpdateNetworkSession();
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        if (networkSession_ == nullptr) {
            // If we are not in a network session, draw the menu screen that will let us
            // create or join one.
            DrawMenuScreen();
        } else {
            // If we are in a network session, draw it.
            DrawNetworkSession();
        }

        Game::Draw(gameTime);
    }

private:
    static constexpr int kScreenWidth = 1067;
    static constexpr int kScreenHeight = 600;
    static constexpr int kMaxGamers = 16;
    static constexpr int kMaxLocalGamers = 4;

    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont>  font_;

    KeyboardState currentKeyboardState_;
    GamePadState  currentGamePadState_;

    NetworkSession* networkSession_ = nullptr;

    PacketWriter packetWriter_;
    PacketReader packetReader_;

    std::string errorMessage_;

    // Ownership for the Tank objects referenced by each NetworkGamer's Tag (std::any).
    std::vector<std::unique_ptr<Tank>> tanks_;

    std::unique_ptr<GamerServicesComponent> gamerServices_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // Menu screen provides options to create or join network sessions.
    void UpdateMenuScreen() {
        if (getIsActiveProperty()) {
            if (Gamer::getSignedInGamersProperty()->getCountProperty() == 0) {
                // If there are no profiles signed in, we cannot proceed. Show the Guide
                // so the user can sign in. (No-op in CNA -- see missing.md; never
                // reached in practice, since GamerServicesComponent already populates
                // SignedInGamers with stub gamers before the first Update().)
                Guide::ShowSignIn(kMaxLocalGamers, false);
            } else if (IsPressed(Keys::A, Buttons::A)) {
                // Create a new session?
                CreateSession();
            } else if (IsPressed(Keys::B, Buttons::B)) {
                // Join an existing session?
                JoinSession();
            }
        }
    }

    // Starts hosting a new network session.
    void CreateSession() {
        try {
            networkSession_ = NetworkSession::Create(NetworkSessionType::SystemLink,
                                                      kMaxLocalGamers, kMaxGamers);
            HookSessionEvents();
        } catch (const System::Exception& e) {
            errorMessage_ = e.getMessageProperty();
        }
    }

    // Joins an existing network session.
    void JoinSession() {
        try {
            // Search for sessions.
            AvailableNetworkSessionCollection availableSessions =
                NetworkSession::Find(NetworkSessionType::SystemLink,
                                      kMaxLocalGamers, NetworkSessionProperties());

            if (availableSessions.getCountProperty() == 0) {
                errorMessage_ = "No network sessions found.";
                return;
            }

            // Join the first session we found.
            networkSession_ = NetworkSession::Join(&availableSessions[0]);
            HookSessionEvents();
        } catch (const System::Exception& e) {
            errorMessage_ = e.getMessageProperty();
        }
    }

    // After creating or joining a network session, we must subscribe to some events so
    // we will be notified when the session changes state.
    void HookSessionEvents() {
        networkSession_->GamerJoined += [this](System::Object* sender, const GamerJoinedEventArgs& e) {
            GamerJoinedEventHandler(sender, e);
        };
        networkSession_->SessionEnded += [this](System::Object* sender, const NetworkSessionEndedEventArgs& e) {
            SessionEndedEventHandler(sender, e);
        };
    }

    // This event handler will be called whenever a new gamer joins the session. We use
    // it to allocate a Tank object, and associate it with the new gamer.
    void GamerJoinedEventHandler(System::Object* /*sender*/, const GamerJoinedEventArgs& e) {
        NetworkGamer* gamer = e.getGamerProperty();
        int gamerIndex = 0;
        for (NetworkGamer* g : networkSession_->getAllGamersProperty()) {
            if (g == gamer) break;
            ++gamerIndex;
        }

        tanks_.push_back(std::make_unique<Tank>(gamerIndex, getContentProperty(),
                                                 kScreenWidth, kScreenHeight));
        gamer->setTagProperty(std::any(tanks_.back().get()));
    }

    // Event handler notifies us when the network session has ended.
    void SessionEndedEventHandler(System::Object* /*sender*/, const NetworkSessionEndedEventArgs& e) {
        errorMessage_ = ToString(e.getEndReasonProperty());

        networkSession_->Dispose();
        networkSession_ = nullptr;
    }

    // Updates the state of the network session, moving the tanks around and
    // synchronizing their state over the network.
    void UpdateNetworkSession() {
        // Update our locally controlled tanks, and send their latest position data to
        // everyone in the session.
        for (LocalNetworkGamer* gamer : networkSession_->getLocalGamersProperty()) {
            UpdateLocalGamer(gamer);
        }

        // Pump the underlying session object.
        networkSession_->Update();

        // Make sure the session has not ended.
        if (networkSession_ == nullptr)
            return;

        // Read any packets telling us the positions of remotely controlled tanks.
        for (LocalNetworkGamer* gamer : networkSession_->getLocalGamersProperty()) {
            ReadIncomingPackets(gamer);
        }
    }

    // Helper for updating a locally controlled gamer.
    void UpdateLocalGamer(LocalNetworkGamer* gamer) {
        // Look up what tank is associated with this local player.
        Tank* localTank = std::any_cast<Tank*>(gamer->getTagProperty());

        // Update the tank.
        ReadTankInputs(localTank, gamer->getSignedInGamerProperty()->getPlayerIndexProperty());

        localTank->Update();

        // Write the tank state into a network packet.
        packetWriter_.Write(localTank->Position);
        packetWriter_.Write(localTank->TankRotation);
        packetWriter_.Write(localTank->TurretRotation);

        // Send the data to everyone in the session (no explicit recipient -- unlike
        // ClientServerSample's client->server-only send, every peer broadcasts to
        // every other peer here; this *is* the peer-to-peer topology this sample
        // demonstrates).
        gamer->SendData(packetWriter_, SendDataOptions::InOrder);
    }

    // Helper for reading incoming network packets.
    void ReadIncomingPackets(LocalNetworkGamer* gamer) {
        // Keep reading as long as incoming packets are available.
        while (gamer->getIsDataAvailableProperty()) {
            NetworkGamer* sender = nullptr;

            // Read a single packet from the network.
            gamer->ReceiveData(packetReader_, sender);

            // Discard packets sent by local gamers: we already know their state!
            if (sender == nullptr || sender->getIsLocalProperty())
                continue;

            // Look up the tank associated with whoever sent this packet.
            Tank* remoteTank = std::any_cast<Tank*>(sender->getTagProperty());

            // Read the state of this tank from the network packet.
            remoteTank->Position = packetReader_.ReadVector2();
            remoteTank->TankRotation = packetReader_.ReadSingle();
            remoteTank->TurretRotation = packetReader_.ReadSingle();
        }
    }

    // Draws the startup screen used to create and join network sessions.
    void DrawMenuScreen() {
        std::string message;

        if (!errorMessage_.empty())
            message += "Error:\n" + errorMessage_ + "\n\n";

        message += "A = create session\nB = join session";

        spriteBatch_->Begin();

        spriteBatch_->DrawString(*font_, message, Vector2(161.0f, 161.0f), Color(0, 0, 0, 255));
        spriteBatch_->DrawString(*font_, message, Vector2(160.0f, 160.0f), Color(255, 255, 255, 255));

        DrawHelpOverlay();
        spriteBatch_->End();
    }

    // Draws the state of an active network session.
    void DrawNetworkSession() {
        spriteBatch_->Begin();

        // For each person in the session...
        for (NetworkGamer* gamer : networkSession_->getAllGamersProperty()) {
            // Look up the tank object belonging to this network gamer.
            Tank* tank = std::any_cast<Tank*>(gamer->getTagProperty());

            // Draw the tank.
            tank->Draw(*spriteBatch_);

            // Draw a gamertag label.
            std::string label = gamer->getGamertagProperty();
            Color labelColor = Color(0, 0, 0, 255);
            Vector2 labelOffset(100.0f, 150.0f);

            if (gamer->getIsHostProperty())
                label += " (host)";

            // Flash the gamertag to yellow when the player is talking.
            if (gamer->getIsTalkingProperty())
                labelColor = Color(255, 255, 0, 255);

            spriteBatch_->DrawString(*font_, label, tank->Position, labelColor, 0.0f,
                                      labelOffset, 0.6f, SpriteEffects::None, 0.0f);
        }

        DrawHelpOverlay();
        spriteBatch_->End();
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        float sx = (float)((vp.getWidthProperty()  - hw) / 2);
        float sy = (float)((vp.getHeightProperty() - hh) / 2);
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
    }

    // Handles input.
    void HandleInput() {
        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_  = GamePad::GetState(PlayerIndex::One);

        // Check for exit.
        if (getIsActiveProperty() && IsPressed(Keys::Escape, Buttons::Back)) {
            Exit();
        }
    }

    // Checks if the specified button is pressed on either keyboard or gamepad.
    bool IsPressed(Keys key, Buttons button) {
        return currentKeyboardState_.IsKeyDown(key) || currentGamePadState_.IsButtonDown(button);
    }

    // Reads input data from keyboard and gamepad, and stores it into the specified
    // tank object.
    void ReadTankInputs(Tank* tank, PlayerIndex playerIndex) {
        // Read the gamepad.
        GamePadState gamePad = GamePad::GetState(playerIndex);

        Vector2 tankInput = gamePad.getThumbSticksProperty().getLeftProperty();
        Vector2 turretInput = gamePad.getThumbSticksProperty().getRightProperty();

        // Read the keyboard.
        KeyboardState keyboard = Keyboard::GetState(playerIndex);

        if (keyboard.IsKeyDown(Keys::Left))
            tankInput.X = -1.0f;
        else if (keyboard.IsKeyDown(Keys::Right))
            tankInput.X = 1.0f;

        if (keyboard.IsKeyDown(Keys::Up))
            tankInput.Y = 1.0f;
        else if (keyboard.IsKeyDown(Keys::Down))
            tankInput.Y = -1.0f;

        if (keyboard.IsKeyDown(Keys::A))
            turretInput.X = -1.0f;
        else if (keyboard.IsKeyDown(Keys::D))
            turretInput.X = 1.0f;

        if (keyboard.IsKeyDown(Keys::W))
            turretInput.Y = 1.0f;
        else if (keyboard.IsKeyDown(Keys::S))
            turretInput.Y = -1.0f;

        // Normalize the input vectors.
        if (tankInput.Length() > 1)
            tankInput.Normalize();

        if (turretInput.Length() > 1)
            turretInput.Normalize();

        // Store these input values into the tank object.
        tank->TankInput = tankInput;
        tank->TurretInput = turretInput;
    }

    static std::string ToString(NetworkSessionEndReason reason) {
        switch (reason) {
            case NetworkSessionEndReason::ClientSignedOut:    return "ClientSignedOut";
            case NetworkSessionEndReason::HostEndedSession:   return "HostEndedSession";
            case NetworkSessionEndReason::RemovedByHost:      return "RemovedByHost";
            case NetworkSessionEndReason::Disconnected:       return "Disconnected";
            default:                                         return "Unknown";
        }
    }
};

} // namespace PeerToPeer
