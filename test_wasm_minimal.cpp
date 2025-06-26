#include <emscripten/emscripten.h>

extern "C"
{

    EMSCRIPTEN_KEEPALIVE
    int createEngine()
    {
        return 42; // Simple test value
    }

    EMSCRIPTEN_KEEPALIVE
    int testFunction(int a, int b)
    {
        return a + b;
    }
}