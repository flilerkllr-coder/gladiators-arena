// =============================================================================
//  ai_controller.cpp — поведение гладиатора: FSM + utility-выбор действий
//
//  Логика: на каждом такте контроллер оценивает «полезность» четырёх действий
//  (атака, защита, отход, сближение) и выбирает лучшее. FSM при этом запрещает
//  недопустимые переходы (например, нельзя атаковать в оглушении).
// =============================================================================
#include "gladiator/ai_controller.hpp"
#include "gladiator/gladiator.hpp"

#include <cmath>
#include <algorithm>

namespace glad {

// -----------------------------------------------------------------------------
// Utility-функции. Все аргументы нормированы к [0..1], чтобы их можно было
// суммировать со случайными весами без рассинхронизации масштабов.
// -----------------------------------------------------------------------------
float AiController::utilityAttack(const Gladiator& s, const Gladiator& o, float dist) const {
    // Пригодность дистанции: максимум при dist <= range, спад за её пределами
    float range = s.params().attackRange;
    float proximity = std::clamp(1.0f - std::max(0.0f, dist - range) / range, 0.0f, 1.0f);

    // Выносливость: без сил атаковать нельзя
    float staminaOk = s.stamina() >= s.params().attackStaminaCost ? 1.0f : 0.0f;

    // Чем меньше HP у противника — тем привлекательнее добивать
    float foeWeak = 1.0f - o.hp() / o.maxHp();

    // Собственная низкая выносливость снижает желание атаковать
    float fresh = s.stamina() / s.maxStamina();

    return s.mods().attackFrequencyScale * s.params().visionAggression * 0.5f
         + proximity * 0.35f + foeWeak * 0.25f + fresh * 0.15f;
}

float AiController::utilityDefend(const Gladiator& s, const Gladiator& o, float dist) const {
    // Защита полезна, когда противник близко и сам замахивается
    bool threat = (o.phase() == AttackPhase::Windup || o.phase() == AttackPhase::Strike)
               && dist <= o.params().attackRange + 20.0f;
    float threatU = threat ? 1.0f : 0.15f;

    // Низкое собственное HP => чаще прячемся за щит
    float hurt = 1.0f - s.hp() / s.maxHp();

    return s.mods().blockWillingness * (0.6f * threatU + 0.4f * hurt);
}

float AiController::utilityRetreat(const Gladiator& s, const Gladiator& /*o*/, float /*dist*/) const {
    // Отход нужен при падении выносливости ниже порога тактики
    float stamRatio = s.stamina() / s.maxStamina();
    float threshold = s.mods().retreatStaminaLevel;
    if (stamRatio >= threshold) return 0.05f;
    return 0.4f + (threshold - stamRatio) * 2.0f;
}

float AiController::utilityApproach(const Gladiator& s, const Gladiator& /*o*/, float dist) const {
    // Сближение ценно, если мы дальше желаемой дистанции
    float desired = s.params().attackRange * s.mods().preferredDistance;
    if (dist <= desired) return 0.1f;
    return std::clamp((dist - desired) / 150.0f, 0.0f, 1.0f) * 0.8f;
}

// -----------------------------------------------------------------------------
void AiController::update(Gladiator& self, const Gladiator& opponent, float dist, float dt, Rng& rng) {
    wantAttack_ = false;
    wantNet_    = false;
    wantBlock_  = false;
    desiredVel_ = Vec2f{0, 0};

    if (!self.alive() || !opponent.alive()) return;

    // Оглушение/обездвиженность сеть: никаких решений не принимаем
    if (self.stunned()) return;

    // Занят ударом: фаза windup/strike/recovery обрабатывается движком,
    // ИИ может только поворачиваться к противнику.
    if (self.phase() != AttackPhase::None) {
        self.setFacing(std::atan2(opponent.position().y - self.position().y,
                                  opponent.position().x - self.position().x));
        return;
    }

    // Модель задержки реакции: решения пересматриваются не каждый такт,
    // раз в reactionTime секунд (как у живого человека).
    reactionTimer_ -= dt;
    const bool canDecide = reactionTimer_ <= 0.0f;
    if (canDecide) reactionTimer_ = reactionTime_ + rng.uniform(-0.05f, 0.10f);

    // Постоянно держим курс на противника (гладиаторы «сканируют» соперника)
    float targetFacing = std::atan2(opponent.position().y - self.position().y,
                                    opponent.position().x - self.position().x);
    float dA = targetFacing - self.facing();
    while (dA >  M_PI) dA -= 2.0f * M_PI;
    while (dA < -M_PI) dA += 2.0f * M_PI;
    self.setFacing(self.facing() + std::clamp(dA, -6.0f * dt, 6.0f * dt));

    if (!canDecide) {
        // Между решениями сохраняем прошлое состояние движения, но не атакуем
        if (self.state() == AIState::Defend) wantBlock_ = true;
        return;
    }

    // --- выбор действия по полезности -----------------------------------------
    float uAtk  = utilityAttack(self, opponent, dist)  + rng.uniform(0.0f, 0.15f);
    float uDef  = utilityDefend(self, opponent, dist)  + rng.uniform(0.0f, 0.15f);
    float uRet  = utilityRetreat(self, opponent, dist) + rng.uniform(0.0f, 0.15f);
    float uAppr = utilityApproach(self, opponent, dist)+ rng.uniform(0.0f, 0.15f);

    float best = std::max(std::max(uAtk, uDef), std::max(uRet, uAppr));

    Vec2f toFoe = (opponent.position() - self.position()).normalized();

    if (best == uAtk && dist <= self.params().attackRange + kFighterRadius * 2.0f &&
        self.stamina() >= self.params().attackStaminaCost) {
        self.setState(AIState::Attack);
        wantAttack_ = true;
    } else if (best == uDef) {
        self.setState(AIState::Defend);
        wantBlock_ = true;
        // В защите делаем микрошаг назад
        desiredVel_ = toFoe * (-self.params().moveSpeed * 0.25f);
    } else if (best == uRet) {
        self.setState(AIState::Retreat);
        desiredVel_ = toFoe * (-self.params().moveSpeed);
    } else {
        self.setState(AIState::Approach);
        // Выход на желаемую дистанцию, а не «в упор»
        float desired = self.params().attackRange * self.mods().preferredDistance;
        float err = dist - desired;
        desiredVel_ = toFoe * std::clamp(err * 2.0f, -self.params().moveSpeed, self.params().moveSpeed);
    }

    // --- специальная способность ретиария: бросок сети --------------------------
    if (self.glClass() == GladiatorClass::Retiarius &&
        self.netCooldown() <= 0.0f &&
        dist < kNetRange && dist > self.params().attackRange * 0.9f &&
        !opponent.snared() && rng.chance(0.35f)) {
        wantNet_ = true;
    }

    // Обездвиженность сетью: движение невозможно
    if (self.snared()) desiredVel_ = Vec2f{0, 0};
}

// -----------------------------------------------------------------------------
// Реакция на начало замаха противника. Вызывается движком в момент, когда
// соперник переходит в фазу Windup. Шанс среагировать зависит от тактики и
// «боевого зрения» класса — это создаёт асимметрию между наблюдательными
// бойцами (сектор) и невнимательными (ретиарий без шлема).
// -----------------------------------------------------------------------------
void AiController::onEnemyWindup(Gladiator& self, const Gladiator& opponent, Rng& rng) {
    if (!self.alive() || !opponent.alive()) return;
    if (self.stunned() || self.snared()) return;
    if (self.phase() != AttackPhase::None) return; // сам атакует — не реагирует

    float dist = Vec2f::dist(self.position(), opponent.position());
    if (dist > opponent.params().attackRange + 30.0f) return; // удар не достанет

    float reactP = self.params().visionAggression * 0.5f + self.mods().blockWillingness * 0.6f;
    if (rng.chance(std::clamp(reactP, 0.05f, 0.95f))) {
        self.setState(AIState::Defend);
        wantBlock_ = true;
        reactionTimer_ = reactionTime_; // после реакции следующее решение — с задержкой
    }
}

} // namespace glad
