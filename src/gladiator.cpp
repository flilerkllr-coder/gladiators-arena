// =============================================================================
//  gladiator.cpp — реализация сущности гладиатора
// =============================================================================
#include "gladiator/gladiator.hpp"

#include <algorithm>

namespace glad {

Gladiator::Gladiator(int id, const std::string& name, GladiatorClass cls,
                     Tactic tactic, Vec2f pos)
    : id_(id), name_(name), class_(cls), tactic_(tactic),
      params_(&paramsFor(cls)), mods_(modsFor(tactic)),
      position_(pos), hp_(params_->maxHp), stamina_(params_->maxStamina) {
}

void Gladiator::tickPhase(float dt) {
    if (attackPhase_ == AttackPhase::None) return;
    phaseTimer_ -= dt;
    if (phaseTimer_ > 0.0f) return;

    // Переход по фазам: Windup -> Strike -> Recovery -> None
    switch (attackPhase_) {
        case AttackPhase::Windup:
            attackPhase_ = AttackPhase::Strike;
            phaseTimer_  = params_->strikeTime;
            break;
        case AttackPhase::Strike:
            attackPhase_ = AttackPhase::Recovery;
            phaseTimer_  = params_->recoveryTime;
            break;
        case AttackPhase::Recovery:
            attackPhase_ = AttackPhase::None;
            phaseTimer_  = 0.0f;
            if (state_ == AIState::Attack) state_ = AIState::Idle;
            break;
        default:
            attackPhase_ = AttackPhase::None;
            break;
    }
}

float Gladiator::applyDamage(float raw, DamageType /*type*/, bool blocked, bool dodged, bool crit) {
    if (!alive()) return 0.0f;
    if (dodged) return 0.0f;

    float dmg = raw;
    // Пассивная броня не действует на блокированные удары щитом (они уже погашены),
    // но «добирает» незащищённые попадания.
    if (!blocked) dmg -= params_->armor * 0.5f;
    if (crit && !blocked) dmg *= kCritMultiplier;
    dmg = std::max(dmg, 1.0f);

    hp_ -= dmg;
    damageTaken += dmg;

    if (hp_ <= 0.0f) {
        hp_ = 0.0f;
        markDead();
    } else if (!blocked && crit) {
        // Критическое попадание оглушает на короткое время
        applyStun(0.6f);
    }
    return dmg;
}

void Gladiator::decayTimers(float dt) {
    if (snareTimer_ > 0.0f) snareTimer_ -= dt;
    if (stunTimer_  > 0.0f) {
        stunTimer_ -= dt;
        if (stunTimer_ <= 0.0f && state_ == AIState::Stunned) state_ = AIState::Idle;
    }
    if (alive()) timeAlive += dt;
}

} // namespace glad
