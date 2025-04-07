
#include <sv.cpp/sense-voice/include/asr_handler.hpp>

int main(int argc, char **argv) {
    ASRServer server;
    return server.cli_main(argc, argv);
}
