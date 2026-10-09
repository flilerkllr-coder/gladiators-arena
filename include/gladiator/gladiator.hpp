// =============================================================================
//  gladiator.hpp — сущность «гладиатор»: состояние, характеристики, события
// =============================================================================
#pragma once

#include <string>
#include "gladiator/types.hpp"
#include "gladiator/config.hpp"

namespace glad {

// -----------------------------------------------------------------------------
// Одно событие журнала боя (для статистики и вывода в интерфейсе)
// -----------------------------------------------------------------------------
struct LogEvent {
    float time = 0.0f;          // время боя, с
    int   actorId = -1;         // кто является инициатором
    std::string text;           // человекочитаемое описание на русском языке
};

// -----------------------------------------------------------------------------
// Гладиатор
// -----------------------------------------------------------------------------
class Gladiator {
public:
    Gladiator() = default;
    Gladiator(int id, const std::string& name, GladiatorClass cls, Tactic tactic, Vec2f pos);

    // --- доступ к параметрам -------------------------------------------------
    const ClassParams& params() const { return *params_; }
    const TacticMods&  mods()   const { return mods_; }

    // --- базовые геттеры -----------------------------------------------------
    int                id()       const { return id_; }
    const std::string& name()     const { return name_; }
    GladiatorClass     glClass()  const { return class_; }
    Tactic             tactic()   const { return tactic_; }
    AIState            state()    const { return state_; }
    AttackPhase        phase()    const { return attackPhase_; }
    Vec2f              position() const { return position_; }
    Vec2f&             positionRef() { return position_; } // для физики расталкивания
    float              facing()   const { return facing_; }        // угол взгляда, рад
    float              hp()       const { return hp_; }
    float              maxHp()    const { return params_->maxHp; }
    float              stamina()  const { return stamina_; }
    float              maxStamina() const { return params_->maxStamina; }
    bool               alive()    const { return state_ != AIState::Dead && !surrendered_; }
    bool               surrendered() const { return surrendered_; }
    bool               blocking() const { return state_ == AIState::Defend; }
    bool               snared()   const { return snareTimer_ > 0.0f; }
    bool               stunned()  const { return state_ == AIState::Stunned; }
    float              netCooldown() const { return netCooldown_; }

    // --- управление извне (движком / ИИ) --------------------------------------
    void setState(AIState s)      { state_ = s; }
    void setPosition(Vec2f p)     { position_ = p; }
    void setFacing(float a)       { facing_ = a; }
    void startAttack(AttackPhase ph, float t) { attackPhase_ = ph; phaseTimer_ = t; }
    void tickPhase(float dt);                  // продвижение фазы удара по времени

    // Применение урона; возвращает фактически нанесённый урон.
    float applyDamage(float raw, DamageType type, bool blocked, bool dodged, bool crit);

    void applySnare(float seconds) { snareTimer_ = seconds; }
    void applyStun(float seconds)  { state_ = AIState::Stunned; stunTimer_ = seconds; }
    void markDead()                { state_ = AIState::Dead; hp_ = 0.0f; }
    void markSurrendered()         { surrendered_ = true; }

    void spendStamina(float v)     { stamina_ -= v; if (stamina_ < 0) stamina_ = 0; }
    void regenStamina(float dt)    { stamina_ += params_->staminaRegen * dt;
                                     if (stamina_ > params_->maxStamina) stamina_ = params_->maxStamina; }

    float stunTimer() const   { return stunTimer_; }
    float phaseTimer() const  { return phaseTimer_; }
    float snareTimer() const  { return snareTimer_; }
    void  decayTimers(float dt);

    void tickNetCooldown(float dt) { if (netCooldown_ > 0) netCooldown_ -= dt; }
    void fireNet()                 { netCooldown_ = kNetCooldown; }

    // --- агрегированная статистика за бой ------------------------------------
    int   attacksLanded   = 0;
    int   attacksThrown   = 0;
    int   blocksSuccess   = 0;
    int   dodgesSuccess   = 0;
    float damageDealt     = 0.0f;
    float damageTaken     = 0.0f;
    float timeAlive       = 0.0f;

private:
    int           id_ = 0;
    std::string   name_;
    GladiatorClass class_ = GladiatorClass::Murmillo;
    Tactic        tactic_ = Tactic::Balanced;

    const ClassParams* params_ = &paramsFor(GladiatorClass::Murmillo);
    TacticMods         mods_   = modsFor(Tactic::Balanced);

    Vec2f position_;
    float facing_ = 0.0f;

    float hp_       = 100.0f;
    float stamina_  = 100.0f;

    AIState      state_       = AIState::Idle;
    AttackPhase  attackPhase_ = AttackPhase::None;
    float        phaseTimer_  = 0.0f;
    float        stunTimer_   = 0.0f;
    float        snareTimer_  = 0.0f;
    float        netCooldown_ = 0.0f;
    bool         surrendered_ = false;
};

} // namespace glad
