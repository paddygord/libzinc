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
        surface = cairo_gl_surface_create_for_window(cairo_device, x11_window, 800, 800);
        cairo_device_destroy(cairo_device);
        cr = cairo_create(surface);
    }
    zinc::morton::AABB<2, 32> aabb = {256, 256 + 64 * 3 + 16 + 4};
    while (!glfwWindowShouldClose(window)) {
        {
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_paint(cr);
            morton_code<2, 32> m {0};
            float scale = 16;
            cairo_save(cr);
            cairo_translate(cr, scale, scale);
            cairo_scale(cr, scale, scale);
            for (uint32_t x = 0, y = 0; m.data < 1024; m.data++) {
                cairo_move_to(cr, x, y);
                auto a = m.decode(m);
                x = a[0];
                y = a[1];
                cairo_line_to(cr, x, y);
            }
            cairo_restore(cr);
            cairo_set_line_width(cr, 1);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);

            cairo_save(cr);
            cairo_translate(cr, scale, scale);
            cairo_scale(cr, scale, scale);
            cairo_translate(cr, -0.5, -0.5);

            double mouse_x, mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            /*
            mouse_x -= scale;
            mouse_y -= scale;
            */
            mouse_x /= scale;
            mouse_y /= scale;
            if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
                aabb.min = morton_code<2, 32>::encode({(uint32_t)mouse_x, (uint32_t)mouse_y});
            } else if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
                aabb.max = morton_code<2, 32>::encode({(uint32_t)mouse_x, (uint32_t)mouse_y});
            } else if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE)) {
                aabb.min = morton_code<2, 32>::encode({(uint32_t)mouse_x, (uint32_t)mouse_y});
            }
            {
                uint32_t min_x, min_y, max_x, max_y;
                std::array<uint32_t, 2> pack;

                pack = morton_code<2, 32>::decode({aabb.min});
                min_x = pack[0];
                min_y = pack[1];
                pack = morton_code<2, 32>::decode({aabb.max});
                max_x = pack[0];
                max_y = pack[1];

                cairo_rectangle(cr, min_x, min_y, max_x - min_x, max_y - min_y);

                auto i = aabb.get_first_interval();
                pack = morton_code<2, 32>::decode({i.start});
                min_x = pack[0];
                min_y = pack[1];
                pack = morton_code<2, 32>::decode({i.end});
                max_x = pack[0];
                max_y = pack[1];

                cairo_rectangle(cr, min_x, min_y, 1, 1);
                cairo_rectangle(cr, max_x, max_y, 1, 1);
            }
            cairo_restore(cr);
            cairo_set_source_rgba(cr, 0.8, 0.9, 0.95, 0.5);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);
        }
        cairo_gl_surface_swapbuffers(surface);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    {
        zinc::morton::AABB<2, 32> aabb = {256, 256 + 64 * 3 + 16 + 4};
        auto i = aabb.get_first_interval();
        std::cout << i.start.data << ", " << i.end.data << std::endl;
        std::cout << "expected:" << std::endl;
        std::cout << 256 << ", " << 256 + 64 + 16 + 4 << std::endl;
    }
    {
        zinc::morton::AABB<2, 32> aabb = {51, 193};
        uint64_t litmax = 0, bigmin = 0;
        std::tie(litmax, bigmin) = aabb.morton_get_next_address();
        assert(litmax == 107);
        assert(bigmin == 145);
        aabb.max = 107;
        std::tie(litmax, bigmin) = aabb.morton_get_next_address();
        assert(litmax == 63);
        assert(bigmin == 98);
        aabb.min = 98;
        std::tie(litmax, bigmin) = aabb.morton_get_next_address();
        assert(litmax == 99);
        assert(bigmin == 104);
        aabb.min = 145;
        aabb.max = 193;
        std::tie(litmax, bigmin) = aabb.morton_get_next_address();
        assert(litmax == 149);
        assert(bigmin == 192);
    }
}
