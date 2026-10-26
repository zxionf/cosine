// GLFW 初始化冒烟测试。
// 项目只用 Vulkan，不使用 OpenGL：这里显式关闭 GLFW 的 OpenGL 客户端 API，
// 并阻止 glfw3.h 引入 <GL/gl.h>，避免在缺少 GL 开发包的环境（如 CI）中编译失败。
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

int main()
{
    if (!glfwInit()) {
        return 1;
    }

    // GLFW_NO_API：不创建任何图形上下文，由 Vulkan 侧自行接管 surface。
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "glfw init smoke", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
