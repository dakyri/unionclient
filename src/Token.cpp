/*
 * Token.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#include "Token.h"

/**
 * @class Token Token.h
 * wrapper class for tokens used in UPC comms
 */

const char * Token::RS = "|"; /** main separator in serialized lists */
const char * Token::WILDCARD = "*";  /** wildcard in serialized lists */
const char * Token::GLOBAL_ATTR = ""; /** prefix for global attributes */
const char * Token::CUSTOM_CLASS_ATTR = "_CLASS"; /** what it says */
const char * Token::MAX_CLIENTS_ATTR = "_MAX_CLIENTS"; /** what it says */
const char * Token::REMOVE_ON_EMPTY_ATTR = "_DIE_ON_EMPTY"; /** what it says */
const char * Token::PASSWORD_ATTR = "_PASSWORD"; /** what it says */
const char * Token::ROLES_ATTR = "_ROLES"; /** what it says */
