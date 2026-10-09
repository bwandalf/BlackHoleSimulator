#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_RAYS 60
#define TRAIL_LENGTH 40

double c = 1.0;
double G = 1.0;

typedef struct {
    double x;
    double y;
} vec2;

typedef struct {
    double x;
    double y;
    double z;
} vec3;

typedef struct {
    vec3 pos;
    vec3 dir;
    double L2;
    int active;
    vec3 trail[TRAIL_LENGTH];
    int trailHead;
    int trailCount;
} Ray;

Ray rays[NUM_RAYS];

double PO4(double x) { return x * x * x * x; }

vec3 vec3_add(vec3 a, vec3 b) {
    return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 vec3_scale(vec3 v, double scalar) {
    return (vec3){v.x * scalar, v.y * scalar, v.z * scalar};
}

vec3 vec3_sub(vec3 a, vec3 b) {
    return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

double vec3_length(vec3 v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 vec3_normalize(vec3 v) {
    double len = vec3_length(v);
    if (len == 0.0) return (vec3){0.0, 0.0, 0.0};
    return (vec3){v.x / len, v.y / len, v.z / len};
}

vec3 cross_prod(vec3 a, vec3 b) {
    return (vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

typedef struct {
    float m[16];
} mat4;

mat4 mat4_indetity(void) {
    mat4 res = {0};
    res.m[0] = 1.0f; res.m[5] = 1.0f; res.m[10] = 1.0f; res.m[15] = 1.0f;
    return res;
}

mat4 mat4_perspective(float fov_rad, float aspect, float zNear, float zFar) {
    mat4 out = {0};
    float totalHalfFov = tanf(fov_rad / 2.0f);
    out.m[0] = 1.0f / (totalHalfFov * aspect);
    out.m[5] = 1.0f / totalHalfFov;
    out.m[10] = -(zFar + zNear) / (zFar - zNear);
    out.m[11] = -1.0f;
    out.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return out;
}

mat4 mat4_transpose(mat4 m) {
    mat4 out;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            out.m[r * 4 + c] = m.m[c * 4 + r];
        }
    }
    return out;
}

mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up) {
    vec3 f = vec3_normalize(vec3_sub(center, eye));
    vec3 s = vec3_normalize(cross_prod(f, up));
    vec3 u = cross_prod(s, f);

    mat4 out = mat4_indetity();

    out.m[0] = (float)s.x;
    out.m[1] = (float)u.x;
    out.m[2] = -(float)f.x;

    out.m[4] = (float)s.y;
    out.m[5] = (float)u.y;
    out.m[6] = -(float)f.y;

    out.m[8]  = (float)s.z;
    out.m[9]  = (float)u.z;
    out.m[10] = -(float)f.z;

    out.m[12] = -(float)(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    out.m[13] = -(float)(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    out.m[14] = (float)(f.x * eye.x + f.y * eye.y + f.z * eye.z);

    return out;
}

typedef struct {
    float radius;
    float yaw;
    float pitch;
    double lastX;
    double lastY;
    int is_dragging;
} OrbitalCam;

OrbitalCam cam = {
    .radius = 25.0f,
    .yaw = 0.0f,
    .pitch = 0.2f,
    .is_dragging = 0
};

struct Engine {
    GLFWwindow* window;
    int WIDTH;
    int HEIGHT;
    float width;
    float height;
};

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cam.radius -= (float)yoffset * 1.5f;
    if (cam.radius < 3.0f) cam.radius = 3.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            cam.is_dragging = 1;
            glfwGetCursorPos(window, &cam.lastX, &cam.lastY);
        } else if (action == GLFW_RELEASE) {
            cam.is_dragging = 0;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!cam.is_dragging) return;
    float dx = (float)(xpos - cam.lastX);
    float dy = (float)(ypos - cam.lastY);
    cam.lastX = xpos;
    cam.lastY = ypos;
    float sensitivity = 0.005f;
    cam.yaw   += dx * sensitivity;
    cam.pitch += dy * sensitivity;
    float max_pitch = 1.50f;
    if (cam.pitch > max_pitch)  cam.pitch = max_pitch;
    if (cam.pitch < -max_pitch) cam.pitch = -max_pitch;
}

void framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight) {
    glViewport(0, 0, newWidth, newHeight);
}

int engine_init(struct Engine* engine) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    engine->window = glfwCreateWindow(engine->WIDTH, engine->HEIGHT, "Black Hole sim", NULL, NULL);
    if (!engine->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(engine->window);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    glViewport(0, 0, engine->WIDTH, engine->HEIGHT);
    glfwSetFramebufferSizeCallback(engine->window, framebuffer_size_callback);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return 0;
}

struct Blackhole {
    vec3 pos;
    double mass;
    double r_s;
};

struct Blackhole bh = {
    .pos = {0.0, 0.0, 0.0},
    .mass = 0.5,
    .r_s = 1.0
};

void blackhole_init(struct Blackhole* bh) {
    bh->r_s = (2.0 * G * bh->mass) / (c * c);
}

void spawn_single_ray(int idx, double startY) {
    rays[idx].pos = (vec3){-18.0, startY, 0.0};
    rays[idx].dir = (vec3){c, 0.0, 0.0};
    rays[idx].active = 1;
    rays[idx].trailHead = 0;
    rays[idx].trailCount = 0;

    vec3 relPos = vec3_sub(rays[idx].pos, bh.pos);
    vec3 L_vec = cross_prod(relPos, rays[idx].dir);
    rays[idx].L2 = L_vec.x * L_vec.x + L_vec.y * L_vec.y + L_vec.z * L_vec.z;
}

void rays_init(struct Engine* engine) {
    double startY_min = -7.0;
    double startY_max = 7.0;
    double stepY = (startY_max - startY_min) / NUM_RAYS;

    for (int i = 0; i < NUM_RAYS; i++) {
        spawn_single_ray(i, startY_min + i * stepY);
    }
}

void drawSphere(struct Blackhole* blackhole, int stacks, int slices) {
    float r = (float)blackhole->r_s;
    glColor3f(1.0f, 0.0f, 0.0f);

    for (int i = 0; i < stacks; i++) {
        float lat0 = (float)M_PI * (-0.5f + (float)(i) / stacks);
        float z0   = sinf(lat0) * r;
        float zr0  = cosf(lat0) * r;

        float lat1 = (float)M_PI * (-0.5f + (float)(i + 1) / stacks);
        float z1   = sinf(lat1) * r;
        float zr1  = cosf(lat1) * r;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float lng = 2.0f * (float)M_PI * (float)(j) / slices;
            float x = cosf(lng);
            float y = sinf(lng);

            glVertex3f(x * zr0, y * zr0, z0);
            glVertex3f(x * zr1, y * zr1, z1);
        }
        glEnd();
    }
}

void drawTrails(void) {
    glLineWidth(2.0f);
    for (int i = 0; i < NUM_RAYS; i++) {
        if (rays[i].trailCount < 2) continue;

        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < rays[i].trailCount; j++) {
            int idx = (rays[i].trailHead - 1 - j + TRAIL_LENGTH) % TRAIL_LENGTH;
            float alpha = 1.0f - (float)j / (float)TRAIL_LENGTH;
            glColor4f(1.0f, 0.9f, 0.2f, alpha);
            glVertex3f((float)rays[i].trail[idx].x,
                       (float)rays[i].trail[idx].y,
                       (float)rays[i].trail[idx].z);
        }
        glEnd();
    }
}

void drawRays(void) {
    glPointSize(5.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_RAYS; i++) {
        if (!rays[i].active) continue;
        glVertex3f((float)rays[i].pos.x,
                   (float)rays[i].pos.y,
                   (float)rays[i].pos.z);
    }
    glEnd();
}

void update_ray(Ray* ray, struct Blackhole* bh, double dt, int index) {
    if (!ray->active) {
        double startY_min = -7.0;
        double startY_max = 7.0;
        double stepY = (startY_max - startY_min) / NUM_RAYS;
        spawn_single_ray(index, startY_min + index * stepY);
        return;
    }

    ray->trail[ray->trailHead] = ray->pos;
    ray->trailHead = (ray->trailHead + 1) % TRAIL_LENGTH;
    if (ray->trailCount < TRAIL_LENGTH) ray->trailCount++;

    vec3 r = vec3_sub(bh->pos, ray->pos);
    double dist = vec3_length(r);

    if (dist <= bh->r_s || dist > 30.0 || ray->pos.x > 20.0) {
        ray->active = 0;
        return;
    }

    vec3 dir_to_bh = vec3_normalize(r);
    double accel_mag = 1.5 * bh->r_s * ray->L2 / PO4(dist);
    vec3 accel = vec3_scale(dir_to_bh, accel_mag);
    ray->dir = vec3_add(ray->dir, vec3_scale(accel, dt));
    ray->dir = vec3_scale(vec3_normalize(ray->dir), c);
    ray->pos = vec3_add(ray->pos, vec3_scale(ray->dir, dt));
}

void engine_run(struct Engine* engine, struct Blackhole* bh) {
    glEnable(GL_DEPTH_TEST);
    double dt = 0.08;

    while (!glfwWindowShouldClose(engine->window)) {
        int fbW, fbH;
        glfwGetFramebufferSize(engine->window, &fbW, &fbH);
        if (fbH == 0) fbH = 1;

        glViewport(0, 0, fbW, fbH);
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float cam_x = cam.radius * cosf(cam.pitch) * sinf(cam.yaw);
        float cam_y = cam.radius * sinf(cam.pitch);
        float cam_z = cam.radius * cosf(cam.pitch) * cosf(cam.yaw);

        vec3 eye    = { cam_x, cam_y, cam_z };
        vec3 center = { 0.0f, 0.0f, 0.0f };
        vec3 up     = { 0.0f, 1.0f, 0.0f };

        mat4 view = mat4_look_at(eye, center, up);
        float aspect = (float)fbW / (float)fbH;
        mat4 proj = mat4_perspective((float)(45.0 * (M_PI / 180.0)), aspect, 0.1f, 100.0f);

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(proj.m);

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(view.m);

        for (int i = 0; i < NUM_RAYS; i++) {
            update_ray(&rays[i], bh, dt, i);
        }

        drawSphere(bh, 20, 20);
        drawTrails();
        drawRays();

        glfwSwapBuffers(engine->window);
        glfwPollEvents();
    }
}

int main() {
    struct Engine engine = {
        .WIDTH = 1280,
        .HEIGHT = 720,
        .width = 30.0f,
        .height = 16.875f
    };

    blackhole_init(&bh);
    rays_init(&engine);

    if (engine_init(&engine) != 0) {
        return -1;
    }

    glfwSetMouseButtonCallback(engine.window, mouse_button_callback);
    glfwSetCursorPosCallback(engine.window, cursor_position_callback);
    glfwSetScrollCallback(engine.window, scroll_callback);

    engine_run(&engine, &bh);

    glfwDestroyWindow(engine.window);
    glfwTerminate();

    return 0;
}
