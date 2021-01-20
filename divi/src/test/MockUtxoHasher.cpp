#include "MockUtxoHasher.h"

#include "hash.h"
#include "primitives/transaction.h"

uint256 MockUtxoHasher::Add(const CTransaction& tx)
{
  ++cnt;
  const uint256 value = Hash(&cnt, (&cnt) + 1);
  customHashes.emplace(tx.GetHash(), value);
  return value;
}

uint256 MockUtxoHasher::GetUtxoHash(const CTransaction& tx) const
{
  const uint256 txid = tx.GetHash();
  const auto mit = customHashes.find(txid);
  if (mit != customHashes.end())
    return mit->second;
  return txid;
}
