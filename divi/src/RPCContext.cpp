#include "RPCContext.h"

#include "spork.h"
#include "MasternodeModule.h"

RPCContext::RPCContext ()
  : sporkManager(GetSporkManager ()), masternodeModule(GetMasternodeModule ())
{}

RPCContext::~RPCContext () = default;
