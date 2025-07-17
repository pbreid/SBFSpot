# SBFSpot MQTT Power Control Feature

## Overview
This implementation adds MQTT command subscription functionality to SBFSpot, allowing you to remotely control the maximum output power of your SMA 5000TL-21 and 4000TL-21 inverters via MQTT messages.

## Features Added

### 1. MQTT Command Subscription
- Real-time command processing via MQTT
- Support for JSON-formatted commands
- Automatic response publishing with status updates
- Thread-safe command queue processing

### 2. Power Limit Control
- Set maximum output power in watts
- Automatic validation against inverter limits
- Retry logic for Bluetooth connections
- Real-time verification of applied settings

### 3. Power Limit Status Reporting
- Current power limit published via MQTT
- Power limit percentage based on inverter rating
- Integration with existing MQTT data export

### 4. Bluetooth Optimizations
- Connection stability improvements
- Retry logic for command failures
- Automatic delays for Bluetooth reliability

## Configuration

### Enable MQTT Commands
Edit `SBFspot.cfg` and modify these settings:

```ini
# Enable MQTT command subscription
MQTT_Commands=1

# MQTT broker settings (existing)
MQTT_Host=your.mqtt.broker.com
MQTT_Port=1883

# Command and response topics
MQTT_CommandTopic=sbfspot_{serial}/commands
MQTT_ResponseTopic=sbfspot_{serial}/response

# Add power limit data to MQTT exports
MQTT_Data=Timestamp,InvSerial,InvName,EToday,ETotal,PACTot,PowerLimit,PowerLimitPct
```

### Authentication Requirements
Power control requires **installer-level authentication**:

```ini
# Set user group to installer (required for power control)
UserGroup=INSTALLER

# Set installer password (contact your installer)
Password=your_installer_password
```

## Usage

### Command Format
Send JSON commands to the command topic:

```json
{
  "command": "set_power_limit",
  "inverter_serial": "12345678",
  "power_limit_watts": 3000,
  "command_id": "optional_tracking_id"
}
```

### Example Commands

#### Set Power Limit to 3000W
```bash
mosquitto_pub -h your.broker.com -t "sbfspot_12345678/commands" -m '{
  "command": "set_power_limit",
  "inverter_serial": "12345678", 
  "power_limit_watts": 3000,
  "command_id": "cmd_001"
}'
```

#### Set Power Limit to 2500W
```bash
mosquitto_pub -h your.broker.com -t "sbfspot_12345678/commands" -m '{
  "command": "set_power_limit",
  "inverter_serial": "12345678",
  "power_limit_watts": 2500
}'
```

### Response Format
Responses are published to the response topic:

```json
{
  "timestamp": "17/07/2025 10:30:00",
  "inverter_serial": "12345678",
  "command_id": "cmd_001",
  "status": "success",
  "message": "Power limit set to 3000 W",
  "power_limit_set": 3000
}
```

### Error Responses
```json
{
  "timestamp": "17/07/2025 10:30:00",
  "inverter_serial": "12345678",
  "command_id": "cmd_001",
  "status": "error",
  "message": "Power limit 6000 exceeds maximum allowed 5000"
}
```

## Power Limit Validation

The system automatically validates power limits:
- **SB 5000TL-21**: Maximum 5000W
- **SB 4000TL-21**: Maximum 4000W
- Commands exceeding limits are rejected
- Minimum limits are also enforced

## MQTT Data Export

The power limit status is included in regular MQTT data exports:

```json
{
  "InvSerial": "12345678",
  "InvName": "SB 5000TL-21",
  "PACTot": 2800,
  "PowerLimit": 3000,
  "PowerLimitPct": 60.0
}
```

## Bluetooth Connection Tips

For optimal Bluetooth performance:
1. Keep the Bluetooth adapter close to the inverter
2. Ensure stable power supply to the Bluetooth adapter
3. Monitor connection quality via `BT_Signal` in MQTT data
4. Use the built-in retry logic for reliable command execution

## Safety Features

- **Authentication**: Installer-level access required
- **Validation**: Power limits checked against inverter specifications
- **Logging**: All commands and responses logged
- **Verification**: Settings verified after application
- **Error Handling**: Comprehensive error reporting

## Troubleshooting

### Common Issues

1. **Command Not Processed**
   - Check `MQTT_Commands=1` in config
   - Verify MQTT broker connectivity
   - Ensure correct topic format

2. **Permission Denied**
   - Use installer password, not user password
   - Set `UserGroup=INSTALLER` in config

3. **Power Limit Rejected**
   - Check inverter maximum rating
   - Verify inverter is online and responding
   - Review power limit validation messages

4. **Bluetooth Connection Issues**
   - Check Bluetooth signal strength
   - Verify inverter Bluetooth is enabled
   - Try reconnecting or restarting SBFSpot

### Debug Mode
Enable verbose logging for troubleshooting:
```bash
./SBFspot -v3 -mqtt
```

## Integration Example

### Home Assistant Integration
```yaml
mqtt:
  - name: "Inverter Power Limit"
    state_topic: "sbfspot_12345678/data"
    value_template: "{{ value_json.PowerLimit }}"
    unit_of_measurement: "W"
    
  - name: "Inverter Power Limit %"
    state_topic: "sbfspot_12345678/data"
    value_template: "{{ value_json.PowerLimitPct }}"
    unit_of_measurement: "%"

automation:
  - alias: "Set Inverter Power Limit"
    trigger:
      platform: state
      entity_id: input_number.power_limit_setting
    action:
      service: mqtt.publish
      data:
        topic: "sbfspot_12345678/commands"
        payload: >
          {
            "command": "set_power_limit",
            "inverter_serial": "12345678",
            "power_limit_watts": {{ states('input_number.power_limit_setting') | int }}
          }
```

## Advanced Usage

### Script for Automated Control
```bash
#!/bin/bash
INVERTER_SERIAL="12345678"
MQTT_BROKER="your.broker.com"
COMMAND_TOPIC="sbfspot_${INVERTER_SERIAL}/commands"

set_power_limit() {
    local power_limit=$1
    local cmd_id=$(date +%s)
    
    mosquitto_pub -h $MQTT_BROKER -t $COMMAND_TOPIC -m "{
        \"command\": \"set_power_limit\",
        \"inverter_serial\": \"$INVERTER_SERIAL\",
        \"power_limit_watts\": $power_limit,
        \"command_id\": \"script_$cmd_id\"
    }"
}

# Usage examples
set_power_limit 3000  # Set to 3000W
set_power_limit 4000  # Set to 4000W
```

## Support

For issues or questions:
1. Check the SBFSpot verbose logs
2. Verify MQTT broker connectivity
3. Ensure correct inverter serial numbers
4. Review configuration settings
5. Test with simple commands first

## Technical Notes

- Uses existing SBFSpot communication protocol
- Leverages SMA's InverterWLim LRI (0x00832A00)
- Thread-safe command processing
- Integrates with existing MQTT export functionality
- Optimized for Bluetooth connections
- Compatible with SBFSpot 3.10.0+