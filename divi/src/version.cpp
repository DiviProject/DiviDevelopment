#include <version.h>

#include <chain.h>
#include <chainparams.h>
#include <ForkActivation.h>
#include <Settings.h>

extern CChain chainActive;
extern Settings& settings;

int ActiveProtocol()
{
    if (settings.ParameterIsSet("-activeversion"))
        return settings.GetArg("-activeversion", 0);

    /* On regtest, we set the new protocol as active without any further
       ado.  This allows proper testing of the changed logic.  */
    if (Params().NetworkID() == CBaseChainParams::REGTEST)
        return MN_REWARD_SCRIPT_VERSION;

    /* Otherwise, the protocol update is tied to time-based activation of
       a network fork (which in fact does not fork consensus but just the
       network protocol).  */
    const CBlockIndex* tip = chainActive.Tip();
    if (tip != nullptr && ActivationState(tip).IsActive(Fork::CustomRewardAddresses))
        return MN_REWARD_SCRIPT_VERSION;

    return MIN_PEER_PROTO_VERSION_AFTER_ENFORCEMENT;
}
