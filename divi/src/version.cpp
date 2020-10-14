#include <version.h>

#include <chainparams.h>
#include <Settings.h>

extern Settings& settings;

// Note: whenever a protocol update is needed toggle between both implementations (comment out the formerly active one)
//       so we can leave the existing clients untouched (old SPORK will stay on so they don't see even older clients).
//       Those old clients won't react to the changes of the other (new) SPORK because at the time of their implementation
//       it was the one which was commented out
int ActiveProtocol()
{
    if (settings.ParameterIsSet("-activeversion"))
        return settings.GetArg("-activeversion", 0);

    /* On regtest, we set the new protocol as active without any further
       ado.  This allows proper testing of the changed logic.  */
    if (Params().NetworkID() == CBaseChainParams::REGTEST)
        return MN_REWARD_SCRIPT_VERSION;

    return MIN_PEER_PROTO_VERSION_AFTER_ENFORCEMENT;
}
