// =============================================================================
//  main.cpp — точка входа: два режима работы программы
//
//  1) GUI-режим (по умолчанию): интерактивный бой с визуализацией Raylib.
//     Управление: R — новый бой, Space — пауза, +/- — скорость, Esc — выход.
//
//  2) Headless-режим: ./gladiator_sim --simulate --runs 500 [--seed N]
//     Массовый прогон боёв без окна, таблица результатов + экспорт CSV.
//     Именно эти данные идут в раздел «Исследование модели» пояснительной записки.
// =============================================================================
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <chrono>

#include "gladiator/battle.hpp"
#include "gladiator/statistics.hpp"

using namespace glad;

// -----------------------------------------------------------------------------
// Случайная конфигурация боя (для режима «новый бой» по клавише R)
// -----------------------------------------------------------------------------
static MatchConfig randomMatch(uint64_t seed) {
    std::mt19937 g(static_cast<unsigned>(seed));
    auto pickClass = [&](void)->GladiatorClass {
        static const GladiatorClass all[] = {
            GladiatorClass::Murmillo, GladiatorClass::Thraex, GladiatorClass::Hoplomachus,
            GladiatorClass::Retiarius, GladiatorClass::Secutor, GladiatorClass::Provocator };
        return all[std::uniform_int_distribution<int>(0, 5)(g)];
    };
    auto pickTactic = [&](void)->Tactic {
        static const Tactic all[] = { Tactic::Aggressive, Tactic::Defensive, Tactic::Balanced };
        return all[std::uniform_int_distribution<int>(0, 2)(g)];
    };
    static const char* names[] = {"Спартак", "Криксп", "Приск", "Верес", "Фламма",
                                  "Коммод", "Тетис", "Карпилфор", "Рудий"};
    MatchConfig c;
    c.nameA  = names[std::uniform_int_distribution<int>(0, 8)(g)];
    c.nameB  = names[std::uniform_int_distribution<int>(0, 8)(g)];
    c.classA = pickClass();
    c.classB = pickClass();
    c.tacticA = pickTactic();
    c.tacticB = pickTactic();
    c.seed   = seed;
    return c;
}

// -----------------------------------------------------------------------------
// Headless-режим: серия экспериментов
// -----------------------------------------------------------------------------
static int runSimulationMode(int argc, char** argv) {
    int runs = 200;
    uint64_t baseSeed = 1000;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--runs" && i + 1 < argc) runs = std::atoi(argv[++i]);
        else if (a == "--seed" && i + 1 < argc) baseSeed = std::strtoull(argv[++i], nullptr, 10);
    }

    std::cout << "Режим статистического анализа: " << runs << " боёв на серию...\n";

    std::vector<SeriesSummary> summaries;
    std::vector<RunRecord> allRecords;

    // Эксперимент 1: класс против класса (одинаковая тактика — баланс)
    struct Pair { const char* label; GladiatorClass a; GladiatorClass b; };
    const Pair pairs[] = {
        {"Мурмиллон vs Фракиец",   GladiatorClass::Murmillo,    GladiatorClass::Thraex},
        {"Ретиарий vs Сектор",      GladiatorClass::Retiarius,   GladiatorClass::Secutor},
        {"Гопломах vs Провокатор",  GladiatorClass::Hoplomachus, GladiatorClass::Provocator},
        {"Ретиарий vs Мурмиллон",   GladiatorClass::Retiarius,   GladiatorClass::Murmillo},
    };
    for (const auto& p : pairs) {
        MatchConfig c;
        c.classA = p.a; c.classB = p.b;
        c.tacticA = Tactic::Balanced; c.tacticB = Tactic::Balanced;
        auto recs = StatisticsCollector::runBatch(c, runs, baseSeed);
        allRecords.insert(allRecords.end(), recs.begin(), recs.end());
        summaries.push_back(StatisticsCollector::summarize(recs, p.label));
    }

    // Эксперимент 2: влияние тактики (один класс, разные стили)
    struct Tac { const char* label; Tactic a; Tactic b; };
    const Tac tacs[] = {
        {"Агрессия vs Защита (мурмиллоны)", Tactic::Aggressive, Tactic::Defensive},
        {"Баланс vs Агрессия (фракийцы)",   Tactic::Balanced,   Tactic::Aggressive},
        {"Защита vs Баланс (секторы)",      Tactic::Defensive,  Tactic::Balanced},
    };
    for (const auto& t : tacs) {
        MatchConfig c;
        c.classA = (t.a == Tactic::Aggressive || t.b == Tactic::Aggressive)
                   ? GladiatorClass::Murmillo : GladiatorClass::Thraex;
        c.classB = c.classA;
        c.tacticA = t.a; c.tacticB = t.b;
        auto recs = StatisticsCollector::runBatch(c, runs, baseSeed + 7777);
        allRecords.insert(allRecords.end(), recs.begin(), recs.end());
        summaries.push_back(StatisticsCollector::summarize(recs, t.label));
    }

    StatisticsCollector::printReport(summaries);

    if (StatisticsCollector::exportCsv("results_raw.csv", allRecords))
        std::cout << "Сырые результаты сохранены: results_raw.csv\n";
    if (StatisticsCollector::exportSummaryCsv("results_summary.csv", summaries))
        std::cout << "Сводка сохранена: results_summary.csv\n";
    return 0;
}

// -----------------------------------------------------------------------------
#ifdef GLAD_HAS_RAYLIB
#include "gladiator/renderer.hpp"

static int runGuiMode() {
    const int W = 1280, H = 720;
    Renderer renderer(W, H, "GLADIATORI — симулятор гладиаторских боёв (курсовая работа)");

    uint64_t seedCounter =
        static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    MatchConfig cfg = randomMatch(seedCounter++);
    BattleEngine engine(cfg);

    float timeScale = 1.0f;
    bool paused = false;
    float accumulator = 0.0f;

    while (!renderer.shouldClose()) {
        float frameTime = GetFrameTime();
        if (frameTime > 0.1f) frameTime = 0.1f; // защита от «спирали смерти» при лагах

        // --- ввод -------------------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_R)) {
            cfg = randomMatch(seedCounter++);
            engine = BattleEngine(cfg);
            paused = false;
        }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) timeScale = std::min(timeScale + 0.5f, 4.0f);
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) timeScale = std::max(timeScale - 0.5f, 0.5f);
        if (IsKeyPressed(KEY_ESCAPE)) break;

        // --- фиксированный шаг симуляции ------------------------------------
        if (!paused && !engine.finished()) {
            accumulator += frameTime * timeScale;
            int guard = 0;
            while (accumulator >= kFixedDt && guard++ < 600) {
                engine.step();
                accumulator -= kFixedDt;
            }
        }

        renderer.drawFrame(engine, timeScale, paused);
    }
    return 0;
}
#endif // GLAD_HAS_RAYLIB

// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--simulate") {
        return runSimulationMode(argc, argv);
    }
#ifdef GLAD_HAS_RAYLIB
    return runGuiMode();
#else
    std::cerr << "Программа собрана без Raylib. Используйте режим: --simulate --runs N\n";
    return runSimulationMode(argc, argv);
#endif
}
