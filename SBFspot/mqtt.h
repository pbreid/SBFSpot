/************************************************************************************************
    SBFspot - Yet another tool to read power production of SMA solar inverters
    (c)2012-2022, SBF

    Latest version found at https://github.com/SBFspot/SBFspot

    License: Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)
    http://creativecommons.org/licenses/by-nc-sa/3.0/

    You are free:
        to Share - to copy, distribute and transmit the work
        to Remix - to adapt the work
    Under the following conditions:
    Attribution:
        You must attribute the work in the manner specified by the author or licensor
        (but not in any way that suggests that they endorse you or your use of the work).
    Noncommercial:
        You may not use this work for commercial purposes.
    Share Alike:
        If you alter, transform, or build upon this work, you may distribute the resulting work
        only under the same or similar license to this one.

DISCLAIMER:
    A user of SBFspot software acknowledges that he or she is receiving this
    software on an "as is" basis and the user is not relying on the accuracy
    or functionality of the software for any purpose. The user further
    acknowledges that any use of this software will be at his own risk
    and the copyright owner accepts no responsibility whatsoever arising from
    the use or application of the software.

    SMA is a registered trademark of SMA Solar Technology AG

************************************************************************************************/

#pragma once

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include "hash.h"

struct Config;
struct InverterData;

struct MqttCommand
{
    std::string command;
    std::string inverter_serial;
    uint32_t power_limit_watts;
    std::string command_id;
    time_t timestamp;
};

class MqttExport
{
public:
    MqttExport(const Config& config);
    ~MqttExport();

    int exportInverterData(const std::vector<InverterData>& inverterData);

private:
    const Config& m_config;

    std::string to_keyvalue(const std::string key, const std::string value) const;
    time_t to_time_t(float time_f);
};

class MqttCommandSubscriber
{
public:
    MqttCommandSubscriber(const Config& config);
    ~MqttCommandSubscriber();

    int start();
    void stop();
    bool hasCommands() const;
    MqttCommand getNextCommand();
    void publishResponse(const std::string& inverter_serial, const std::string& command_id, 
                        const std::string& status, const std::string& message, 
                        uint32_t power_limit_set = 0);

private:
    const Config& m_config;
    std::thread m_subscriber_thread;
    std::atomic<bool> m_running;
    std::queue<MqttCommand> m_command_queue;
    mutable std::mutex m_queue_mutex;
    
    void subscribeLoop();
    bool parseCommand(const std::string& message, MqttCommand& cmd);
    void publishMessage(const std::string& topic, const std::string& message);
};
