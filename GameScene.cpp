#include "Scene.hpp"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>

Camera3d::Camera3d()
{
	controls_[Forward] = GLFW_KEY_W;
	controls_[Back] = GLFW_KEY_S;
	controls_[Left] = GLFW_KEY_A;
	controls_[Right] = GLFW_KEY_D;
	controls_[Up] = GLFW_KEY_SPACE;
	controls_[Down] = GLFW_KEY_LEFT_SHIFT;
	controls_[ToggleMouseCapture] = GLFW_KEY_1;
}

void Camera3d::captureMouse()
{
	glfwSetInputMode(gGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	firstCursorEvent_ = true;
	mouseCapture_ = true;
}

void Camera3d::processEvent(const WinEvent& event)
{
	if (event.type == WinEvent::Key)
	{
		int idx = -1;
		for (int i = 0; i < NumControls; ++i)
		{
			if (event.key.key == controls_[i])
			{
				idx = i;
				break;
			}
		}

		if (idx == -1)
			return;

		if (event.key.action == GLFW_PRESS)
		{
			keys_.pressed[idx] = true;
			keys_.held[idx] = true;

			if (idx == ToggleMouseCapture)
			{
				if (!mouseCapture_)
					captureMouse();
				else
				{
					mouseCapture_ = false;
					glfwSetInputMode(gGlfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
			}
		}
		else if (event.key.action == GLFW_RELEASE)
		{
			keys_.held[idx] = false;
		}
	}
	else if (event.type == WinEvent::Cursor && mouseCapture_)
	{
		const vec2 offset = event.cursor.pos - cursorPos_;
		cursorPos_ = event.cursor.pos;

		if (!firstCursorEvent_)
		{
			pitch -= offset.y * sensitivity;
			pitch = min(89.f, pitch);
			pitch = max(-89.f, pitch);
			yaw = fmodf(yaw - offset.x * sensitivity, 360.f);
		}
		else
			firstCursorEvent_ = false;
	}
}

void Camera3d::update(const float time)
{
	up = normalize(up);

	const vec3 dir = normalize(vec3(
		cosf(toRadians(pitch)) * sinf(toRadians(yaw)) * -1.f,
		sinf(toRadians(pitch)),
		cosf(toRadians(pitch)) * cosf(toRadians(yaw)) * -1.f));

	vec3 moveDir(0.f);

	{
		const auto forwardDir = forwardXZonly ? normalize(vec3(dir.x, 0.f, dir.z)) : dir;
		if (cActive(Forward)) moveDir += forwardDir;
		if (cActive(Back)) moveDir -= forwardDir;
	}
	{
		const auto right = normalize(cross(dir, up));
		if (cActive(Left)) moveDir -= right;
		if (cActive(Right)) moveDir += right;
	}

	if (cActive(Up)) moveDir += up;
	if (cActive(Down)) moveDir -= up;

	if (length(moveDir) != 0.f)
		normalize(moveDir);

	pos += moveDir * speed * time;
	view = lookAt(pos, pos + dir, up);

	for (auto& key : keys_.pressed)
		key = false;
}

void Camera3d::imgui()
{
	ImGui::Text("enable / disable mouse capture - 1");
	ImGui::Text("pitch / yaw - mouse");
	ImGui::Text("move - wsad, space (up), lshift (down)");
	ImGui::Text("pos: x: %.3f, y: %.3f, z: %.3f", pos.x, pos.y, pos.z);
	ImGui::Text("pitch: %.3f, yaw: %.3f", pitch, yaw);
	ImGui::Checkbox("disable flying with WS", &forwardXZonly);
}

GameScene::GameScene()
{
	glBuffers_ = createGLBuffers();
	font_ = createFontFromFile("res/Exo2-Black.otf", 38, 512);
}

GameScene::~GameScene()
{
	deleteGLBuffers(glBuffers_);
	deleteFont(font_);
}

void GameScene::processInput(const Array<WinEvent>& events)
{
	for (const WinEvent& event : events)
		camera_.processEvent(event);
}

void GameScene::update()
{
	camera_.update(frame_.time);
}

void GameScene::render(const GLuint program)
{
	bindProgram(program);

	Camera camera;
	camera.pos = vec2(0.f);
	camera.size = frame_.fbSize;
	uniform2f(program, "cameraPos", camera.pos);
	uniform2f(program, "cameraSize", camera.size);

	Rect rects[100];
	
	Text text;
	text.color = { 1.f, 1.f, 0.f, 1.f };
	text.pos = vec2(600.f);
	text.str = "T E T R I S  3D\nHELL YEA!";

	const int textCount = writeTextToBuffer(text, font_, rects, getSize(rects));
	uniform1i(program, "mode", FragmentMode::Font);
    bindTexture(font_.texture);
    updateGLBuffers(glBuffers_, rects, textCount);
	renderGLBuffers(glBuffers_, textCount);

    ImGui::Begin("main");
    ImGui::Spacing();
	camera_.imgui();
    ImGui::End();
}
