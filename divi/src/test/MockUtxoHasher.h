#ifndef MOCKUTXOHASHER_H
#define MOCKUTXOHASHER_H

#include "UtxoCheckingAndUpdating.h"

#include "uint256.h"

#include <map>

class CTransaction;

/** A TransactionUtxoHasher instance that returns normal txid's (as per before
 *  segwit-light) by default, but can be instructed to return custom hashes
 *  for certain transactions.  */
class MockUtxoHasher : public TransactionUtxoHasher
{

private:

  /** Custom hashes to return for given txid's.  */
  std::map<uint256, uint256> customHashes;

  /** Internal counter to produce unique fake hashes.  */
  unsigned cnt = 0;

public:

  MockUtxoHasher()
  {}

  /** Marks the given transaction for returning a custom hash.  The hash
   *  is generated uniquely and returned from the function.  */
  uint256 Add(const CTransaction& tx);

  uint256 GetUtxoHash(const CTransaction& tx) const override;

};

#endif // MOCKUTXOHASHER_H
