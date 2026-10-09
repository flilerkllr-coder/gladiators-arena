// =============================================================================
//  types.hpp — базовые типы, перечисления и константы симуляции гладиаторов
//  Курсовая работа: «Симуляция гладиаторских боёв на C++ с визуализацией Raylib»
// =============================================================================
#pragma once

#include <string>
#include <cmath>
#include <cstdint>

namespace glad {

// -----------------------------------------------------------------------------
// Математика: простая 2D-структура вектора (симуляция ведётся на плоскости арены)
// -----------------------------------------------------------------------------
struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;

    Vec2f() = default;
    Vec2f(float xx, float yy) : x(xx), y(yy) {}

    Vec2f operator+(const Vec2f& o) const { return {x + o.x, y + o.y}; }
    Vec2f operator-(const Vec2f& o) const { return {x - o.x, y - o.y}; }
    Vec2f operator*(float s)        const { return {x * s,   y * s};   }

    float lengthSq() const { return x * x + y * y; }
    float length()   const { return std::sqrt(lengthSq()); }

    Vec2f normalized() const {
        float len = length();
        if (len < 1e-6f) return {0.0f, 0.0f};
        return {x / len, y / len};
    }

    static float dot(const Vec2f& a, const Vec2f& b) { return a.x * b.x + a.y * b.y; }
    static float dist(const Vec2f& a, const Vec2f& b) { return (a - b).length(); }
};

// -----------------------------------------------------------------------------
// Классы гладиаторов (исторические прототипы)
// -----------------------------------------------------------------------------
enum class GladiatorClass {
    Murmillo,     // тяжёлый шлем, короткий меч, большой щит (scutum)
    Thraex,       // изогнутый меч sica, малый щит, удары в голову
    Hoplomachus,  // копьё + кинжал, круглый щит
    Retiarius,    // сеть и трезубец, без доспехов, дальний бой
    Secutor,      // «преследователь» ретиариев, закрытый шлем
    Provocator    // средневес, баланс атаки и защиты
};

inline const char* className(GladiatorClass c) {
    switch (c) {
        case GladiatorClass::Murmillo:     return "Мурмиллон";
        case GladiatorClass::Thraex:       return "Фракиец";
        case GladiatorClass::Hoplomachus:  return "Гопломах";
        case GladiatorClass::Retiarius:    return "Ретиарий";
        case GladiatorClass::Secutor:      return "Сектор";
        case GladiatorClass::Provocator:   return "Провокатор";
    }
    return "?";
}

inline const char* classNameLatin(GladiatorClass c) {
    switch (c) {
        case GladiatorClass::Murmillo:     return "Murmillo";
        case GladiatorClass::Thraex:       return "Thraex";
        case GladiatorClass::Hoplomachus:  return "Hoplomachus";
        case GladiatorClass::Retiarius:    return "Retiarius";
        case GladiatorClass::Secutor:      return "Secutor";
        case GladiatorClass::Provocator:   return "Provocator";
    }
    return "?";
}

// -----------------------------------------------------------------------------
// Тактики ИИ — три стиля ведения боя (см. раздел 3 пояснительной записки)
// -----------------------------------------------------------------------------
enum class Tactic {
    Aggressive,   // сокращает дистанцию, часто атакует
    Defensive,    // держит дистанцию, блокирует, контратакует
    Balanced      // компромиссный вариант
};

inline const char* tacticName(Tactic t) {
    switch (t) {
        case Tactic::Aggressive: return "Агрессивная";
        case Tactic::Defensive:  return "Защитная";
        case Tactic::Balanced:   return "Сбалансированная";
    }
    return "?";
}

// -----------------------------------------------------------------------------
// Конечный автомат поведения гладиатора (FSM)
// -----------------------------------------------------------------------------
enum class AIState {
    Idle,       // ожидание / начало боя
    Approach,   // сближение с противником
    Attack,     // нанесение удара (анимация: замах -> удар -> восстановление)
    Defend,     // блок / уклонение
    Retreat,    // отход для восстановления выносливости
    Stunned,    // оглушение после тяжёлого удара или неудачного блока
    Dead        // гладиатор выбыл
};

inline const char* aiStateName(AIState s) {
    switch (s) {
        case AIState::Idle:     return "Ожидание";
        case AIState::Approach: return "Сближение";
        case AIState::Attack:   return "Атака";
        case AIState::Defend:   return "Защита";
        case AIState::Retreat:  return "Отход";
        case AIState::Stunned:  return "Оглушён";
        case AIState::Dead:     return "Выбыл";
    }
    return "?";
}

// -----------------------------------------------------------------------------
// Фаза анимации удара
// -----------------------------------------------------------------------------
enum class AttackPhase {
    None,
    Windup,   // замах (окно для реакции противника)
    Strike,   // активная фаза — проверяется попадание
    Recovery  // восстановление (гладиатор уязвим)
};

// -----------------------------------------------------------------------------
// Исход поединка
// -----------------------------------------------------------------------------
enum class MatchOutcome {
    None,
    Death,        // гибель одного из бойцов
    Surrender,    // сдача (низкая воля к бою)
    Draw          // обе стороны выбыли одновременно
};

// -----------------------------------------------------------------------------
// Тип наносимого урона — важен для механики блоков
// -----------------------------------------------------------------------------
enum class DamageType {
    Slash,  // рубящий (меч)
    Thrust, // колющий (копьё, кинжал)
    Net     // захват сетью (специальный тип ретиария)
};

// -----------------------------------------------------------------------------
// Глобальные константы симуляции
// -----------------------------------------------------------------------------
constexpr float kArenaRadius   = 300.0f;         // радиус арены в «симуляционных» единицах
constexpr float kFixedDt       = 1.0f / 120.0f;  // шаг физики, с (120 Гц)
constexpr float kFighterRadius = 14.0f;          // радиус тела гладиатора (коллизии)

} // namespace glad
