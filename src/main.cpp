#include <iostream>
#include "swap.h"
#include "GLFW/glfw3.h"
#include "glm/vec2.hpp"
using namespace std;

int main(int argc,char** argv){
    int a = 1;
    int b = 2;
    glm::vec2 v(1,2);
    glm::vec2 v2(1,2);
    cout<<"length:"<<v.length()<<endl;
    v = v + v2;
    cout<<v.x<<"length:"<<v.length()<<endl;
    swap(a,b);
    cout<<a<<b<<endl;
    cout<<a<<b<<endl;

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;
        
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(3840, 2560, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        //glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}