#!/usr/bin/env python3
# Copyright (c) 2020 The DIVI developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests the policy changes in wallet and mempool around the
# segwit-light activation time that prevent spending of
# unconfirmed outputs.

from test_framework import BitcoinTestFramework
from util import *

from PowToPosTransition import createPoSStacks, generatePoSBlocks

ACTIVATION_TIME = 2_100_000_000


class SegwitLightTest (BitcoinTestFramework):

    def setup_network (self, split=False):
        args = ["-debug", "-spendzeroconfchange"]
        self.nodes = start_nodes (2, self.options.tmpdir, extra_args=[args]*2)
        connect_nodes (self.nodes[0], 1)
        self.is_network_split = False

    def run_test (self):
        # Activate the fork and PoS.  We go beyond the fork to ensure
        # the mempool/wallet limitations are lifted already.
        set_node_times (self.nodes, ACTIVATION_TIME + 3_600 * 24 * 7)
        reconnect_all (self.nodes)
        self.nodes[0].setgenerate (True, 1)
        sync_blocks (self.nodes)
        createPoSStacks (self.nodes[:1], self.nodes)
        generatePoSBlocks (self.nodes, 0, 100)
        blk = self.nodes[1].getblockheader (self.nodes[1].getbestblockhash ())
        assert_greater_than (blk["time"], ACTIVATION_TIME)

        # Send some normal transactions from the wallet (but in a chain).
        self.nodes[0].sendtoaddress (self.nodes[1].getnewaddress (), 1_000)
        generatePoSBlocks (self.nodes, 0, 1)
        assert_equal (self.nodes[1].getbalance (), 1_000)
        addr = self.nodes[0].getnewaddress ()
        id1 = self.nodes[1].sendtoaddress (addr, 900)
        id2 = self.nodes[1].sendtoaddress (addr, 90)
        id3 = self.nodes[1].sendtoaddress (addr, 9)
        assert_equal (set (self.nodes[1].getrawmempool ()), set ([id1, id2, id3]))
        sync_mempools (self.nodes)
        generatePoSBlocks (self.nodes, 0, 1)
        assert_equal (self.nodes[1].getrawmempool (), [])
        assert_greater_than (1, self.nodes[1].getbalance ())

        # Build a transaction on top of an unconfirmed one, that we will malleate.
        # The prepared transaction should still be valid.  For malleating, we use
        # funds on a 1-of-2 multisig address, and then change which wallet
        # is signing.
        keys = [
          n.validateaddress (n.getnewaddress ())["pubkey"]
          for n in self.nodes
        ]
        multisig = self.nodes[0].addmultisigaddress (1, keys)
        assert_equal (self.nodes[1].addmultisigaddress (1, keys), multisig)
        txid0 = self.nodes[0].sendtoaddress (multisig, 1_000)
        data0 = self.nodes[0].getrawtransaction (txid0, 1)
        btxid = data0["baretxid"]
        outputIndex = None
        for i in range (len (data0["vout"])):
          if data0["vout"][i]["scriptPubKey"]["addresses"] == [multisig]:
            assert outputIndex is None
            outputIndex = i
        assert outputIndex is not None
        generatePoSBlocks (self.nodes, 0, 1)
        out = self.nodes[0].gettxout (btxid, outputIndex)
        assert_equal (out["confirmations"], 1)
        assert_equal (out["value"], 1_000)
        assert_equal (out["scriptPubKey"]["addresses"], [multisig])

        inputs = [{"txid": btxid, "vout": outputIndex}]
        tempAddr = self.nodes[0].getnewaddress ("temp")
        outputs = {tempAddr: 999}
        unsigned1 = self.nodes[0].createrawtransaction (inputs, outputs)
        signed1 = self.nodes[0].signrawtransaction (unsigned1)
        assert_equal (signed1["complete"], True)
        signed1 = signed1["hex"]
        data1 = self.nodes[0].decoderawtransaction (signed1)

        prevtx = [
          {
            "txid": data1["baretxid"],
            "vout": 0,
            "scriptPubKey": self.nodes[0].validateaddress (tempAddr)["scriptPubKey"],
          }
        ]
        inputs = [{"txid": data1["baretxid"], "vout": 0}]
        finalAddr = self.nodes[1].getnewaddress ("final")
        outputs = {finalAddr: 998}
        unsigned2 = self.nodes[0].createrawtransaction (inputs, outputs)
        signed2 = self.nodes[0].signrawtransaction (unsigned2, prevtx)
        assert_equal (signed2["complete"], True)
        signed2 = signed2["hex"]
        data2 = self.nodes[0].decoderawtransaction (signed2)

        signed1p = self.nodes[1].signrawtransaction (unsigned1)
        assert_equal (signed1p["complete"], True)
        signed1p = signed1p["hex"]
        data1p = self.nodes[0].decoderawtransaction (signed1p)
        assert_equal (data1["baretxid"], data1p["baretxid"])
        assert data1["txid"] != data1p["txid"]

        self.nodes[0].sendrawtransaction (signed1p)
        self.nodes[0].sendrawtransaction (signed2)
        generatePoSBlocks (self.nodes, 0, 1)
        sync_blocks (self.nodes)
        assert_equal (self.nodes[1].getbalance ("final"), 998)

if __name__ == '__main__':
    SegwitLightTest ().main ()
