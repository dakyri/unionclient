/*
 * UCLowerTypes.h
 *
 *  Created on: Apr 24, 2014
 *      Author: dak
 */

#ifndef UCLOWERTYPES_H_
#define UCLOWERTYPES_H_

typedef AbstractConnection* CnxRef;

typedef int ConnectionStatus;

typedef NXR<CnxRef, std::string, ConnectionStatus> NXConnection;
typedef NXConnection::CB CBConnection;
typedef std::shared_ptr<CBConnection> CBConnectionRef;

#ifndef DEBUG_OUT
#ifdef DEBUG_ENABLE
#define DEBUG_OUT(X) (std::cerr << X << std::endl)
#else
#define DEBUG_OUT(X)
#endif
#endif

#endif /* UCLOWERTYPES_H_ */
