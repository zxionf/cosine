// 单文件 GLFW + OpenGL 透明窗口测试
// 编译（Linux）: g++ transparent_test.cpp -o transparent_test -lglfw -lGL -lGLEW
// 编译（Windows + MinGW）: g++ transparent_test.cpp -o transparent_test.exe -lglfw3 -lopengl32 -lgdi32

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

// 如果没装 GLEW，可以直接用 GLFW 的 getProcAddress 加载需要的函数
// 这里只用到 glClearColor 和 glClear，它们是 OpenGL 1.0 函数，
// Windows 上由 opengl32.lib 直接提供，不需要 GLEW。

static void error_callback(int error, const char* description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main() {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    // 关键设置：透明帧缓冲
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 可选：无边框，某些驱动下透明更容易生效
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Transparent Window Test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 打印一些诊断信息
    std::printf("GLFW version: %s\n", glfwGetVersionString());
    std::printf("OpenGL vendor:  %s\n", glGetString(GL_VENDOR));
    std::printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    std::printf("OpenGL version:  %s\n", glGetString(GL_VERSION));

    // 检查透明帧缓冲提示是否被接受
    int transparent = glfwGetWindowAttrib(window, GLFW_TRANSPARENT_FRAMEBUFFER);
    std::printf("GLFW_TRANSPARENT_FRAMEBUFFER = %s\n",
                transparent ? "TRUE" : "FALSE");

    // 如果想测试整体窗口不透明度，可以打开下面这行
    // glfwSetWindowOpacity(window, 0.7f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // 半透明红色清屏：Alpha = 0.3
        glClearColor(0.0f, 0.1f, 0.0f, 0.05f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}