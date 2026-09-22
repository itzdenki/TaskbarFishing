#include <raylib.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdio>

int main() {
    // Force platform initialization failure, without requiring a broken GPU.
    // In unpatched raylib 5.5 InitWindow continues into rlgl and can crash.
    SetTraceLogCallback([](int, const char*, va_list) {});
    glfwInitHint(GLFW_PLATFORM, -1);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "Failed initialization test");
    if (IsWindowReady()) return 1;
    glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
#ifdef NDEBUG
    // GLFW's release validation returns null for a negative width. Its debug
    // build asserts before validation, so only inject this failure in Release.
    InitWindow(-1, 64, "Failed window creation test");
    if (IsWindowReady()) return 3;
#endif
    InitWindow(64, 64, "Initialization recovery test");
    if (!IsWindowReady()) return 2;
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();
    CloseWindow();
    std::puts("Graphics initialization failures returned safely; next initialization succeeded.");
    return 0;
}
