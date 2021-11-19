#include <IndexDatabaseUpdates.h>

#include <primitives/transaction.h>
#include <UtxoCheckingAndUpdating.h>

IndexDatabaseUpdates::IndexDatabaseUpdates(
    ): addressIndex()
    , addressUnspentIndex()
    , spentIndex()
    , txLocationData()
{
}

TransactionLocationReference::TransactionLocationReference(
    const TransactionUtxoHasher& utxoHasher,
    const CTransaction& tx,
    unsigned blockheightValue,
    int transactionIndexValue
    ): hash(tx.GetHash())
    , utxoHash(utxoHasher.GetUtxoHash(tx))
    , blockHeight(blockheightValue)
    , transactionIndex(transactionIndexValue)
{
}
