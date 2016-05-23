/*
This file is part of "Rigs of Rods Server" (Relay mode)

Copyright 2016   Petr Ohlídal
Copyright 2016+  Rigs of Rods Community

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

#include "master-server.h"

#include "config.h"
#include "rornet.h"
#include "logger.h"
#include "http.h"
#include "json/json.h"

#include <assert.h>

namespace MasterServer {

Client::Client():
    m_trust_level(-1),
    m_is_registered(false)
{}

char* BeginUrl(char* buffer, const char* path)
{
    char* pos = buffer;
    pos += sprintf(buffer, "/%s/%s", Config::GetMasterServerUrlPrefix().c_str(), path);
    return pos;
}

int HttpRequest(const char* method, const char* url, const char* content_type, const char* payload, Http::Response* response)
{
    const char* host = Config::GetMasterServerHost().c_str();
    return Http::Request(method, host, url, content_type, payload, response);
}

bool Client::Register()
{
    char url[1000] = { 0 };
    char* url_pos = BeginUrl(url, "server-list");
    url_pos += sprintf(url_pos, "?ip=%s&port=%d&name=%s&terrain-name=%s&max-clients=%d&version=%s&pw=%d",
        Config::getIPAddr().c_str(),
        Config::getListenPort(),
        Config::getServerName().c_str(),
        Config::getTerrainName().c_str(),
        Config::getMaxClients(),
        RORNET_VERSION,
        Config::isPublic());

    Logger::Log(LOG_INFO, "Attempting to register on serverlist...");

    Http::Response response;
    int result_code = HttpRequest(Http::METHOD_POST, url, "application/json", "", &response);
    if (result_code < 0)
    {
        Logger::Log(LOG_ERROR, "Registration failed");
        return false;
    }
    else if (result_code != 200)
    {
        Logger::Log(LOG_INFO, "Registration failed, response: %s", response.GetBody().c_str());
        return false;
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response.GetBody().c_str(), root))
    {
        Logger::Log(LOG_ERROR, "Registration failed, couldn't parse response");
        return false;
    }

    Json::Value trust_level = root["verified-level"];
    Json::Value challenge = root["challenge"];
    if (!root.isObject() || !trust_level.isNumeric() || !challenge.isString())
    {
        Logger::Log(LOG_ERROR, "Registration failed, incorrect response from server");
        return false;
    }

    m_token = challenge.asString();
    m_trust_level = trust_level.asInt();
    m_is_registered = true;
    return true;
}

bool Client::SendHeatbeat(Json::Value user_list)
{
    char url[200] = { 0 };
    BeginUrl(url, "server-list");

    Json::Value data(Json::objectValue);
    data["challenge"] = m_token;
    data["users"] = user_list;

    Http::Response response;
    int result_code = HttpRequest(Http::METHOD_PUT, url, "application/json", data.asCString(), &response);
    if (result_code < 0)
    {
        Logger::Log(LOG_ERROR, "Heatbeat failed");
        return false;
    }
    return true;
}

bool Client::UnRegister()
{
    assert(m_is_registered == true);

    char url[200] = { 0 };
    char* url_pos = BeginUrl(url, "server-list");
    sprintf(url_pos, "?challenge=%s", m_token.c_str());

    Http::Response response;
    int result_code = HttpRequest(Http::METHOD_DELETE, url, "application/json", "", &response);
    if (result_code < 0)
    {
        Logger::Log(LOG_ERROR, "Failed to unregister server");
        return false;
    }
    m_is_registered = false;
    return true;
}

bool RetrievePublicIp(std::string* out_ip)
{
    char url[300] = { 0 };
    BeginUrl(url, "get-public-ip");

    Http::Response response;
    int result_code = HttpRequest(Http::METHOD_GET, url, "application/json", "", &response);
    if (result_code < 0)
    {
        Logger::Log(LOG_ERROR, "Failed to retrieve public IP address");
        return false;
    }
    *out_ip = response.GetBody();
    return true;
}

} // namespace MasterServer

