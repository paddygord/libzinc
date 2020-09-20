#include <cairo/cairo.h>
#include <cairo/cairo-gl.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <cmath>
#include <algorithm>

#include <libzinc/zinc.hh>

#include <stdlib.h>

int main() {
    cairo_t *cr = NULL;
    cairo_surface_t *surface = NULL;
    GLFWwindow* window = NULL;
    {
        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
        window = glfwCreateWindow(800, 800, "cairo glfw demo", NULL, NULL);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        Display* x11_display = glfwGetX11Display();
        GLXContext glx_context = glfwGetGLXContext(window);
        Window x11_window = glfwGetX11Window(window);

        cairo_device_t* cairo_device = cairo_glx_device_create(x11_display, glx_context);
        int window_width, window_height;
        glfwGetWindowSize(window, &window_width, &window_height);
        surface = cairo_gl_surface_create_for_window(cairo_device, x11_window, window_width, window_height);
        cairo_device_destroy(cairo_device);
        cr = cairo_create(surface);
    }
    zinc::morton::AABB<2, 32> aabb = {0, 1024 * 3};
    cairo_matrix_t screen_mat, world_mat;
    float scale = 16;
    cairo_get_matrix(cr, &screen_mat);
    cairo_scale(cr, scale, scale);
    cairo_get_matrix(cr, &world_mat);
    while (!glfwWindowShouldClose(window)) {
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);
        {
            cairo_set_matrix(cr, &world_mat);

            cairo_translate(cr, 0.5, 0.5);
            cairo_move_to(cr, 0, 0);
            for (morton_code<2, 32> m = 0; m.data < 1024 * 4; m.data++) {
                auto a = m.decode(m);
                cairo_line_to(cr, a[0], a[1]);
            }

            cairo_set_matrix(cr, &screen_mat);
            cairo_set_line_width(cr, 1);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);
        }
        {
            double mouse_x, mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            uint32_t x_pos = (mouse_x - 0.5) / scale;
            uint32_t y_pos = (mouse_y - 0.5) / scale;
            auto m = morton_code<2, 32>::encode({x_pos, y_pos});
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                aabb.max = m;
            } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                aabb.min = m;
            }
        }
        {
            cairo_set_matrix(cr, &world_mat);

            uint32_t min_x, min_y, max_x, max_y;
            std::array<uint32_t, 2> pack;
            pack = morton_code<2, 32>::decode({aabb.min});
            min_x = pack[0];
            min_y = pack[1];
            pack = morton_code<2, 32>::decode({aabb.max});
            max_x = pack[0];
            max_y = pack[1];
            cairo_rectangle(cr, min_x, min_y, max_x - min_x, max_y - min_y);

            cairo_set_matrix(cr, &screen_mat);
            cairo_set_source_rgba(cr, 0.8, 0.9, 0.95, 0.5);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);
        }
        {
            cairo_set_matrix(cr, &world_mat);

            uint32_t min_x, min_y, max_x, max_y;
            std::array<uint32_t, 2> pack;
            auto i = aabb.get_first_interval();
            pack = morton_code<2, 32>::decode({i.start});
            min_x = pack[0];
            min_y = pack[1];
            cairo_rectangle(cr, min_x, min_y, 1, 1);
            pack = morton_code<2, 32>::decode({i.end});
            max_x = pack[0];
            max_y = pack[1];
            cairo_rectangle(cr, max_x, max_y, 1, 1);

            cairo_set_matrix(cr, &screen_mat);
            cairo_set_source_rgba(cr, 0.8, 0.0, 0.0, 0.5);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);
        }
        cairo_gl_surface_swapbuffers(surface);
        glfwPollEvents();
    }
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}
