// =============================================================================
//  config.hpp — числовые параметры классов гладиаторов и модели боя
//  Все коэффициенты собраны в одном месте, чтобы их можно было приводить
//  в пояснительной записке как «таблицы параметров экспериментальной установки».
// =============================================================================
#pragma once

#include "gladiator/types.hpp"

namespace glad {

// -----------------------------------------------------------------------------
// Параметры конкретного класса гладиатора (снаряжение и боевая модель)
// -----------------------------------------------------------------------------
struct ClassParams {
    const char* weaponName      = "gladius"; // название основного оружия
    float maxHp                 = 100.0f;    // запас здоровья
    float armor                 = 10.0f;     // пассивная защита (вычитается из урона)
    float attackDamage          = 12.0f;     // базовый урон удара
    DamageType damageType       = DamageType::Slash;
    float attackRange           = 55.0f;     // дальность атаки (от центра до центра)
    float windupTime            = 0.35f;     // длительность замаха, с
    float strikeTime            = 0.10f;     // активная фаза, с
    float recoveryTime          = 0.45f;     // восстановление (уязвимость), с
    float attackStaminaCost     = 18.0f;     // стоимость атаки по выносливости
    float maxStamina            = 100.0f;    // запас выносливости
    float staminaRegen          = 9.0f;      // регенерация выносливости, ед/с
    float moveSpeed             = 70.0f;     // скорость передвижения, ед/с
    float blockChance           = 0.35f;     // вероятность успешного блока при защите
    float blockDamageReduction  = 0.80f;     // доля урона, гасимая блоком
    float dodgeChance           = 0.10f;     // вероятность полного уклонения
    float visionAggression      = 0.5f;      // склонность к атаке (0..1) — для ИИ
};

// Исторически вдохновлённые, но не воспроизводящие классы параметры.
inline const ClassParams& paramsFor(GladiatorClass c) {
    static const ClassParams kTable[] = {
        /* Murmillo    */ {"гладиус",   130, 16, 13, DamageType::Slash,  55, 0.40f, 0.10f, 0.55f, 20, 100,  8, 62, 0.45f, 0.85f, 0.05f, 0.45f},
        /* Thraex      */ {"сика",      110, 12, 15, DamageType::Slash,  52, 0.32f, 0.10f, 0.45f, 17, 100, 10, 76, 0.35f, 0.75f, 0.12f, 0.65f},
        /* Hoplomachus */ {"копьё",     115, 14, 14, DamageType::Thrust, 72, 0.45f, 0.10f, 0.50f, 18, 100,  9, 70, 0.40f, 0.80f, 0.08f, 0.50f},
        /* Retiarius   */ {"трезубец",   85,  2, 16, DamageType::Thrust, 88, 0.38f, 0.12f, 0.42f, 14, 120, 13, 92, 0.15f, 0.50f, 0.30f, 0.55f},
        /* Secutor     */ {"гладиус",   125, 15, 12, DamageType::Slash,  55, 0.36f, 0.10f, 0.50f, 19, 100,  8, 66, 0.42f, 0.85f, 0.06f, 0.55f},
        /* Provocator  */ {"гладиус",   120, 13, 12, DamageType::Slash,  56, 0.35f, 0.10f, 0.48f, 17, 100,  9, 72, 0.40f, 0.80f, 0.08f, 0.50f},
    };
    return kTable[static_cast<int>(c)];
}

// -----------------------------------------------------------------------------
// Специальная механика ретиария: бросок сети
// -----------------------------------------------------------------------------
constexpr float kNetRange        = 80.0f;   // дальность броска сети
constexpr float kNetCooldown     = 8.0f;    // кулдаун, с
constexpr float kNetSnareTime    = 1.6f;    // время обездвиживания при попадании, с
constexpr float kNetMissPenalty  = 1.2f;    // штраф (заминка) при промахе сетью, с

// -----------------------------------------------------------------------------
// Общая модель урона
// -----------------------------------------------------------------------------
// Урон = max(1, (база * случайный разброс * модификатор тактики) - броня - блок-редукция)
constexpr float kDamageVarianceMin = 0.75f; // нижняя граница множителя разброса
constexpr float kDamageVarianceMax = 1.25f; // верхняя граница
constexpr float kCritMultiplier    = 1.7f;  // критический удар (шанс = 5% + ловкость)
constexpr float kBackstabBonus     = 1.3f;  // удар во время восстановления противника

// Воля к бою: при HP ниже порога гладиатор может сдаться
constexpr float kSurrenderHpThreshold   = 0.18f; // доля HP
constexpr float kSurrenderBaseChance    = 0.02f; // базовая вероятность за такт проверки
constexpr float kSurrenderCheckInterval = 1.0f;  // период проверок, с

// -----------------------------------------------------------------------------
// Параметры ИИ-тактик (корректировки базовых значений класса)
// -----------------------------------------------------------------------------
struct TacticMods {
    float attackFrequencyScale;  // >1 — чаще атакует
    float preferredDistance;     // желаемая дистанция относительно attackRange
    float blockWillingness;      // готовность уходить в блок (0..1)
    float retreatStaminaLevel;   // порог выносливости для отхода
};

inline TacticMods modsFor(Tactic t) {
    switch (t) {
        case Tactic::Aggressive: return {1.35f, 0.75f, 0.25f, 0.15f};
        case Tactic::Defensive:  return {0.70f, 1.30f, 0.85f, 0.45f};
        case Tactic::Balanced:   return {1.00f, 1.00f, 0.55f, 0.30f};
    }
    return {1.0f, 1.0f, 0.5f, 0.3f};
}

} // namespace glad
