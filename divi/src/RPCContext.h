#ifndef RPC_CONTEXT_H
#define RPC_CONTEXT_H

class CSporkManager;

/** This class holds all the "context" (such as the chainstate, masternode
 *  and other references) needed to handle RPC method calls.  */
class RPCContext
{

private:

  CSporkManager& sporkManager;

public:

  /** Constructs a new instance based on globals.  */
  RPCContext ();

  RPCContext (const RPCContext&) = delete;
  void operator= (const RPCContext&) = delete;

  ~RPCContext ();

  /** Returns the global instance that should be used during RPC calls
   *  (called from the RPC method handler).
   *
   *  This method is implemented in rpcserver.cpp.
   */
  static RPCContext& Get ();

  CSporkManager&
  SporkManager ()
  {
    return sporkManager;
  }

};

#endif // RPC_CONTEXT_H
