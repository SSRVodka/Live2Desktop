
#include "modules/stt/server.h"

int main(int argc, char *argv[]) {
    STT::Server server;
    return server.cli_main(argc, argv);
}
