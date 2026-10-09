// =============================================================================
//  ai_controller.hpp — контроллер ИИ гладиатора (конечный автомат + utility-выбор)
//  Реализована гибридная модель: FSM задаёт «каркас» поведения, а переходы
//  выбираются по полезностной функции (utility-based AI).
// =============================================================================
#pragma once

#include "gladiator/types.hpp"
#include "gladiator/rng.hpp"

namespace glad {

class Gladiator;

class AiController {
public:
    explicit AiController(float reactionTime = 0.18f) : reactionTimer_(0.0f), reactionTime_(reactionTime) {}

    // Основной вызов: обновляет намерения гладиатора self против opponent.
    // Вызывается движком на каждом фиксированном такте.
    void update(Gladiator& self, const Gladiator& opponent, float dist, float dt, Rng& rng);

    // --- результат работы контроллера на текущем такте ------------------------
    Vec2f desiredVelocity() const { return desiredVel_; } // желаемое движение
    bool  wantsToAttack()   const { return wantAttack_; } // запрос на начало удара
    bool  wantsToThrowNet() const { return wantNet_; }    // запрос броска сети
    bool  wantsToBlock()    const { return wantBlock_; }  // удержание блока

private:
    // Utility-оценки возможных действий (см. формулы в разделе 3 ПЗ)
    float utilityAttack  (const Gladiator& s, const Gladiator& o, float dist) const;
    float utilityDefend  (const Gladiator& s, const Gladiator& o, float dist) const;
    float utilityRetreat (const Gladiator& s, const Gladiator& o, float dist) const;
    float utilityApproach(const Gladiator& s, const Gladiator& o, float dist) const;

    // Реакция на замах противника: шанс «почувствовать» атаку и уйти в блок.
    // Вызывается при переходе противника из Idle/Approach в фазу Windup.
    void onEnemyWindup(Gladiator& self, const Gladiator& opponent, Rng& rng);

    float reactionTimer_;
    float reactionTime_;   // имитация человеческой задержки реакции

    Vec2f desiredVel_ {0, 0};
    bool  wantAttack_ = false;
    bool  wantNet_    = false;
    bool  wantBlock_  = false;
};

} // namespace glad
