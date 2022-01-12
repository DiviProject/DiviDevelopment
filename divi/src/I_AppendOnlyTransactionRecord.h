#ifndef I_APPEND_ONLY_TRANSACTION_RECORD_H
#define I_APPEND_ONLY_TRANSACTION_RECORD_H
#include <uint256.h>
#include <WalletTx.h>
#include <map>
class uint256;
class CWalletTx;
class I_AppendOnlyTransactionRecord
{
public:
    virtual ~I_AppendOnlyTransactionRecord() {}
    virtual const CWalletTx* GetWalletTx(const uint256& hash) const = 0;
    virtual const std::map<uint256, CWalletTx>& GetWalletTransactions() const = 0;
    virtual std::pair<std::map<uint256, CWalletTx>::iterator, bool> AddTransaction(const CWalletTx& newlyAddedTransaction) = 0;
    virtual unsigned size() const = 0;
};

#endif// I_APPEND_ONLY_TRANSACTION_RECORD_H