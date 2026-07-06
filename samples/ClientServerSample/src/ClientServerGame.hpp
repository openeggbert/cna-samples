#pragma once

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

namespace ClientServer {

class ClientServerGame : public Game {
public:
    ClientServerGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(kScreenWidth);
        graphics_->setPreferredBackBufferHeightProperty(kScreenHeight);

        getContentProperty().setRootDirectoryProperty("Content");

        // Matches the C# original's own `Components.Add(new GamerServicesComponent(this));`.
        // Previously omitted entirely: confirmed by live testing that doing so used to hang
        // every NetworkSession::Create/Find/Join call forever (CNA's own
        // GamerServicesDispatcher::Update() no-op never completed the synchronous wrapper's
        // polling loop once a GamerServicesComponent existed). Fixed upstream in cna_net
        // (Task 12.1, DEFERRED.md item #19) — this component is now safe to add, and its real
        // Initialize() populates Gamer::SignedInGamers with 4 stub gamers before this game's
        // first Update() call, so the manual SignedInGamers override this sample previously
        // needed is gone too. See missing.md.
        gamerServices_ = std::make_unique<GamerServicesComponent>(*this);
        getComponentsProperty().Add(gamerServices_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "ClientServerGame";
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
            if (IsPressed(Keys::A, Buttons::A)) {
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
            // Flush the initial GamerJoin event(s) for our own local gamer(s) now,
            // rather than waiting for the next Update() — this is the permanent,
            // correct pattern (not a stopgap): real XNA's GamerJoined replays itself
            // immediately upon subscription for every gamer already in the session,
            // which CNA's plain System::EventHandler<T> has no hook to reproduce (see
            // cna_net's plan_net.md Task 12.3 investigation). Without this call,
            // UpdateLocalGamer() below would read an empty Tag on the very first frame.
            networkSession_->Update();
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
            // See CreateSession()'s comment: flush the initial GamerJoin event(s) now.
            networkSession_->Update();
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
        // Read inputs for locally controlled tanks, and send them to the server.
        for (LocalNetworkGamer* gamer : networkSession_->getLocalGamersProperty()) {
            UpdateLocalGamer(gamer);
        }

        // If we are the server, update all the tanks and transmit their latest
        // positions back out over the network.
        if (networkSession_->getIsHostProperty()) {
            UpdateServer();
        }

        // Pump the underlying session object.
        networkSession_->Update();

        // Make sure the session has not ended.
        if (networkSession_ == nullptr)
            return;

        // Read any incoming network packets.
        for (LocalNetworkGamer* gamer : networkSession_->getLocalGamersProperty()) {
            if (networkSession_->getIsHostProperty()) {
                ServerReadInputFromClients(gamer);
            } else {
                ClientReadGameStateFromServer(gamer);
            }
        }
    }

    // Helper for updating a locally controlled gamer.
    void UpdateLocalGamer(LocalNetworkGamer* gamer) {
        // Look up what tank is associated with this local player, and read the latest
        // user inputs for it. The server will later use these values to control the
        // tank movement.
        Tank* localTank = std::any_cast<Tank*>(gamer->getTagProperty());

        ReadTankInputs(localTank, gamer->getSignedInGamerProperty()->getPlayerIndexProperty());

        // Only send if we are not the server. There is no point sending packets to
        // ourselves, because we already know what they will contain!
        if (!networkSession_->getIsHostProperty()) {
            // Write our latest input state into a network packet.
            packetWriter_.Write(localTank->TankInput);
            packetWriter_.Write(localTank->TurretInput);

            // Send our input data to the server.
            gamer->SendData(packetWriter_, SendDataOptions::InOrder, networkSession_->getHostProperty());
        }
    }

    // This method only runs on the server. It calls Update on all the tank instances,
    // both local and remote, using inputs that have been received over the network. It
    // then sends the resulting tank position data to everyone in the session.
    void UpdateServer() {
        // Loop over all the players in the session, not just the local ones!
        for (NetworkGamer* gamer : networkSession_->getAllGamersProperty()) {
            // Look up what tank is associated with this player.
            Tank* tank = std::any_cast<Tank*>(gamer->getTagProperty());

            // Update the tank.
            tank->Update();

            // Write the tank state into the output network packet.
            packetWriter_.Write(gamer->getIdProperty());
            packetWriter_.Write(tank->Position);
            packetWriter_.Write(tank->TankRotation);
            packetWriter_.Write(tank->TurretRotation);
        }

        // Send the combined data for all tanks to everyone in the session.
        auto* server = (LocalNetworkGamer*)networkSession_->getHostProperty();
        server->SendData(packetWriter_, SendDataOptions::InOrder);
    }

    // This method only runs on the server. It reads tank inputs that have been sent
    // over the network by a client machine, storing them for later use by the
    // UpdateServer method.
    void ServerReadInputFromClients(LocalNetworkGamer* gamer) {
        // Keep reading as long as incoming packets are available.
        while (gamer->getIsDataAvailableProperty()) {
            NetworkGamer* sender = nullptr;

            // Read a single packet from the network.
            gamer->ReceiveData(packetReader_, sender);

            if (sender != nullptr && !sender->getIsLocalProperty()) {
                // Look up the tank associated with whoever sent this packet.
                Tank* remoteTank = std::any_cast<Tank*>(sender->getTagProperty());

                // Read the latest inputs controlling this tank.
                remoteTank->TankInput = packetReader_.ReadVector2();
                remoteTank->TurretInput = packetReader_.ReadVector2();
            }
        }
    }

    // This method only runs on client machines. It reads tank position data that has
    // been computed by the server.
    void ClientReadGameStateFromServer(LocalNetworkGamer* gamer) {
        // Keep reading as long as incoming packets are available.
        while (gamer->getIsDataAvailableProperty()) {
            NetworkGamer* sender = nullptr;

            // Read a single packet from the network.
            gamer->ReceiveData(packetReader_, sender);

            // This packet contains data about all the players in the session. We keep
            // reading from it until we have processed all the data.
            while (packetReader_.getPositionProperty() < packetReader_.getLengthProperty()) {
                // Read the state of one tank from the network packet.
                SharpRuntime::bytecs gamerId = packetReader_.ReadByte();
                Vector2 position = packetReader_.ReadVector2();
                float tankRotation = packetReader_.ReadSingle();
                float turretRotation = packetReader_.ReadSingle();

                // Look up which gamer this state refers to.
                NetworkGamer* remoteGamer = networkSession_->FindGamerById(gamerId);

                // This might come back null if the gamer left the session after the
                // host sent the packet but before we received it. If that happens, we
                // just ignore the data for this gamer.
                if (remoteGamer != nullptr) {
                    // Update our local state with data from the network packet.
                    Tank* tank = std::any_cast<Tank*>(remoteGamer->getTagProperty());

                    tank->Position = position;
                    tank->TankRotation = tankRotation;
                    tank->TurretRotation = turretRotation;
                }
            }
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

            // Matches the C# original exactly: gamer->getIsHostProperty() is now real
            // per-instance state (cna_net Task 12.2), correct for every *local* gamer.
            // Residual, documented cna_net limitation: a *remote* gamer representing the
            // actual host still reports IsHost == false as seen from a client machine (the
            // wire roster carries no host flag), so a client won't see "(server)" on the
            // host's own tank — this machine's own gamer is always labeled correctly.
            if (gamer->getIsHostProperty())
                label += " (server)";

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

} // namespace ClientServer
