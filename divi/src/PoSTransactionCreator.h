#ifndef COINSTAKE_CREATOR_H
#define COINSTAKE_CREATOR_H

#include <stdint.h>
#include <amount.h>
#include <set>
#include <utility>
#include <vector>
#include <map>
#include <memory>

class CWallet;
class CBlock;
class CMutableTransaction;
class CKeyStore;
class CWalletTx;
class CChainParams;
class SuperblockSubsidyContainer;
class BlockIncentivesPopulator;
class CChain;
class I_PoSStakeModifierService;
class ProofOfStakeGenerator;

class I_PoSTransactionCreator
{
public:
    virtual ~I_PoSTransactionCreator(){}
    virtual bool CreateProofOfStake(
        uint32_t blockBits,
        int64_t nSearchTime,
        int64_t& nLastCoinStakeSearchTime,
        CMutableTransaction& txCoinStake,
        unsigned int& nTxNewTime) = 0;
};
class PoSTransactionCreator: public I_PoSTransactionCreator
{
private:
    const CChainParams& chainParameters_;
    CChain& activeChain_;
    std::shared_ptr<SuperblockSubsidyContainer> superblockSubsidies_;
    std::shared_ptr<BlockIncentivesPopulator> incentives_;
    std::shared_ptr<I_PoSStakeModifierService> stakeModifierService_;
    std::shared_ptr<ProofOfStakeGenerator> proofGenerator_;
    CWallet& wallet_;
    int64_t& coinstakeSearchInterval_;
    std::map<unsigned int, unsigned int>& hashedBlockTimestamps_;

    void CombineUtxos(
        const CAmount& allowedStakingAmount,
        CMutableTransaction& txNew,
        CAmount& nCredit,
        std::set<std::pair<const CWalletTx*, unsigned int> >& setStakeCoins,
        std::vector<const CWalletTx*>& walletTransactions);

    bool SetSuportedStakingScript(
        const std::pair<const CWalletTx*, unsigned int>& transactionAndIndexPair,
        CMutableTransaction& txNew);

    bool SelectCoins(
        CAmount allowedStakingBalance,
        int& nLastStakeSetUpdate,
        std::set<std::pair<const CWalletTx*, unsigned int> >& setStakeCoins);

    bool PopulateCoinstakeTransaction(
        const CKeyStore& keystore,
        unsigned int nBits,
        int64_t nSearchInterval,
        CMutableTransaction& txNew,
        unsigned int& nTxNewTime);
    bool FindHashproof(
        unsigned int nBits,
        unsigned int& nTxNewTime,
        std::pair<const CWalletTx*, unsigned int>& stakeData,
        CMutableTransaction& txNew);
public:
    PoSTransactionCreator(
        const CChainParams& chainParameters,
        CChain& activeChain,
        CWallet& wallet,
        int64_t& coinstakeSearchInterval,
        std::map<unsigned int, unsigned int>& hashedBlockTimestamps);
    virtual bool CreateProofOfStake(
        uint32_t blockBits,
        int64_t nSearchTime,
        int64_t& nLastCoinStakeSearchTime,
        CMutableTransaction& txCoinStake,
        unsigned int& nTxNewTime);
};
#endif // COINSTAKE_CREATOR_H