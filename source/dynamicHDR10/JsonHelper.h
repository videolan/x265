/**
 * Copyright (C) 2013-2017 MulticoreWare, Inc
 *
 * Authors: Bhavna Hariharan <bhavna@multicorewareinc.com>
 *          Kavitha Sampath <kavitha@multicorewareinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
**/

#ifndef JSON_H
#define JSON_H
#include <string>
#include <iostream>
#include "json11/json11.h"

using std::string;
using namespace json11;
typedef Json::object JsonObject;
typedef Json::array JsonArray;

class JsonHelper
{
public:
    static JsonObject readJson(string path);
    static JsonArray readJsonArray(const string &path);
    static string dump(JsonArray json);
    static string dump(JsonObject json, int extraTab = 0);

    static bool writeJson(JsonObject json , string path);
    static bool writeJson(JsonArray json, string path);
    static JsonObject add(string key, string value, JsonObject &json);
private:
    static void printTabs(string &out, int tabCounter);
    JsonObject mJson;
    static bool validatePathExtension(string &path);
};

#endif // JSON_H
