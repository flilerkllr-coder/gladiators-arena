// =============================================================================
//  arena.cpp — реализация арены
// =============================================================================
#include "gladiator/arena.hpp"

#include <cmath>

namespace glad {

Arena::Arena(float radius) : center_(0.0f, 0.0f), radius_(radius) {
    // Декоративные «пятна» на песке — имитируют участки разной плотности песка
    // и следы предыдущих боёв. Чисто визуальный элемент.
    zones_ = {
        {"lua",            {-140.0f,  -90.0f}, 34.0f,  90, 70, 50, 255},   // воротная клетка
        {"haruspex spot",  { 160.0f,   80.0f}, 26.0f, 120, 95, 60, 120},
        {"sand patch A",   {  40.0f, -150.0f}, 60.0f, 210, 180, 120, 60},
        {"sand patch B",   { -60.0f,  140.0f}, 70.0f, 200, 170, 110, 60},
        {"blood stain",    {  10.0f,   20.0f}, 18.0f, 130, 40, 40, 40},
    };
}

Vec2f Arena::clampToArena(Vec2f p) const {
    Vec2f d = p - center_;
    float len = d.length();
    if (len <= radius_ - kFighterRadius) return p;
    // Проекция на окружность с запасом на радиус тела бойца
    return center_ + d.normalized() * (radius_ - kFighterRadius);
}

void Arena::separateBodies(Vec2f& a, Vec2f& b, float minDist) const {
    Vec2f diff = b - a;
    float dist = diff.length();
    if (dist < 1e-5f) {
        // Тела совпали — раздвигаем в случайном направлении нельзя (детерминизм),
        // поэтому сдвигаем по оси X.
        a.x -= minDist * 0.5f;
        b.x += minDist * 0.5f;
        return;
    }
    if (dist < minDist) {
        Vec2f push = diff.normalized() * ((minDist - dist) * 0.5f);
        a = a - push;
        b = b + push;
        a = clampToArena(a);
        b = clampToArena(b);
    }
}

} // namespace glad
