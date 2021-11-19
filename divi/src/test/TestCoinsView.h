#ifndef TESTCOINSVIEW_H
#define TESTCOINSVIEW_H

#include "coins.h"

/** A coins view for use in tests.  It is a CoinsViewCache subclass that
 *  uses an "empty" coins view as its backing.  Tests can modify it to insert
 *  coins as needed into the cache, and then access them.
 */
class TestCoinsView : public CCoinsViewCache
{

public:

  TestCoinsView();

};

#endif // TESTCOINSVIEW_H
