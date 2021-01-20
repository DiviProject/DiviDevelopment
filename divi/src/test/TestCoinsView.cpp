#include "TestCoinsView.h"

namespace
{

/** An empty CCoinsView implementation.  */
class EmptyCoinsView : public CCoinsView
{

public:

  bool GetCoins(const uint256& txid, CCoins& coins) const override
  {
    return false;
  }

  bool HaveCoins(const uint256& txid) const override
  {
    return false;
  }

  uint256 GetBestBlock() const override
  {
    return uint256(0);
  }

  bool BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock) override
  {
    return false;
  }

  bool GetStats(CCoinsStats& stats) const override
  {
    return false;
  }

  /** Returns a singleton instance that can be used in tests.  Note that
   *  instances are always immutable (independent of their const state).  */
  static EmptyCoinsView& Instance()
  {
    static EmptyCoinsView obj;
    return obj;
  }

};

} // anonymous namespace

TestCoinsView::TestCoinsView ()
  : CCoinsViewCache(&EmptyCoinsView::Instance())
{}
