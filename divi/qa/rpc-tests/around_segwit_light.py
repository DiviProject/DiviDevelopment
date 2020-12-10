#!/usr/bin/env python3
# Copyright (c) 2020 The DIVI developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests the policy changes in wallet and mempool around the
# segwit-light activation time that prevent spending of
# unconfirmed outputs.

from test_framework import BitcoinTestFramework
from authproxy import JSONRPCException
from util import *

from segwit_light import ACTIVATION_TIME

# Offset around the activation time where no restrictions are in place.
NO_RESTRICTIONS = 24 * 3_600

# Offset around the activation time where the wallet disallows
# spending unconfirmed change, but the mempool still accepts it.
WALLET_RESTRICTED = 10 * 3_600

# Offset around the activation time where wallet and mempool
# disallow spending unconfirmed outputs.
MEMPOOL_RESTRICTED = 3_600


class AroundSegwitLightTest (BitcoinTestFramework):

    def setup_network (self, split=False):
        args = ["-debug", "-spendzeroconfchange"]
        self.nodes = start_nodes (1, self.options.tmpdir, extra_args=[args])
        self.node = self.nodes[0]
        self.is_network_split = False

    def build_spending_chain (self, key, addr, initialValue):
        """
        Spends the initialValue to addr, and then builds a follow-up
        transaction that spends that output again back to addr with
        a smaller value (for fees).  The second transaction is built
        using the raw-transactions API and returned as hex, not
        submitted to the node already.
        """

        txid = self.node.sendtoaddress (addr, initialValue)
        data = self.node.getrawtransaction (txid, 1)
        outputIndex = None
        for i in range (len (data["vout"])):
          if data["vout"][i]["value"] == initialValue:
            outputIndex = i
            break
        assert outputIndex is not None

        inputs = [{"txid": data[key], "vout": outputIndex}]
        outputs = {addr: initialValue - 10}
        tx = self.node.createrawtransaction (inputs, outputs)
        signed = self.node.signrawtransaction (tx)
        assert signed["complete"]

        return signed["hex"]

    def expect_unrestricted (self):
        """
        Checks that spending of unconfirmed change is possible without
        restrictions of wallet or mempool.
        """

        balance = self.node.getbalance ()
        addr = self.node.getnewaddress ()

        self.node.sendtoaddress (addr, balance - 10)
        self.node.sendtoaddress (addr, balance - 20)
        self.node.setgenerate (True, 1)

    def expect_wallet_restricted (self, key):
        """
        Checks that the wallet forbids spending unconfirmed change,
        while the mempool still allows it.
        """

        balance = self.node.getbalance ()
        addr = self.node.getnewaddress ()

        self.node.sendtoaddress (addr, balance - 10)
        assert_raises (JSONRPCException, self.node.sendtoaddress, addr, balance - 20)
        self.node.setgenerate (True, 1)

        balance = self.node.getbalance ()
        tx = self.build_spending_chain (key, addr, balance - 10)
        self.node.sendrawtransaction (tx)
        self.node.setgenerate (True, 1)

    def expect_mempool_restricted (self, key):
        """
        Checks that the mempool does not allow spending unconfirmed
        outputs (even if the transaction is built and submitted directly),
        while blocks should still allow it.
        """

        balance = self.node.getbalance ()
        addr = self.node.getnewaddress ()

        tx = self.build_spending_chain (key, addr, balance - 10)
        assert_raises (JSONRPCException, self.node.sendrawtransaction, tx)
        self.node.generateblock ({"extratx": [tx]})

    def run_test (self):
        self.node.setgenerate (True, 30)

        # Before restrictions come into force, doing a normal
        # spend of unconfirmed change through the wallet is fine.
        set_node_times (self.nodes, ACTIVATION_TIME - NO_RESTRICTIONS)
        self.expect_unrestricted ()

        # Next the wallet doesn't allow those spends, but the mempool
        # will (if done directly).
        set_node_times (self.nodes, ACTIVATION_TIME - WALLET_RESTRICTED)
        self.expect_wallet_restricted ("txid")

        # Very close to the fork (on both sides), even the mempool won't
        # allow spending unconfirmed change.  If we include it directly in
        # a block, it works.
        set_node_times (self.nodes, ACTIVATION_TIME - MEMPOOL_RESTRICTED)
        self.expect_mempool_restricted ("txid")
        set_node_times (self.nodes, ACTIVATION_TIME + MEMPOOL_RESTRICTED)
        self.node.setgenerate (True, 1)
        self.expect_mempool_restricted ("baretxid")

        # Finally, we should run into mempool-only or no restrictions
        # at all if we go further into the future, away from the fork.
        set_node_times (self.nodes, ACTIVATION_TIME + WALLET_RESTRICTED)
        self.expect_wallet_restricted ("baretxid")
        set_node_times (self.nodes, ACTIVATION_TIME + NO_RESTRICTIONS)
        self.expect_unrestricted ()

if __name__ == '__main__':
    AroundSegwitLightTest ().main ()
