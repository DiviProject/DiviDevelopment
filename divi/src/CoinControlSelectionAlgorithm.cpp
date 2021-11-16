#include <CoinControlSelectionAlgorithm.h>
#include <coincontrol.h>
#include <wallet.h>
#include <WalletTx.h>

CoinControlSelectionAlgorithm::CoinControlSelectionAlgorithm(
    const CCoinControl* coinControl,
    const CWallet& wallet
    ): coinControl_(coinControl), wallet_(wallet)
{
}

std::set<COutput> CoinControlSelectionAlgorithm::SelectCoins(
    const CMutableTransaction& transactionToSelectCoinsFor,
    const std::vector<COutput>& vCoins,
    CAmount& fees) const
{
    CAmount totalInputs = 0;
    std::set<COutput> setCoinsRet;
    if(coinControl_ && coinControl_->HasSelected())
    {
        for(const COutput& out: vCoins)
        {
            if (!out.fSpendable ||
                (!coinControl_->fAllowOtherInputs && !coinControl_->IsSelected(wallet_.GetUtxoHash(*out.tx),out.i)))
            {
                continue;
            }

            totalInputs += out.Value();
            setCoinsRet.insert(out);
        }
    }
    fees = totalInputs - transactionToSelectCoinsFor.GetValueOut();
    return setCoinsRet;
}
