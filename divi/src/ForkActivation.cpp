// Copyright (c) 2020 The DIVI Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ForkActivation.h"

#include "chain.h"

#include <unordered_map>

namespace
{

/**
 * For forks that get activated at a certain block time, the associated
 * activation times.
 */
const std::unordered_map<Fork, int64_t> ACTIVATION_TIMES = {
  {Fork::TestByTimestamp, 1000000000},
};

} // anonymous namespace

ActivationState::ActivationState(const CBlockIndex* pi)
  : pindex(pi)
{}

bool ActivationState::IsActive(const Fork f) const
{
  const auto mit = ACTIVATION_TIMES.find(f);
  assert(mit != ACTIVATION_TIMES.end());
  return pindex->nTime >= mit->second;
}
