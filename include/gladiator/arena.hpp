// =============================================================================
//  arena.hpp — арена: геометрия, разделение зон, ограничение движения
// =============================================================================
#pragma once

#include <vector>
#include "gladiator/types.hpp"

namespace glad {

// Зона арены для визуализации (песок, портовая стена, трибуны)
struct ArenaZone {
    const char* label;
    Vec2f center;
    float radius;
    unsigned char r, g, b, a; // цвет зоны
};

class Arena {
public:
    explicit Arena(float radius = kArenaRadius);

    float radius() const { return radius_; }
    Vec2f center() const { return center_; }

    // Запрет выхода за пределы арены: возвращает скорректированную позицию
    Vec2f clampToArena(Vec2f p) const;

    // Разрешение перекрытия двух тел (простое расталкивание кругов)
    void separateBodies(Vec2f& a, Vec2f& b, float minDist) const;

    // Декоративные зоны для отрисовки
    const std::vector<ArenaZone>& zones() const { return zones_; }

private:
    float center_x_ = 0.0f, center_y_ = 0.0f;
    Vec2f center_;
    float radius_;
    std::vector<ArenaZone> zones_;
};

} // namespace glad
