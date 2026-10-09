// =============================================================================
//  combat_system.hpp — расчёт исхода удара (урон, блок, уклонение, крит)
//  Выделен в отдельный «чистый» модуль: его можно тестировать без визуализации
//  и использовать в headless-режиме массовых прогонов (раздел 5 ПЗ).
// =============================================================================
#pragma once

#include "gladiator/types.hpp"
#include "gladiator/rng.hpp"

namespace glad {

class Gladiator;

// Результат разрешения одного эпизода атаки
struct StrikeResult {
    bool  hit        = false;   // удар достиг цели (не полностью заблокирован/уклон)
    bool  blocked    = false;   // цель успешно блокировала
    bool  dodged     = false;   // цель увернулась
    bool  crit       = false;   // критический удар
    float damage     = 0.0f;    // итоговый нанесённый урон
};

class CombatSystem {
public:
    // Разрешает активную фазу удара a против b.
    static StrikeResult resolveStrike(Gladiator& a, Gladiator& b, Rng& rng);

    // Чистая функция расчёта урона — вынесена для тестирования и для формул в ПЗ.
    static float computeDamage(const Gladiator& attacker, const Gladiator& target,
                               bool blocked, bool crit, float variance, Rng& rng);

    // Проверка: находится ли цель в зоне поражения с учётом угла обзора атакующего
    static bool inRangeAndArc(const Gladiator& a, const Gladiator& b, float maxAngleRad);
};

} // namespace glad
