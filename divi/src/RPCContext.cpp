#include "RPCContext.h"

#include "spork.h"

RPCContext::RPCContext ()
  : sporkManager(GetSporkManager ())
{}

RPCContext::~RPCContext () = default;
