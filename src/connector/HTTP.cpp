/*
 * HTTP.cpp
 *
 *  Created on: May 22, 2014
 *      Author: dak
 */


#include <string>
#include <vector>
#include <unordered_map>
#include "CommonTypes.h"
#include "connector/Base64.h"
#include "connector/Url.h"
#include "connector/HTTP.h"

/**
 * @class HTTP HTTP.h
 *
 * simple helper class of HTTP constants and useful structures
 */


inline bool iscrlf(char c) { return (c=='\n' || c== '\r'); }
inline bool isrvsep(char c) { return (c==':' || c== ' '); }

std::string
HTTP::PostData::Serialize() {
	std::string s;
	bool first = true;
	auto it=begin();
	while (it!=end()) {
		if (first) {
			first = false;
		} else {
			s += "&";
		}
		s += it->first + "=" + Url::Encode(it->second);
		++it;
	}
	return s;
}

/**
 * create a string message to send on http ... TODO perhaps occasional uses for the body being binary, but not right now
 */
std::string
HTTP::Message(std::string req, std::string host, std::string res, HTTP::Headers headers, std::string body)
{
	std::string msg;

	msg = req+" "+res+" HTTP/1.1\r\n";
	bool hasContentType = false;
	bool hasContentLength = false;
	msg += HTTP_HEADER_HOST ": " + host+"\r\n";
	for (auto it: headers) {
		msg += it.first+": "+it.second+"\r\n";
		if (it.first == HTTP_HEADER_CONTENT_TYPE) {
			hasContentType = true;
		} else if (it.first == HTTP_HEADER_CONTENT_LENGTH) {
			hasContentLength = true;
		}
	}

	if (body.size() > 0) {
		if (!hasContentType) {
			msg += HTTP_HEADER_CONTENT_TYPE ": " "text/plain" "\r\n";
		}
		if (!hasContentLength) {
			msg += HTTP_HEADER_CONTENT_LENGTH ": " + std_to_string((unsigned)body.size()) + "\r\n";
		}
		msg += "\r\n";
		msg.append(body);
	}
	msg += "\r\n";

	return msg;
}


int
HTTP::SplitResponseHeaders(const std::string& response, HTTP::Response& r)
{
	std::string request;
	auto it=response.begin();
	bool hasV = false;
	for (;it != response.end() && !isspace(*it); ++it ) {
		if (*it == '/') {
			hasV = true;
		} else if (hasV) {
			r.version.push_back(*it);
		} else {
			request.push_back(*it);
		}
	}
	for (;it != response.end() && isspace(*it); ++it ) {}
	if (request.find("HTTP") == std::string::npos) {
		return -1;
	}
	for (;it != response.end() && !isspace(*it); ++it ) {
		r.responseCode.push_back(*it);
	}
	for (;it != response.end() && isspace(*it); ++it ) {}
	for (;it != response.end() && !iscrlf(*it); ++it ) {
		r.statusMessage.push_back(*it);
	}
	for (;it != response.end() && iscrlf(*it); ++it ) {}

	std::string key;
	std::string value;
	while (it != response.end()) {
		key = "";
		value = "";
		for (;it != response.end() && *it != ':'; ++it ) {
			key.push_back(*it);
		}
		for (;it != response.end() && isrvsep(*it); ++it ) {}
		for (;it != response.end() && !iscrlf(*it); ++it ) {
			value.push_back(*it);
		}
		r.headers[key] = value;
		if (it != response.end()) { // either \r or \n
			++it;
			if (it != response.end() && *it == '\n') ++it;
			if (it != response.end()) { // we should be either at a second crlf or at the start of the next header item
				if (iscrlf(*it)) {
					++it;
					if (it != response.end() && *it == '\n') ++it;
					// now at the start of the message body, if it is in this string
					break;
				}
			}
		}
	}
	auto jt=r.headers.find(HTTP_HEADER_CONTENT_LENGTH);
	if (jt != r.headers.end()) {
		r.contentLength = atoi(jt->second.c_str());
	}
	jt=r.headers.find(HTTP_HEADER_CONTENT_TYPE);
	if (jt != r.headers.end()) {
		r.contentType = jt->second;
	}
	return (int)(it-response.begin());
}

/**
 * @return an error message if the link is in an error state. if we are in a good way, returns a null string
 */
std::string
HTTP::ErrorResponseMessage(std::string code)
{
	std::string m;
	//3xx: Redirection
	if (code == HTTP_RESPONSE_MULTIPLE_CHOICES) m = " A link list. The user can select a link and go to that location. Maximum five addresses";
	else if (code == HTTP_RESPONSE_MOVED_PERMANENTLY) m = " The requested page has moved to a new URL";
	else if (code == HTTP_RESPONSE_FOUND) m = " The requested page has moved temporarily to a new URL";
	else if (code == HTTP_RESPONSE_SEE_OTHER) m = " The requested page can be found under a different URL";
	else if (code == HTTP_RESPONSE_NOT_MODIFIED) m = " Indicates the requested page has not been modified since last requested";
	else if (code == HTTP_RESPONSE_SWITCH_PROXY) m = " No longer used";
	else if (code == HTTP_RESPONSE_TEMPORARY_REDIRECT) m = " The requested page has moved temporarily to a new URL";
	else if (code == HTTP_RESPONSE_RESUME_INCOMPLETE) m = " Used in the resumable requests proposal to resume aborted PUT or POST requests";
	//4xx: Client Error
	else if (code == HTTP_RESPONSE_BAD_REQUEST) m = " The request cannot be fulfilled due to bad syntax";
	else if (code == HTTP_RESPONSE_UNAUTHORIZED) m = " The request was a legal request, but the server is refusing to respond to it. For use when authentication is possible but has failed or not yet been provided";
	else if (code == HTTP_RESPONSE_PAYMENT_REQUIRED) m = " Reserved for future use";
	else if (code == HTTP_RESPONSE_FORBIDDEN) m = " The request was a legal request, but the server is refusing to respond to it";
	else if (code == HTTP_RESPONSE_NOT_FOUND) m = " The requested page could not be found but may be available again in the future";
	else if (code == HTTP_RESPONSE_METHOD_NOT_ALLOWED) m = " A request was made of a page using a request method not supported by that page";
	else if (code == HTTP_RESPONSE_NOT_ACCEPTABLE) m = " The server can only generate a response that is not accepted by the client";
	else if (code == HTTP_RESPONSE_PROXY_AUTHENTICATION_REQUIRED) m = " The client must first authenticate itself with the proxy";
	else if (code == HTTP_RESPONSE_REQUEST_TIMEOUT) m = " The server timed out waiting for the request";
	else if (code == HTTP_RESPONSE_CONFLICT) m = " The request could not be completed because of a conflict in the request";
	else if (code == HTTP_RESPONSE_GONE) m = " The requested page is no longer available";
	else if (code == HTTP_RESPONSE_LENGTH_REQUIRED) m = " The \"Content-Length\" is not defined. The server will not accept the request without it";
	else if (code == HTTP_RESPONSE_PRECONDITION_FAILED) m = " The precondition given in the request evaluated to false by the server";
	else if (code == HTTP_RESPONSE_REQUEST_ENTITY_TOO_LARGE) m = " The server will not accept the request, because the request entity is too large";
	else if (code == HTTP_RESPONSE_REQUEST_URI_TOO_LONG) m = " The server will not accept the request, because the URL is too long. Occurs when you convert a POST request to a GET request with a long query information";
	else if (code == HTTP_RESPONSE_UNSUPPORTED_MEDIA_TYPE) m = " The server will not accept the request, because the media type is not supported";
	else if (code == HTTP_RESPONSE_REQUESTED_RANGE_NOT_SATISFIABLE) m = " The client has asked for a portion of the file, but the server cannot supply that portion";
	else if (code == HTTP_RESPONSE_EXPECTATION_FAILED) m = " The server cannot meet the requirements of the Expect request-header field";
	//5xx: Server Error
	else if (code == HTTP_RESPONSE_INTERNAL_SERVER_ERROR) m = " A generic error message, given when no more specific message is suitable";
	else if (code == HTTP_RESPONSE_NOT_IMPLEMENTED) m = " The server either does not recognize the request method, or it lacks the ability to fulfill the request";
	else if (code == HTTP_RESPONSE_BAD_GATEWAY) m = " The server was acting as a gateway or proxy and received an invalid response from the upstream server";
	else if (code == HTTP_RESPONSE_SERVICE_UNAVAILABLE) m = " The server is currently unavailable (overloaded or down)";
	else if (code == HTTP_RESPONSE_GATEWAY_TIMEOUT) m = " The server was acting as a gateway or proxy and did not receive a timely response from the upstream server";
	else if (code == HTTP_RESPONSE_HTTP_VERSION_NOT_SUPPORTED) m = " The server does not support the HTTP protocol version used in the request";
	else if (code == HTTP_RESPONSE_NETWORK_AUTHENTICATION_REQUIRED) m = " The client needs to authenticate to gain network access";

	return m;
}

bool
HTTP::IsCompleteResponse(const std::string& response)
{
	return response.find_last_of("\r\n\r\n") != std::string::npos;
}

