/*
 * HTTP.h
 *
 *  Created on: May 21, 2014
 *      Author: dak
 */

#ifndef HTTP_H_
#define HTTP_H_

#define HTTP_HEADER_CONTENT_TYPE "Content-Type"
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length"
#define HTTP_HEADER_HOST "Host"
#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_POST "POST"

#define HTTP_RESPONSE_CONTINUE "100" /**  The server has received the request headers, and the client should proceed to send the request body */
#define HTTP_RESPONSE_SWITCHING_PROTOCOLS "101" /**  The requester has asked the server to switch protocols */
#define HTTP_RESPONSE_CHECKPOINT "103" /**  Used in the resumable requests proposal to resume aborted PUT or POST requests */
//2xx: Successful
#define HTTP_RESPONSE_OK "200" /**  The request is OK (this is the standard response for successful HTTP requests) */
#define HTTP_RESPONSE_CREATED "201" /**  The request has been fulfilled, and a new resource is created */
#define HTTP_RESPONSE_ACCEPTED "202" /**  The request has been accepted for processing, but the processing has not been completed */
#define HTTP_RESPONSE_NONAUTHORITATIVE_INFORMATION "203" /**  The request has been successfully processed, but is returning information that may be from another source */
#define HTTP_RESPONSE_NO_CONTENT "204" /**  The request has been successfully processed, but is not returning any content */
#define HTTP_RESPONSE_RESETCONTENT "205" /**  The request has been successfully processed, but is not returning any content, and requires that the requester reset the document view */
#define HTTP_RESPONSE_PARTIALCONTENT "206" /**  The server is delivering only part of the resource due to a range header sent by the client */
//3xx: Redirection
#define HTTP_RESPONSE_MULTIPLE_CHOICES "300" /**  A link list. The user can select a link and go to that location. Maximum five addresses */
#define HTTP_RESPONSE_MOVED_PERMANENTLY "301" /**  The requested page has moved to a new URL */
#define HTTP_RESPONSE_FOUND "302" /**  The requested page has moved temporarily to a new URL */
#define HTTP_RESPONSE_SEE_OTHER "303" /**  The requested page can be found under a different URL */
#define HTTP_RESPONSE_NOT_MODIFIED "304" /**  Indicates the requested page has not been modified since last requested */
#define HTTP_RESPONSE_SWITCH_PROXY "306" /**  No longer used */
#define HTTP_RESPONSE_TEMPORARY_REDIRECT "307" /**  The requested page has moved temporarily to a new URL */
#define HTTP_RESPONSE_RESUME_INCOMPLETE "308" /**  Used in the resumable requests proposal to resume aborted PUT or POST requests */
//4xx: Client Error
#define HTTP_RESPONSE_BAD_REQUEST "400" /**  The request cannot be fulfilled due to bad syntax */
#define HTTP_RESPONSE_UNAUTHORIZED "401" /**  The request was a legal request, but the server is refusing to respond to it. For use when authentication is possible but has failed or not yet been provided */
#define HTTP_RESPONSE_PAYMENT_REQUIRED "402" /**  Reserved for future use */
#define HTTP_RESPONSE_FORBIDDEN "403" /**  The request was a legal request, but the server is refusing to respond to it */
#define HTTP_RESPONSE_NOT_FOUND "404" /**  The requested page could not be found but may be available again in the future */
#define HTTP_RESPONSE_METHOD_NOT_ALLOWED "405" /**  A request was made of a page using a request method not supported by that page */
#define HTTP_RESPONSE_NOT_ACCEPTABLE "406" /**  The server can only generate a response that is not accepted by the client */
#define HTTP_RESPONSE_PROXY_AUTHENTICATION_REQUIRED "407" /**  The client must first authenticate itself with the proxy */
#define HTTP_RESPONSE_REQUEST_TIMEOUT "408" /**  The server timed out waiting for the request */
#define HTTP_RESPONSE_CONFLICT "409" /**  The request could not be completed because of a conflict in the request */
#define HTTP_RESPONSE_GONE "410" /**  The requested page is no longer available */
#define HTTP_RESPONSE_LENGTH_REQUIRED "411" /**  The "Content-Length" is not defined. The server will not accept the request without it */
#define HTTP_RESPONSE_PRECONDITION_FAILED "412" /**  The precondition given in the request evaluated to false by the server */
#define HTTP_RESPONSE_REQUEST_ENTITY_TOO_LARGE "413" /**  The server will not accept the request, because the request entity is too large */
#define HTTP_RESPONSE_REQUEST_URI_TOO_LONG "414" /**  The server will not accept the request, because the URL is too long. Occurs when you convert a POST request to a GET request with a long query information */
#define HTTP_RESPONSE_UNSUPPORTED_MEDIA_TYPE "415" /**  The server will not accept the request, because the media type is not supported */
#define HTTP_RESPONSE_REQUESTED_RANGE_NOT_SATISFIABLE "416" /**  The client has asked for a portion of the file, but the server cannot supply that portion */
#define HTTP_RESPONSE_EXPECTATION_FAILED "417" /**  The server cannot meet the requirements of the Expect request-header field */
//5xx: Server Error
#define HTTP_RESPONSE_INTERNAL_SERVER_ERROR "500" /**  A generic error message, given when no more specific message is suitable */
#define HTTP_RESPONSE_NOT_IMPLEMENTED "501" /**  The server either does not recognize the request method, or it lacks the ability to fulfill the request */
#define HTTP_RESPONSE_BAD_GATEWAY "502" /**  The server was acting as a gateway or proxy and received an invalid response from the upstream server */
#define HTTP_RESPONSE_SERVICE_UNAVAILABLE "503" /**  The server is currently unavailable (overloaded or down) */
#define HTTP_RESPONSE_GATEWAY_TIMEOUT "504" /**  The server was acting as a gateway or proxy and did not receive a timely response from the upstream server */
#define HTTP_RESPONSE_HTTP_VERSION_NOT_SUPPORTED "505" /**  The server does not support the HTTP protocol version used in the request */
#define HTTP_RESPONSE_NETWORK_AUTHENTICATION_REQUIRED "511" /**  The client needs to authenticate to gain network access */

#include "Url.h"
class HTTP {
public:
	typedef std::unordered_map<std::string, std::string> Headers;
	typedef std::string Body;
	typedef std::string ContentType;

	struct PostData: public std::unordered_map<std::string, std::string>
	{
		std::string Serialize();
	};

	struct Response
	{
	public:
		void Init() {
			version = "";
			responseCode = "";
			statusMessage = "";
			headers.clear();
			contentLength = 0;
			body = "";
			contentType = "";
		}
		std::string version;
		std::string responseCode;
		std::string statusMessage;
		Headers headers;
		ContentType contentType;
		size_t contentLength = 0;
		Body body;
	};

	static bool IsCompleteResponse(const std::string& response);
	static int SplitResponseHeaders(const std::string& response, HTTP::Response& r);
	static std::string Message(std::string req, std::string host, std::string res, HTTP::Headers headers, std::string body="");
	static std::string ErrorResponseMessage(std::string code);
};



#endif /* HTTP_H_ */
