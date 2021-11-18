#include <version.h>

#include <Logging.h>

static int version = MN_REWARD_SCRIPT_VERSION;
const int& PROTOCOL_VERSION(version);

void SetProtocolVersion(const int newVersion)
{
    LogPrintf("Setting protocol version to %d from %d\n", newVersion, PROTOCOL_VERSION);
    version = newVersion;
}
