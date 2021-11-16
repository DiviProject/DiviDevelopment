#include <wallet.h>
#include <WalletTx.h>
#include <script/standard.h>

#include <set>
#include <stdint.h>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>
#include <test/test_only.h>

#include <script/StakingVaultScript.h>
#include <random.h>
#include <chain.h>
#include <blockmap.h>
#include <test/FakeWallet.h>
#include <test/MockUtxoHasher.h>
#include <StakableCoin.h>
#include <I_CoinSelectionAlgorithm.h>
#include <I_MerkleTxConfirmationNumberCalculator.h>

class WalletCoinManagementTestFixture
{
public:
    FakeBlockIndexWithHashes fakeChain;
    FakeWallet wallet;
    MockUtxoHasher* utxoHasher;

    WalletCoinManagementTestFixture()
      : fakeChain(1, 1600000000, 1), wallet(fakeChain)
    {
        ENTER_CRITICAL_SECTION(wallet.cs_wallet);

        std::unique_ptr<MockUtxoHasher> hasher(new MockUtxoHasher());
        utxoHasher = hasher.get();
        wallet.SetUtxoHasherForTesting(std::move(hasher));
    }
    ~WalletCoinManagementTestFixture()
    {
        LEAVE_CRITICAL_SECTION(wallet.cs_wallet);
    }

    CScript vaultScriptAsOwner() const
    {
        CKey managerKey;
        managerKey.MakeNewKey(true);
        return CreateStakingVaultScript(
            ToByteVector(wallet.GetDefaultKey().GetID()),
            ToByteVector(managerKey.GetPubKey().GetID()));
    }
    CScript vaultScriptAsManager() const
    {
        CKey ownerKey;
        ownerKey.MakeNewKey(true);
        return CreateStakingVaultScript(
            ToByteVector(ownerKey.GetPubKey().GetID()),
            ToByteVector(wallet.GetDefaultKey().GetID()) );
    }
};

BOOST_FIXTURE_TEST_SUITE(WalletCoinManagementTests,WalletCoinManagementTestFixture)

BOOST_AUTO_TEST_CASE(willAllowSpendingUnlockedCoin)
{
    const CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable));
    BOOST_CHECK(fIsSpendable);
}

BOOST_AUTO_TEST_CASE(willNotAllowSpendingLockedCoin)
{
    const CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());

    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);
    wallet.LockCoin(COutPoint(OutputHash(wtx.GetHash()), outputIndex));

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable));
    BOOST_CHECK(!fIsSpendable);/**/
}

BOOST_AUTO_TEST_CASE(willNotAllowSpendingFromKeyOutsideWallet)
{
    CKey key; key.MakeNewKey(true);
    CPubKey nonWalletPubKey = key.GetPubKey();
    CScript defaultScript = GetScriptForDestination(nonWalletPubKey.GetID());

    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable));
    BOOST_CHECK(!fIsSpendable);/**/
}
BOOST_AUTO_TEST_CASE(willNotAllowSpendingFromWatchOnlyAddress)
{
    CKey key; key.MakeNewKey(true);
    CPubKey nonWalletPubKey = key.GetPubKey();
    CScript defaultScript = GetScriptForDestination(nonWalletPubKey.GetID());


    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);
    wallet.AddWatchOnly(defaultScript);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable));
    BOOST_CHECK(!fIsSpendable);/**/
}

BOOST_AUTO_TEST_CASE(willNotAllowSpendingFromWatchOnlyAddressEvenIfOwned)
{
    CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);
    wallet.AddWatchOnly(defaultScript);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable));
    BOOST_CHECK(!fIsSpendable);/**/
}

BOOST_AUTO_TEST_CASE(willAllowSpendingLockedCoinAfterUnlock)
{
    CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    wallet.LockCoin(COutPoint(OutputHash(wtx.GetHash()), outputIndex));
    wallet.UnlockCoin(COutPoint(OutputHash(wtx.GetHash()), outputIndex));

    bool fIsSpendable = false;
    BOOST_CHECK(wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable));
    BOOST_CHECK(fIsSpendable);
}

BOOST_AUTO_TEST_CASE(willMakeNoDistinctionBetweenAllCoinsAndStakableCoins)
{
    CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,ALL_SPENDABLE_COINS));
    BOOST_CHECK(fIsSpendable);
    fIsSpendable = false;
    BOOST_CHECK(wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,STAKABLE_COINS));
    BOOST_CHECK(fIsSpendable);
}

BOOST_AUTO_TEST_CASE(willDisallowSelectingVaultFundsIfManagedAndSpendableCoinsSelected)
{
    CScript defaultScript =  vaultScriptAsManager();
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);
    wallet.AddCScript(defaultScript);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,ALL_SPENDABLE_COINS));
}

BOOST_AUTO_TEST_CASE(willDisallowSelectingVaultFundsIfOwnerAndSpendableCoinsSelected)
{
    CScript defaultScript =  vaultScriptAsOwner();
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,ALL_SPENDABLE_COINS));
}

BOOST_AUTO_TEST_CASE(willDisallowSelectingVaultFundsIfOwnedAndStakableCoinsSelected)
{
    CScript defaultScript =  vaultScriptAsOwner();
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,STAKABLE_COINS));
}
BOOST_AUTO_TEST_CASE(willAllowSelectingVaultFundsIfManagedAndStakableCoinsSelected)
{
    CScript defaultScript =  vaultScriptAsManager();
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);
    wallet.AddCScript(defaultScript);

    bool fIsSpendable = false;
    BOOST_CHECK(wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,STAKABLE_COINS));
}
BOOST_AUTO_TEST_CASE(willDisallowSelectingVaultFundsIfManagedButScriptNotAdded)
{
    CScript defaultScript =  vaultScriptAsManager();
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,STAKABLE_COINS));
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,ALL_SPENDABLE_COINS));
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,OWNED_VAULT_COINS));
}

BOOST_AUTO_TEST_CASE(willDisallowSelectingVaultFundsIfManagedAndOwnedVaultCoinsSelected)
{
    CScript defaultScript =  vaultScriptAsManager();
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);
    wallet.AddCScript(defaultScript);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,OWNED_VAULT_COINS));
}

BOOST_AUTO_TEST_CASE(willAllowSelectingVaultFundsIfOwnerAndOwnedVaultCoinsSelected)
{
    CScript defaultScript =  vaultScriptAsOwner();
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,OWNED_VAULT_COINS));
}

BOOST_AUTO_TEST_CASE(willDisallowSelectingNonVaultFundsIfOwnedVaultCoinsRequested)
{
    CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex);

    bool fIsSpendable = false;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,OWNED_VAULT_COINS));
    BOOST_CHECK(!fIsSpendable);
    fIsSpendable = false;
    BOOST_CHECK(wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,ALL_SPENDABLE_COINS));
    BOOST_CHECK(fIsSpendable);
}

BOOST_AUTO_TEST_CASE(willFindThatTransactionsByDefaultHaveNegativeDepth)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());
    unsigned outputIndex=0;
    auto normalTx = wallet.AddDefaultTx(normalScript,outputIndex,100*COIN);
    BOOST_CHECK_MESSAGE(wallet.getConfirmationCalculator().GetNumberOfBlockConfirmations(normalTx)==-1,"Found wallet transaction has non-negative depth in empty chain!");
}

BOOST_AUTO_TEST_CASE(willFindThatTransactionsWillHaveDepthAccordingToLengthOfChain)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& normalTx = wallet.AddDefaultTx(normalScript,outputIndex,100*COIN);
    wallet.FakeAddToChain(normalTx);

    int someNumberOfBlocksToAdd = GetRand(25)+1;
    int startingNumberOfConfirmations = 1;
    for(int numberOfAdditionalBlocks = 0; numberOfAdditionalBlocks < someNumberOfBlocksToAdd ; ++numberOfAdditionalBlocks )
    {
        BOOST_CHECK_EQUAL(wallet.getConfirmationCalculator().GetNumberOfBlockConfirmations(normalTx),startingNumberOfConfirmations+numberOfAdditionalBlocks);
        wallet.AddBlock();
    }
}

BOOST_AUTO_TEST_CASE(willReturnZeroBalancesWhenTransactionsAreUnconfirmed)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());
    CScript ownedVaultScript = vaultScriptAsOwner();
    CScript managedVaultScript = vaultScriptAsManager();

    unsigned outputIndex=0;
    wallet.AddDefaultTx(normalScript,outputIndex,100*COIN);
    wallet.AddDefaultTx(ownedVaultScript,outputIndex,1000*COIN);
    wallet.AddDefaultTx(managedVaultScript,outputIndex,10000*COIN);

    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalance(), 0,"Total balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetStakingBalance(), 0,"Staking balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetSpendableBalance(), 0,"Spendable balance was not the expected amount");
}

BOOST_AUTO_TEST_CASE(willReturnCorrectBalanceWhenTransactionIsConfirmed)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& normalTx = wallet.AddDefaultTx(normalScript,outputIndex,100*COIN);
    wallet.FakeAddToChain(normalTx);

    BOOST_CHECK_MESSAGE(wallet.getConfirmationCalculator().GetNumberOfBlockConfirmations(normalTx) >= 1,"Transaction is not at least one block deep!");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalance(), 100*COIN,"Total balance was not the expected amount");
}

BOOST_AUTO_TEST_CASE(willFindStakingBalanceMatchesBalanceWhenOwnWalletFundsAreNotVaulted)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());
    CScript managedVaultScript = vaultScriptAsManager();
    wallet.AddCScript(managedVaultScript);

    unsigned outputIndex=0;
    const CWalletTx& normalTx = wallet.AddDefaultTx(normalScript,outputIndex,100*COIN);
    const CWalletTx& managedVaultTx = wallet.AddDefaultTx(managedVaultScript,outputIndex,10000*COIN);
    wallet.FakeAddToChain(normalTx);
    wallet.FakeAddToChain(managedVaultTx);

    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalance(), 10100*COIN,"Total balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetStakingBalance(), 10100*COIN,"Staking balance was not the expected amount");
}

BOOST_AUTO_TEST_CASE(willHaveAStakingBalanceFromAVaultThatIsntSpendable)
{
    CScript managedVaultScript = vaultScriptAsManager();
    wallet.AddCScript(managedVaultScript);

    unsigned outputIndex=0;
    const CWalletTx& managedVaultTx = wallet.AddDefaultTx(managedVaultScript,outputIndex,10000*COIN);
    wallet.FakeAddToChain(managedVaultTx);

    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalance(), 10000*COIN,"Total balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetStakingBalance(), 10000*COIN,"Staking balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetSpendableBalance(), 0*COIN,"Spendable balance was not the expected amount");
}

BOOST_AUTO_TEST_CASE(willTreatOwnedVaultAsUnspendableButWillRecordBalance)
{
    CScript ownedVaultScript = vaultScriptAsOwner();

    unsigned outputIndex=0;
    const CWalletTx& ownedVaultTx = wallet.AddDefaultTx(ownedVaultScript,outputIndex,10000*COIN);
    wallet.FakeAddToChain(ownedVaultTx);

    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalance(), 10000*COIN,"Total balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetStakingBalance(), 0*COIN,"Staking balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetSpendableBalance(), 0*COIN,"Spendable balance was not the expected amount");
}

BOOST_AUTO_TEST_CASE(willEnsureBalancesAreAsExpected)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());
    CScript ownedVaultScript = vaultScriptAsOwner();
    CScript managedVaultScript = vaultScriptAsManager();
    wallet.AddCScript(managedVaultScript);


    CAmount normalTxValue = (GetRand(1000)+1)*COIN;
    CAmount ownedVaultTxValue = (GetRand(1000)+1)*COIN;
    CAmount managedVaultTxValue = (GetRand(1000)+1)*COIN;
    CAmount totalBalance = normalTxValue + ownedVaultTxValue + managedVaultTxValue;

    unsigned outputIndex=0;
    const CWalletTx& normalTx = wallet.AddDefaultTx(normalScript,outputIndex,normalTxValue);
    const CWalletTx& ownedVaultTx = wallet.AddDefaultTx(ownedVaultScript,outputIndex,ownedVaultTxValue);
    const CWalletTx& managedVaultTx = wallet.AddDefaultTx(managedVaultScript,outputIndex,managedVaultTxValue);

    wallet.FakeAddToChain(normalTx);
    wallet.FakeAddToChain(ownedVaultTx);
    wallet.FakeAddToChain(managedVaultTx);

    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalance(), totalBalance,"Total balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetStakingBalance(), normalTxValue+managedVaultTxValue,"Staking balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetSpendableBalance(), normalTxValue,"Spendable balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalanceByCoinType(OWNED_VAULT_COINS), ownedVaultTxValue,"Owned vault balance was not the expected amount");
}

BOOST_AUTO_TEST_CASE(willEnsureLockedCoinsDoNotCountTowardStakableBalance)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());
    CScript ownedVaultScript = vaultScriptAsOwner();
    CScript managedVaultScript = vaultScriptAsManager();
    wallet.AddCScript(managedVaultScript);


    CAmount firstNormalTxValue = (GetRand(1000)+1)*COIN;
    CAmount secondNormalTxValue = (GetRand(1000)+1)*COIN;

    unsigned firstTxOutputIndex=0;
    const CWalletTx& firstNormalTx = wallet.AddDefaultTx(normalScript,firstTxOutputIndex,firstNormalTxValue);
    unsigned secondTxOutputIndex=0;
    const CWalletTx& secondNormalTx = wallet.AddDefaultTx(normalScript,secondTxOutputIndex,secondNormalTxValue);
    wallet.FakeAddToChain(firstNormalTx);
    wallet.FakeAddToChain(secondNormalTx);

    wallet.LockCoin(COutPoint(OutputHash(firstNormalTx.GetHash()), firstTxOutputIndex));

    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetBalance(), firstNormalTxValue+secondNormalTxValue,"Total balance was not the expected amount");
    BOOST_CHECK_EQUAL_MESSAGE(wallet.GetStakingBalance(), secondNormalTxValue,"Staking balance was not the expected amount");
}


BOOST_AUTO_TEST_CASE(willEnsureStakingBalanceAndTotalBalanceAgreeEvenIfTxsBelongToCommonBlock)
{
    CScript normalScript = GetScriptForDestination(wallet.GetDefaultKey().GetID());

    CAmount firstNormalTxValue = (GetRand(1000)+1)*COIN;
    CAmount secondNormalTxValue = (GetRand(1000)+1)*COIN;

    unsigned firstTxOutputIndex=0;
    const CWalletTx& firstNormalTx = wallet.AddDefaultTx(normalScript,firstTxOutputIndex,firstNormalTxValue);
    unsigned secondTxOutputIndex=0;
    const CWalletTx& secondNormalTx = wallet.AddDefaultTx(normalScript,secondTxOutputIndex,secondNormalTxValue);
    wallet.FakeAddToChain(firstNormalTx);
    wallet.FakeAddToChain(secondNormalTx);

    std::set<StakableCoin> stakableCoins;
    stakableCoins.insert(StakableCoin(firstNormalTx, COutPoint(OutputHash(firstNormalTx.GetHash()), firstTxOutputIndex), firstNormalTx.hashBlock));
    stakableCoins.insert(StakableCoin(secondNormalTx, COutPoint(OutputHash(secondNormalTx.GetHash()), secondTxOutputIndex), secondNormalTx.hashBlock));
    BOOST_CHECK_EQUAL_MESSAGE(stakableCoins.size(),2,"Missing coins in the stakable set");
}

BOOST_AUTO_TEST_CASE(willUseUtxoHasherForCoinLock)
{
    CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex, COIN);
    const auto id = utxoHasher->Add(wtx);

    wallet.LockCoin(COutPoint(id, outputIndex));

    bool fIsSpendable;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,ALL_SPENDABLE_COINS));

    BOOST_CHECK_EQUAL(wallet.ComputeCredit(wtx, isminetype::ISMINE_SPENDABLE, REQUIRE_UNLOCKED), 0);
    BOOST_CHECK_EQUAL(wallet.ComputeCredit(wtx, isminetype::ISMINE_SPENDABLE, REQUIRE_LOCKED), COIN);
}

BOOST_AUTO_TEST_CASE(willMarkOutputsSpentBasedOnUtxoHasher)
{
    CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex, COIN);
    const auto id = utxoHasher->Add(wtx);

    CMutableTransaction mtx;
    mtx.vin.emplace_back(id, outputIndex);
    const auto& spendTx = wallet.Add(mtx);

    /* The wallet's IsSpent check verifies also the spending tx' number
       of confirmations, which requires it to be either in the chain or
       in the mempool at least.  */
    wallet.FakeAddToChain(wtx);
    wallet.FakeAddToChain(spendTx);

    BOOST_CHECK(wallet.IsSpent(wtx, outputIndex));
    bool fIsSpendable;
    BOOST_CHECK(!wallet.IsAvailableForSpending(&wtx,outputIndex,false,fIsSpendable,ALL_SPENDABLE_COINS));
}

/** Fake coin selector that just uses all available outputs.  */
class AllCoinSelector : public I_CoinSelectionAlgorithm
{
public:

  AllCoinSelector() = default;

  std::set<COutput> SelectCoins(const CMutableTransaction& tx, const std::vector<COutput>& vCoins, CAmount& fees) const override
  {
    fees = COIN / 100;
    return std::set<COutput>(vCoins.begin(), vCoins.end());
  }

};

BOOST_AUTO_TEST_CASE(willUseUtxoHashForSpendingCoins)
{
    CScript defaultScript = GetScriptForDestination(wallet.getNewKey().GetID());
    unsigned outputIndex=0;
    const CWalletTx& wtx = wallet.AddDefaultTx(defaultScript,outputIndex, COIN);
    const auto id = utxoHasher->Add(wtx);
    wallet.FakeAddToChain(wtx);

    const CScript sendTo = CScript() << OP_TRUE;
    std::string strError;
    CReserveKey reserveKey(wallet);
    CAmount nFeeRequired;
    CWalletTx wtxNew;
    AllCoinSelector coinSelector;
    BOOST_CHECK(wallet.CreateTransaction({{sendTo, COIN / 10}}, wtxNew, reserveKey, &coinSelector).second);

    BOOST_CHECK_EQUAL(wtxNew.vin.size(), 1);
    const auto& prevout = wtxNew.vin[0].prevout;
    BOOST_CHECK(prevout.hash == id);
    BOOST_CHECK_EQUAL(prevout.n, outputIndex);
}

BOOST_AUTO_TEST_SUITE_END()
