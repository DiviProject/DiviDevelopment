// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <stdint.h>
#include <map>
#include <uint256.h>

class CBlockHeader;
class CBlockIndex;
class uint256;
class arith_uint256;
class CChainParams;


class I_ProofOfStakeGenerator;
class CBlock;
class BlockMap;
class Settings;
class CChain;
class I_DifficultyAdjuster;

bool CheckWork(
    const CChainParams& chainParameters,
    const I_DifficultyAdjuster& difficultyAdjuster,
    const I_ProofOfStakeGenerator& posGenerator,
    const BlockMap& blockIndicesByHash,
    const Settings& settings,
    const CBlock& block,
    uint256& hashProofOfStake,
    CBlockIndex* const pindexPrev);
#endif // BITCOIN_POW_H
