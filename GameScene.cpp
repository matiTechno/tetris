#include "Scene.hpp"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

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
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        spawnNewTetrimino(tetNext_);
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

                        else if (event.key.key == GLFW_KEY_ENTER && gameOver_)
                        {
                                for(Rect& r: map_.rects)
                                    r.color = map_.baseColor;

                                spawnNewTetrimino(tetrimino_);
                                spawnNewTetrimino(tetNext_);

                                gameOver_ = false;

                        }

                        if(gameOver_)
                            continue;

                        ivec2 move(0);

                        if (event.key.key == GLFW_KEY_A || event.key.key == GLFW_KEY_LEFT)
                                move.x -= 1;

                        else if (event.key.key == GLFW_KEY_D || event.key.key == GLFW_KEY_RIGHT)
                                move.x += 1;

                        else if (event.key.key == GLFW_KEY_S || event.key.key == GLFW_KEY_DOWN)
                                move.y += 1;

                        else if (event.key.key == GLFW_KEY_SPACE)
                        {
                                // in case while loop will break immediately
                                tetrimino_.pos.y += 1;

                                while (!isCollision(tetrimino_, map_))
                                        tetrimino_.pos.y += 1;

                                tetrimino_.pos.y -= 1;
                                executeStep_ = true;
                        }
                        else if (event.key.key == GLFW_KEY_Z)
                        {
                                rotate(false, tetrimino_);

                                if(isCollision(tetrimino_, map_))
                                        rotate(true, tetrimino_);
                        }

                        else if (event.key.key == GLFW_KEY_X)
                        {
                                rotate(true, tetrimino_);

                                if(isCollision(tetrimino_, map_))
                                        rotate(false, tetrimino_);
                        }

                        if (move.x || move.y)
                        {
                                tetrimino_.pos += move;
                                if (isCollision(tetrimino_, map_))
                                {
                                        tetrimino_.pos -= move;
                                        if(move.y)
                                            executeStep_ = true;
                                }
                        }
                }
        }
}

void GameScene::update()
{
        camera_.update(frame_.time);

        static float accumulator = 0.f;

        if(gameOver_)
            return;

        accumulator += frame_.time;

        if (accumulator >= 0.5f || executeStep_)
        {
                executeStep_ = false;
                tetrimino_.pos.y += 1;
                accumulator = 0.f;
        }
        else return;

        if (isCollision(tetrimino_, map_))
        {
            tetrimino_.pos.y -= 1;

            if(tetrimino_.pos.y == 0)
            {
                gameOver_ = true;
                return;
            }

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

            int numCompletedRows = 0;

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

                            ++numCompletedRows;

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

            tetrimino_ = tetNext_;
            spawnNewTetrimino(tetNext_);

            int coeff = 0;

            switch(numCompletedRows)
                {
                    case 1:
                        coeff = 1;
                        break;

                    case 2:
                        coeff = 4;
                        break;

                    case 3:
                        coeff = 8;
                        break;

                    case 4:
                        coeff = 16;
                        break;

                }

            score_ += coeff * 100;

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


        int rectIdx = 0;

        // tetrimino
        for (int j = 0; j < tetrimino_.boxSide; ++j)
        {
                for (int i = 0; i < tetrimino_.boxSide; ++i)
                {
                        if (tetrimino_.box.d[j * tetrimino_.boxSide + i])
                        {
                                rects[rectIdx].color = tetrimino_.color;
                                rects[rectIdx].size = vec2(1.f);
                                rects[rectIdx].rotation = 0.f;
                                rects[rectIdx].pos = vec2(tetrimino_.pos + ivec2(i, j));
                                ++rectIdx;
                        }
                }
        }

        // next tetrimino

        for (int j = 0; j < tetNext_.boxSide; ++j)
        {
                for (int i = 0; i < tetNext_.boxSide; ++i)
                {
                        if (tetNext_.box.d[j * tetNext_.boxSide + i])
                        {
                                rects[rectIdx].color = tetNext_.color;
                                rects[rectIdx].size = vec2(1.f);
                                rects[rectIdx].rotation = 0.f;
                                rects[rectIdx].pos = vec2(ivec2(map_.size.x + 2, 2) + ivec2(i, j));
                                ++rectIdx;
                        }
                }
        }

        // drop shadow
        const ivec2 savePos = tetrimino_.pos;

        // in case while loop will break immediately
        tetrimino_.pos.y += 1;

        while(!isCollision(tetrimino_, map_))
            tetrimino_.pos.y += 1;

        tetrimino_.pos.y -= 1;

        ivec2 shadowPos = tetrimino_.pos;
        tetrimino_.pos = savePos;

        for (int j = 0; j < tetrimino_.boxSide; ++j)
        {
                for (int i = 0; i < tetrimino_.boxSide; ++i)
                {
                        if (tetrimino_.box.d[j * tetrimino_.boxSide + i])
                        {
                                ivec2 shadowTilePos(shadowPos + ivec2(i, j));
                                vec4 color(0.f, 0.f, 0.f, 0.5f);

                                for (int j2 = 0; j2 < tetrimino_.boxSide; ++j2)
                                {
                                    for (int i2 = 0; i2 < tetrimino_.boxSide; ++i2)
                                    {
                                        if(tetrimino_.box.d[tetrimino_.boxSide * j2 + i2])
                                        {
                                            ivec2 tilePos = tetrimino_.pos + ivec2(i2, j2);

                                            if(tilePos == shadowTilePos)
                                                color = vec4(0.f);
                                        }
                                    }
                                }


                                rects[rectIdx].color = color;
                                rects[rectIdx].size = vec2(1.f);
                                rects[rectIdx].rotation = 0.f;
                                rects[rectIdx].pos = vec2(shadowTilePos);

                                ++rectIdx;
                        }
                }
        }

        updateGLBuffers(glBuffers_, rects, rectIdx);
        renderGLBuffers(glBuffers_, rectIdx);

        int textCount = 0;

        // game over text
        if(gameOver_)
        {
            static float accumulator = 0.f;
            static bool show = true;
            accumulator += frame_.time;

            if(accumulator > 1.f)
            {
                show = !show;
                accumulator = 0.f;
            }

            Text text;
            text.color = { 1.f, 1.f, 1.f, 1.f };
            text.str = "Game Over. Press ENTER to start new game!";
            text.scale = 0.04f;

            const vec2 size = getTextSize(text, font_);

            text.pos = ( vec2(map_.size) - (size + vec2(0.5f)) ) / vec2(2.f);

            if(show)
            {
                Text text2 =  text;
                text2.color = vec4(1.f, 0.f, 0.f, 1.f);
                text2.pos.y += 0.1f;

                textCount += writeTextToBuffer(text2, font_, rects, getSize(rects));
            }

            textCount += writeTextToBuffer(text, font_, rects + textCount, getSize(rects));

        }

        // render score
        {
            Text text;
            char buff[126];
            sprintf(buff, "score: %d", score_);
            text.color = { 1.f, 1.f, 1.f, 1.f };
            text.str = buff;
            text.scale = 0.04f;

            text.pos = vec2(-10, 8);

            textCount += writeTextToBuffer(text, font_, rects + textCount, getSize(rects));
            
            uniform1i(program, "mode", FragmentMode::Font);
            bindTexture(font_.texture);
            updateGLBuffers(glBuffers_, rects, textCount);
            renderGLBuffers(glBuffers_, textCount);
        }

        // render random text
        camera.pos = vec2(0.f);
        camera.size = frame_.fbSize;
        uniform2f(program, "cameraPos", camera.pos);
        uniform2f(program, "cameraSize", camera.size);

        Text text;
        text.color = { 1.f, 1.f, 0.f, 1.f };
        text.pos = vec2(200.f, 600.f);
        text.str = "T E T R I S  3D\nHELL YEA!";

        textCount = writeTextToBuffer(text, font_, rects, getSize(rects));
        uniform1i(program, "mode", FragmentMode::Font);
        bindTexture(font_.texture);
        updateGLBuffers(glBuffers_, rects, textCount);
        renderGLBuffers(glBuffers_, textCount);

    ImGui::Begin("main");
    ImGui::Spacing();
        camera_.imgui();
    ImGui::End();
}
