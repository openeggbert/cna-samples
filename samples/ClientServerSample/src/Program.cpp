#include "CNA/Entrypoint.hpp"
#include "ClientServerGame.hpp"

int main() {
    ClientServer::ClientServerGame game;
    game.Run();
    return 0;
}
