// Copyright (c) 2014-2016 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ACTIVEMASTERNODE_H
#define ACTIVEMASTERNODE_H

#include <pubkey.h>
#include <masternodeconfig.h>
#include <netbase.h>
#include <primitives/transaction.h>
#include <sync.h>

#define ACTIVE_MASTERNODE_INITIAL 0 // initial state
#define ACTIVE_MASTERNODE_SYNC_IN_PROCESS 1
#define ACTIVE_MASTERNODE_INPUT_TOO_NEW 2
#define ACTIVE_MASTERNODE_NOT_CAPABLE 3
#define ACTIVE_MASTERNODE_STARTED 4

class CMasternodeBroadcast;

// Responsible for activating the Masternode and pinging the network
class CActiveMasternode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    /// Ping Masternode
    bool SendMasternodePing(std::string& errorMessage);

    CMasternodeConfig& masternodeConfigurations_;

    bool& fMasterNode_;
public:
    // Initialized by init.cpp
    // Keys for the main Masternode
    CPubKey pubKeyMasternode;

    // Initialized while registering Masternode
    CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveMasternode(
        CMasternodeConfig& masternodeConfigurations,
        bool& masterNodeEnabled
        ): masternodeConfigurations_(masternodeConfigurations)
        , fMasterNode_(masterNodeEnabled)
    {
        status = ACTIVE_MASTERNODE_INITIAL;
    }

    /// Manage status of main Masternode
    void ManageStatus();
    std::string GetStatus();

    /// Register any Masternode
    static bool Register(CMasternodeBroadcast &mnb, bool deferRelay = false);

    /// Enable cold wallet mode (run a Masternode with no funds)
    bool EnableHotColdMasterNode(CTxIn& vin, CService& addr);
};

extern CActiveMasternode activeMasternode;

#endif
