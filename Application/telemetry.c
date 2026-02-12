/*
 * telemetry.c
 * Implementation of Modbus RTU Logic
 * Handles parsing, CRC validation, and command execution.
 */

#include "telemetry.h"
#include "hal_uart.h"
#include "mppt_manager.h"    // To access Voltage/Current
#include "valve_controller.h" // To control Valve
#include "hal_motor.h"       // To access Encoder
#include "hal_adc.h"         // To access Raw Current

// Temporary buffers for processing
static uint8_t rx_data[16];
static uint8_t tx_data[16];

// --- Helper: CRC16 Calculation (Modbus Standard) ---
uint16_t calculate_crc(uint8_t *buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void Telemetry_Init(void) {
    HAL_UART_Init();
}

void Telemetry_Process(void) {
    // 1. Check if mail has arrived
    if (!HAL_UART_IsDataReady()) {
        return;
    }

    // 2. Retrieve the mail
    uint16_t len = HAL_UART_GetData(rx_data);
    if (len < 8) return; // Incomplete packet, ignore.

    // 3. Verify CRC (Signature Check)
    // Calculate CRC of the first 6 bytes
    uint16_t calculated_crc = calculate_crc(rx_data, 6);
    // Combine the last 2 bytes to form received CRC
    uint16_t received_crc = (rx_data[7] << 8) | rx_data[6]; // Modbus is Little Endian for CRC

    if (calculated_crc != received_crc) {
        return; // CRC Error, ignore packet.
    }

    // 4. Verify Device ID
    if (rx_data[0] != DEVICE_ID) {
        return; // Not addressed to us.
    }

    // 5. Parse Command
    uint8_t func_code = rx_data[1];
    uint16_t reg_addr = (rx_data[2] << 8) | rx_data[3];
    uint16_t reg_data = (rx_data[4] << 8) | rx_data[5]; // Data for write command

    uint16_t response_val = 0;
    
    // --- SCENARIO 1: READ HOLDING REGISTERS (0x03) ---
    if (func_code == 0x03) {
        // Map Address to Actual System Data
        switch (reg_addr) {
            case REG_VALVE_STATE:
                response_val = (uint16_t)Valve_GetState();
                break;
            case REG_BATT_VOLTAGE:
                response_val = MPPT_GetStatus().v_batt_mv;
                break;
            case REG_PV_VOLTAGE:
                response_val = MPPT_GetStatus().v_pv_mv;
                break;
            case REG_MOTOR_CURRENT:
                // Return live current in mA
                response_val = MPPT_GetStatus().i_batt_ma; // Example mapping
                break;
            case REG_ENCODER_COUNT:
                response_val = (uint16_t)HAL_Motor_GetEncoderCount();
                break;
            default:
                response_val = 0xFFFF; // Error indicator
        }

        // Construct Response Packet: [ID] [03] [ByteCount] [DataH] [DataL] [CRCL] [CRCH]
        tx_data[0] = DEVICE_ID;
        tx_data[1] = 0x03;
        tx_data[2] = 0x02; // Byte count (2 bytes for 1 register)
        tx_data[3] = (response_val >> 8) & 0xFF; // Data High
        tx_data[4] = response_val & 0xFF;        // Data Low
        
        // Append CRC
        uint16_t crc = calculate_crc(tx_data, 5);
        tx_data[5] = crc & 0xFF;        // CRC Low
        tx_data[6] = (crc >> 8) & 0xFF; // CRC High
        
        HAL_UART_TxBuffer(tx_data, 7);
    }

    // --- SCENARIO 2: WRITE SINGLE REGISTER (0x06) ---
    else if (func_code == 0x06) {
        if (reg_addr == REG_COMMAND_VALVE) {
            if (reg_data == 1) {
                Valve_Open(); 
            } else {
                Valve_Close(); 
            }
            
            // Response: Echo the Request (Standard Modbus behavior for Write)
            HAL_UART_TxBuffer(rx_data, 8);
        }
    }
}