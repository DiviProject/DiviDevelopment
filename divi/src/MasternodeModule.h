#ifndef MASTERNODE_MODULE_H
#define MASTERNODE_MODULE_H
#include <string>
#include <primitives/transaction.h>
#include <stdint.h>
#include <vector>
#include <memory>

class Settings;
class CBlockIndex;
class CDataStream;
class CNode;
class CMasternodeSync;
class UIMessenger;
class CMasternodePayments;
class CKeyStore;

class ChainstateManager;
class MasternodeNetworkMessageManager;
class MasternodePaymentData;
class CMasternodeConfig;
class CMasternodeMan;
class CActiveMasternode;
class I_PeerSyncQueryService;
class I_Clock;
class CNetFulfilledRequestManager;
class CAddrMan;
class StoredMasternodeBroadcasts;

class MasternodeModule
{
private:
    mutable int64_t lastBlockVotedOn;
    mutable bool fMasterNode_;
    std::unique_ptr<CNetFulfilledRequestManager> networkFulfilledRequestManager_;
    std::unique_ptr<MasternodeNetworkMessageManager> networkMessageManager_;
    std::unique_ptr<MasternodePaymentData> masternodePaymentData_;
    std::unique_ptr<CMasternodeConfig> masternodeConfig_;
    std::unique_ptr<CActiveMasternode> activeMasternode_;
    std::unique_ptr<CMasternodeSync> masternodeSync_;
    std::unique_ptr<CMasternodeMan> mnodeman_;
    std::unique_ptr<CMasternodePayments> masternodePayments_;
    std::unique_ptr<StoredMasternodeBroadcasts> storedBroadcasts_;
public:
    MasternodeModule(
        const I_Clock& clock,
        const I_PeerSyncQueryService& peerSyncQueryService,
        const ChainstateManager& chainstate,
        CAddrMan& addressManager);
    ~MasternodeModule();

    CNetFulfilledRequestManager& getNetworkFulfilledRequestManager() const;
    MasternodeNetworkMessageManager& getNetworkMessageManager() const;
    MasternodePaymentData& getMasternodePaymentData() const;
    CMasternodeConfig& getMasternodeConfigurations() const;
    CMasternodeMan& getMasternodeManager() const;
    CActiveMasternode& getActiveMasternode() const;
    CMasternodePayments& getMasternodePayments() const;
    CMasternodeSync& getMasternodeSynchronization() const;
    StoredMasternodeBroadcasts& getStoredBroadcasts() const;
    bool localNodeIsAMasternode() const;
    void designateLocalNodeAsMasternode() const;

    // Used in main.cpp for managing p2p signals
    bool voteForMasternodePayee(const CBlockIndex* pindex) const;
    void processMasternodeMessages(CNode* pfrom, std::string strCommand, CDataStream& vRecv) const;
    bool masternodeWinnerIsKnown(const uint256& inventoryHash) const;
    bool masternodeIsKnown(const uint256& inventoryHash) const;
    bool masternodePingIsKnown(const uint256& inventoryHash) const;
    bool shareMasternodeBroadcastWithPeer(CNode* peer,const uint256& inventoryHash) const;
    bool shareMasternodePingWithPeer(CNode* peer,const uint256& inventoryHash) const;
    bool shareMasternodeWinnerWithPeer(CNode* peer,const uint256& inventoryHash) const;

    //Used in rpcmisc.cpp for manual restart of mn sync
    void forceMasternodeResync() const;

    std::vector<COutPoint> getMasternodeAllocationUtxos() const;
};

// Used for downstream constructors and use cases
const MasternodeModule& GetMasternodeModule();

// Used for initialization
void ThreadMasternodeBackgroundSync(const MasternodeModule* mod);
bool LoadMasternodeDataFromDisk(const MasternodeModule& mod, UIMessenger& uiMessenger,std::string pathToDataDir);
void SaveMasternodeDataToDisk(const MasternodeModule& mod);
bool InitializeMasternodeIfRequested(const MasternodeModule& mnModule, const Settings& settings, bool transactionIndexEnabled, std::string& errorMessage);
#endif //MASTERNODE_MODULE_H
