#include "main.h"
#include "NetworkOps.h"
#include "Network.h"
#include <stdio.h>
#include <sys/types.h>
#include "Statistics.h"

#ifndef NO_SDL
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#endif

namespace ImageStack {

void Send::help() {
    printf("\n-send sends an image over a TCP connection. It has an optional first and second\n"
           "argument.  The first argument specifies which server to contact, and the second\n"
           "argument specifies the port. By default, 127.0.0.1:5678 is used.\n\n"
           "Usage: ImageStack -load a.tga -remotedisplay localhost 5678\n");
}

#ifdef NO_SDL
bool Send::test() {
    printf("SDL required to test send\n");
    return false;
}
#else

// An anonymous namespace confines these globals to this translation unit
namespace {
int Send_test_port;
Image Send_test_image;
int Send_test_thread(void *) {
    Send::apply(Send_test_image, "localhost", Send_test_port);
    return 0;
}
}

bool Send::test() {
    Send_test_image = Image(123, 234, 5, 2);
    Noise::apply(Send_test_image, 0, 1);

    Send_test_port = randomInt(10000, 15000);

    // Create a child thread to send myself an image
    SDL_Thread *thread = SDL_CreateThread(Send_test_thread, NULL);

    // Wait to receive it
    Image im = Receive::apply(Send_test_port);

    // Check it's identical to a
    im -= Send_test_image;
    Stats s(im);

    // Wait for the thread to terminate
    SDL_WaitThread(thread, NULL);

    return s.mean() == 0 && s.variance() == 0;
}
#endif

void Send::parse(vector<string> args) {
    switch (args.size()) {
    case 0:
        apply(stack(0));
        return;
    case 1:
        apply(stack(0), args[0]);
        return;
    case 2:
        apply(stack(0), args[0], readInt(args[1]));
        return;
    default:
        panic("-send takes at most two arguments\n");
    }
}

void Send::apply(Image im, string host, int port) {
    TCPConnection conn(Address(host, port));
    conn.sendImage(im);
}



void Receive::help() {
    printf("\n-receive sets up a TCP server and listens for an image (such as that sent by\n"
           "-send). The sole optional argument is the port to listen on. It defaults\n"
           "to 5678.\n\n"
           "Usage: ImageStack -receive 5243 -save image.jpg\n\n"
           "       ImageStack -loop --receive --scale 2 --send somewhereelse\n\n");

}

bool Receive::test() {
    // tested by -send
    return true;
}

void Receive::parse(vector<string> args) {
    assert(args.size() < 2, "-receive takes zero or one argument\n");
    if (args.size() == 0) { push(apply()); }
    else { push(apply(readInt(args[0]))); }
}


Image Receive::apply(int port) {
    // create and bind the server if it hasn't already been created
    if (!servers[port]) {
        servers[port] = new TCPServer(port);
    }

    printf("Listening on port %i\n", port);
    TCPConnection *conn = servers[port]->listen();
    printf("Got a connection, reading image...\n");

    Image im = conn->recvImage();

    delete conn;
    return im;
}

map<int, TCPServer *> Receive::servers;
}
