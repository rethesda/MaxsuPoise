#include "StaggerHandler.h"
#include "PoiseDamageCalculator.h"
#include "PoiseHealthHandler.h"
#include "PoiseRegenHandler.h"
#include "Utils.h"

namespace MaxsuPoise
{
	static inline void TryStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor)
	{
		using func_t = decltype(&TryStagger);
		REL::Relocation<func_t> func{ REL::RelocationID(36700, 37710) };
		func(a_target, a_staggerMult, a_aggressor);
	}

	void StaggerHandler::ProcessStagger(const RE::HitData* a_hitData)
	{
		auto target = a_hitData && a_hitData->target ? a_hitData->target.get().get() : nullptr;
		if (!target)
			return;

		auto aggressor = a_hitData->aggressor ? a_hitData->aggressor.get().get() : nullptr;

		auto currentPoiseHealth = PoiseHealthHandler::GetCurrentPoiseHealth(target);
		auto regenDelayTimer = PoiseRegenHandler::GetPoiseRegenDelayTimer(target);

		if (currentPoiseHealth <= 0.f && (target->IsStaggering() || regenDelayTimer))
			return;

		auto totalPoiseHealth = PoiseHealthHandler::GetTotalPoiseHealth(target);
		auto poiseDamage = PoiseDamageCalculator::GetPhysicalPoiseDamage(a_hitData);
		currentPoiseHealth -= poiseDamage;
		PoiseHealthHandler::SetCurrentPoiseHealth(target, currentPoiseHealth);

		if (currentPoiseHealth <= 0.f) {
			TryStagger(target, 1.0f, aggressor);
		} else {
			auto staggerLevel = GetStaggerLevel(poiseDamage / totalPoiseHealth);
			auto immuneLevel = ImmuneLevelCalculator::GetTotalImmuneLevel(target);
			if (staggerLevel && staggerLevel > immuneLevel) {
				TryStagger(target, 0.25f * (staggerLevel) + 0.01f, aggressor);
			}
		}

		//if (target->IsStaggering())
		PoiseRegenHandler::SetPoiseRegenDelayTimer(target, PoiseRegenHandler::GetMaxRegenDelayTime());
	}

	StaggerLevel StaggerHandler::GetStaggerLevel(const float& a_DamagePercent)
	{
		float damageTHLD_Arr[StaggerLevel::kLargest] = {
			GetGameSettingFloat("fMaxsuPoise_SmallStaggerTHLD", 0.15f),
			GetGameSettingFloat("fMaxsuPoise_MediumStaggerTHLD", 0.30f),
			GetGameSettingFloat("fMaxsuPoise_LargeStaggerTHLD", 0.5f),
			1.0f
		};

		for (std::uint32_t i = StaggerLevel::kNone; i < StaggerLevel::kLargest; i++) {
			if (a_DamagePercent < damageTHLD_Arr[i])
				return static_cast<StaggerLevel>(i);
		}

		return StaggerLevel::kLargest;
	}
}
