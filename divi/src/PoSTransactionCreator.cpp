#include <PoSTransactionCreator.h>

#include <wallet.h>
#include <kernel.h>
#include <masternode-payments.h>
#include <script/sign.h>
#include <utilmoneystr.h>
#include <SuperblockHelpers.h>
#include <Settings.h>
#include <BlockIncentivesPopulator.h>

extern Settings& settings;
extern const int nHashDrift;
extern const int maximumFutureBlockDrift = 180; // seconds

PoSTransactionCreator::PoSTransactionCreator(
    CWallet& wallet,
    int64_t& coinstakeSearchInterval,
    std::map<unsigned int, unsigned int>& hashedBlockTimestamps
    ): wallet_(wallet)
    , coinstakeSearchInterval_(coinstakeSearchInterval)
    , hashedBlockTimestamps_(hashedBlockTimestamps)
{
}

bool PoSTransactionCreator::SelectCoins(
    CAmount allowedStakingBalance,
    int& nLastStakeSetUpdate,
    std::set<std::pair<const CWalletTx*, unsigned int> >& setStakeCoins)
{
    if (allowedStakingBalance <= 0)
        return false;

    if (GetTime() - nLastStakeSetUpdate > wallet_.nStakeSetUpdateTime) {
        setStakeCoins.clear();
        if (!wallet_.SelectStakeCoins(setStakeCoins, allowedStakingBalance)) {
            return error("failed to select coins for staking");
        }

        nLastStakeSetUpdate = GetTime();
    }

    if (setStakeCoins.empty()) {
        return error("no coins available for staking");
    }

    return true;
}

void MarkTransactionAsCoinstake(CMutableTransaction& txNew)
{
    txNew.vin.clear();
    txNew.vout.clear();
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));
}

bool PoSTransactionCreator::SetSuportedStakingScript(
    const std::pair<const CWalletTx*, unsigned int>& transactionAndIndexPair,
    CMutableTransaction& txNew)
{
    CScript scriptPubKeyOut = transactionAndIndexPair.first->vout[transactionAndIndexPair.second].scriptPubKey;
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(scriptPubKeyOut, whichType, vSolutions)) {
        LogPrintf("CreateCoinStake : failed to parse kernel\n");
        return false;
    }
    if (fDebug && GetBoolArg("-printcoinstake", false)) LogPrintf("CreateCoinStake : parsed kernel type=%d\n", whichType);
    if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH)
    {
        if (fDebug && GetBoolArg("-printcoinstake", false))
            LogPrintf("CreateCoinStake : no support for kernel type=%d\n", whichType);
        return false; // only support pay to public key and pay to address
    }

    txNew.vin.push_back(CTxIn(transactionAndIndexPair.first->GetHash(), transactionAndIndexPair.second));
    txNew.vout.push_back(CTxOut(0, scriptPubKeyOut));

    if (fDebug && GetBoolArg("-printcoinstake", false))
        LogPrintf("CreateCoinStake : added kernel type=%d\n", whichType);

    return true;
}

void PoSTransactionCreator::CombineUtxos(
    const CAmount& allowedStakingAmount,
    CMutableTransaction& txNew,
    CAmount& nCredit,
    std::set<std::pair<const CWalletTx*, unsigned int> >& setStakeCoins,
    std::vector<const CWalletTx*>& walletTransactions)
{
    const CAmount nCombineThreshold = (wallet_.nStakeSplitThreshold / 2) * COIN;
    using Entry = std::pair<const CWalletTx*, unsigned int>;
    std::vector<Entry> vCombineCandidates;
    for(auto &&pcoin : setStakeCoins)
    {
        if (pcoin.first->vout[pcoin.second].scriptPubKey == txNew.vout[1].scriptPubKey &&
                pcoin.first->GetHash() != txNew.vin[0].prevout.hash)
        {
            if (pcoin.first->vout[pcoin.second].nValue + nCredit > nCombineThreshold)
                continue;

            vCombineCandidates.push_back(pcoin);
        }
    }

    std::sort(std::begin(vCombineCandidates), std::end(vCombineCandidates), [](const Entry &left, const Entry &right) {
        return left.first->vout[left.second].nValue < right.first->vout[right.second].nValue;
    });

    for(auto &&pcoin : vCombineCandidates)
    {
        if (txNew.vin.size() >= MAX_KERNEL_COMBINED_INPUTS||
            nCredit > nCombineThreshold ||
            nCredit + pcoin.first->vout[pcoin.second].nValue > allowedStakingAmount ||
            nCredit + pcoin.first->vout[pcoin.second].nValue > nCombineThreshold)
            break;

        txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
        nCredit += pcoin.first->vout[pcoin.second].nValue;
        walletTransactions.push_back(pcoin.first);
    }
}

bool PoSTransactionCreator::FindHashproof(
    unsigned int nBits,
    unsigned int& nTxNewTime,
    std::pair<const CWalletTx*, unsigned int>& stakeData,
    CMutableTransaction& txNew)
{
    BlockMap::iterator it = mapBlockIndex.find(stakeData.first->hashBlock);
    if (it == mapBlockIndex.end())
    {
        if (fDebug) LogPrintf("CreateCoinStake() failed to find block index \n");
        return false;
    }

    uint256 hashProofOfStake = 0;

    if (CreateHashProofForProofOfStake(
            hashedBlockTimestamps_,
            nBits,
            it->second->GetBlockHeader(),
            COutPoint(stakeData.first->GetHash(), stakeData.second),
            stakeData.first->vout[stakeData.second].nValue,
            nTxNewTime,
            false,
            hashProofOfStake))
    {
        if (nTxNewTime <= chainActive.Tip()->GetMedianTimePast())
        {
            LogPrintf("CreateCoinStake() : kernel found, but it is too far in the past \n");
            return false;
        }
        if (fDebug && GetBoolArg("-printcoinstake", false))
            LogPrintf("CreateCoinStake : kernel found\n");

        if(!SetSuportedStakingScript(stakeData,txNew))
        {
            return false;
        }

        return true;
    }
    return false;
}

bool PoSTransactionCreator::PopulateCoinstakeTransaction(
    const CKeyStore& keystore,
    unsigned int nBits,
    int64_t nSearchInterval,
    CMutableTransaction& txNew,
    unsigned int& nTxNewTime)
{
    if (settings.ParameterIsSet("-reservebalance") && !ParseMoney(settings.GetParameter("-reservebalance"), nReserveBalance))
        return error("CreateCoinStake : invalid reserve balance amount");

    CAmount allowedStakingAmount = wallet_.GetBalance() - nReserveBalance;
    MarkTransactionAsCoinstake(txNew);

    static std::set<std::pair<const CWalletTx*, unsigned int> > setStakeCoins;
    static int nLastStakeSetUpdate = 0;
    if(!SelectCoins(allowedStakingAmount,nLastStakeSetUpdate,setStakeCoins)) return false;

    auto adjustedTime = GetAdjustedTime();
    int64_t minimumTime = chainActive.Tip()->GetMedianTimePast() + 1;
    const int64_t maximumTime = adjustedTime + maximumFutureBlockDrift;
    if (Params().RetargetDifficulty())
    {
        /* Normally, we want to start with an additional offset of nHashDrift
           so that when working backwards in time, we are still beyond the
           required median time.  On regtest with minimum difficulty, this is
           not needed (as the hash target will be hit anyway); by using a
           smaller time there, we ensure that more blocks can be mined in a
           sequence before the mock time needs to be updated.  */
        minimumTime += nHashDrift;
    }
    if(minimumTime>=maximumTime) return false;
    nTxNewTime = std::min(std::max(adjustedTime, minimumTime), maximumTime);

    std::vector<const CWalletTx*> vwtxPrev;
    CAmount nCredit = 0;
    const CChainParams& chainParameters = Params();
    SuperblockSubsidyContainer subsidyContainer(chainParameters);

    const CBlockIndex* chainTip = chainActive.Tip();
    int newBlockHeight = chainTip->nHeight + 1;
    auto blockSubsidity = subsidyContainer.blockSubsidiesProvider().GetBlockSubsidity(newBlockHeight);

    BOOST_FOREACH (PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setStakeCoins)
    {
        if(FindHashproof(nBits, nTxNewTime, pcoin,txNew))
        {
            vwtxPrev.push_back(pcoin.first);
            nCredit += pcoin.first->vout[pcoin.second].nValue;
            break;
        }
    }
    if (nCredit == 0 || nCredit > allowedStakingAmount)
        return false;

    CAmount nReward = blockSubsidity.nStakeReward;
    nCredit += nReward;
    if (nCredit > static_cast<CAmount>(wallet_.nStakeSplitThreshold) * COIN)
    {
        txNew.vout.push_back(txNew.vout.back());
        txNew.vout[1].nValue = nCredit / 2;
        txNew.vout[2].nValue = nCredit - txNew.vout[1].nValue;
    }
    else
    {
        CombineUtxos(allowedStakingAmount,txNew,nCredit,setStakeCoins,vwtxPrev);
        txNew.vout[1].nValue = nCredit;
    }

    BlockIncentivesPopulator(
        chainParameters,
        chainActive,
        masternodePayments,
        subsidyContainer.superblockHeightValidator(),
        subsidyContainer.blockSubsidiesProvider())
        .FillBlockPayee(txNew,blockSubsidity,newBlockHeight,true);

    int nIn = 0;
    for (const CWalletTx* pcoin : vwtxPrev) {
        if (!SignSignature(wallet_, *pcoin, txNew, nIn++))
            return error("CreateCoinStake : failed to sign coinstake");
    }

    nLastStakeSetUpdate = 0; //this will trigger stake set to repopulate next round
    return true;
}

bool PoSTransactionCreator::CreateProofOfStake(
    uint32_t blockBits,
    int64_t nSearchTime,
    int64_t& nLastCoinStakeSearchTime,
    CMutableTransaction& txCoinStake,
    unsigned int& nTxNewTime)
{

    bool fStakeFound = false;
    if (nSearchTime >= nLastCoinStakeSearchTime) {
        if (PopulateCoinstakeTransaction(wallet_, blockBits, nSearchTime - nLastCoinStakeSearchTime, txCoinStake, nTxNewTime))
        {
            fStakeFound = true;
        }
        coinstakeSearchInterval_ = nSearchTime - nLastCoinStakeSearchTime;
        nLastCoinStakeSearchTime = nSearchTime;
    }
    return fStakeFound;
}
