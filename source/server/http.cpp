/*
This file is part of "Rigs of Rods Server" (Relay mode)

Copyright 2007   Pierre-Michel Ricordel
Copyright 2014+  Rigs of Rods Community

"Rigs of Rods Server" is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

"Rigs of Rods Server" is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar. If not, see <http://www.gnu.org/licenses/>.
*/

#include "http.h"

#include "utils.h"
#include "logger.h"
#include "SocketW.h"

#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

namespace Http {

bool Request(
    const char* method,
    const char* host,
    const char* url,
    const char* content_type,
    const char* payload,
    char*       out_response_buffer,
    unsigned    response_buf_len)
{
    method = (method == nullptr) ? METHOD_GET : method;

    SWInetSocket socket;
    SWInetSocket::SWBaseError result;
    if (!socket.connect(80, host, &result) || (result != SWInetSocket::ok))
    {
        Logger::Log(LOG_ERROR,
            "Could not process HTTP %s request %s%s failed, error: ",
            method, host, url, result.get_error().c_str());
        return false;
    }
    char query[2000] = {0};
    char* query_pos = query;

    query_pos += sprintf(query_pos, "%s %s HTTP/1.1"     "\r\n", method, url);
    query_pos += sprintf(query_pos, "Host: %s"           "\r\n", host);
    query_pos += sprintf(query_pos, "Content-Type: %s"   "\r\n", content_type);
    query_pos += sprintf(query_pos, "Content-Length: %d" "\r\n", strnlen(payload, 16000));

    sprintf(query_pos, "\r\n%s", payload);

    if (socket.sendmsg(query, &result) < 0)
    {
        Logger::Log(LOG_ERROR,
            "Could not process HTTP %s request %s%s failed, error: ",
            method, host, url, result.get_error().c_str());
        return false;
    }

    int response_len = socket.recv(out_response_buffer, response_buf_len, &result);
    if (response_len < 0)
    {
        Logger::Log(LOG_ERROR,
            "Could not process HTTP %s request %s%s failed, invalid response length, error message: ",
            method, host, url, result.get_error().c_str());
        return false;
    }
    out_response_buffer[response_len] = 0;

    socket.disconnect();
    return true;
}

} // namespace Http

HttpMsg::HttpMsg()
{
}

HttpMsg::HttpMsg( const std::string& message )
{
    assign( message );
}

HttpMsg::~HttpMsg()
{
}


HttpMsg& HttpMsg::operator=( const std::string& message )
{
    assign( message );
    return *this;
}

HttpMsg& HttpMsg::operator=( const char* message )
{
    assign( message );
    return *this;
}

bool HttpMsg::operator==( const std::string& message )
{
    return getBody() == message;
}

const std::string& HttpMsg::getBody()
{
    return headermap["body"];
}

const std::vector<std::string> HttpMsg::getBodyLines()
{
    std::vector<std::string> lines;
    strict_tokenize( headermap["body"], lines, "\n" );
    return lines;
}

bool HttpMsg::isChunked()
{
    return "chunked" == headermap["Transfer-Encoding"];
}

void HttpMsg::assign( const std::string& message )
{
    std::size_t locHolder;
    locHolder = message.find("\r\n\r\n");
    std::vector<std::string> header;
    std::vector<std::string> tmp;
    
    strict_tokenize( message.substr( 0, locHolder ), header, "\r\n" );
    
    headermap["httpcode"] = header[0];
    for( unsigned short index = 1; index < header.size(); index++ )
    {
        tmp.clear();
        tokenize( header[index], tmp, ":" );
        if( tmp.size() != 2 )
        {
            continue;
        }
        headermap[ trim( tmp[0] ) ] = trim( tmp[1] );
    }
    
    tmp.clear();
    locHolder = message.find_first_not_of("\r\n", locHolder);
    if( std::string::npos == locHolder )
    {
        std::string error_msg("Message does not appear to contain a body: \n");
        error_msg +=message;
        throw std::runtime_error( error_msg );
    }
    
    strict_tokenize( message.substr( locHolder ), tmp, "\r\n" );
    if( isChunked() )
        headermap["body"] = tmp[1];
    else
        headermap["body"] = tmp[0];
    
#if 0
    // debuging stuff
    std::cout << "headermap contents " << headermap.size() << ": \n"; 
    for( std::map<std::string,std::string>::iterator it = headermap.begin();
        it != headermap.end();
        it++)
    {
        std::cout << "headermap[\"" << (*it).first << "\"]: "
                << (*it).second << std::endl;
        
    }
#endif
}