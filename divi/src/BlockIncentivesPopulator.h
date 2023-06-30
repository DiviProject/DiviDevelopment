#ifndef BLOCK_INCENTIVES_POPULATOR_H
#define BLOCK_INCENTIVES_POPULATOR_H
#include "I_BlockIncentivesPopulator.h"
#include <string>
#include <amount.h>

class CMutableTransaction;
class CBlockRewards;
class CBlockIndex;
class CChainParams;
class CChain;
class I_SuperblockHeightValidator;
class I_BlockSubsidyProvider;
class CTransaction;

class BlockIncentivesPopulator : public I_BlockIncentivesPopulator
{
private:
    const CChainParams& chainParameters_;
    const I_SuperblockHeightValidator& heightValidator_;
    const I_BlockSubsidyProvider& blockSubsidies_;
    const std::string treasuryPaymentAddress_;
    const std::string charityPaymentAddress_;

private:
    void FillTreasuryPayment(CMutableTransaction &tx, int nHeight) const;
    void FillLotteryPayment(CMutableTransaction &tx, const CBlockRewards &rewards, const CBlockIndex *currentBlockIndex) const;

public:
    BlockIncentivesPopulator(
        const CChainParams& chainParameters,
        const I_SuperblockHeightValidator& heightValidator,
        const I_BlockSubsidyProvider& blockSubsidies);

    void FillBlockPayee(CMutableTransaction& txNew, const CBlockRewards &payments, const CBlockIndex* chainTip) const override;
    bool IsBlockValueValid(const CBlockRewards &nExpectedValue, CAmount nMinted, int nHeight) const override;
    bool HasValidPayees(const CTransaction &txNew, const CBlockIndex* pindex) const override;
};

#endif // BLOCK_INCENTIVES_POPULATOR_H
