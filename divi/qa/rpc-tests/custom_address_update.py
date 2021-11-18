#!/usr/bin/env python3
# Copyright (c) 2020-2021 The DIVI developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests that nodes with support for custom reward addresses are compatible
# to nodes without it before the change actually activates.  For this,
# we run two masternodes (one with and one without support) and check
# that they can both see each other's masternode.
#
# We use five nodes:
# - node 0 is a masternode with default protocol (i.e. "new")
# - node 1 is a masternode with previous protocol (i.e. "old")
# - nodes 2-4 are just used to get above the "three full nodes" threshold

from test_framework import BitcoinTestFramework
from util import *
from masternode import *

import collections
import copy
import time

OLD_PROTOCOL = 70915
NEW_PROTOCOL = 71000


class CustomAddressUpdateTest (MnTestFramework):

  def __init__ (self):
    super ().__init__ ()
    self.base_args = ["-debug", "-nolistenonion", "-activeversion=%d" % OLD_PROTOCOL]
    self.number_of_nodes = 5

  def args_for (self, n):
    res = copy.deepcopy (self.base_args)
    if n == 1:
      res.extend (["-protocolversion=%d" % OLD_PROTOCOL])
    return res

  def setup_network (self, config_line=None, extra_args=[]):
    self.nodes = [
      start_node (i, self.options.tmpdir, extra_args=self.args_for (i))
      for i in range (self.number_of_nodes)
    ]
    self.setup = [None] * self.number_of_nodes

    # We want to work with mock times that are beyond the genesis
    # block timestamp but before current time (so that nodes being
    # started up and before they get on mocktime aren't rejecting
    # the on-disk blockchain).
    self.time = 1580000000
    assert self.time < time.time ()
    set_node_times (self.nodes, self.time)

    connect_nodes (self.nodes[0], 2)
    connect_nodes (self.nodes[1], 2)
    connect_nodes (self.nodes[2], 3)
    connect_nodes (self.nodes[2], 4)
    connect_nodes (self.nodes[3], 4)

    self.is_network_split = False

  def advance_time (self, dt=1):
    """Advances mocktime by the given number of seconds."""

    self.time += dt
    set_node_times (self.nodes, self.time)

  def mine_blocks (self, n):
    """Mines blocks with node 2."""

    self.nodes[2].setgenerate(True, n)
    sync_blocks (self.nodes)

  def run_test (self):
    assert_equal (self.nodes[0].getnetworkinfo ()["protocolversion"], NEW_PROTOCOL)
    assert_equal (self.nodes[1].getnetworkinfo ()["protocolversion"], OLD_PROTOCOL)

    self.fund_masternodes ()
    self.start_masternodes ()

  def fund_masternodes (self):
    print ("Funding masternodes...")

    # The collateral needs 15 confirmations, and the masternode broadcast
    # signature must be later than that block's timestamp.  Thus we start
    # with a very early timestamp.
    genesis = self.nodes[0].getblockhash (0)
    genesisTime = self.nodes[0].getblockheader (genesis)["time"]
    assert genesisTime < self.time
    set_node_times (self.nodes, genesisTime)

    self.nodes[0].setgenerate (True, 1)
    sync_blocks (self.nodes)
    self.nodes[1].setgenerate (True, 1)
    sync_blocks (self.nodes)
    self.mine_blocks (25)
    assert_equal (self.nodes[0].getbalance (), 1250)
    assert_equal (self.nodes[1].getbalance (), 1250)

    self.setup_masternode (0, 0, "mn1", "copper")
    self.setup_masternode (1, 1, "mn2", "copper")
    self.mine_blocks (15)
    set_node_times (self.nodes, self.time)
    self.mine_blocks (1)

  def start_masternodes (self):
    print ("Starting masternodes...")

    self.stop_masternode_daemons ()
    self.start_masternode_daemons ()
    self.connect_masternodes_to_peers ([2, 3, 4], updateMockTime=True)

    # Start the masternodes after the nodes are back up and connected
    # (so they will receive each other's broadcast).
    assert_equal (self.broadcast_start ("mn1", True)["status"], "success")
    assert_equal (self.broadcast_start ("mn2", True)["status"], "success")

    # Check status of the masternodes themselves.
    self.wait_for_masternodes_to_be_locally_active (updateMockTime=True)

    # Both masternodes should see each other, independent of the
    # protocol version used.
    def sortByTxhash (entry):
      return entry["txhash"]
    lists = [
      sorted (self.wait_for_mn_list_to_sync (self.nodes[i], 2), key=sortByTxhash)
      for i in [0, 1]
    ]
    assert_equal (lists[0], lists[1])

    lst = lists[0]
    assert_equal (len (lst), 2)
    for val in lst:
      assert_equal (val["tier"], "COPPER")
      assert_equal (val["status"], "ENABLED")


if __name__ == '__main__':
  CustomAddressUpdateTest ().main ()
