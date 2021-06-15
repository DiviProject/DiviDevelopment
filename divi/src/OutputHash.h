#ifndef OUTPUT_HASH_H
#define OUTPUT_HASH_H

#include "serialize.h"
#include "uint256.h"

/** This class is "equivalent" to a uint256, but semantically it represents
 *  a specific use, namely the hash used for an outpoint in the UTXO set
 *  and referred to by following transactions.  This is the normal txid
 *  originally, but may be changed in the future e.g. with segwit-light.  */
class OutputHash
{

private:

  /** The actual hash value.  */
  uint256 value;

public:

  OutputHash () = default;
  OutputHash (const OutputHash&) = default;
  OutputHash& operator= (const OutputHash&) = default;

  explicit OutputHash (const uint256& val)
    : value(val)
  {}

  ADD_SERIALIZE_METHODS;

  template<typename Stream, typename Operation>
  inline void SerializationOp (Stream& s, Operation ser_action,
                               int nType, int nVersion)
  {
    READWRITE (value);
  }

  inline const uint256& GetValue () const
  {
    return value;
  }

  inline void SetNull ()
  {
    value.SetNull ();
  }

  inline bool IsNull () const
  {
    return value.IsNull ();
  }

  inline std::string ToString () const
  {
    return value.ToString ();
  }

  inline std::string GetHex () const
  {
    return value.GetHex ();
  }

  inline friend bool operator== (const OutputHash& a, const OutputHash& b)
  {
    return a.value == b.value;
  }

  inline friend bool operator!= (const OutputHash& a, const OutputHash& b)
  {
    return !(a == b);
  }

  inline friend bool operator< (const OutputHash& a, const OutputHash& b)
  {
    return a.value < b.value;
  }

};

#endif  // OUTPUT_HASH_H
