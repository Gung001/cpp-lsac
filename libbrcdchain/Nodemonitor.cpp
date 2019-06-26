#include "Nodemonitor.h"
#include "libdevcore/RLP.h"
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Common.h>
#include <thread>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonJS.h>
#include <json/json.h>
#include <jsonrpccpp/common/exception.h>
#include <jsonrpccpp/common/errors.h>
#include <time.h>

using namespace dev;
using namespace brc;
using namespace std;
using namespace jsonrpc;

vector<string> split(const string &s, const string &seperator){
    vector<string> result;
    typedef string::size_type string_size;
    string_size i = 0;

    while(i != s.size()){
        int flag = 0;
        while(i != s.size() && flag == 0){
            flag = 1;
            for(string_size x = 0; x < seperator.size(); ++x)
            {
                if(s[i] == seperator[x]){
                    ++i;
                    flag = 0;
                    break;
                    }
            }
        }


        flag = 0;
        string_size j = i;
        while(j != s.size() && flag == 0){
            for(string_size x = 0; x < seperator.size(); ++x)
                if(s[j] == seperator[x]){
                    flag = 1;
                    break;
                    }
            if(flag == 0)
                ++j;
        }
        if(i != j){
            result.push_back(s.substr(i, j-i));
            i = j;
        }
    }
    return result;
}




KeyPair networkAlias(bytes _b)
{
    RLP r(_b);
    if (r.itemCount() == 3 && r[0].isInt() && r[0].toInt<unsigned>() >= 3)
        return KeyPair(Secret(r[1].toBytes()));
    else
        return KeyPair::create();
}


NodeMonitor::NodeMonitor(bytes _networkrlp, std::string _ip):
    m_networkrlp(_networkrlp),
    m_ip(_ip)
{
    cnote << "ip:" << m_ip;
    //m_ipStats = checkIP();
    getClientVersion();
    getKeypair();
    // if(m_ipStats)
    // {
        std::thread t(&NodeMonitor::run, this);
        t.detach();
    // }
}

bool NodeMonitor::checkIP()
{
    vector<string> _v = split(m_ip, ".:");
    if(_v.size() == 5)
    {
        jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(m_ip);
        try
        {
            std::string _post = "hello";
            std::string _ret;
           _httpClient.SendRPCMessage(_post, _ret);
           if(_ret.empty())
           {
                cerror << "The specified node server address is not a valid address";
               return false;
           }
        }
        catch(...)
        {
            cerror << "The specified node server address is not a valid address";
            return false;
        }
        return true;
    }
    return false;
}

void NodeMonitor::getKeypair()
{
    KeyPair _keypair = networkAlias(m_networkrlp);
    m_secret = _keypair.secret();
    m_public = _keypair.pub();
    m_addr = _keypair.address();
}


void NodeMonitor::getClientVersion()
{
    const auto *info = brcd_get_buildinfo();
    m_clientVersion = info->git_commit_hash;
}

void NodeMonitor::setData(monitorData _data)
{
    m_mutex.lock();
    // if(m_ipStats == false)
    // {
    //     m_mutex.unlock();
    //     return;
    // }
    m_data.push_back(_data);
    m_mutex.unlock();
}

Signature NodeMonitor::signatureData() const
{
    if(m_data.size() == 0)
    {
        return Signature();
    }
    monitorData _data = m_data.back();
    RLPStream _rlp(8);
    _rlp << m_public << _data.blocknum << _data.blockhash << _data.time <<m_clientVersion << _data.nodenum << _data.packagetranscations << _data.pendingpoolsnum;
    Signature _sign = sign(m_secret, sha3(_rlp.out()));
    return _sign;
}

std::string NodeMonitor::getNodeStatsStr(Signature _sign)
{
    if(m_data.size() == 0)
    {
        return std::string();
    }
    monitorData _data = m_data.back();
    clock_t time;
    Json::Value _jv;
    _jv["nodeID"] = toJS(m_public);
    _jv["blockNum"] = _data.blocknum;
    _jv["blockHash"] = toJS(_data.blockhash);
    _jv["serverDelay"] = toJS(_data.time);
    _jv["clientVersion"] = m_clientVersion;
    _jv["nodeNum"] = toJS(_data.nodenum);
    _jv["packageTranscations"] = toJS(_data.packagetranscations);
    _jv["pendingpoolsnum"] = toJS(_data.pendingpoolsnum);
    _jv["signatureData"] = toJS(_sign);

    std::string _value = _jv.toStyledString();
    return _value;
}

void NodeMonitor::run()
{
    jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(m_ip);
    int errorNum = 0;
    int inputNum = 0;
    while(1)
    {
        m_mutex.lock();
        if(m_data.size() > 0)
        {
            Signature _sign = signatureData();
            std::string _str = getNodeStatsStr(_sign);
            if(!_str.empty())
            {
                try
                {
                    std::string _ret;
                    _httpClient.SendRPCMessage(_str, _ret);
                    cnote << "http ret:" << _ret;
                    errorNum = 0;
                    inputNum = 0;
                }
                catch(JsonRpcException &e)
                {
                    errorNum++;
                    if(errorNum > 10)
                    {
                        inputNum++;
                        if(inputNum == 10)
                        {
                            cerror << e.GetMessage();
                            inputNum = 0;
                        }
                    }else{
                        cerror << e.GetMessage();
                    }
                }
            }
            //TO DO:signature data
            m_data.clear();
        }

        if(m_threadExit)
        {
            cerror << "thread return";
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
    }
    return;
}