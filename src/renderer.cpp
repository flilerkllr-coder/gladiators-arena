// =============================================================================
//  renderer.cpp — отрисовка арены, бойцов, эффектов и интерфейса (Raylib)
//
//  Вид сверху: арена — круг из песка, бойцы — стилизованные фигуры с щитами
//  и оружием. HUD показывает полосы HP/выносливости, журнал событий и подсказки.
// =============================================================================
#include "gladiator/renderer.hpp"

#define RAYLIB_HEADER_IMPLEMENTATION_NOT_NEEDED
#include "raylib.h"

#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

namespace glad {

// Палитра классов гладиаторов (исторические цвета команд — «группы»)
static Color classColor(GladiatorClass c) {
    switch (c) {
        case GladiatorClass::Murmillo:    return { 70, 130, 220, 255 }; // синяя группа
        case GladiatorClass::Thraex:      return { 220,  80,  60, 255 }; // красная группа
        case GladiatorClass::Hoplomachus: return { 240, 200,  60, 255 }; // жёлтая
        case GladiatorClass::Retiarius:   return {  60, 200, 120, 255 }; // зелёная
        case GladiatorClass::Secutor:     return { 180, 100, 220, 255 }; // пурпурная
        case GladiatorClass::Provocator:  return { 240, 140,  60, 255 }; // оранжевая
    }
    return WHITE;
}

static std::string fmt(const std::string& s) { return s; } // hook для будущ. кодировок

// -----------------------------------------------------------------------------
Renderer::Renderer(int width, int height, const std::string& title)
    : width_(width), height_(height) {
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
    // Масштаб: чтобы арена (радиус 300) с трибунами помещалась в левую часть окна
    scale_ = std::min((width - 340.0f) / (2.0f * (kArenaRadius + 130.0f)),
                      (height - 100.0f) / (2.0f * (kArenaRadius + 130.0f)));
    screenCenter_ = { (width - 330.0f) * 0.5f, height * 0.5f };
}

Renderer::~Renderer() { CloseWindow(); }
bool Renderer::shouldClose() const { return WindowShouldClose(); }

Vec2f Renderer::toScreen(Vec2f w) const {
    return { screenCenter_.x + w.x * scale_, screenCenter_.y + w.y * scale_ };
}

void Renderer::drawArena(const Arena& arena) {
    ClearBackground({ 35, 30, 28, 255 }); // фон за стенами арены

    Vec2f c = toScreen(arena.center());
    float rPix = arena.radius() * scale_;

    // Трибуны — концентрические «ступени»
    for (int i = 5; i >= 1; --i) {
        DrawCircle((int)c.x, (int)c.y, (int)(rPix + i * 22.0f),
                   { (unsigned char)(60 + i * 8), (unsigned char)(50 + i * 6), (unsigned char)(48 + i * 6), 255 });
    }
    // Стена арены
    DrawCircle((int)c.x, (int)c.y, (int)(rPix + 10.0f), { 90, 78, 60, 255 });
    // Песок
    DrawCircle((int)c.x, (int)c.y, (int)rPix, { 200, 170, 115, 255 });

    // Декоративные зоны-пятна
    for (const auto& z : arena.zones()) {
        Vec2f p = toScreen(z.center);
        DrawCircle((int)p.x, (int)p.y, (int)(z.radius * scale_),
                   { z.r, z.g, z.b, z.a });
    }

    // Lua (ворота гладиаторов) — тёмный проём на западе
    DrawRectangle((int)(c.x - rPix - 26), (int)(c.y - 30), 30, 60, { 40, 32, 26, 255 });
    DrawText("LUA", (int)(c.x - rPix - 22), (int)(c.y - 8), 14, { 200, 190, 170, 255 });
}

void Renderer::drawHpBar(const Gladiator& g, float x, float y, float w) {
    float hpRatio = std::clamp(g.hp() / g.maxHp(), 0.0f, 1.0f);
    float stRatio = std::clamp(g.stamina() / g.maxStamina(), 0.0f, 1.0f);

    DrawRectangle((int)x, (int)y, (int)w, 8, { 30, 30, 30, 220 });
    Color hpCol = hpRatio > 0.5f ? GREEN : (hpRatio > 0.25f ? ORANGE : RED);
    DrawRectangle((int)x + 1, (int)y + 1, (int)((w - 2) * hpRatio), 6, hpCol);

    DrawRectangle((int)x, (int)(y + 10), (int)w, 5, { 30, 30, 30, 220 });
    DrawRectangle((int)x + 1, (int)(y + 11), (int)((w - 2) * stRatio), 3, SKYBLUE);
}

void Renderer::drawWeapon(const Gladiator& g) {
    Vec2f p = toScreen(g.position());
    float a = g.facing();
    Color col = classColor(g.glClass());
    Color steel = { 220, 220, 230, 255 };

    float reach = g.params().attackRange * scale_ * 0.55f;
    // Анимация замаха/удара: меняем угол оружия по фазам удара
    float swing = 0.0f;
    if (g.phase() == AttackPhase::Windup)   swing = -0.9f;
    if (g.phase() == AttackPhase::Strike)   swing = 0.55f;
    if (g.phase() == AttackPhase::Recovery) swing = 0.15f;

    float wa = a + swing;
    Vec2f tip(p.x + std::cos(wa) * reach, p.y + std::sin(wa) * reach);
    Vec2f h  (p.x + std::cos(a) * 8.0f,    p.y + std::sin(a) * 8.0f);

    switch (g.glClass()) {
        case GladiatorClass::Retiarius: {
            // Трезубец: три коротких зубца
            for (float da = -0.25f; da <= 0.25f; da += 0.25f) {
                Vec2f t(tip.x + std::cos(wa + da) * 10.0f, tip.y + std::sin(wa + da) * 10.0f);
                DrawLine((int)h.x, (int)h.y, (int)t.x, (int)t.y, steel);
            }
            break;
        }
        case GladiatorClass::Hoplomachus:
            DrawLine((int)h.x, (int)h.y, (int)tip.x, (int)tip.y, steel); // копьё
            break;
        case GladiatorClass::Thraex: {
            // Сика — изогнутый меч: две линии под углом
            Vec2f mid((h.x + tip.x) / 2 + std::cos(wa + 1.2f) * 8.0f,
                      (h.y + tip.y) / 2 + std::sin(wa + 1.2f) * 8.0f);
            DrawLine((int)h.x, (int)h.y, (int)mid.x, (int)mid.y, steel);
            DrawLine((int)mid.x, (int)mid.y, (int)tip.x, (int)tip.y, steel);
            break;
        }
        default:
            DrawLine((int)h.x, (int)h.y, (int)tip.x, (int)tip.y, steel); // гладиус
            break;
    }

    // Щит — круг со стороны противника (кроме ретиария)
    if (g.glClass() != GladiatorClass::Retiarius) {
        float shieldOff = g.blocking() ? 12.0f : 9.0f;
        Vec2f sp(p.x + std::cos(a) * shieldOff, p.y + std::sin(a) * shieldOff);
        Color shieldCol = g.blocking() ? Fade(col, 1.0f) : Fade(col, 0.75f);
        DrawCircle((int)sp.x, (int)sp.y, g.blocking() ? 11.0f : 8.0f, shieldCol);
    }
}

void Renderer::drawGladiator(const Gladiator& g, const Gladiator& /*foe*/) {
    Vec2f p = toScreen(g.position());
    Color col = classColor(g.glClass());

    if (!g.alive()) {
        // Павший боец — тёмное пятно и «тело»
        DrawCircle((int)p.x, (int)p.y, 12, { 60, 20, 20, 200 });
        DrawText("R.I.P.", (int)p.x - 18, (int)p.y - 6, 12, { 200, 200, 200, 255 });
        return;
    }

    drawWeapon(g);

    // Тело
    DrawCircle((int)p.x, (int)p.y, kFighterRadius * scale_, col);
    DrawCircleLines((int)p.x, (int)p.y, kFighterRadius * scale_, Fade(col, 0.6f));

    // Шлем (точка-«гребень» по направлению взгляда)
    Vec2f nose(p.x + std::cos(g.facing()) * kFighterRadius * scale_,
               p.y + std::sin(g.facing()) * kFighterRadius * scale_);
    DrawCircle((int)nose.x, (int)nose.y, 4, { 240, 240, 240, 255 });

    // Индикаторы состояний
    if (g.blocking()) DrawCircleLines((int)p.x, (int)p.y, kFighterRadius * scale_ + 5, WHITE);
    if (g.stunned())  DrawText("*ЗВЁЗДЫ*", (int)p.x - 24, (int)p.y - 34, 12, YELLOW);
    if (g.snared())   DrawText("[СЕТЬ]",  (int)p.x - 22, (int)p.y - 34, 12, { 200, 200, 255, 255 });

    // Имя и класс
    std::string label = fmt(g.name()) + " — " + className(g.glClass());
    DrawText(label.c_str(), (int)p.x - 60, (int)p.y + 22, 12, { 250, 245, 235, 255 });
}

void Renderer::drawParticles(const std::vector<Particle>& ps) {
    for (const auto& pt : ps) {
        Vec2f p = toScreen(pt.pos);
        float alphaF = std::clamp(pt.life / pt.maxLife, 0.0f, 1.0f);
        Color c{ pt.r, pt.g, pt.b, (unsigned char)(alphaF * 255) };
        DrawCircle((int)p.x, (int)p.y, pt.size, c);
    }
}

void Renderer::drawHud(const BattleEngine& engine, float timeScale, bool paused) {
    const auto& f = engine.fighters();
    // Верхняя панель
    DrawRectangle(0, 0, width_, 58, { 20, 18, 16, 230 });
    drawHpBar(f[0], 16, 10, 260);
    drawHpBar(f[1], width_ - 276, 10, 260);

    std::ostringstream t;
    t << "t = " << engine.time() << " c";
    DrawText(t.str().c_str(), width_ / 2 - 40, 8, 18, WHITE);
    std::string ts = "скорость: x" + std::to_string((int)timeScale) + (paused ? "  [ПАУЗА]" : "");
    DrawText(ts.c_str(), width_ / 2 - 80, 30, 14, { 200, 200, 200, 255 });

    std::string names = fmt(f[0].name()) + " vs " + fmt(f[1].name());
    DrawText(names.c_str(), width_ / 2 - (int)names.size() * 4, 42, 12, GOLD);

    // Нижняя строка подсказок
    DrawRectangle(0, height_ - 26, width_, 26, { 20, 18, 16, 230 });
    DrawText("R — новый бой | Space — пауза | +/- — скорость | Esc — выход",
             12, height_ - 20, 12, { 210, 205, 195, 255 });

    if (engine.finished()) {
        std::string msg;
        const auto& r = engine.result();
        if (r.winnerId == -1) msg = "НИЧЬЯ!";
        else {
            const Gladiator& w = (r.winnerId == f[0].id()) ? f[0] : f[1];
            msg = w.name() + " ПОБЕДИЛ";
            if (r.outcome == MatchOutcome::Surrender) msg += " (противник сдался)";
        }
        DrawRectangle(width_ / 2 - 220, height_ / 2 - 40, 440, 70, { 10, 8, 6, 220 });
        DrawText(msg.c_str(), width_ / 2 - (int)msg.size() * 6, height_ / 2 - 14, 24, GOLD);
    }
}

void Renderer::drawLogPanel(const BattleEngine& engine) {
    // Правая панель журнала боя: последние ~18 событий
    const auto& log = engine.log();
    int panelW = 320;
    DrawRectangle(width_ - panelW, 58, panelW, height_ - 84, { 15, 13, 11, 200 });
    DrawText("ЖУРНАЛ БОЯ", width_ - panelW + 10, 64, 14, GOLD);

    int maxLines = (height_ - 100) / 16;
    size_t start = log.size() > (size_t)maxLines ? log.size() - maxLines : 0;
    int y = 84;
    for (size_t i = start; i < log.size(); ++i, y += 16) {
        std::ostringstream line;
        line << std::fixed << std::setprecision(1) << "[" << log[i].time << "] " << log[i].text;
        std::string s = line.str();
        if ((int)s.size() > 48) s = s.substr(0, 48) + "...";
        DrawText(s.c_str(), width_ - panelW + 8, y, 10, { 230, 225, 215, 255 });
    }
}

void Renderer::drawFrame(const BattleEngine& engine, float timeScale, bool paused) {
    BeginDrawing();
    drawArena(engine.arena());
    drawParticles(engine.particles());
    const auto& f = engine.fighters();
    drawGladiator(f[0], f[1]);
    drawGladiator(f[1], f[0]);
    drawHud(engine, timeScale, paused);
    drawLogPanel(engine);
    EndDrawing();
}

} // namespace glad
