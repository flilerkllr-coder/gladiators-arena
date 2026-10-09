// =============================================================================
//  battle.cpp — движок поединка: такт симуляции, физика, события, итог боя
// =============================================================================
#include "gladiator/battle.hpp"

#include <cmath>
#include <sstream>
#include <algorithm>

namespace glad {

// -----------------------------------------------------------------------------
BattleEngine::BattleEngine(const MatchConfig& cfg)
    : cfg_(cfg), arena_(kArenaRadius), rng_(cfg.seed),
      aiA_(0.16f), aiB_(0.20f) {
    // Бойцы стартуют на противоположных краях арены (id 10 и 20 — как номера «пар» в лудусе)
    fighters_.emplace_back(10, cfg.nameA, cfg.classA, cfg.tacticA, Vec2f(-arena_.radius() * 0.55f, 0));
    fighters_.emplace_back(20, cfg.nameB, cfg.classB, cfg.tacticB, Vec2f( arena_.radius() * 0.55f, 0));
    ctrl_.push_back(&aiA_);
    ctrl_.push_back(&aiB_);

    std::ostringstream os;
    os << "Бой начался: " << cfg.nameA << " (" << className(cfg.classA) << ") против "
       << cfg.nameB << " (" << className(cfg.classB) << ")";
    addLog(0, os.str());
}

// -----------------------------------------------------------------------------
void BattleEngine::step() {
    if (finished_) return;
    const float dt = kFixedDt;
    time_ += dt;

    Gladiator& a = fighters_[0];
    Gladiator& b = fighters_[1];

    // 1. Таймеры состояний и фаз ударов
    a.decayTimers(dt);
    b.decayTimers(dt);
    a.tickPhase(dt);
    b.tickPhase(dt);
    a.tickNetCooldown(dt);
    b.tickNetCooldown(dt);

    // 1b. Детект начала замаха -> реакция противника (событие для ИИ-реакции)
    static thread_local AttackPhase prevA = AttackPhase::None;
    static thread_local AttackPhase prevB = AttackPhase::None;
    if (a.phase() == AttackPhase::Windup && prevA != AttackPhase::Windup)
        aiB_.onEnemyWindup(b, a, rng_);
    if (b.phase() == AttackPhase::Windup && prevB != AttackPhase::Windup)
        aiA_.onEnemyWindup(a, b, rng_);
    prevA = a.phase();
    prevB = b.phase();

    // 2. ИИ принимает решения
    float dist = Vec2f::dist(a.position(), b.position());
    aiA_.update(a, b, dist, dt, rng_);
    aiB_.update(b, a, dist, dt, rng_);

    // 3. Обработка намерений атаки / сети
    processAttackIntent(a, b);
    processAttackIntent(b, a);
    if (aiA_.wantsToThrowNet()) throwNet(a, b);
    if (aiB_.wantsToThrowNet()) throwNet(b, a);

    // 4. Активная фаза удара — разрешение боя
    if (a.phase() == AttackPhase::Strike) resolveActiveStrike(a, b);
    if (b.phase() == AttackPhase::Strike) resolveActiveStrike(b, a);

    // 5. Движение + столкновения + границы арены
    integrateMotion(a, b, dt);

    // 6. Восстановление выносливости (в атаке тратится, в простое восстанавливается)
    a.regenStamina(dt * (a.state() == AIState::Attack ? 0.2f : 1.0f));
    b.regenStamina(dt * (b.state() == AIState::Attack ? 0.2f : 1.0f));

    // 7. Проверка условий окончания боя (смерть / сдача)
    checkEndConditions();

    // 8. Частицы (для визуала)
    for (auto& p : particles_) {
        p.pos = p.pos + p.vel * dt;
        p.vel = p.vel * 0.92f; // затухание
        p.life -= dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                        [](const Particle& p){ return p.life <= 0.0f; }),
                     particles_.end());
}

// -----------------------------------------------------------------------------
MatchResult BattleEngine::runToEnd(float maxSeconds) {
    const int maxSteps = static_cast<int>(maxSeconds / kFixedDt);
    for (int i = 0; i < maxSteps && !finished_; ++i) step();
    return result_;
}

// -----------------------------------------------------------------------------
void BattleEngine::processAttackIntent(Gladiator& att, Gladiator& def) {
    AiController& c = (att.id() == fighters_[0].id()) ? aiA_ : aiB_;
    if (!c.wantsToAttack()) return;
    if (att.phase() != AttackPhase::None) return;   // уже бьёт
    if (att.stunned() || att.snared()) return;
    if (att.stamina() < att.params().attackStaminaCost) return;

    att.spendStamina(att.params().attackStaminaCost);
    att.setState(AIState::Attack);
    att.startAttack(AttackPhase::Windup, att.params().windupTime);
}

// -----------------------------------------------------------------------------
void BattleEngine::resolveActiveStrike(Gladiator& att, Gladiator& def) {
    // Разрешаем удар ровно один раз за фазу Strike: срабатываем на первом такте
    // активной фазы (phaseTimer ещё не успел «стечь» от strikeTime).
    if (att.phaseTimer() < att.params().strikeTime - kFixedDt * 1.5f) return;

    StrikeResult r = CombatSystem::resolveStrike(att, def, rng_);

    std::ostringstream os;
    if (r.dodged) {
        os << def.name() << " увернулся от удара [" << att.params().weaponName << "]";
        addLog(att.id(), os.str());
    } else if (r.blocked) {
        os << def.name() << " заблокировал удар (" << r.damage << " урона прошло)";
        addLog(def.id(), os.str());
        spawnSparks((att.position() + def.position()) * 0.5f);
    } else if (r.hit) {
        os << att.name() << " попал [" << att.params().weaponName << "]";
        if (r.crit) os << " КРИТ!";
        os << ", урон " << r.damage;
        addLog(att.id(), os.str());
        spawnBlood(def.position(), r.damage);
        // Отбрасывание цели лёгким импульсом не моделируем: позиция меняется только ИИ
    } else {
        os << att.name() << " промахнулся (удар вне зоны поражения)";
        addLog(att.id(), os.str());
    }
}

// -----------------------------------------------------------------------------
void BattleEngine::throwNet(Gladiator& att, Gladiator& def) {
    if (att.glClass() != GladiatorClass::Retiarius) return;
    att.fireNet();

    float dist = Vec2f::dist(att.position(), def.position());
    // Вероятность попадания сети: базовая 0.55, падает с дистанцией, выше против неподвижной цели
    float pHit = 0.55f * (1.0f - dist / (kNetRange * 1.4f));
    if (def.stunned()) pHit += 0.3f;
    pHit = std::clamp(pHit, 0.05f, 0.95f);

    std::ostringstream os;
    if (rng_.chance(pHit)) {
        def.applySnare(kNetSnareTime);
        os << att.name() << " метнул сеть — " << def.name() << " опутан на " << kNetSnareTime << " с";
        addLog(att.id(), os.str());
    } else {
        att.applyStun(kNetMissPenalty); // промах сетью — атакующий раскрывается
        os << att.name() << " промахнулся сетью";
        addLog(att.id(), os.str());
    }
}

// -----------------------------------------------------------------------------
void BattleEngine::integrateMotion(Gladiator& a, Gladiator& b, float dt) {
    auto apply = [&](Gladiator& g, AiController& c) {
        if (!g.alive()) return;
        Vec2f v = c.desiredVelocity();
        g.setPosition(arena_.clampToArena(g.position() + v * dt));
    };
    apply(a, aiA_);
    apply(b, aiB_);
    arena_.separateBodies(a.positionRef(), b.positionRef(), kFighterRadius * 2.0f);
}

// -----------------------------------------------------------------------------
void BattleEngine::checkEndConditions() {
    Gladiator& a = fighters_[0];
    Gladiator& b = fighters_[1];

    if (!a.alive() || !b.alive() || a.surrendered() || b.surrendered()) {
        finished_ = true;
        result_.duration = time_;
        result_.statsA = a;
        result_.statsB = b;
        result_.events = static_cast<int>(log_.size());

        bool deadA = (a.state() == AIState::Dead);
        bool deadB = (b.state() == AIState::Dead);

        if (deadA && deadB) {
            result_.outcome = MatchOutcome::Draw;
            result_.winnerId = -1;
            addLog(0, "Ничья: оба бойца выбыли");
        } else if (deadB || a.surrendered()) {
            result_.outcome = deadB ? MatchOutcome::Death : MatchOutcome::Surrender;
            result_.winnerId = a.id();
            addLog(0, a.name() + (deadB ? " убил противника и победил!" : " одержал победу — противник сдался"));
        } else if (deadA || b.surrendered()) {
            result_.outcome = deadA ? MatchOutcome::Death : MatchOutcome::Surrender;
            result_.winnerId = b.id();
            addLog(0, b.name() + (deadA ? " убил противника и победил!" : " одержал победу — противник сдался"));
        }
        return;
    }

    // Периодическая проверка воли к бою
    surrenderCheckTimer_ -= kFixedDt;
    if (surrenderCheckTimer_ <= 0.0f) {
        surrenderCheckTimer_ = kSurrenderCheckInterval;
        for (int i = 0; i < 2; ++i) {
            Gladiator& g = fighters_[i];
            Gladiator& foe = fighters_[1 - i];
            if (!g.alive()) continue;
            float hpRatio = g.hp() / g.maxHp();
            if (hpRatio > kSurrenderHpThreshold) continue;
            // Агрессивные сдают реже, защитные — чаще; загнанный в угол сильный враг — чаще
            float p = kSurrenderBaseChance;
            if (g.tactic() == Tactic::Defensive)  p *= 1.6f;
            if (g.tactic() == Tactic::Aggressive) p *= 0.5f;
            if (foe.hp() / foe.maxHp() < 0.3f)    p *= 0.4f; // враг тоже измотан — стоит драться
            if (rng_.chance(p)) g.markSurrendered();
        }
    }
}

// -----------------------------------------------------------------------------
void BattleEngine::spawnBlood(Vec2f pos, float amount) {
    int n = std::min(4 + static_cast<int>(amount / 3.0f), 14);
    for (int i = 0; i < n; ++i) {
        float ang = rng_.uniform(0.0f, 2.0f * M_PI);
        float spd = rng_.uniform(20.0f, 90.0f);
        particles_.push_back({pos, Vec2f(std::cos(ang), std::sin(ang)) * spd,
                              rng_.uniform(0.3f, 0.9f), 0.9f, rng_.uniform(1.5f, 3.5f),
                              160, 20, 20});
    }
}

void BattleEngine::spawnSparks(Vec2f pos) {
    for (int i = 0; i < 6; ++i) {
        float ang = rng_.uniform(0.0f, 2.0f * M_PI);
        float spd = rng_.uniform(40.0f, 120.0f);
        particles_.push_back({pos, Vec2f(std::cos(ang), std::sin(ang)) * spd,
                              rng_.uniform(0.15f, 0.35f), 0.35f, rng_.uniform(1.0f, 2.0f),
                              255, 220, 120});
    }
}

void BattleEngine::addLog(int actorId, const std::string& text) {
    if (static_cast<int>(log_.size()) >= 4096) log_.erase(log_.begin()); // кольцевой журнал
    log_.push_back({time_, actorId, text});
}

} // namespace glad
