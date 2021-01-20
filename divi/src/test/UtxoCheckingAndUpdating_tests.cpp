#include <test_only.h>
#include <UtxoCheckingAndUpdating.h>

#include "MockUtxoHasher.h"
#include "primitives/transaction.h"
#include "TestCoinsView.h"

namespace
{

class UpdateCoinsTestFixture
{

protected:

  TestCoinsView coins;
  MockUtxoHasher utxoHasher;

};

BOOST_FIXTURE_TEST_SUITE(UpdateCoins_tests, UpdateCoinsTestFixture)

BOOST_AUTO_TEST_CASE(addsCorrectOutputs)
{
  CMutableTransaction mtx;
  mtx.vout.emplace_back(1, CScript() << OP_TRUE);
  const CTransaction tx1(mtx);

  mtx.vout.clear();
  mtx.vout.emplace_back(2, CScript() << OP_TRUE);
  const CTransaction tx2(mtx);
  const auto id2 = utxoHasher.Add(tx2);

  CTxUndo txundo;
  UpdateCoins(tx1, coins, txundo, utxoHasher, 101);
  UpdateCoins(tx2, coins, txundo, utxoHasher, 102);

  BOOST_CHECK(coins.HaveCoins(tx1.GetHash()));
  BOOST_CHECK(!coins.HaveCoins(tx2.GetHash()));
  BOOST_CHECK(coins.HaveCoins(id2));
}

BOOST_AUTO_TEST_SUITE_END()

} // anonymous namespace
