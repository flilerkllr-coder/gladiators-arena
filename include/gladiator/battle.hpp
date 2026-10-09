// =============================================================================
//  battle.hpp — движок боя: связывает ИИ, боевую систему, физику и статистику
//  Работает в двух режимах: интерактивный (с визуализацией) и headless
//  (массовые прогоны для статистики без окна — режим --simulate).
// =============================================================================
#pragma once

#include <vector>
#include <string>
#include <functional>

#include "gladiator/types.hpp"
#include "gladiator/rng.hpp"
#include "gladiator/arena.hpp"
#include "gladiator/gladiator.hpp"
#include "gladiator/ai_controller.hpp"
#include "gladiator/combat_system.hpp"

namespace glad {

// Настройки одного поединка
struct MatchConfig {
    std::string nameA = "Спартак";
    std::string nameB = "Криксп";
    GladiatorClass classA = GladiatorClass::Murmillo;
    GladiatorClass classB = GladiatorClass::Thraex;
    Tactic tacticA = Tactic::Aggressive;
    Tactic tacticB = Tactic::Balanced;
    uint64_t seed  = 20260101ULL;
};

// Итог поединка для отчёта
struct MatchResult {
    int   winnerId = -1;         // id победителя, -1 при ничьей
    MatchOutcome outcome = MatchOutcome::None;
    float duration = 0.0f;       // длительность боя, с
    int   events = 0;            // количество записанных событий
    Gladiator statsA, statsB;    // копия бойцов со статистикой
};

// Частица эффекта (кровь / искры от оружия) — только для визуализации
struct Particle {
    Vec2f pos;
    Vec2f vel;
    float life;
    float maxLife;
    float size;
    unsigned char r, g, b;
};

class BattleEngine {
public:
    explicit BattleEngine(const MatchConfig& cfg);

    // --- основной цикл ---------------------------------------------------------
    // Один фиксированный такт симуляции (dt = kFixedDt).
    void step();
    // Прогнать бой до конца без визуализации (headless). Возвращает итог.
    MatchResult runToEnd(float maxSeconds = 300.0f);
    // Текущий итог (если бой уже завершён)
    bool finished() const { return finished_; }
    MatchResult result() const { return result_; }

    // --- доступ к состоянию для рендера ------------------------------------------
    const std::vector<Gladiator>& fighters() const { return fighters_; }
    const Arena& arena() const { return arena_; }
    const std::vector<Particle>& particles() const { return particles_; }
    const std::vector<LogEvent>& log() const { return log_; }
    float time() const { return time_; }
    uint64_t seed() const { return cfg_.seed; }
    const MatchConfig& config() const { return cfg_; }

private:
    void updateAI(Gladiator& self, Gladiator& other, Rng& rng);
    void integrateMotion(Gladiator& a, Gladiator& b, float dt);
    void processAttackIntent(Gladiator& a, Gladiator& b);
    void resolveActiveStrike(Gladiator& a, Gladiator& b);
    void throwNet(Gladiator& a, Gladiator& b);
    void checkEndConditions();
    void spawnBlood(Vec2f pos, float amount);
    void spawnSparks(Vec2f pos);
    void addLog(int actorId, const std::string& text);

    MatchConfig cfg_;
    Arena       arena_;
    Rng         rng_;
    AiController aiA_, aiB_;

    std::vector<Gladiator> fighters_;   // [0] — A, [1] — B
    std::vector<AiController*> ctrl_;   // соответствующие контроллеры

    std::vector<LogEvent> log_;
    std::vector<Particle> particles_;

    float time_ = 0.0f;
    float surrenderCheckTimer_ = 0.0f;
    bool  finished_ = false;
    MatchResult result_;
};

} // namespace glad
