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

template<typename T>
size_t ctz(T t) {
    if (t == 0) {
        return std::numeric_limits<T>::digits;
    }
    if constexpr (sizeof(T) == sizeof(long long)) {
        return __builtin_ctzll(t);
    } else if constexpr (sizeof(T) == sizeof(long)) {
        return __builtin_ctzl(t);
    } else if constexpr (sizeof(T) == sizeof(unsigned)) {
        return __builtin_ctz(t);
    }
}

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
    //zinc::morton::AABB<2, 32> aabb = {0, 1024 * 3};
    zinc::morton::AABB<2, 32> aabb = {0, 1024 * 4 - 64};
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

            auto min_c = morton_code<2, 32>::decode({aabb.min});
            auto max_c = morton_code<2, 32>::decode({aabb.max});
            cairo_rectangle(cr, min_c[0], min_c[1], max_c[0] - min_c[0], max_c[1] - min_c[1]);

            cairo_set_matrix(cr, &screen_mat);
            cairo_set_source_rgba(cr, 0.8, 0.9, 0.95, 0.5);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);

            cairo_set_matrix(cr, &world_mat);
            for (morton_code<2, 32> cur_z = aabb.min; cur_z < aabb.max;) {
                morton_code<2, 32> next_z = cur_z + 1;
                auto cur_c = cur_z.decode(cur_z);
                auto next_c = next_z.decode(next_z);

                if (false) {
                    auto aabb_max_c = aabb.max.decode(aabb.max);
                    decltype(aabb_max_c) out_crossing_c, in_crossing_c;
                    uint32_t out_crossing_length = 1 << ctz(aabb_max_c[0]);
                    out_crossing_c[0] = aabb_max_c[0] - 1;
                    out_crossing_c[1] = cur_c[1] + out_crossing_length - 1;
                    size_t dim_y = 1;
                    uint32_t in_crossing_bits = dim_y + ctz(cur_c[1] + out_crossing_length);
                    uint32_t in_crossing_length = 1 << in_crossing_bits;
                    in_crossing_c[1] = cur_c[1] + out_crossing_length;
                    in_crossing_c[0] = (aabb_max_c[0] >> in_crossing_bits) << in_crossing_bits;

                    cairo_rectangle(cr, in_crossing_c[0], in_crossing_c[1], 1, 1);
                    cairo_rectangle(cr, out_crossing_c[0], out_crossing_c[1], 1, 1);

                    cur_z = cur_z.encode(in_crossing_c);
                } else {
                    //going out
                    if (aabb.contains(cur_z) && !aabb.contains(next_z)) {
                        cairo_rectangle(cr, cur_c[0], cur_c[1], 1, 1);
                        cairo_rectangle(cr, next_c[0], next_c[1], 1, 1);
                        printf("%lu\n", cur_z.data ^ next_z.data);
                    }
                    //coming in
                    if (!aabb.contains(cur_z) && aabb.contains(next_z)) {
                        cairo_rectangle(cr, next_c[0], next_c[1], 1, 1);
                    }

                    cur_z.data += 1;
                }
            }
            cairo_set_matrix(cr, &screen_mat);
            cairo_set_source_rgba(cr, 0.0, 0.8, 0.0, 0.5);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);
        }
        {
            cairo_set_matrix(cr, &world_mat);

            auto i = aabb.get_first_interval();
            auto min = morton_code<2, 32>::decode({i.start});
            cairo_rectangle(cr, min[0], min[1], 1, 1);
            auto max = morton_code<2, 32>::decode({i.end});
            cairo_rectangle(cr, max[0], max[1], 1, 1);

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
