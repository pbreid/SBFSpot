/************************************************************************************************
    SBFspot - Yet another tool to read power production of SMA solar inverters
    (c)2012-2025, SBF

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

#include "osselect.h" // #define _USE_32BIT_TIME_T on windows
#include "mqtt.h"
#include "SBFspot.h"
#include <boost/algorithm/string.hpp>
#include "mppt.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <cstdio>

MqttExport::MqttExport(const Config& config)
    : m_config(config)
{
}

MqttExport::~MqttExport()
{
}

int MqttExport::exportInverterData(const std::vector<InverterData>& inverterData)
{
    int rc = 0;

    // Split message body
    std::vector<std::string> items;
    boost::split(items, m_config.mqtt_publish_data, boost::is_any_of(","));

    std::stringstream mqtt_message;
    std::string key;
    char value[80];
    const int prec = m_config.precision;
    const char dp = '.';

    for (const auto& inv : inverterData)
    {
#if defined(_WIN32)
        std::string mqtt_command_line = "\"\"" + m_config.mqtt_publish_exe + "\" " + m_config.mqtt_publish_args + "\"";
#else
        std::string mqtt_command_line = m_config.mqtt_publish_exe + " " + m_config.mqtt_publish_args;
        // On Linux, message must be inside single quotes
        boost::replace_all(mqtt_command_line, "\"", "'");
#endif

        // Fill host/port/topic
        boost::replace_first(mqtt_command_line, "{host}", m_config.mqtt_host);
        boost::replace_first(mqtt_command_line, "{port}", m_config.mqtt_port);
        boost::replace_first(mqtt_command_line, "{topic}", m_config.mqtt_topic);

        mqtt_message.str("");

        for (const auto& item : items)
        {
            bool add_to_msg = true;
            key = item;
            memset(value, 0, sizeof(value));
            std::transform(key.begin(), key.end(), key.begin(), [](auto ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

// suppress warning C4307: '*': integral constant overflow
#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4307)
#endif

            switch (djb::hash(key.c_str()))
            {
            case "prgversion"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", m_config.prgVersion);
                break;
            case "plantname"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", m_config.plantname);
                break;
            case "timestamp"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", strftime_t(m_config.DateTimeFormat, time(nullptr)).c_str());
                break;
            case "sunrise"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", strftime_t(m_config.DateTimeFormat, to_time_t(m_config.sunrise)).c_str());
                break;
            case "sunset"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", strftime_t(m_config.DateTimeFormat, to_time_t(m_config.sunset)).c_str());
                break;
            case "invserial"_:
                snprintf(value, sizeof(value) - 1, "%lu", inv.Serial);
                break;
            case "invname"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", inv.DeviceName.c_str());
                break;
            case "invclass"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", inv.DeviceClass.c_str());
                break;
            case "invtype"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", inv.DeviceType.c_str());
                break;
            case "invswver"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", inv.SWVersion.c_str());
                break;
            case "invtime"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", strftime_t(m_config.DateTimeFormat, inv.InverterDatetime).c_str());
                break;
            case "invstatus"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", tagdefs.getDesc(inv.DeviceStatus, "?").c_str());
                break;
            case "invtemperature"_:
                FormatFloat(value, is_NaN(inv.Temperature) ? 0.0f : (float)inv.Temperature / 100, 0, prec, dp);
                break;
            case "invgridrelay"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", tagdefs.getDesc(inv.GridRelayStatus, "?").c_str());
                break;
            case "pdc1"_:
                FormatFloat(value, (float)inv.mpp.at(1).Pdc(), 0, prec, dp);
                break;
            case "pdc2"_:
                FormatFloat(value, (float)inv.mpp.at(2).Pdc(), 0, prec, dp);
                break;
            case "pdctot"_:
                FormatFloat(value, (float)inv.calPdcTot, 0, prec, dp);
                break;
            case "idc1"_:
                FormatFloat(value, (float)inv.mpp.at(1).Idc() / 1000, 0, prec, dp);
                break;
            case "idc2"_:
                FormatFloat(value, (float)inv.mpp.at(2).Idc() / 1000, 0, prec, dp);
                break;
            case "udc1"_:
                FormatFloat(value, (float)inv.mpp.at(1).Udc() / 100, 0, prec, dp);
                break;
            case "udc2"_:
                FormatFloat(value, (float)inv.mpp.at(2).Udc() / 100, 0, prec, dp);
                break;
            case "etotal"_:
                FormatDouble(value, (double)inv.ETotal / 1000, 0, prec, dp);
                break;
            case "etoday"_:
                FormatDouble(value, (double)inv.EToday / 1000, 0, prec, dp);
                break;
            case "pactot"_:
                FormatFloat(value, (float)inv.TotalPac, 0, prec, dp);
                break;
            case "pac1"_:
                FormatFloat(value, (float)inv.Pac1, 0, prec, dp);
                break;
            case "pac2"_:
                FormatFloat(value, (float)inv.Pac2, 0, prec, dp);
                break;
            case "pac3"_:
                FormatFloat(value, (float)inv.Pac3, 0, prec, dp);
                break;
            case "uac1"_:
                FormatFloat(value, (float)inv.Uac1 / 100, 0, prec, dp);
                break;
            case "uac2"_:
                FormatFloat(value, (float)inv.Uac2 / 100, 0, prec, dp);
                break;
            case "uac3"_:
                FormatFloat(value, (float)inv.Uac3 / 100, 0, prec, dp);
                break;
            case "iac1"_:
                FormatFloat(value, (float)inv.Iac1 / 1000, 0, prec, dp);
                break;
            case "iac2"_:
                FormatFloat(value, (float)inv.Iac2 / 1000, 0, prec, dp);
                break;
            case "iac3"_:
                FormatFloat(value, (float)inv.Iac3 / 1000, 0, prec, dp);
                break;
            case "gridfreq"_:
                FormatFloat(value, (float)inv.GridFreq / 100, 0, prec, dp);
                break;
            case "opertm"_:
                FormatDouble(value, (double)inv.OperationTime / 3600, 0, prec, dp);
                break;
            case "feedtm"_:
                FormatDouble(value, (double)inv.FeedInTime / 3600, 0, prec, dp);
                break;
            case "btsignal"_:
                FormatFloat(value, inv.BT_Signal, 0, prec, dp);
                break;
            case "battmpval"_:
                FormatFloat(value, ((float)inv.BatTmpVal) / 10, 0, prec, dp);
                break;
            case "batvol"_:
                FormatFloat(value, ((float)inv.BatVol) / 100, 0, prec, dp);
                break;
            case "batamp"_:
                FormatFloat(value, ((float)inv.BatAmp) / 1000, 0, prec, dp);
                break;
            case "batchastt"_:
                FormatFloat(value, ((float)inv.BatChaStt), 0, prec, dp);
                break;
            case "invwakeuptm"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", strftime_t(m_config.DateTimeFormat, inv.WakeupTime).c_str());
                break;
            case "invsleeptm"_:
                snprintf(value, sizeof(value) - 1, "\"%s\"", strftime_t(m_config.DateTimeFormat, inv.SleepTime).c_str());
                break;
            case "meteringwin"_:
                FormatFloat(value, (float)inv.MeteringGridMsTotWIn, 0, prec, dp);
                break;
            case "meteringwout"_:
                FormatFloat(value, (float)inv.MeteringGridMsTotWOut, 0, prec, dp);
                break;
            case "meteringwtot"_:
                FormatFloat(value, (float)(inv.MeteringGridMsTotWIn - inv.MeteringGridMsTotWOut), 0, prec, dp);
                break;
            case "powerlimit"_:
                FormatFloat(value, (float)inv.PowerLimit, 0, prec, dp);
                break;
            case "powerlimitpct"_:
                FormatFloat(value, inv.PowerLimitPct, 0, prec, dp);
                break;
            case "pdc"_:
                for (const auto& dc : inv.mpp)
                {
                    FormatFloat(value, (float)dc.second.Pdc(), 0, prec, dp);
                    mqtt_message << to_keyvalue(std::string("PDC") + std::to_string(dc.first), value);
                    add_to_msg = false;
                }
                break;
            case "idc"_:
                for (const auto& dc : inv.mpp)
                {
                    FormatFloat(value, (float)dc.second.Idc() / 1000, 0, prec, dp);
                    mqtt_message << to_keyvalue(std::string("IDC") + std::to_string(dc.first), value);
                    add_to_msg = false;
                }
                break;
            case "udc"_:
                for (const auto& dc : inv.mpp)
                {
                    FormatFloat(value, (float)dc.second.Udc() / 100, 0, prec, dp);
                    mqtt_message << to_keyvalue(std::string("UDC") + std::to_string(dc.first), value);
                    add_to_msg = false;
                }
                break;
            case "null"_:   // empty string (MQTT stream to CSV)
                mqtt_message << m_config.mqtt_item_delimiter;
                add_to_msg = false;
                break;
            default:
                add_to_msg = false;
                if (VERBOSE_NORMAL) std::cout << "MQTT: Don't know what to do with '" << item << "'" << std::endl;
                break;
            }

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

            if (add_to_msg)
                mqtt_message << to_keyvalue(item, value);
        }

        boost::replace_first(mqtt_command_line, "{plantname}", m_config.plantname);
        boost::replace_first(mqtt_command_line, "{serial}", std::to_string(inv.Serial));
        boost::replace_first(mqtt_command_line, "{message}", mqtt_message.str().substr(1));

        if (VERBOSE_NORMAL) std::cout << "MQTT: Publishing (" << m_config.mqtt_topic << ')' << m_config.mqtt_item_delimiter << mqtt_message.str().substr(1) << std::endl;

        int system_rc = ::system(mqtt_command_line.c_str());

        if (system_rc != 0) // Error
        {
            std::cout << "MQTT: Failed to execute '" << m_config.mqtt_publish_exe << "' mosquitto client installed?" << std::endl;
            rc = system_rc;
        }
    }

    return rc;
}

std::string MqttExport::to_keyvalue(const std::string key, const std::string value) const
{
    std::string key_value = m_config.mqtt_item_delimiter + m_config.mqtt_item_format;
    boost::replace_all(key_value, "{key}", key);
    boost::replace_first(key_value, "{value}", value);

    boost::replace_all(key_value, "\"\"", "\"");
#if defined(_WIN32)
    // When exporting MQTT stream to CSV, don't use double/double quotes
    if (m_config.mqtt_topic != "CSV")   // Case sensitive!
        boost::replace_all(key_value, "\"", "\"\"");
#endif

    return key_value;
}

time_t MqttExport::to_time_t(float time_f)
{
    auto timestamp = time(nullptr);
    auto localtm = std::localtime(&timestamp);
    localtm->tm_sec = 0;
    localtm->tm_hour = (int)time_f;
    localtm->tm_min = (int)((time_f - (int)time_f) * 60);
    return std::mktime(localtm);
}

// MQTT Command Subscriber Implementation
MqttCommandSubscriber::MqttCommandSubscriber(const Config& config)
    : m_config(config), m_running(false)
{
}

MqttCommandSubscriber::~MqttCommandSubscriber()
{
    stop();
}

int MqttCommandSubscriber::start()
{
    if (!m_config.mqtt_commands_enabled)
        return 0;

    m_running = true;
    m_subscriber_thread = std::thread(&MqttCommandSubscriber::subscribeLoop, this);
    
    if (VERBOSE_NORMAL) 
        std::cout << "MQTT Command Subscriber started" << std::endl;
    
    return 0;
}

void MqttCommandSubscriber::stop()
{
    if (m_running)
    {
        m_running = false;
        if (m_subscriber_thread.joinable())
            m_subscriber_thread.join();
        
        if (VERBOSE_NORMAL) 
            std::cout << "MQTT Command Subscriber stopped" << std::endl;
    }
}

bool MqttCommandSubscriber::hasCommands() const
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    return !m_command_queue.empty();
}

MqttCommand MqttCommandSubscriber::getNextCommand()
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (m_command_queue.empty())
        return MqttCommand{};
    
    MqttCommand cmd = m_command_queue.front();
    m_command_queue.pop();
    return cmd;
}

void MqttCommandSubscriber::subscribeLoop()
{
    std::string mqtt_command_line = m_config.mqtt_subscribe_exe + " " + m_config.mqtt_subscribe_args;
    
    // Replace placeholders in command line
    boost::replace_first(mqtt_command_line, "{host}", m_config.mqtt_host);
    boost::replace_first(mqtt_command_line, "{port}", m_config.mqtt_port);
    boost::replace_first(mqtt_command_line, "{topic}", m_config.mqtt_command_topic);
    
    // For Bluetooth connections, add some connection stability optimizations
    if (m_config.ConnectionType == CT_BLUETOOTH)
    {
        // Add keepalive for Bluetooth connections
        mqtt_command_line += " -k 30";
    }
    
    if (VERBOSE_NORMAL)
        std::cout << "MQTT Subscribe Command: " << mqtt_command_line << std::endl;
    
    while (m_running)
    {
        FILE* pipe = popen(mqtt_command_line.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Failed to start MQTT subscriber: " << mqtt_command_line << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }
        
        char buffer[1024];
        while (m_running && fgets(buffer, sizeof(buffer), pipe))
        {
            std::string message(buffer);
            // Remove trailing newline
            if (!message.empty() && message.back() == '\n')
                message.pop_back();
            
            MqttCommand cmd;
            if (parseCommand(message, cmd))
            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                m_command_queue.push(cmd);
                
                if (VERBOSE_NORMAL)
                    std::cout << "MQTT Command received: " << cmd.command 
                              << " for inverter " << cmd.inverter_serial << std::endl;
            }
        }
        
        pclose(pipe);
        if (m_running)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool MqttCommandSubscriber::parseCommand(const std::string& message, MqttCommand& cmd)
{
    try
    {
        // Simple JSON parsing for command format:
        // {"command":"set_power_limit","inverter_serial":"12345678","power_limit_watts":3000,"command_id":"123"}
        
        size_t cmd_pos = message.find("\"command\":");
        size_t serial_pos = message.find("\"inverter_serial\":");
        size_t power_pos = message.find("\"power_limit_watts\":");
        size_t id_pos = message.find("\"command_id\":");
        
        if (cmd_pos == std::string::npos || serial_pos == std::string::npos)
            return false;
        
        // Extract command
        size_t cmd_start = message.find("\"", cmd_pos + 10) + 1;
        size_t cmd_end = message.find("\"", cmd_start);
        if (cmd_start == std::string::npos || cmd_end == std::string::npos)
            return false;
        cmd.command = message.substr(cmd_start, cmd_end - cmd_start);
        
        // Extract inverter serial
        size_t serial_start = message.find("\"", serial_pos + 18) + 1;
        size_t serial_end = message.find("\"", serial_start);
        if (serial_start == std::string::npos || serial_end == std::string::npos)
            return false;
        cmd.inverter_serial = message.substr(serial_start, serial_end - serial_start);
        
        // Extract power limit for set_power_limit command
        if (cmd.command == "set_power_limit" && power_pos != std::string::npos)
        {
            size_t power_start = message.find(":", power_pos + 20) + 1;
            size_t power_end = message.find_first_of(",}", power_start);
            if (power_start != std::string::npos && power_end != std::string::npos)
            {
                std::string power_str = message.substr(power_start, power_end - power_start);
                cmd.power_limit_watts = std::stoul(power_str);
            }
        }
        
        // Extract command ID (optional)
        if (id_pos != std::string::npos)
        {
            size_t id_start = message.find("\"", id_pos + 13) + 1;
            size_t id_end = message.find("\"", id_start);
            if (id_start != std::string::npos && id_end != std::string::npos)
                cmd.command_id = message.substr(id_start, id_end - id_start);
        }
        
        cmd.timestamp = time(nullptr);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing MQTT command: " << e.what() << std::endl;
        return false;
    }
}

void MqttCommandSubscriber::publishResponse(const std::string& inverter_serial, 
                                           const std::string& command_id,
                                           const std::string& status, 
                                           const std::string& message,
                                           uint32_t power_limit_set)
{
    std::string response_topic = m_config.mqtt_response_topic;
    boost::replace_first(response_topic, "{serial}", inverter_serial);
    
    std::stringstream response_json;
    response_json << "{";
    response_json << "\"timestamp\":\"" << strftime_t(m_config.DateTimeFormat, time(nullptr)) << "\",";
    response_json << "\"inverter_serial\":\"" << inverter_serial << "\",";
    if (!command_id.empty())
        response_json << "\"command_id\":\"" << command_id << "\",";
    response_json << "\"status\":\"" << status << "\",";
    response_json << "\"message\":\"" << message << "\"";
    if (power_limit_set > 0)
        response_json << ",\"power_limit_set\":" << power_limit_set;
    response_json << "}";
    
    publishMessage(response_topic, response_json.str());
}

void MqttCommandSubscriber::publishMessage(const std::string& topic, const std::string& message)
{
    std::string mqtt_command_line = m_config.mqtt_publish_exe + " " + m_config.mqtt_publish_args;
    
    // Replace placeholders
    boost::replace_first(mqtt_command_line, "{host}", m_config.mqtt_host);
    boost::replace_first(mqtt_command_line, "{port}", m_config.mqtt_port);
    boost::replace_first(mqtt_command_line, "{topic}", topic);
    boost::replace_first(mqtt_command_line, "{message}", message);
    
    if (VERBOSE_NORMAL)
        std::cout << "MQTT Response: " << message << std::endl;
    
    int system_rc = ::system(mqtt_command_line.c_str());
    if (system_rc != 0)
    {
        std::cerr << "Failed to publish MQTT response: " << mqtt_command_line << std::endl;
    }
}
