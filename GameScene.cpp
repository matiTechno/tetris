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

bool isCollision(Tetrimino& tetrimino, const Map& map)
{
	for (int j = 0; j < tetrimino.boxSide; ++j)
	{
		for (int i = 0; i < tetrimino.boxSide; ++i)
		{
			const int idx = j * tetrimino.boxSide + i;
			const int tileIdx = map.size.x * (tetrimino.pos.y + j) + tetrimino.pos.x + i;

			bool tileOccupied = map.rects[tileIdx].color != map.baseColor;

			if (tetrimino.box.d[idx] && (tileOccupied || tetrimino.pos.y + j == map.size.y
				|| tetrimino.pos.x + i == map.size.x || tetrimino.pos.x + i == -1))
			{
				return true;
			}
		}
	}

	return false;
}

Tetrimino::Box getBoxI(int rotation)
{
	Tetrimino::Box box;
	switch (rotation)
	{
	case 0:
		box = {
			0, 1, 0, 0,
			0, 1, 0, 0,
			0, 1, 0, 0,
			0, 1, 0, 0 };
		break;
	case 90:
		box = {
			0, 0, 0, 0,
			0, 0, 0, 0,
			1, 1, 1, 1,
			0, 0, 0, 0 };
		break;
	case 180:
		box = {
			0, 0, 1, 0,
			0, 0, 1, 0,
			0, 0, 1, 0,
			0, 0, 1, 0 };
		break;
	case 270:
		box = {
			0, 0, 0, 0,
			1, 1, 1, 1,
			0, 0, 0, 0,
			0, 0, 0, 0 };
		break;
	}
	return box;
}

Tetrimino::Box getBoxO(int rotation)
{
	Tetrimino::Box box;
	switch (rotation)
	{
	case 0:
		box = {
			0, 1, 1, 0,
			0, 1, 1, 0,
			0, 0, 0, 0,
			0, 0, 0, 0 };
		break;
	case 90:
		box = {
			0, 1, 1, 0,
			0, 1, 1, 0,
			0, 0, 0, 0,
			0, 0, 0, 0 };
		break;
	case 180:
		box = {
			0, 1, 1, 0,
			0, 1, 1, 0,
			0, 0, 0, 0,
			0, 0, 0, 0 };
		break;
	case 270:
		box = {
			0, 1, 1, 0,
			0, 1, 1, 0,
			0, 0, 0, 0,
			0, 0, 0, 0 };
		break;
	}
	return box;
}

Tetrimino::Box getBoxT(int rotation)
{
	Tetrimino::Box box;
	switch (rotation)
	{
	case 0:
		box = {
			0, 1, 0,
			1, 1, 1,
			0, 0, 0 };
		break;
	case 90:
		box = {
			0, 1, 0,
			1, 1, 0,
			0, 1, 0 };
		break;
	case 180:
		box = {
			0, 0, 0,
			1, 1, 1,
			0, 1, 0 };
		break;
	case 270:
		box = {
			0, 1, 0,
			0, 1, 1,
			0, 1, 0 };
		break;
	}
	return box;
}

Tetrimino::Box getBoxS(int rotation)
{
	Tetrimino::Box box;
	switch (rotation)
	{
	case 0:
		box = {
			0, 1, 1,
			1, 1, 0,
			0, 0, 0 };
		break;
	case 90:
		box = {
			1, 0, 0,
			1, 1, 0,
			0, 1, 0 };
		break;
	case 180:
		box = {
			0, 0, 0,
			0, 1, 1,
			1, 1, 0 };
		break;
	case 270:
		box = {
			0, 1, 0,
			0, 1, 1,
			0, 0, 1 };
		break;
	}
	return box;
}

Tetrimino::Box getBoxZ(int rotation)
{
	Tetrimino::Box box;
	switch (rotation)
	{
	case 0:
		box = {
			1, 1, 0,
			0, 1, 1,
			0, 0, 0 };
		break;
	case 90:
		box = {
			0, 1, 0,
			1, 1, 0,
			1, 0, 0 };
		break;
	case 180:
		box = {
			0, 0, 0,
			1, 1, 0,
			0, 1, 1 };
		break;
	case 270:
		box = {
			0, 0, 1,
			0, 1, 1,
			0, 1, 0 };
		break;
	}
	return box;
}

Tetrimino::Box getBoxJ(int rotation)
{
	Tetrimino::Box box;
	switch (rotation)
	{
	case 0:
		box = {
			1, 0, 0,
			1, 1, 1,
			0, 0, 0 };
		break;
	case 90:
		box = {
			0, 1, 0,
			0, 1, 0,
			1, 1, 0 };
		break;
	case 180:
		box = {
			0, 0, 0,
			1, 1, 1,
			0, 0, 1 };
		break;
	case 270:
		box = {
			0, 1, 1,
			0, 1, 0,
			0, 1, 0 };
		break;
	}
	return box;
}

Tetrimino::Box getBoxL(int rotation)
{
	Tetrimino::Box box;
	switch (rotation)
	{
	case 0:
		box = {
			0, 0, 1,
			1, 1, 1,
			0, 0, 0 };
		break;
	case 90:
		box = {
			1, 1, 0,
			0, 1, 0,
			0, 1, 0 };
		break;
	case 180:
		box = {
			0, 0, 0,
			1, 1, 1,
			1, 0, 0 };
		break;
	case 270:
		box = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 1 };
		break;
	}
	return box;
}

void spawnNewTetrimino(Tetrimino& t)
{
	t.pos = ivec2(3.f, 0.f);
	t.type = getRandomInt(0, Tetrimino::NumTypes - 1);
	t.rotation = 0;

	switch (t.type)
	{
	case Tetrimino::I:;
		t.color = vec4(0.f, 1.f, 1.f, 1.f);
		t.boxSide = 4;
		t.box = getBoxI(t.rotation);
		break;

	case Tetrimino::O:
		t.color = vec4(1.f, 1.f, 0.f, 1.f);
		t.boxSide = 4;
		t.box = getBoxO(t.rotation);
		break;

	case Tetrimino::T:;
		t.color = vec4(1.f, 0.f, 1.f, 1.f);
		t.boxSide = 3;
		t.box = getBoxT(t.rotation);
		break;

	case Tetrimino::S:;
		t.color = vec4(0.f, 1.f, 0.f, 1.f);
		t.boxSide = 3;
		t.box = getBoxS(t.rotation);
		break;

	case Tetrimino::Z:;
		t.color = vec4(1.f, 0.f, 0.f, 1.f);
		t.boxSide = 3;
		t.box = getBoxZ(t.rotation);
		break;

	case Tetrimino::J:;
		t.color = vec4(0.f, 0.f, 1.f, 1.f);
		t.boxSide = 3;
		t.box = getBoxJ(t.rotation);
		break;

	case Tetrimino::L:;
		t.color = vec4(1.f, 0.5f, 0.f, 1.f);
		t.boxSide = 3;
		t.box = getBoxL(t.rotation);
		break;
	}
}

void rotate(bool cw, Tetrimino& t)
{
	t.rotation += cw ? -90 : 90;

	if (t.rotation == -90)
		t.rotation = 270;

	else if (t.rotation == 360)
		t.rotation = 0;

	switch (t.type)
	{
	case Tetrimino::I:;
		t.box = getBoxI(t.rotation);
		break;

	case Tetrimino::O:
		t.box = getBoxO(t.rotation);
		break;

	case Tetrimino::T:;
		t.box = getBoxT(t.rotation);
		break;

	case Tetrimino::S:;
		t.box = getBoxS(t.rotation);
		break;

	case Tetrimino::Z:;
		t.box = getBoxZ(t.rotation);
		break;

	case Tetrimino::J:;
		t.box = getBoxJ(t.rotation);
		break;

	case Tetrimino::L:;
		t.box = getBoxL(t.rotation);
		break;
	}
}

GameScene::GameScene()
{
	glBuffers_ = createGLBuffers();
	font_ = createFontFromFile("res/Exo2-Black.otf", 38, 512);

	for (int y = 0; y < map_.size.y; ++y)
	{
		for (int x = 0; x < map_.size.x; ++x)
		{
			const int idx = y * map_.size.x + x;
			Rect& rect = map_.rects[idx];
			rect.color = map_.baseColor;
			rect.rotation = 0.f;
			rect.size = vec2(1.f);
			rect.pos = vec2(x, y);
		}
	}

	spawnNewTetrimino(tetrimino_);
}

GameScene::~GameScene()
{
	deleteGLBuffers(glBuffers_);
	deleteFont(font_);
}

void GameScene::processInput(const Array<WinEvent>& events)
{
	for (const WinEvent& event : events)
	{
		camera_.processEvent(event);

		if (event.type == WinEvent::Key && event.key.action != GLFW_RELEASE)
		{
			if (event.key.key == GLFW_KEY_ESCAPE)
				frame_.popMe = true;

			ivec2 move(0);

			if (event.key.key == GLFW_KEY_A || event.key.key == GLFW_KEY_LEFT)
				move.x -= 1;

			else if (event.key.key == GLFW_KEY_D || event.key.key == GLFW_KEY_RIGHT)
				move.x += 1;

			else if (event.key.key == GLFW_KEY_S || event.key.key == GLFW_KEY_DOWN)
				move.y += 1;

			else if (event.key.key == GLFW_KEY_SPACE)
			{
				while (!isCollision(tetrimino_, map_))
					tetrimino_.pos.y += 1;

				tetrimino_.pos.y -= 1;
			}
			else if (event.key.key == GLFW_KEY_Z)
				rotate(false, tetrimino_);

			else if (event.key.key == GLFW_KEY_X)
				rotate(true, tetrimino_);

			if (move.x || move.y)
			{
				tetrimino_.pos += move;
				if (isCollision(tetrimino_, map_))
					tetrimino_.pos -= move;
			}
		}
	}
}

void GameScene::update()
{
	camera_.update(frame_.time);

	static float accumulator = 0.f;

	accumulator += frame_.time;

	if (accumulator >= 0.3f)
	{
		tetrimino_.pos.y += 1;
		accumulator = 0.f;
	}

	if (isCollision(tetrimino_, map_))
	{
		tetrimino_.pos.y -= 1;

		for (int j = 0; j < tetrimino_.boxSide; ++j)
		{
			for (int i = 0; i < tetrimino_.boxSide; ++i)
			{
				const int idx = j * tetrimino_.boxSide + i;
				if (tetrimino_.box.d[idx])
				{
					const int tileIdx = map_.size.x * (tetrimino_.pos.y + j) + tetrimino_.pos.x + i;
					map_.rects[tileIdx].color = tetrimino_.color;
					map_.rects[tileIdx].size = vec2(1.f);
					map_.rects[tileIdx].rotation = 0.f;
					map_.rects[tileIdx].pos = vec2(tetrimino_.pos + ivec2(i, j));
				}
			}
		}

		for (int _i = 0; _i < tetrimino_.boxSide; ++_i)
		{
		    const int clearRowIdx = tetrimino_.pos.y + _i;

			if (clearRowIdx >= map_.size.y)
				break;

			{
				const int rectIdx = map_.size.x * clearRowIdx;

				bool complete = true;

				for (int i = 0; i < map_.size.x; ++i)
				{
					if (map_.rects[rectIdx + i].color == map_.baseColor) // is free
					{
						complete = false;
						break;
					}
				}

				if (!complete)
					continue;

				// clear the completed row
				for (int i = 0; i < map_.size.x; ++i)
				{
					map_.rects[rectIdx + i].color = map_.baseColor;
				}
			}

			int firstNonEmptyRow = map_.size.y;
			for (int j = 0; j < map_.size.y; ++j)
			{
				for (int i = 0; i < map_.size.x; ++i)
				{
					if (map_.rects[j * map_.size.x + i].color != map_.baseColor)
					{
						firstNonEmptyRow = j;
						goto endLoop;
					}
				}
			}
			endLoop:

			if (firstNonEmptyRow > clearRowIdx)
				continue;

			for (int j = clearRowIdx; j > firstNonEmptyRow; --j)
			{
				for (int i = 0; i < map_.size.x; ++i)
				{
					const int rectIdx = j * map_.size.x + i;
					map_.rects[rectIdx].color = map_.rects[rectIdx - map_.size.x].color;
				}
			}

			for (int i = 0; i < map_.size.x; ++i)
				map_.rects[firstNonEmptyRow * map_.size.x + i].color = map_.baseColor;

		}

		spawnNewTetrimino(tetrimino_);
	}
}

void GameScene::render(const GLuint program)
{
	Rect rects[256];
	bindProgram(program);
	Camera camera;

	// render tetris
	camera.pos = vec2(0.f);
	camera.size = vec2(map_.size);
	camera = expandToMatchAspectRatio(camera, frame_.fbSize);
	uniform2f(program, "cameraPos", camera.pos);
	uniform2f(program, "cameraSize", camera.size);

	uniform1i(program, "mode", FragmentMode::Color);

    updateGLBuffers(glBuffers_, map_.rects, getSize(map_.rects));
	renderGLBuffers(glBuffers_, getSize(map_.rects));

	for (int j = 0; j < tetrimino_.boxSide; ++j)
	{
		for (int i = 0; i < tetrimino_.boxSide; ++i)
		{
			const int idx = j * tetrimino_.boxSide + i;
			if (tetrimino_.box.d[idx])
			{
				rects[idx].color = tetrimino_.color;
				rects[idx].size = vec2(1.f);
				rects[idx].rotation = 0.f;
				rects[idx].pos = vec2(tetrimino_.pos + ivec2(i, j));
			}
		}
	}

    updateGLBuffers(glBuffers_, rects, tetrimino_.boxSide * tetrimino_.boxSide);
	renderGLBuffers(glBuffers_, tetrimino_.boxSide * tetrimino_.boxSide);

	// render ui
	camera.pos = vec2(0.f);
	camera.size = frame_.fbSize;
	uniform2f(program, "cameraPos", camera.pos);
	uniform2f(program, "cameraSize", camera.size);
	
	Text text;
	text.color = { 1.f, 1.f, 0.f, 1.f };
	text.pos = vec2(200.f, 600.f);
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
