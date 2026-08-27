#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_RAYS 50

double lastPrintTime = 0.0;
int    framesCount   = 0;
double c = 299792458.0;
double G = 6.67430e-11;

typedef struct
{
    double x;
    double y;
} vec2;

typedef struct
{
    vec2 pos;
    vec2 dir;
    double L;
    int active;
} Ray;

Ray rays[NUM_RAYS];

//for 2d spaces
vec2 vec2_add(vec2 a, vec2 b)
{
    vec2 res = {a.x + b.x, a.y + b.y};
    return res;
}

vec2 vec2_scale(vec2 v, double scalar) {
    vec2 res = {v.x * scalar, v.y * scalar};
    return res;
}

vec2 vec2_sub(vec2 a, vec2 b) {
    vec2 res = {a.x - b.x, a.y - b.y};
    return res;
}

double vec2_length(vec2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

vec2 vec2_normalize(vec2 v) {
    double len = vec2_length(v);
    if (len == 0.0) return (vec2){0.0, 0.0};
    return (vec2){v.x / len, v.y / len};
}

//for 3d spaces
vec3 vec3_add(vec3 a, vec3 b) {
    vec3 res = {a.x + b.x, a.y + b.y, a.z + b.z};
    return res;
}

vec3 vec3_scale(vec3 v, double scalar) {
    vec3 res = {v.x * scalar, v.y * scalar, v.z * scalar};
    return res;
}

vec3 vec3_sub(vec3 a, vec3 b) {
    vec3 res = {a.x - b.x, a.y - b.y, a.z - b.z};
    return res;
}

double vec3_length(vec3 v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 vec3_normalize(vec3 v) {
    double len = vec3_length(v);
    if (len == 0.0) return (vec2){0.0, 0.0, 0.0}; // Or (vec3){0.0, 0.0, 0.0}
    return (vec3){v.x / len, v.y / len, v.z / len};
}


struct Engine {
    GLFWwindow* window;
    int WIDTH;
    int HEIGHT;
    float width;
    float height;
};

void framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight) {
    glViewport(0, 0, newWidth, newHeight);
}

int engine_init(struct Engine* engine)
{
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
    }

    engine -> window = glfwCreateWindow(engine ->WIDTH, engine ->HEIGHT, "Black Hole sim", NULL, NULL);
    if (!engine -> window)
    {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;

    }
    glfwMakeContextCurrent(engine -> window);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }
    glViewport(0,0, engine ->WIDTH, engine ->HEIGHT);
    glfwSetFramebufferSizeCallback(engine -> window, framebuffer_size_callback);

    return 0;
}

struct Blackhole
{
    vec2 pos;
    double mass;
    double r_s;
};

struct Blackhole bh =
{
    .pos = {0.0, 0.0},
    .mass = 1e36,
    .r_s = 0.0
};

void blackhole_init(struct Blackhole* bh)
{
    bh->r_s = (2.0 * G * bh -> mass) / (c*c);
}

void rays_init(struct Engine* engine) {
    double startX = -engine->width / 2.0;
    double startY_min = -engine->height / 2.0;
    double startY_max = engine->height / 2.0;
    double stepY = (startY_max - startY_min) / NUM_RAYS;

    for (int i = 0; i < NUM_RAYS; i++) {
        rays[i].pos = (vec2){startX, startY_min + i * stepY};
        rays[i].dir = (vec2){c, 0.0};
        rays[i].active = 1;
        vec2 relPos = vec2_sub(rays[i].pos, bh.pos);
        rays[i].L = relPos.x * rays[i].dir.y - relPos.y * rays[i].dir.x;
    }
}

void drawCircle(struct Blackhole* blackhole, struct Engine* engine, int segments)
{
    float aspect = (float)engine->HEIGHT / (float)engine->WIDTH;
    float ndcX = (float)blackhole->pos.x / (engine -> width / 2.0f);
    float ndcY = (float)blackhole->pos.y / (engine -> height / 2.0f);

    float ndcR_y = (float)blackhole->r_s / (engine -> height / 2.0f);
    float ndcR_x = ndcR_y * aspect;

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(ndcX, ndcY);
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = ndcX + cosf(angle) * ndcR_x;
        float y = ndcY + sinf(angle) * ndcR_y;
        glVertex2f(x,y);
    }
    glEnd();

}

void drawRays(struct Engine* engine) {
    glPointSize(4.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_RAYS; i++) {
        if (!rays[i].active) continue;

        float ndcX = (float)(rays[i].pos.x / (engine->width / 2.0));
        float ndcY = (float)(rays[i].pos.y / (engine->height / 2.0));

        glVertex2f(ndcX, ndcY);
    }
    glEnd();
}

void update_ray(Ray* ray, struct Blackhole* bh, double dt) {
    if (!ray->active) {
        return;
    }
    vec2 r = vec2_sub(bh->pos, ray->pos);
    double dist = vec2_length(r);

    if (dist <= bh -> r_s) {
        ray ->active = 0;
        return;
    }

    vec2 dir_to_bh = vec2_normalize(r);
    double accel_mag = 1.5 * bh->r_s * ray->L * ray->L / (dist * dist * dist * dist);
    vec2 accel = vec2_scale(dir_to_bh, accel_mag);
    ray->dir = vec2_add(ray->dir, vec2_scale(accel, dt));
    ray->dir = vec2_scale(vec2_normalize(ray->dir), c);
    ray->pos = vec2_add(ray->pos, vec2_scale(ray->dir, dt));
}

void engine_run(struct Engine* engine, struct Blackhole* bh)
{
    double dt = 1.0;
    while (!glfwWindowShouldClose(engine -> window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        for (int i = 0; i < NUM_RAYS; i++) {
            update_ray(&rays[i], bh, dt);
        }

        drawCircle(bh, engine, 64);
        drawRays(engine);

        glfwSwapBuffers(engine -> window);
        glfwPollEvents();
    }
}

int main()
{
    struct Engine engine =
    {
        .WIDTH = 1280,
        .HEIGHT = 720,
        .width = 1e11,
        .height = 5.625e10
    };

    blackhole_init(&bh);
    rays_init(&engine);

    if (engine_init(&engine) != 0)
    {
        return -1;
    }
    engine_run(&engine, &bh);

    glfwDestroyWindow(engine.window);
    glfwTerminate();

    return 0;
}
