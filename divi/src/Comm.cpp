
// Copyright (c) 2017-2020 The DIVI Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <Comm.h>

CComm::CComm(SOCKET hSocketIn, CAddress addrIn){
    socket = hSocketIn;
    address = addrIn;
};

SOCKET CComm::getSocket() {
    return socket;    
}