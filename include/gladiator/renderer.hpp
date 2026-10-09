// =============================================================================
//  renderer.hpp — визуализация боя средствами Raylib
//  Модуль только читает состояние движка; никаких изменений симуляции здесь нет
//  (принцип разделения модели и представления).
// =============================================================================
#pragma once

#include <string>
#include "gladiator/battle.hpp"

namespace glad {

class Renderer {
public:
    Renderer(int width, int height, const std::string& title);
    ~Renderer();

    // Невозможно копировать — окно одно
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool shouldClose() const;

    // Отрисовать один кадр по текущему состоянию движка
    void drawFrame(const BattleEngine& engine, float timeScale, bool paused);

    // Конвертация «симуляционных» координат в экранные
    Vec2f toScreen(Vec2f world) const;

private:
    void drawArena(const Arena& arena);
    void drawGladiator(const Gladiator& g, const Gladiator& foe);
    void drawWeapon(const Gladiator& g);
    void drawHpBar(const Gladiator& g, float x, float y, float w);
    void drawParticles(const std::vector<Particle>& ps);
    void drawHud(const BattleEngine& engine, float timeScale, bool paused);
    void drawLogPanel(const BattleEngine& engine);

    int width_, height_;
    float scale_ = 1.0f;   // пикселей на единицу симуляции
    Vec2f screenCenter_ {0, 0};
};

} // namespace glad
