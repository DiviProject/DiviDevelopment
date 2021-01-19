#ifndef UTXO_CHECKING_AND_UPDATING_H
#define UTXO_CHECKING_AND_UPDATING_H
#include <vector>
#include <scriptCheck.h>
#include <amount.h>
#include <uint256.h>

class BlockMap;
class CTransaction;
class CValidationState;
class CCoinsViewCache;
class CTxUndo;
class TransactionLocationReference;

enum class TxReversalStatus
{
    ABORT_NO_OTHER_ERRORS,
    ABORT_WITH_OTHER_ERRORS,
    CONTINUE_WITH_ERRORS,
    OK,
};

/** Implementations of this interface translate transactions into the hashes
 *  that will be used to refer to the UTXOs their outputs create.
 *
 *  This class abstracts away the segwit-light fork and its activation
 *  from the places that need to refer to / update UTXOs.
 *
 *  For unit tests, this class can be subclassed and mocked.  */
class TransactionUtxoHasher
{

public:

  TransactionUtxoHasher() = default;
  virtual ~TransactionUtxoHasher() = default;

  TransactionUtxoHasher(const TransactionUtxoHasher&) = delete;
  void operator=(const TransactionUtxoHasher&) = delete;

  virtual uint256 GetUtxoHash(const CTransaction& tx) const = 0;

};

/** A TransactionUtxoHasher for transactions contained in a particular
 *  block, e.g. for processing that block's connect or disconnect.  Initially
 *  these are just the txid (as also with upstream Bitcoin), but for
 *  segwit-light, they are changed to the bare txid.
 *
 *  This class abstracts away the segwit-light fork and its activation
 *  from the places that need to refer to / update UTXOs.
 *
 *  For unit tests, this class can be subclassed and mocked.  */
class BlockUtxoHasher : public TransactionUtxoHasher
{

public:

  uint256 GetUtxoHash(const CTransaction& tx) const override;

};

void UpdateCoinsWithTransaction(const CTransaction& tx, CCoinsViewCache& inputs, CTxUndo& txundo, const TransactionUtxoHasher& hasher, int nHeight);
TxReversalStatus UpdateCoinsReversingTransaction(const CTransaction& tx, const TransactionLocationReference& txLocationReference, CCoinsViewCache& inputs, const CTxUndo* txundo);
bool CheckInputs(
    const CTransaction& tx,
    CValidationState& state,
    const CCoinsViewCache& inputs,
    const BlockMap& blockIndexMap,
    bool fScriptChecks,
    unsigned int flags,
    bool cacheStore,
    std::vector<CScriptCheck>* pvChecks = nullptr);
bool CheckInputs(
    const CTransaction& tx,
    CValidationState& state,
    const CCoinsViewCache& inputs,
    const BlockMap& blockIndexMap,
    CAmount& nFees,
    CAmount& nValueIn,
    bool fScriptChecks,
    unsigned int flags,
    bool cacheStore,
    std::vector<CScriptCheck>* pvChecks = nullptr,
    bool connectBlockDoS = false);
#endif// UTXO_CHECKING_AND_UPDATING_H
