#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Mat4 {
    float m[16] = {};
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
};

struct Mesh {
    std::vector<Vertex> vertices;
    Vec3 center;
    float scale = 1.0f;
};

struct Material {
    Vec3 ambient;
    Vec3 diffuse;
    Vec3 specular;
    float shininess = 32.0f;
};

static const std::vector<Material> kMaterials = {
    {{0.18f, 0.08f, 0.03f}, {0.72f, 0.36f, 0.13f}, {0.35f, 0.22f, 0.12f}, 24.0f},
    {{0.10f, 0.08f, 0.05f}, {0.78f, 0.57f, 0.24f}, {0.95f, 0.82f, 0.45f}, 72.0f},
    {{0.02f, 0.08f, 0.09f}, {0.05f, 0.55f, 0.62f}, {0.55f, 0.90f, 0.95f}, 96.0f},
    {{0.04f, 0.04f, 0.045f}, {0.28f, 0.30f, 0.32f}, {0.85f, 0.88f, 0.90f}, 128.0f},
};

static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << '\n';
}

static Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }

static float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

static Vec3 normalize(Vec3 v) {
    float len = std::sqrt(std::max(0.000001f, dot(v, v)));
    return v * (1.0f / len);
}

static Mat4 identity() {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            r.m[c * 4 + row] =
                a.m[0 * 4 + row] * b.m[c * 4 + 0] +
                a.m[1 * 4 + row] * b.m[c * 4 + 1] +
                a.m[2 * 4 + row] * b.m[c * 4 + 2] +
                a.m[3 * 4 + row] * b.m[c * 4 + 3];
        }
    }
    return r;
}

static Mat4 translate(Vec3 t) {
    Mat4 r = identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

static Mat4 scale(float s) {
    Mat4 r = identity();
    r.m[0] = r.m[5] = r.m[10] = s;
    return r;
}

static Mat4 perspective(float fovy, float aspect, float nearPlane, float farPlane) {
    float f = 1.0f / std::tan(fovy * 0.5f);
    Mat4 r;
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return r;
}

static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up);

static Mat4 makeModelMatrix(const Mesh& mesh) {
    return multiply(scale(mesh.scale), translate(mesh.center * -1.0f));
}

static Mat4 makeViewMatrix(Vec3 camera, Vec3 target) {
    return lookAt(camera, target, {0.0f, 1.0f, 0.0f});
}

static Mat4 makeProjectionMatrix(int width, int height) {
    float aspect = width > 0 && height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    return perspective(45.0f * 3.14159265f / 180.0f, aspect, 0.05f, 100.0f);
}

static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = normalize(center - eye);
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);

    Mat4 r = identity();
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;
    r.m[12] = -dot(s, eye);
    r.m[13] = -dot(u, eye);
    r.m[14] = dot(f, eye);
    return r;
}

static int parseObjIndex(const std::string& token) {
    std::size_t slash = token.find('/');
    std::string head = slash == std::string::npos ? token : token.substr(0, slash);
    return std::stoi(head) - 1;
}

static int parseNormalIndex(const std::string& token) {
    std::size_t first = token.find('/');
    if (first == std::string::npos) {
        return -1;
    }
    std::size_t second = token.find('/', first + 1);
    if (second == std::string::npos || second + 1 >= token.size()) {
        return -1;
    }
    return std::stoi(token.substr(second + 1)) - 1;
}

static Mesh loadObj(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Unable to open " + path);
    }

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    Mesh mesh;

    Vec3 minP{1e9f, 1e9f, 1e9f};
    Vec3 maxP{-1e9f, -1e9f, -1e9f};
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string tag;
        in >> tag;

        if (tag == "v") {
            Vec3 p;
            in >> p.x >> p.y >> p.z;
            positions.push_back(p);
            minP = {std::min(minP.x, p.x), std::min(minP.y, p.y), std::min(minP.z, p.z)};
            maxP = {std::max(maxP.x, p.x), std::max(maxP.y, p.y), std::max(maxP.z, p.z)};
        } else if (tag == "vn") {
            Vec3 n;
            in >> n.x >> n.y >> n.z;
            normals.push_back(normalize(n));
        } else if (tag == "f") {
            std::vector<std::string> face;
            std::string token;
            while (in >> token) {
                face.push_back(token);
            }

            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                std::string tri[3] = {face[0], face[i], face[i + 1]};
                Vertex out[3];
                for (int j = 0; j < 3; ++j) {
                    int pi = parseObjIndex(tri[j]);
                    int ni = parseNormalIndex(tri[j]);
                    if (pi < 0 || pi >= static_cast<int>(positions.size())) {
                        throw std::runtime_error("OBJ face references an invalid vertex");
                    }
                    out[j].position = positions[pi];
                    out[j].normal = (ni >= 0 && ni < static_cast<int>(normals.size())) ? normals[ni] : Vec3{};
                }

                Vec3 faceNormal = normalize(cross(out[1].position - out[0].position, out[2].position - out[0].position));
                for (Vertex& v : out) {
                    if (dot(v.normal, v.normal) < 0.01f) {
                        v.normal = faceNormal;
                    }
                    mesh.vertices.push_back(v);
                }
            }
        }
    }

    mesh.center = (minP + maxP) * 0.5f;
    Vec3 extent = maxP - minP;
    float maxExtent = std::max(extent.x, std::max(extent.y, extent.z));
    mesh.scale = maxExtent > 0.0f ? 2.8f / maxExtent : 1.0f;
    return mesh;
}

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        throw std::runtime_error(log);
    }
    return shader;
}

static GLuint makeProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        throw std::runtime_error(log);
    }
    return program;
}

static const char* kMeshVertexShader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out float vHeight;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPos = world.xyz;
    vNormal = transpose(inverse(mat3(uModel))) * aNormal;
    vHeight = aPosition.y;
    gl_Position = uProjection * uView * world;
}
)GLSL";

static const char* kMeshFragmentShader = R"GLSL(
#version 330 core
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 vWorldPos;
in vec3 vNormal;
in float vHeight;

uniform vec3 uCameraPos;
uniform Material uMaterial;
uniform Light uLight;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 lightDir = normalize(uLight.position - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, n);

    float diff = max(dot(n, lightDir), 0.0);
    float spec = diff > 0.0 ? pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess) : 0.0;
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 2.0);

    float bands = 0.5 + 0.5 * sin(vHeight * 18.0);
    vec3 patternedDiffuse = uMaterial.diffuse * mix(0.78, 1.18, bands);

    vec3 ambient = uLight.ambient * uMaterial.ambient;
    vec3 diffuse = uLight.diffuse * diff * patternedDiffuse;
    vec3 specular = uLight.specular * spec * uMaterial.specular;
    vec3 rimLight = uMaterial.specular * rim * 0.10;

    vec3 color = ambient + diffuse + specular + rimLight;
    fragColor = vec4(color, 1.0);
}
)GLSL";

static const char* kLineVertexShader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

out float vDistance;

void main() {
    vDistance = length(aPosition.xz);
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
}
)GLSL";

static const char* kLineFragmentShader = R"GLSL(
#version 330 core
in float vDistance;

out vec4 fragColor;

void main() {
    float fade = smoothstep(6.0, 0.0, vDistance);
    vec3 base = mix(vec3(0.05, 0.07, 0.08), vec3(0.24, 0.28, 0.30), fade);
    fragColor = vec4(base, 0.75);
}
)GLSL";

static std::vector<Vec3> makeGrid() {
    std::vector<Vec3> lines;
    constexpr int count = 24;
    constexpr float step = 0.25f;
    constexpr float extent = count * step;
    for (int i = -count; i <= count; ++i) {
        float p = i * step;
        lines.push_back({-extent, -1.42f, p});
        lines.push_back({extent, -1.42f, p});
        lines.push_back({p, -1.42f, -extent});
        lines.push_back({p, -1.42f, extent});
    }
    return lines;
}

static void setVec3Uniform(GLuint program, const char* name, Vec3 value) {
    glUniform3f(glGetUniformLocation(program, name), value.x, value.y, value.z);
}

static void setMaterialUniforms(GLuint program, const Material& material) {
    setVec3Uniform(program, "uMaterial.ambient", material.ambient);
    setVec3Uniform(program, "uMaterial.diffuse", material.diffuse);
    setVec3Uniform(program, "uMaterial.specular", material.specular);
    glUniform1f(glGetUniformLocation(program, "uMaterial.shininess"), material.shininess);
}

static void setLightUniforms(GLuint program) {
    setVec3Uniform(program, "uLight.position", {-2.8f, 3.6f, 3.2f});
    setVec3Uniform(program, "uLight.ambient", {0.30f, 0.30f, 0.32f});
    setVec3Uniform(program, "uLight.diffuse", {0.92f, 0.88f, 0.80f});
    setVec3Uniform(program, "uLight.specular", {1.0f, 0.96f, 0.88f});
}

static void handleInput(GLFWwindow* window, float dt, float& yaw, float& pitch, float& distance, int& materialIndex) {
    float orbitSpeed = 1.6f * dt;
    float zoomSpeed = 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) yaw -= orbitSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) yaw += orbitSpeed;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pitch += orbitSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pitch -= orbitSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) distance += zoomSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) distance -= zoomSpeed;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        yaw = 0.65f;
        pitch = 0.38f;
        distance = 5.4f;
    }

    for (int i = 0; i < static_cast<int>(kMaterials.size()); ++i) {
        if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS) {
            materialIndex = i;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    pitch = std::clamp(pitch, -0.2f, 1.15f);
    distance = std::clamp(distance, 2.4f, 9.0f);
}

int main() {
    try {
        Mesh mesh = loadObj("TungTungTungSahur.obj");
        std::cout << "Loaded " << mesh.vertices.size() << " render vertices\n";

        glfwSetErrorCallback(glfwErrorCallback);
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4)
        // GLEW is GLX-oriented on many Linux installs, so prefer X11 over Wayland/EGL.
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        GLFWwindow* window = glfwCreateWindow(1280, 720, "Tung Tung Sahur - OpenGL", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        glewExperimental = GL_TRUE;
        GLenum glewStatus = glewInit();
        if (glewStatus != GLEW_OK) {
            std::ostringstream message;
            message << "Failed to initialize GLEW: " << glewGetErrorString(glewStatus);
            throw std::runtime_error(message.str());
        }
        glGetError();

        GLuint meshProgram = makeProgram(kMeshVertexShader, kMeshFragmentShader);
        GLuint lineProgram = makeProgram(kLineVertexShader, kLineFragmentShader);

        GLuint meshVao = 0;
        GLuint meshVbo = 0;
        glGenVertexArrays(1, &meshVao);
        glGenBuffers(1, &meshVbo);
        glBindVertexArray(meshVao);
        glBindBuffer(GL_ARRAY_BUFFER, meshVbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(1);

        std::vector<Vec3> grid = makeGrid();
        GLuint gridVao = 0;
        GLuint gridVbo = 0;
        glGenVertexArrays(1, &gridVao);
        glGenBuffers(1, &gridVbo);
        glBindVertexArray(gridVao);
        glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(grid.size() * sizeof(Vec3)), grid.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), nullptr);
        glEnableVertexAttribArray(0);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float yaw = 0.65f;
        float pitch = 0.38f;
        float distance = 5.4f;
        int materialIndex = 0;
        double previous = glfwGetTime();

        while (!glfwWindowShouldClose(window)) {
            double now = glfwGetTime();
            float dt = static_cast<float>(now - previous);
            previous = now;
            handleInput(window, dt, yaw, pitch, distance, materialIndex);

            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);

            Vec3 target{0.0f, 0.0f, 0.0f};
            Vec3 camera{
                std::sin(yaw) * std::cos(pitch) * distance,
                std::sin(pitch) * distance,
                std::cos(yaw) * std::cos(pitch) * distance,
            };

            Mat4 model = makeModelMatrix(mesh);
            Mat4 view = makeViewMatrix(camera, target);
            Mat4 projection = makeProjectionMatrix(width, height);

            glClearColor(0.035f, 0.045f, 0.052f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(lineProgram);
            glUniformMatrix4fv(glGetUniformLocation(lineProgram, "uView"), 1, GL_FALSE, view.m);
            glUniformMatrix4fv(glGetUniformLocation(lineProgram, "uProjection"), 1, GL_FALSE, projection.m);
            glBindVertexArray(gridVao);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(grid.size()));

            glUseProgram(meshProgram);
            glUniformMatrix4fv(glGetUniformLocation(meshProgram, "uModel"), 1, GL_FALSE, model.m);
            glUniformMatrix4fv(glGetUniformLocation(meshProgram, "uView"), 1, GL_FALSE, view.m);
            glUniformMatrix4fv(glGetUniformLocation(meshProgram, "uProjection"), 1, GL_FALSE, projection.m);
            setVec3Uniform(meshProgram, "uCameraPos", camera);
            setMaterialUniforms(meshProgram, kMaterials[materialIndex]);
            setLightUniforms(meshProgram);
            glBindVertexArray(meshVao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertices.size()));

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteBuffers(1, &meshVbo);
        glDeleteVertexArrays(1, &meshVao);
        glDeleteBuffers(1, &gridVbo);
        glDeleteVertexArrays(1, &gridVao);
        glDeleteProgram(meshProgram);
        glDeleteProgram(lineProgram);
        glfwDestroyWindow(window);
        glfwTerminate();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
