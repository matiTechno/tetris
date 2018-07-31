#pragma once

#include "Array.hpp"
#include "math.hpp"
#include "fmod/fmod.h"

using GLuint = unsigned int;

// use on plain C arrays
template<typename T, int N>
constexpr int getSize(T(&)[N])
{
    return N;
}

struct FragmentMode
{
    enum
    {
        Color = 0,
        Texture = 1,
        Font = 2
    };
};

struct Texture
{
    ivec2 size;
    GLuint id;
};

struct Glyph
{
    vec4 texRect;
    float advance;
    vec2 offset;
};

struct Font
{
    Texture texture;
    Glyph glyphs[127];
    float lineSpace;
};

// @TODO(matiTechno)
// add origin for rotation (needed to properly rotate a text)
struct Rect
{
    vec2 pos;
    vec2 size;
    vec4 color = {1.f, 1.f, 1.f, 1.f};
    vec4 texRect = {0.f, 0.f, 1.f, 1.f};
    float rotation = 0.f;
};

struct Text
{
    vec2 pos;
    vec4 color = {1.f, 1.f, 1.f, 1.f};
    //float rotation = 0.f;
    float scale = 1.f;
    const char* str = "";
};

// @ better naming?
struct GLBuffers
{
    GLuint vao;
    GLuint vbo;
    GLuint rectBo;
};

struct WinEvent
{
    enum Type
    {
        Nil,
        Key,
        Cursor,
        MouseButton,
        Scroll
    };

    Type type;

    // glfw values
    union
    {
        struct
        {
            int key;
            int action;
            int mods;
        } key;

        struct
        {
            vec2 pos;
        } cursor;

        struct
        {
            int button;
            int action;
            int mods;
        } mouseButton;

        struct
        {
            vec2 offset;
        } scroll;
    };
};

// call bindeProgram() first
void uniformMat4(GLuint program, const char* name, const mat4& m);
void uniform1i(GLuint program, const char* name, int i);
void uniform1f(GLuint program, const char* name, float f);
void uniform2f(GLuint program, const char* name, float f1, float f2);
void uniform2f(GLuint program, const char* name, vec2 v);
void uniform3f(GLuint program, const char* name, float f1, float f2, float f3);
void uniform3f(GLuint program, const char* name, vec3 v);
void uniform4f(GLuint program, const char* name, float f1, float f2, float f3, float f4);
void uniform4f(GLuint program, const char* name, vec4 v);

void bindTexture(const Texture& texture, GLuint unit = 0);
// delete with deleteTexture()
Texture createTextureFromFile(const char* filename);
void deleteTexture(const Texture& texture);

// delete with deleteFont()
Font createFontFromFile(const char* filename, int fontSize, int textureWidth);
void deleteFont(Font& font);

// returns 0 on failure
// program must be deleted with deleteProgram() (if != 0)
GLuint createProgram(const char* vertexSrc, const char* fragmentSrc);
void deleteProgram(GLuint program);
void bindProgram(const GLuint program);

// delete with deleteGLBuffers()
GLBuffers createGLBuffers();
// @TODO(matiTechno): we might want updateSubGLBuffers()
void updateGLBuffers(GLBuffers& glBuffers, const Rect* rects, int count);
// call bindProgram() first
void renderGLBuffers(GLBuffers& glBuffers, int numRects);
void deleteGLBuffers(GLBuffers& glBuffers);

// returns the number of rects written
int writeTextToBuffer(const Text& text, const Font& font, Rect* buffer, int maxSize);
// bbox
vec2 getTextSize(const Text& text, const Font& font);

bool fmodCheck(FMOD_RESULT r, const char* file, int line); // don't use this

// wrap fmod calls in this
// returns true if function succeeded
#define FCHECK(x) fmodCheck(x, __FILE__, __LINE__)

extern FMOD_SYSTEM* fmodSystem;
extern GLFWwindow* gGlfwWindow;

struct Camera
{
    vec2 pos;
    vec2 size;
};

Camera expandToMatchAspectRatio(Camera camera, vec2 viewportSize);

// [min, max]
float getRandomFloat(float min, float max);
int getRandomInt(int min, int max);

class Scene
{
public:
    virtual ~Scene() = default;
    virtual void processInput(const Array<WinEvent>& events) {(void)events;}
    virtual void update() {}
    virtual void render(GLuint program) {(void)program;}

    struct
    {
        // these are set in the main loop before processInput() call
        float time;  // seconds
        vec2 fbSize; // fb = framebuffer

        // @TODO(matiTechno): bool updateWhenNotTop = false;
        bool popMe = false;
        Scene* newScene = nullptr; // assigned ptr must be returned by new
                                   // game loop will call delete on it
    } frame_;
};

// TETRIS

// fps camera
class Camera3d
{
public:
	Camera3d();

	void captureMouse(); // off on start
	void processEvent(const WinEvent& event);
	// process events first
	void update(float time);
	void imgui();

	bool forwardXZonly = false; // disable flying with W and S controls
	vec3 up = { 0.f, 1.f, 0.f }; // will be normalized in update()
	float speed = 2.f;
	float sensitivity = 0.1f; // degrees / screen coordinates
							  // from GLFW - screen coordinates are not always pixels

							  // get after update()

	mat4 view;
	vec3 pos = { 0.f, 0.2f, 1.2f };
	// degrees
	float pitch = 0.f;
	float yaw = 0.f;

private:
	enum
	{
		Forward,
		Back,
		Left,
		Right,
		Up,
		Down,
		ToggleMouseCapture,
		NumControls
	};

	int controls_[NumControls];

	struct
	{
		bool pressed[NumControls] = {};
		bool held[NumControls] = {};
	} keys_;

	vec2 cursorPos_;
	bool mouseCapture_ = false;
	bool firstCursorEvent_;

	bool cActive(int control) const { return keys_.pressed[control] || keys_.held[control]; }
};

struct Tetrimino
{
	enum Type
	{
		I,
		O,
		T,
		S,
		Z,
		J,
		L,
		NumTypes
	};

	struct Box { bool d[16]; };

	ivec2 pos;
	int rotation;
	int type;
	vec4 color;
	Box box;
	int boxSide;
};


struct Map
{
	ivec2 size = ivec2(10, 20);
	Rect rects[10 * 20]; // meeh
	const vec4 baseColor = vec4(0.3f, 0.f, 0.5f, 1.f);
};

class GameScene: public Scene
{
public:
    GameScene();
    ~GameScene() override;
    void processInput(const Array<WinEvent>& events) override;
    void update() override;
    void render(GLuint program) override;

private:
	Camera3d camera_;
	GLBuffers glBuffers_;
	Font font_;

	Map map_;
	Tetrimino tetrimino_, tetNext_;
    bool executeStep_ = false;
    bool gameOver_ = false;
    int score_ = 0.f;

	GLuint p3d_;
	GLuint vboQube_;
	GLuint vboLines_;
	GLuint vaoLines_;
	GLuint vboIA_; // instanced attributes
	GLuint vao_;
	GLuint programLines_;

	bool render3d_ = true;

	bool enableCameraInput_ = false;
};
