// Copyright (c) 2020 The DIVI Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ForkActivation.h"

#include "chain.h"
#include "primitives/block.h"
#include "timedata.h"

#include <cmath>
#include <unordered_map>

#include <Settings.h>
#include <set>
extern Settings& settings;

namespace
{
constexpr int64_t unixTimestampForDec31stMidnight = 1609459199;
const std::set<Fork> manualOverrides = {Fork::HardenedStakeModifier,Fork::UniformLotteryWinners};
/**
 * For forks that get activated at a certain block time, the associated
 * activation times.
 */
const std::unordered_map<Fork, int64_t,std::hash<int>> ACTIVATION_TIMES = {
  {Fork::TestByTimestamp, 1000000000},
  {Fork::HardenedStakeModifier, unixTimestampForDec31stMidnight},
  {Fork::UniformLotteryWinners, unixTimestampForDec31stMidnight},
  /* FIXME: Set real activation time for segwit light.  It is after
     staking vaults.  */
  {Fork::SegwitLight, 2000000000},
};

} // anonymous namespace

ActivationState::ActivationState(const CBlockIndex* pi)
  : nTime(pi == nullptr ? 0 : pi->nTime)
{}

ActivationState::ActivationState(const CBlockHeader& block)
  : nTime(block.nTime)
{}

bool ActivationState::IsActive(const Fork f) const
{
  constexpr char manualForkSettingLookup[] = "-manual_fork";
  if(settings.ParameterIsSet(manualForkSettingLookup) && manualOverrides.count(f)>0)
  {
    const int64_t timestampOverride = settings.GetArg(manualForkSettingLookup,0);
    return nTime >= timestampOverride;
  }
  const auto mit = ACTIVATION_TIMES.find(f);
  assert(mit != ACTIVATION_TIMES.end());
  return nTime >= mit->second;
}

bool ActivationState::CloseToSegwitLight(const int maxSeconds)
{
  const int64_t now = GetAdjustedTime();
  const int64_t activation = ACTIVATION_TIMES.at(Fork::SegwitLight);
  return std::abs(now - activation) <= maxSeconds;
}
