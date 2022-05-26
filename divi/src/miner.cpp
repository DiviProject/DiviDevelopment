// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include <ChainstateManager.h>
#include <CoinMintingModule.h>
#include <I_CoinMinter.h>
#include <Logging.h>
#include <ThreadManagementHelpers.h>
#include <chain.h>
#include <sync.h>

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

namespace
{

CCriticalSection cs_coinMintingModule;
std::unique_ptr<CoinMintingModule> coinMintingModule;

} // anonymous namespace

void InitializeCoinMintingModule(
    const Settings& settings,
    const CChainParams& chainParameters,
    const MasternodeModule& masternodeModule,
    const CSporkManager& sporkManager,
    const CFeeRate& minimumRelayFeeRate,
    const I_PeerBlockNotifyService& peerNotificationService,
    const I_BlockSubmitter& blockSubmitter,
    CCriticalSection& mainCS,
    CTxMemPool& mempool,
    I_StakingWallet* pwallet)
{
    LOCK(cs_coinMintingModule);
    assert(coinMintingModule == nullptr);
    coinMintingModule.reset(
        new CoinMintingModule(
            settings,
            chainParameters,
            masternodeModule,
            minimumRelayFeeRate,
            peerNotificationService,
            blockSubmitter,
            sporkManager,
            mainCS,
            mempool,
            *pwallet));
}

void DestructCoinMintingModule()
{
    LOCK(cs_coinMintingModule);
    assert(coinMintingModule != nullptr);
    coinMintingModule.reset();
}

const CoinMintingModule& GetCoinMintingModule()
{
    assert(coinMintingModule != nullptr);
    return *coinMintingModule;
}
//////////////////////////////////////////////////////////////////////////////
//
// DIVIMiner
//

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
void MintCoins(
    I_CoinMinter& minter)
{
    while (minter.mintingHasBeenRequested())
    {
        if (!minter.canMintCoins())
        {
            minter.sleep(5000);
        }
        else
        {
            minter.createNewBlock();
        }
    }

}
void MinterThread(I_CoinMinter& minter)
{
    LogPrintf("%s started\n",__func__);
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("divi-miner");

    // Each thread has its own key and counter
    try {
        MintCoins(minter);
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("%s -- terminated\n",__func__);
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("%s -- runtime error: %s\n", __func__, e.what());
        return;
    }
}

bool CheckHeightForRecentProofOfStakeGeneration(const int blockHeight)
{
    const auto& mapHashedBlocks = GetCoinMintingModule().GetBlockTimestampsByHeight();
    constexpr int64_t fiveMinutes = 5*60;
    const auto it = mapHashedBlocks.find(blockHeight);
    return it != mapHashedBlocks.end() && GetTime() - it->second < fiveMinutes;
}

bool HasRecentlyAttemptedToGenerateProofOfStake()
{
    const ChainstateManager::Reference chainstate;
    int currentChainHeight = chainstate->ActiveChain().Tip()->nHeight;
    for(int offset =0 ; offset < 4; offset++ )
    {
        if(currentChainHeight - offset < 0) break;
        if(CheckHeightForRecentProofOfStakeGeneration(currentChainHeight-offset)) return true;
    }
    return false;
}

// ppcoin: stake minter thread
void ThreadCoinMinter()
{
    boost::this_thread::interruption_point();
    LogPrintf("%s started\n",__func__);
    try {
        /* While the thread is running, we keep the mutex locked; this ensures
           that the module will not be reset or destructed while in use.  */
        LOCK(cs_coinMintingModule);
        const CoinMintingModule& mintingModule = GetCoinMintingModule();
        I_CoinMinter& minter = mintingModule.coinMinter();
        minter.setMintingRequestStatus(true);
        MinterThread(minter);
        boost::this_thread::interruption_point();
    } catch (std::exception& e) {
        LogPrintf("%s exception \n",__func__);
    } catch (...) {
        LogPrintf("%s error \n",__func__);
    }
    LogPrintf("%s exiting,\n",__func__);
}
#endif // ENABLE_WALLET
