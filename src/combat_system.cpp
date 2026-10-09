// =============================================================================
//  combat_system.cpp — разрешение ударов: попадание, блок, уклонение, крит
// =============================================================================
#include "gladiator/combat_system.hpp"
#include "gladiator/gladiator.hpp"

#include <cmath>
#include <algorithm>

namespace glad {

bool CombatSystem::inRangeAndArc(const Gladiator& a, const Gladiator& b, float maxAngleRad) {
    float dist = Vec2f::dist(a.position(), b.position());
    if (dist > a.params().attackRange + kFighterRadius * 2.0f) return false;

    // Угол между направлением взгляда a и вектором на b
    Vec2f toB = (b.position() - a.position()).normalized();
    Vec2f facingDir(std::cos(a.facing()), std::sin(a.facing()));
    float dotv = std::clamp(Vec2f::dot(facingDir, toB), -1.0f, 1.0f);
    return std::acos(dotv) <= maxAngleRad;
}

float CombatSystem::computeDamage(const Gladiator& attacker, const Gladiator& target,
                                  bool blocked, bool crit, float variance, Rng& /*rng*/) {
    float base = attacker.params().attackDamage * variance;

    // Тактический модификатор: агрессивный стиль бьёт чуть сильнее, защитный — слабее
    if (attacker.tactic() == Tactic::Aggressive) base *= 1.08f;
    if (attacker.tactic() == Tactic::Defensive)  base *= 0.92f;

    // Удар по противнику в фазе восстановления (открылся) — штрафной бонус
    if (target.phase() == AttackPhase::Recovery) base *= kBackstabBonus;

    // Оглушённая цель получает увеличенный урон
    if (target.stunned()) base *= 1.5f;

    float dmg = base - target.params().armor;
    if (blocked) dmg *= (1.0f - target.params().blockDamageReduction);
    if (crit && !blocked) dmg *= kCritMultiplier;

    return std::max(dmg, 1.0f);
}

StrikeResult CombatSystem::resolveStrike(Gladiator& a, Gladiator& b, Rng& rng) {
    StrikeResult r;
    a.attacksThrown++;

    // 1. Проверка зоны поражения (дистанция + сектор обзора ±60°)
    if (!inRangeAndArc(a, b, 1.05f)) {
        return r; // промах — удар «ушёл в воздух»
    }

    // 2. Полное уклонение (ретиарий уклоняется лучше всех)
    float dodgeChance = b.params().dodgeChance;
    if (b.blocking()) dodgeChance *= 0.3f; // в блоке уклоняться сложнее
    if (b.snared() || b.stunned()) dodgeChance = 0.0f;
    if (rng.chance(dodgeChance)) {
        r.dodged = true;
        b.dodgesSuccess++;
        return r;
    }

    // 3. Блок: возможен только если цель в состоянии Defend
    bool triedBlock = b.blocking();
    if (triedBlock) {
        float blockP = b.params().blockChance;
        // Щиты плохо держат колющие удары копья, но отлично — рубящие
        if (a.params().damageType == DamageType::Thrust) blockP *= 0.75f;
        // Сеть блокируется только уклонением
        if (a.params().damageType == DamageType::Net) blockP = 0.0f;
        if (rng.chance(blockP)) {
            r.blocked = true;
            b.blocksSuccess++;
        }
    }

    // 4. Критический удар: базовые 5% + надбавка за агрессию
    float critP = 0.05f + 0.10f * a.params().visionAggression;
    if (b.stunned() || b.snared()) critP += 0.15f;
    r.crit = rng.chance(critP);

    // 5. Расчёт урона
    float variance = rng.uniform(kDamageVarianceMin, kDamageVarianceMax);
    r.damage = computeDamage(a, b, r.blocked, r.crit, variance, rng);
    r.hit = true;
    a.attacksLanded++;
    a.damageDealt += r.damage;

    // applyDamage внутри добавит «добор» брони к незаблокированным попаданиям,
    // поэтому компенсируем его здесь, чтобы урон считался ровно один раз.
    float delivered = r.damage + (r.blocked ? 0.0f : b.params().armor * 0.5f);
    r.damage = b.applyDamage(delivered, a.params().damageType, r.blocked, false, false);

    // 6. Неудачная попытка блока — микрo-оглушение (щит не успел)
    if (triedBlock && !r.blocked && rng.chance(0.25f)) {
        b.applyStun(0.35f);
    }
    return r;
}

} // namespace glad
