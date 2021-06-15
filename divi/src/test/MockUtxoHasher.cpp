#include "MockUtxoHasher.h"

#include "hash.h"
#include "primitives/transaction.h"

OutputHash MockUtxoHasher::Add(const CTransaction& tx)
{
  ++cnt;
  const OutputHash value(Hash(&cnt, (&cnt) + 1));
  customHashes.emplace(tx.GetHash(), value);
  return value;
}

void MockUtxoHasher::UseBareTxid(const CTransaction& tx)
{
  customHashes.emplace(tx.GetHash(), OutputHash(tx.GetBareTxid()));
}

OutputHash MockUtxoHasher::GetUtxoHash(const CTransaction& tx) const
{
  const uint256 txid = tx.GetHash();
  const auto mit = customHashes.find(txid);
  if (mit != customHashes.end())
    return mit->second;
  return OutputHash(txid);
}
