/*MIT License

Copyright (c) 2021-2022 L. E. Spalt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <atomic>
#include <iostream>
#include <fstream>
#include "Config.h"

Config              g_cfg;

Config::Config () :
    mJsonData   (NULL),
    mHasChanged (false),
    mFilename ("config.json")
{}

static void configWatcher( std::atomic<bool>* mHasChanged )
{
    HANDLE dir = CreateFile( ".", FILE_LIST_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    if( dir == INVALID_HANDLE_VALUE )
    {
        printf( "Could not start config watch thread.\n" );
        return;
    }

    std::vector<DWORD> buf( 1024*1024 );
    DWORD bytesReturned = 0;

    while( true )
    {        
        if( ReadDirectoryChangesW( dir, buf.data(), (DWORD)buf.size()/sizeof(DWORD), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL ) )
        {
            Sleep( 100 );  // wait a bit to make sure changes are actually picked up when we reload
            *mHasChanged = true;
        }
    }
}

bool Config::load()
{
    std::ifstream t("config.json");
    try
    {
        mJsonData = json::parse(t);
    }
    catch (json::parse_error)
    {
        return false;
    }

    mHasChanged = false;
    return true;
}

bool Config::save()
{
    try
    {
        std::ofstream output(mFilename);
        output << std::setw(4) << mJsonData << std::endl;
    }
    catch (...)
    {
        printf("failed to save config.json");
        return false;
    }

	return true;
}

void Config::watchForChanges()
{
    mConfigWatchThread = std::thread(configWatcher, &mHasChanged);
    mConfigWatchThread.detach();
}

bool Config::hasChanged()
{
    return mHasChanged;
}

bool Config::getBool( const std::string& component, const std::string& key, bool defaultVal )
{
    setIfMissing(component, key, defaultVal);

    return mJsonData[component][key].get<bool>();
}

template <class V>
void Config::setIfMissing(const std::string& component, const std::string& key, V& val)
{
    if (mJsonData.contains(component))
    {
        // block exists
        if (!mJsonData[component].contains(key))
            mJsonData[component][key] = val;
    }
    else
    {
        mJsonData[component][key] = val;
    }
}

int Config::getInt( const std::string& component, const std::string& key, int defaultVal )
{
    double val = (double)defaultVal;
    setIfMissing(component, key, val);

    return (int)mJsonData[component][key].get<double>();
}

float Config::getFloat( const std::string& component, const std::string& key, float defaultVal )
{
    double val = (double)defaultVal;
    setIfMissing(component, key, val);

    return (float)mJsonData[component][key].get<double>();
}

float4 Config::getFloat4( const std::string& component, const std::string& key, const float4& defaultVal )
{
    bool exists = true;
    if (mJsonData.contains(component))
    {
        // block exists
        if (!mJsonData[component].contains(key))
            exists = false;
    }
    else
        exists = false;

    if (!exists)
    {
        std::array<double, 4> c_array{ {(double)defaultVal.x, (double)defaultVal.y, (double)defaultVal.z, (double)defaultVal.w} };
        json j_array(c_array);
        mJsonData[component][key] = j_array;
    }

    float4 ret = {};
    ret.x = (float)mJsonData[component][key][0].get<double>();
    ret.y = (float)mJsonData[component][key][1].get<double>();
    ret.z = (float)mJsonData[component][key][2].get<double>();
    ret.w = (float)mJsonData[component][key][3].get<double>();

    return ret;
}

std::string Config::getString( const std::string& component, const std::string& key, const std::string& defaultVal )
{
    const char* val = defaultVal.c_str();
    setIfMissing(component, key, val);

    return mJsonData[component][key].get<std::string>();
}

// TODO
/*std::vector<std::string> Config::getStringVec(const std::string& component, const std::string& key, const std::vector<std::string>& defaultVal)
{
    bool existed = false;
    picojson::value& value = getOrInsertValue( component, key, &existed );

    if( !existed )
    {
        picojson::array arr( defaultVal.size() );
        for( int i=0; i<(int)defaultVal.size(); ++i )
            arr[i].set<std::string>( defaultVal[i] );
        value.set<picojson::array>( arr );
    }

    picojson::array& arr = value.get<picojson::array>();
    std::vector<std::string> ret( arr.size() );
    for( picojson::value& entry : arr )
        ret.push_back( entry.get<std::string>() );
    return ret;
}*/

void Config::setInt( const std::string& component, const std::string& key, int v )
{
    double val = (double)v;
    mJsonData[component][key] = val;
}

void Config::setBool( const std::string& component, const std::string& key, bool v )
{
    mJsonData[component][key] = v;
}
