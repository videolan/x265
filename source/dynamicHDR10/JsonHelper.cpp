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

#include "JsonHelper.h"
#include "sys/stat.h"
#include <fstream>
#include <iostream>
using namespace std;

JsonObject JsonHelper::add(string key, string value, JsonObject &json)
{

    json[key] =  value;
    return json;
}

bool JsonHelper::writeJson(JsonObject json, string path)
{
    if(!validatePathExtension(path))
    {
        return false;
    }

    ofstream JsonSaveFile(path);
    string jsonString;
    jsonString = dump(json);
    JsonSaveFile << jsonString;
    JsonSaveFile.close();
    return true;
}

bool JsonHelper::writeJson(JsonArray json, string path)
{
    if(!validatePathExtension(path))
    {
        return false;
    }

    string arrayString = "[";
    size_t jsonSize = json.size();
    for(size_t i = 0; i < jsonSize; ++i)
    {
        arrayString += "\n" + dump(json.at(i).object_items(), 1);
        if(i != jsonSize - 1)
        {
            arrayString += ",";
        }
    }
    arrayString += "\n]";

    ofstream JsonSaveFile(path);
    JsonSaveFile << arrayString;
    JsonSaveFile.close();
    return true;
}

string JsonHelper::dump(JsonObject json, int extraTab)
{
    string dump = Json(json).dump();
    //[defensive programming] possibly check if dump returns a invalid string
    string out = "";
    unsigned int tabCounter = extraTab;
    printTabs(out,tabCounter);
    for(unsigned int i = 0; i < dump.size(); ++i)
    {
        if(dump[i] == '{' || dump[i] == '[')
        {            
            out += dump[i];
            out += '\n';
            ++tabCounter;
            printTabs(out, tabCounter);
        }
        else if(dump[i] == '}' || dump[i] == ']')
        {
            out += '\n';
            --tabCounter;
            printTabs(out, tabCounter);
            out += dump[i];
        }
        else if(dump[i] == ',')
        {
            out += dump[i];
            out += '\n';
            printTabs(out, tabCounter);
            ++i; //skip blank space following comma
        }
        else
        {
            out += dump[i];
        }
    }
    return out;
}

void JsonHelper::printTabs(string &out, int tabCounter)
{
    for(int i = 0; i < tabCounter; ++i)
    {
        out += '\t';
    }
}

JsonObject JsonHelper::readJson(string path)
{
    std::size_t ext = path.find_last_of('.');
    std::string extension = path.substr(ext+1);

    if(extension.compare("json") && extension.compare("JSON"))
    {
        std::cout << "Fail open file, extension not valid!" << std::endl;
        return JsonObject();
    }

    struct stat buffer;
    if(stat(path.c_str(),&buffer))
    {
        std::cout << "Fail open file, file doesn't exist" << std::endl;
        return JsonObject();
    }

    std::ifstream ifs(path);
    const std::string json_str2((std::istreambuf_iterator<char>(ifs)),
                                (std::istreambuf_iterator<char>()));

    string err = "";

    return Json::parse(json_str2,err, JsonParse::COMMENTS).object_items();
}

JsonArray JsonHelper::readJsonArray(const string &path)
{
    std::size_t ext = path.find_last_of('.');
    std::string extension = path.substr(ext+1);

    if(extension.compare("json") && extension.compare("JSON"))
    {
        std::cout << "Fail open file, extension not valid!" << std::endl;
        return JsonArray();
    }

    struct stat buffer;
    if(stat(path.c_str(),&buffer))
    {
        std::cout << "Fail open file, file doesn't exist" << std::endl;
        return JsonArray();
    }

    std::ifstream ifs(path);
    const std::string json_str2((std::istreambuf_iterator<char>(ifs)),
                                (std::istreambuf_iterator<char>()));

    string err = "";

    return Json::parse(json_str2,err, JsonParse::COMMENTS).array_items();
}

bool JsonHelper::validatePathExtension(string &path)
{

    if(path.empty())
    {
        std::cout << "Fail saving json file" << std::endl;
        return false;
    }
    std::size_t ext = path.find_last_of('.');
    std::string realFilePath = path;
    if(ext == std::string::npos)
    {
        realFilePath += ".json";
    }
    else
    {
        std::string extension = path.substr(ext+1);
        if(extension.compare("json") && extension.compare("JSON"))
        {
            std::cout << "Fail saving file, extension not valid!" << std::endl;
            return false;
        }
    }
    path = realFilePath;
    return true;
}
