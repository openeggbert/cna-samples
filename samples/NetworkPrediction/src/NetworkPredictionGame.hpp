#pragma once

// Direct port of NetworkPredictionGame.cs. Sample showing how to use prediction and
// smoothing to compensate for the effects of network latency, and for the low packet
// send rates needed to conserve network bandwidth.
//
// See missing.md for the one significant deviation from the C# original: CNA's
// NetworkSession::SessionProperties has no mutable accessor and is never replicated
// over the wire (DEFERRED.md item #27, new). The C# original relies on
// `networkSession.SessionProperties[i] = value` (host) / reading it back (clients) to
// automatically replicate the network-quality/prediction/smoothing toggles to every
// machine in the session. This port instead sends those same four values explicitly, in
// a small host-authoritative "options packet" distinguished from ordinary tank-state
// packets by a leading type byte -- see SendOptionsPacket()/ReadIncomingPackets() below.
// Every other aspect of this sample (prediction, smoothing, RollingAverage, throttled
// send rate, simulated latency/packet loss) uses real, already-working CNA APIs with no
// workaround needed.

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
#include <System/TimeSpan.hpp>

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

namespace NetworkPrediction {

class NetworkPredictionGame : public Game {
public:
    NetworkPredictionGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(kScreenWidth);
        graphics_->setPreferredBackBufferHeightProperty(kScreenHeight);

        getContentProperty().setRootDirectoryProperty("Content");

        // Matches the C# original's own `Components.Add(new GamerServicesComponent(this));`.
        // A real GamerServicesComponent can be constructed and added here directly -- the
        // three ClientServerSample-era workarounds (DEFERRED.md items #19/#20/#21) are all
        // resolved upstream now, and this sample's own Update() loop shape confirms it (see
        // missing.md): its Initialize() populates Gamer::SignedInGamers with 4 stub gamers
        // before this game's first Update() call, and NetworkSession::Create/Find/Join no
        // longer hang now that GamerServicesDispatcher::Update() is wired up.
        gamerServices_ = std::make_unique<GamerServicesComponent>(*this);
        getComponentsProperty().Add(gamerServices_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "NetworkPredictionGame";
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
            UpdateNetworkSession(gameTime);
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

    // Leading byte written to every network packet, distinguishing an ordinary tank-state
    // packet from the host-authoritative options packet -- see this file's own top-of-file
    // comment and missing.md for why this NOXNA addition is needed (no equivalent exists in
    // the C# original, which relies on NetworkSession.SessionProperties instead).
    enum class PacketKind : SharpRuntime::bytecs { TankState = 0, Options = 1 };

    // What kind of network latency and packet loss are we simulating?
    enum class NetworkQuality {
        Typical, // 100 ms latency, 10% packet loss
        Poor,    // 200 ms latency, 20% packet loss
        Perfect, // 0 latency, 0% packet loss
    };

    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont>  font_;

    // Current and previous input states.
    KeyboardState currentKeyboardState_;
    GamePadState  currentGamePadState_;
    KeyboardState previousKeyboardState_;
    GamePadState  previousGamePadState_;

    // Network objects.
    NetworkSession* networkSession_ = nullptr;

    PacketWriter packetWriter_;
    PacketReader packetReader_;

    std::string errorMessage_;

    // Ownership for the Tank objects referenced by each NetworkGamer's Tag (std::any).
    std::vector<std::unique_ptr<Tank>> tanks_;

    std::unique_ptr<GamerServicesComponent> gamerServices_;

    NetworkQuality networkQuality_ = NetworkQuality::Typical;

    // How often should we send network packets?
    int framesBetweenPackets_ = 6;

    // How recently did we send the last network packet?
    int framesSinceLastSend_ = 0;

    // Is prediction and/or smoothing enabled?
    bool enablePrediction_ = true;
    bool enableSmoothing_ = true;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // Menu screen provides options to create or join network sessions.
    void UpdateMenuScreen() {
        if (getIsActiveProperty()) {
            if (Gamer::getSignedInGamersProperty()->getCountProperty() == 0) {
                // If there are no profiles signed in, we cannot proceed. Show the Guide
                // so the user can sign in. (No-op in CNA -- see missing.md; never reached
                // in practice, since GamerServicesComponent already populates
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
    void UpdateNetworkSession(GameTime& gameTime) {
        // Is it time to send outgoing network packets?
        bool sendPacketThisFrame = false;

        ++framesSinceLastSend_;

        if (framesSinceLastSend_ >= framesBetweenPackets_) {
            sendPacketThisFrame = true;
            framesSinceLastSend_ = 0;
        }

        // Update our locally controlled tanks, sending their latest state at periodic
        // intervals.
        for (LocalNetworkGamer* gamer : networkSession_->getLocalGamersProperty()) {
            UpdateLocalGamer(gamer, gameTime, sendPacketThisFrame);
        }

        // The host also periodically broadcasts the current network-quality/prediction/
        // smoothing settings -- see this file's top-of-file comment and missing.md
        // (DEFERRED.md item #27: NetworkSession::SessionProperties has no mutable
        // accessor and is never replicated, unlike real XNA).
        if (networkSession_->getIsHostProperty() && sendPacketThisFrame) {
            SendOptionsPacket();
        }

        // Pump the underlying session object.
        try {
            networkSession_->Update();
        } catch (const System::Exception& e) {
            errorMessage_ = e.getMessageProperty();
            networkSession_->Dispose();
            networkSession_ = nullptr;
        }

        // Make sure the session has not ended.
        if (networkSession_ == nullptr)
            return;

        // Read any packets telling us the state of remotely controlled tanks (or, from
        // the host, the latest options settings).
        for (LocalNetworkGamer* gamer : networkSession_->getLocalGamersProperty()) {
            ReadIncomingPackets(gamer, gameTime);
        }

        // Apply prediction and smoothing to the remotely controlled tanks.
        for (NetworkGamer* gamer : networkSession_->getRemoteGamersProperty()) {
            Tank* tank = std::any_cast<Tank*>(gamer->getTagProperty());

            tank->UpdateRemote(framesBetweenPackets_, enablePrediction_);
        }

        // Update the latency and packet loss simulation options.
        UpdateOptions();
    }

    // Helper for updating a locally controlled gamer.
    void UpdateLocalGamer(LocalNetworkGamer* gamer, const GameTime& gameTime, bool sendPacketThisFrame) {
        // Look up what tank is associated with this local player.
        Tank* tank = std::any_cast<Tank*>(gamer->getTagProperty());

        // Read the inputs controlling this tank.
        PlayerIndex playerIndex = gamer->getSignedInGamerProperty()->getPlayerIndexProperty();

        Vector2 tankInput, turretInput;
        ReadTankInputs(playerIndex, tankInput, turretInput);

        // Update the tank.
        tank->UpdateLocal(tankInput, turretInput);

        // Periodically send our state to everyone in the session.
        if (sendPacketThisFrame) {
            packetWriter_.Write((SharpRuntime::bytecs)PacketKind::TankState);
            tank->WriteNetworkPacket(packetWriter_, gameTime);

            gamer->SendData(packetWriter_, SendDataOptions::InOrder);
        }
    }

    // NOXNA: broadcasts the host's current network-quality/prediction/smoothing
    // settings to every other gamer, working around the missing
    // NetworkSession::SessionProperties replication (see top-of-file comment).
    void SendOptionsPacket() {
        auto* host = (LocalNetworkGamer*)networkSession_->getHostProperty();

        packetWriter_.Write((SharpRuntime::bytecs)PacketKind::Options);
        packetWriter_.Write((int)networkQuality_);
        packetWriter_.Write(framesBetweenPackets_);
        packetWriter_.Write(enablePrediction_);
        packetWriter_.Write(enableSmoothing_);

        host->SendData(packetWriter_, SendDataOptions::InOrder);
    }

    // Helper for reading incoming network packets.
    void ReadIncomingPackets(LocalNetworkGamer* gamer, const GameTime& gameTime) {
        // Keep reading as long as incoming packets are available.
        while (gamer->getIsDataAvailableProperty()) {
            NetworkGamer* sender = nullptr;

            // Read a single packet from the network.
            gamer->ReceiveData(packetReader_, sender);

            // Discard packets sent by local gamers: we already know their state!
            if (sender == nullptr || sender->getIsLocalProperty())
                continue;

            auto kind = (PacketKind)packetReader_.ReadByte();

            if (kind == PacketKind::Options) {
                // Only the host ever sends this packet kind; applied unconditionally
                // below regardless of who the sender claims to be.
                networkQuality_ = (NetworkQuality)packetReader_.ReadInt32();
                framesBetweenPackets_ = packetReader_.ReadInt32();
                enablePrediction_ = packetReader_.ReadBoolean();
                enableSmoothing_ = packetReader_.ReadBoolean();
                continue;
            }

            // Look up the tank associated with whoever sent this packet.
            Tank* tank = std::any_cast<Tank*>(sender->getTagProperty());

            // Estimate how long this packet took to arrive.
            System::TimeSpan latency = networkSession_->getSimulatedLatencyProperty() +
                                        System::TimeSpan::FromTicks(sender->getRoundtripTimeProperty().getTicksProperty() / 2);

            // Read the state of this tank from the network packet.
            tank->ReadNetworkPacket(packetReader_, gameTime, latency, enablePrediction_, enableSmoothing_);
        }
    }

    // Updates the latency and packet loss simulation options. Only the host can alter
    // these values, which are then broadcast to all the client machines via
    // SendOptionsPacket() above (see this file's top-of-file comment for why this
    // differs from the C# original's NetworkSession.SessionProperties replication).
    void UpdateOptions() {
        if (networkSession_->getIsHostProperty()) {
            // Change the network quality simulation?
            if (IsPressed(Keys::A, Buttons::A)) {
                networkQuality_ = (NetworkQuality)((int)networkQuality_ + 1);

                if (networkQuality_ > NetworkQuality::Perfect)
                    networkQuality_ = NetworkQuality::Typical;
            }

            // Change the packet send rate?
            if (IsPressed(Keys::B, Buttons::B)) {
                if (framesBetweenPackets_ == 6)
                    framesBetweenPackets_ = 3;
                else if (framesBetweenPackets_ == 3)
                    framesBetweenPackets_ = 1;
                else
                    framesBetweenPackets_ = 6;
            }

            // Toggle prediction on or off?
            if (IsPressed(Keys::X, Buttons::X))
                enablePrediction_ = !enablePrediction_;

            // Toggle smoothing on or off?
            if (IsPressed(Keys::Y, Buttons::Y))
                enableSmoothing_ = !enableSmoothing_;
        }
        // Client machines learn the current settings from the host's own periodic
        // options packet (ReadIncomingPackets(), above) instead of reading
        // NetworkSession.SessionProperties -- see this file's top-of-file comment.

        // Update the SimulatedLatency and SimulatedPacketLoss properties.
        switch (networkQuality_) {
            case NetworkQuality::Typical:
                networkSession_->setSimulatedLatencyProperty(System::TimeSpan::FromMilliseconds(100));
                networkSession_->setSimulatedPacketLossProperty(0.1f);
                break;

            case NetworkQuality::Poor:
                networkSession_->setSimulatedLatencyProperty(System::TimeSpan::FromMilliseconds(200));
                networkSession_->setSimulatedPacketLossProperty(0.2f);
                break;

            case NetworkQuality::Perfect:
                networkSession_->setSimulatedLatencyProperty(System::TimeSpan::Zero);
                networkSession_->setSimulatedPacketLossProperty(0.0f);
                break;
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

        DrawOptions();

        // For each person in the session...
        for (NetworkGamer* gamer : networkSession_->getAllGamersProperty()) {
            // Look up the tank object belonging to this network gamer.
            Tank* tank = std::any_cast<Tank*>(gamer->getTagProperty());

            // Draw the tank.
            tank->Draw(*spriteBatch_);

            // Draw a gamertag label.
            spriteBatch_->DrawString(*font_, gamer->getGamertagProperty(), tank->Position(),
                                      Color(0, 0, 0, 255), 0.0f, Vector2(100.0f, 150.0f),
                                      0.6f, SpriteEffects::None, 0.0f);
        }

        DrawHelpOverlay();
        spriteBatch_->End();
    }

    // Draws the current latency and packet loss simulation settings.
    void DrawOptions() {
        std::string quality = "Network simulation = " +
            std::to_string((int)networkSession_->getSimulatedLatencyProperty().getTotalMillisecondsProperty()) +
            " ms, " + std::to_string((int)(networkSession_->getSimulatedPacketLossProperty() * 100)) + "% packet loss";

        std::string sendRate = "Packets per second = " + std::to_string(60 / framesBetweenPackets_);

        std::string prediction = std::string("Prediction = ") + (enablePrediction_ ? "on" : "off");

        std::string smoothing = std::string("Smoothing = ") + (enableSmoothing_ ? "on" : "off");

        // If we are the host, include prompts telling how to change the settings.
        if (networkSession_->getIsHostProperty()) {
            quality += " (A to change)";
            sendRate += " (B to change)";
            prediction += " (X to toggle)";
            smoothing += " (Y to toggle)";
        }

        // Draw combined text to the screen.
        std::string message = quality + "\n" + sendRate + "\n" + prediction + "\n" + smoothing;

        spriteBatch_->DrawString(*font_, message, Vector2(161.0f, 321.0f), Color(0, 0, 0, 255));
        spriteBatch_->DrawString(*font_, message, Vector2(160.0f, 320.0f), Color(255, 255, 255, 255));
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
        previousKeyboardState_ = currentKeyboardState_;
        previousGamePadState_ = currentGamePadState_;

        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_  = GamePad::GetState(PlayerIndex::One);

        // Check for exit.
        if (getIsActiveProperty() && IsPressed(Keys::Escape, Buttons::Back)) {
            Exit();
        }
    }

    // Checks if the specified button is pressed on either keyboard or gamepad (rising
    // edge only, matching the C# original -- unlike ClientServerSample's own IsPressed,
    // which is level-triggered).
    bool IsPressed(Keys key, Buttons button) {
        return (currentKeyboardState_.IsKeyDown(key) && previousKeyboardState_.IsKeyUp(key)) ||
               (currentGamePadState_.IsButtonDown(button) && previousGamePadState_.IsButtonUp(button));
    }

    // Reads input data from keyboard and gamepad, and returns this via output parameters
    // ready for use by the tank update.
    static void ReadTankInputs(PlayerIndex playerIndex, Vector2& tankInput, Vector2& turretInput) {
        // Read the gamepad.
        GamePadState gamePad = GamePad::GetState(playerIndex);

        tankInput = gamePad.getThumbSticksProperty().getLeftProperty();
        turretInput = gamePad.getThumbSticksProperty().getRightProperty();

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

        if (keyboard.IsKeyDown(Keys::K))
            turretInput.X = -1.0f;
        else if (keyboard.IsKeyDown(Keys::OemSemicolon))
            turretInput.X = 1.0f;

        if (keyboard.IsKeyDown(Keys::O))
            turretInput.Y = 1.0f;
        else if (keyboard.IsKeyDown(Keys::L))
            turretInput.Y = -1.0f;

        // Normalize the input vectors.
        if (tankInput.Length() > 1)
            tankInput.Normalize();

        if (turretInput.Length() > 1)
            turretInput.Normalize();
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

} // namespace NetworkPrediction
