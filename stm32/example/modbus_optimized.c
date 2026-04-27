// ============================================================================
// OPTIMIZED MODBUS RTU IMPLEMENTATION FOR PIC18F86K22
// Özellikler: Buffer overflow koruması, modular yapı, performans optimizasyonu
// ============================================================================
#include <18F86k22.h>
#device *=16
//#device ADC=12
#case  // Enable case sensitivity for better code quality
//#define MAIN_DEVICE_DEFINED
#fuses NOMCLR,INTRC,PLLEN,NOWDT
#use delay(clock=64m,internal=16m)
#use rs232(baud=115200,xmit=PIN_C6,rcv=PIN_C7,bits=8,parity=N,stop=1,enable=PIN_J4,errors,STREAM=SLAVE)
#use rs232(baud=19200,xmit=PIN_G1,rcv=PIN_G2,bits=8,parity=N,stop=1,enable=PIN_G0,errors,STREAM=MASTER)

// CCS C native types used - no stdint.h needed
#include "holding_register_map_new.h"
#include "safety_monitor.h"
#include "system_diagnostics.h"


// ============================================================================
// MODBUS KONFIGÜRASYON SABITLERI
// ============================================================================

// DEBUG mode - geliştirme aşamasında aktif et
// #define DEBUG_CRC  // Production için comment out edildi

// UART configuration is in main.c to avoid duplication

// Modbus sabitleri
#define MODBUS_SLAVE_ADDRESS        1
#define MODBUS_MAX_FRAME_SIZE       42      // Support 16 registers: 7+32+2=41 bytes + 1 safety
#define MODBUS_TIMEOUT_CYCLES       3       // Frame timeout (3.5 chars at 115200 baud)
#define MODBUS_MASTER_TIMEOUT       100     // Master response timeout (~100ms)

// Fonksiyon kodları
#define MODBUS_READ_COILS           0x01
#define MODBUS_READ_INPUTS          0x02
#define MODBUS_READ_HOLDING_REGS    0x03
#define MODBUS_WRITE_SINGLE_COIL    0x05
#define MODBUS_WRITE_SINGLE_REG     0x06
#define MODBUS_WRITE_MULTIPLE_REGS  0x10

// EC ve pH slave adresleri
#define EC_SLAVE_ADDRESS            0x01
#define PH_SLAVE_ADDRESS            0x02

// Register tanımları
#define HOLDING_REGISTER_COUNT      270
#define COIL_COUNT                  25
#define INPUT_COUNT                 24

// Timer konfigürasyonu - Timer1 @ 64MHz/8 = 8MHz, overflow ~10ms
#define TIMER1_RELOAD_VALUE         55535   // 65535 - 10000 = ~10ms
#define CPU_TIMER_CYCLES            50      // LED toggle every 500ms

// ============================================================================
// DATA STRUCTURES - CCS C Compatible
// ============================================================================

// Master state structure removed - use separate variables for compatibility

// ============================================================================
// GLOBAL DEĞİŞKENLER - Optimized Memory Layout
// ============================================================================

// Slave modbus değişkenleri
static unsigned int8 modbus_slave_buffer[MODBUS_MAX_FRAME_SIZE];
static unsigned int8 modbus_slave_index = 0;
static unsigned int8 modbus_slave_timeout = 0;

// Master modbus değişkenleri 
static unsigned int8 modbus_master_buffer[MODBUS_MAX_FRAME_SIZE];
static unsigned int8 modbus_master_index = 0;
static unsigned int8 modbus_master_timeout = 0;

// Removed separate response buffer - use slave buffer for responses (saves 32 bytes RAM)

// Register ve coil dizileri
unsigned int16 holding_register[HOLDING_REGISTER_COUNT];
extern short coil[25];  // COIL_ARRAY_SIZE from variables.h

// Master query state variables
static unsigned int8 master_slave_address = 0;
static unsigned int8 master_register_start = 0;
static unsigned int8 master_register_count = 0;
static unsigned int8 master_query_state = 0;
static unsigned int8 master_sequence_step = 0;

// Sensor değerleri - CCS C compatible (accessible from main.c)
extern unsigned int16 ec_value , temperature , ph_value ;

// ============================================================================
// CRC16 OPTIMIZED IMPLEMENTATION
// ============================================================================

// CRC16 calculation without lookup tables (saves 512 bytes ROM)
unsigned int16 calculate_crc16(unsigned int8 *data, unsigned int8 length) {
    unsigned int16 crc = 0xFFFF;
    unsigned int8 i, j;
    
    for (i = 0; i < length; i++) {
        crc ^= (unsigned int16)data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    // Swap bytes for correct Modbus RTU order
    return make16(crc & 0xFF, crc >> 8);
}

// ============================================================================
// MODULAR HELPER FUNCTIONS
// ============================================================================

// Buffer overflow korumalı frame alma - CCS C compatible (no inline)
// Removed - not used in current implementation

// Frame temizleme - CCS C compatible (no inline)
void clear_frame_buffer(unsigned int8 *buffer, unsigned int8 *index) {
    unsigned int8 i;
    for(i = 0; i < *index; i++) {
        buffer[i] = 0;
    }
    *index = 0;
}

// Modbus error response - compact
void send_modbus_error(unsigned int8 function_code, unsigned int8 error_code) {
    unsigned int16 crc;
    
    modbus_slave_buffer[0] = MODBUS_SLAVE_ADDRESS;
    modbus_slave_buffer[1] = function_code | 0x80;
    modbus_slave_buffer[2] = error_code;
    
    crc = calculate_crc16(modbus_slave_buffer, 3);
    modbus_slave_buffer[3] = crc >> 8;
    modbus_slave_buffer[4] = crc & 0xFF;
    
    disable_interrupts(GLOBAL);
    fputc(modbus_slave_buffer[0], SLAVE);
    fputc(modbus_slave_buffer[1], SLAVE);
    fputc(modbus_slave_buffer[2], SLAVE);
    fputc(modbus_slave_buffer[3], SLAVE);
    fputc(modbus_slave_buffer[4], SLAVE);
    enable_interrupts(GLOBAL);
}

// ============================================================================
// MODBUS SLAVE FUNCTIONS - Modularized
// ============================================================================

// Read coils - compact implementation
void handle_read_coils(void) {
    unsigned int8 start_addr = modbus_slave_buffer[3] + 1;
    unsigned int8 coil_count = modbus_slave_buffer[5];
    unsigned int8 byte_count = (coil_count + 7) / 8;
    unsigned int8 i, j, byte_val;
    unsigned int16 crc;
    
    // Bounds check
    if (start_addr + coil_count > COIL_COUNT) {
        send_modbus_error(MODBUS_READ_COILS, 0x02);
        return;
    }
    
    // Build response in slave buffer
    modbus_slave_buffer[0] = MODBUS_SLAVE_ADDRESS;
    modbus_slave_buffer[1] = MODBUS_READ_COILS;
    modbus_slave_buffer[2] = byte_count;
    
    // Pack coils
    for (i = 0; i < byte_count; i++) {
        byte_val = 0;
        for (j = 0; j < 8 && (i*8 + j) < coil_count; j++) {
            if (coil[start_addr + i*8 + j]) {
                byte_val |= (1 << j);
            }
        }
        modbus_slave_buffer[3 + i] = byte_val;
    }
    
    crc = calculate_crc16(modbus_slave_buffer, 3 + byte_count);
    modbus_slave_buffer[3 + byte_count] = crc >> 8;
    modbus_slave_buffer[4 + byte_count] = crc & 0xFF;
    
    // Send
    disable_interrupts(GLOBAL);
    for (i = 0; i < 5 + byte_count; i++) {
        fputc(modbus_slave_buffer[i], SLAVE);
    }
    enable_interrupts(GLOBAL);
}

// Read holding registers - compact implementation
void handle_read_holding_registers(void) {
    unsigned int8 start_addr = modbus_slave_buffer[3] + 1;
    unsigned int8 reg_count = modbus_slave_buffer[5];
    unsigned int8 i;
    unsigned int16 crc;
    
    // Check register count fits in buffer (max 16 registers)
    // Frame: 3 header + (N*2) data + 2 CRC = max 42 bytes
    // For 16 registers: 3 + 32 + 2 = 37 bytes (fits in 42 byte buffer)
    if (reg_count > 16) {
        send_modbus_error(MODBUS_READ_HOLDING_REGS, 0x03);  // Too many registers
        return;
    }
    
    // Bounds check
    if ((unsigned int16)start_addr + reg_count > HOLDING_REGISTER_COUNT) {
        send_modbus_error(MODBUS_READ_HOLDING_REGS, 0x02);
        return;
    }
    
    // Build response in slave buffer
    modbus_slave_buffer[0] = MODBUS_SLAVE_ADDRESS;
    modbus_slave_buffer[1] = MODBUS_READ_HOLDING_REGS;
    modbus_slave_buffer[2] = reg_count * 2;
    
    // Pack registers
    for (i = 0; i < reg_count; i++) {
        modbus_slave_buffer[3 + i*2] = holding_register[start_addr + i] >> 8;
        modbus_slave_buffer[4 + i*2] = holding_register[start_addr + i] & 0xFF;
    }
    
    crc = calculate_crc16(modbus_slave_buffer, 3 + reg_count*2);
    modbus_slave_buffer[3 + reg_count*2] = crc >> 8;
    modbus_slave_buffer[4 + reg_count*2] = crc & 0xFF;
    
    // Send
    disable_interrupts(GLOBAL);
    for (i = 0; i < 5 + reg_count*2; i++) {
        fputc(modbus_slave_buffer[i], SLAVE);
    }
    enable_interrupts(GLOBAL);
}

// Write single coil implementation (0x05) - CCS C compatible
void handle_write_single_coil(void) {
    unsigned int8 coil_addr;
    unsigned int16 coil_value;
    unsigned int16 crc;
    unsigned int8 i;
    
    coil_addr = modbus_slave_buffer[3] + 1;
    coil_value = make16(modbus_slave_buffer[4], modbus_slave_buffer[5]);
    
    // Bounds check
    if (coil_addr >= COIL_COUNT) {
        send_modbus_error(MODBUS_WRITE_SINGLE_COIL, 0x02);  // Illegal data address
        return;
    }
    
    // Set coil value
    if (coil_value == 0xFF00) {
        coil[coil_addr] = 1;
    } else if (coil_value == 0x0000) {
        coil[coil_addr] = 0;
    } else {
        send_modbus_error(MODBUS_WRITE_SINGLE_COIL, 0x03);  // Illegal data value
        return;
    }
    
    // Echo request as response
    crc = calculate_crc16(modbus_slave_buffer, 6);
    modbus_slave_buffer[6] = crc >> 8;          // HIGH BYTE FIRST
    modbus_slave_buffer[7] = crc & 0xFF;        // LOW BYTE SECOND
    
    // Send response
    disable_interrupts(GLOBAL);
    for (i = 0; i < 8; i++) {
        fputc(modbus_slave_buffer[i], SLAVE);
    }
    enable_interrupts(GLOBAL);
}

// Write single register implementation (0x06) - CCS C compatible
void handle_write_single_register(void) {
    unsigned int16 reg_addr;
    unsigned int16 reg_value;
    unsigned int16 crc;
    unsigned int8 i;
    
    // Get address from Modbus frame (16-bit address)
    reg_addr = make16(modbus_slave_buffer[2], modbus_slave_buffer[3]);
    reg_value = make16(modbus_slave_buffer[4], modbus_slave_buffer[5]);
    
    // Bounds check
    if (reg_addr >= HOLDING_REGISTER_COUNT) {
        send_modbus_error(MODBUS_WRITE_SINGLE_REG, 0x02);  // Illegal data address
        return;
    }
    
    // Write register
    holding_register[reg_addr] = reg_value;
    
    // Debug mode kaldırıldı
    
    // Prepare response
    modbus_slave_buffer[0] = MODBUS_SLAVE_ADDRESS;
    modbus_slave_buffer[1] = MODBUS_WRITE_SINGLE_REG;
    modbus_slave_buffer[2] = 0x00;
    modbus_slave_buffer[3] = reg_addr;  // Keep original address
    modbus_slave_buffer[4] = reg_value >> 8;
    modbus_slave_buffer[5] = reg_value & 0xFF;
    
    crc = calculate_crc16(modbus_slave_buffer, 6);
    modbus_slave_buffer[6] = crc >> 8;          // HIGH BYTE FIRST
    modbus_slave_buffer[7] = crc & 0xFF;        // LOW BYTE SECOND
    
    // Send response
    disable_interrupts(GLOBAL);
    for (i = 0; i < 8; i++) {
        fputc(modbus_slave_buffer[i], SLAVE);
    }
    enable_interrupts(GLOBAL);
}

// Removed - no longer needed with in-place buffer usage

// ============================================================================
// WRITE MULTIPLE REGISTERS HANDLER (0x10)
// ============================================================================

// Write multiple registers implementation - For HMI time sync
void handle_write_multiple_registers(void) {
    unsigned int16 start_address, register_count, byte_count;
    unsigned int16 response_crc;
    unsigned int8 i, data_index;
    
    // Get parameters from request
    start_address = make16(modbus_slave_buffer[2], modbus_slave_buffer[3]);
    register_count = make16(modbus_slave_buffer[4], modbus_slave_buffer[5]);
    byte_count = modbus_slave_buffer[6];
    
    // Validate parameters (max 16 registers for 40 byte buffer)
    // Frame: 7 header + (N*2) data + 2 CRC = 41 bytes for 16 regs
    // But we need 16 registers for HMI, so accept up to 16
    if (register_count == 0 || register_count > 16) {
        send_modbus_error(MODBUS_WRITE_MULTIPLE_REGS, 0x03);  // Illegal data value
        return;
    }
    
    if ((start_address + register_count) > HOLDING_REGISTER_COUNT) {
        send_modbus_error(MODBUS_WRITE_MULTIPLE_REGS, 0x02);  // Illegal address
        return;
    }
    
    if (byte_count != (register_count * 2)) {
        send_modbus_error(MODBUS_WRITE_MULTIPLE_REGS, 0x03);  // Illegal data value
        return;
    }
    
    // Write registers
    data_index = 7;  // Start of data in request
    for (i = 0; i < register_count; i++) {
        unsigned int16 reg_addr = start_address + i;
        unsigned int16 reg_value = make16(modbus_slave_buffer[data_index], 
                                          modbus_slave_buffer[data_index + 1]);
        
        holding_register[reg_addr] = reg_value;
        
        
        data_index += 2;
    }
    
    // Build response in place (reuse slave buffer)
    // First 6 bytes are already correct (echo of request)
    
    response_crc = calculate_crc16(modbus_slave_buffer, 6);
    modbus_slave_buffer[6] = response_crc >> 8;    // HIGH BYTE FIRST
    modbus_slave_buffer[7] = response_crc & 0xFF;  // LOW BYTE SECOND
    
    // Send response
    disable_interrupts(GLOBAL);
    for (i = 0; i < 8; i++) {
        fputc(modbus_slave_buffer[i], SLAVE);
    }
    enable_interrupts(GLOBAL);
} 


// Main slave frame processor
void process_modbus_slave_frame(void) {
    unsigned int16 received_crc, calculated_crc;
    
    // Minimum frame check
    if (modbus_slave_index < 8) {
        return;
    }
    
    // Address verification
    if (modbus_slave_buffer[0] != MODBUS_SLAVE_ADDRESS) {
        clear_frame_buffer(modbus_slave_buffer, &modbus_slave_index);
        return;
    }
    
    // CRC verification - HIGH byte first, LOW byte second
    received_crc = make16(modbus_slave_buffer[modbus_slave_index - 2], modbus_slave_buffer[modbus_slave_index - 1]);
    calculated_crc = calculate_crc16(modbus_slave_buffer, modbus_slave_index - 2);
    
    
    if (received_crc != calculated_crc) {
        // CRC error - silently ignore (Modbus RTU standard)
        clear_frame_buffer(modbus_slave_buffer, &modbus_slave_index);
        diagnostics_update_comm_stats(1, 0);  // Log CRC error
        return;
    }
    
    // Valid frame received - update communication timestamp
    update_hmi_comm_timestamp();
    diagnostics_update_comm_stats(0, 0);  // Log successful frame
    
    // Function code dispatch
    switch (modbus_slave_buffer[1]) {
        case MODBUS_READ_COILS:
            handle_read_coils();
            break;
            
        case MODBUS_READ_INPUTS:
            // Inputs handled same as coils but with inputs[] array
            break;
            
        case MODBUS_READ_HOLDING_REGS:
            handle_read_holding_registers();
            break;
            
        case MODBUS_WRITE_SINGLE_COIL:
            handle_write_single_coil();
            break;
            
        case MODBUS_WRITE_SINGLE_REG:
            holding_register[REG_DEBUG_MODBUS_ERR]++;  // 249: Count Write Single Reg calls
            handle_write_single_register();
            break;
            
        case MODBUS_WRITE_MULTIPLE_REGS:
            holding_register[REG_DEBUG_WDT_RESET]++;  // 250: Count Write Multiple Regs calls
            handle_write_multiple_registers();
            break;
            
        default:
            // Unknown function code - send error response
            send_modbus_error(modbus_slave_buffer[1], 0x01);  // Illegal function
            clear_frame_buffer(modbus_slave_buffer, &modbus_slave_index);
            break;
    }
}

// ============================================================================
// MODBUS MASTER FUNCTIONS - State Machine Optimized
// ============================================================================

// Master query builder - optimized
void build_master_query(unsigned int8 slave_addr, unsigned int8 start_reg, unsigned int8 reg_count) {
    unsigned int16 crc;
    
    modbus_master_buffer[0] = slave_addr;
    modbus_master_buffer[1] = MODBUS_READ_HOLDING_REGS;
    modbus_master_buffer[2] = 0;
    modbus_master_buffer[3] = start_reg;
    modbus_master_buffer[4] = 0;
    modbus_master_buffer[5] = reg_count;
    
    crc = calculate_crc16(modbus_master_buffer, 6);
    modbus_master_buffer[6] = crc >> 8;
    modbus_master_buffer[7] = crc & 0xFF;
    
    // Send query
    set_tris_g(0b11110100);
    
    fputc(modbus_master_buffer[0], MASTER);
    fputc(modbus_master_buffer[1], MASTER);
    fputc(modbus_master_buffer[2], MASTER);
    fputc(modbus_master_buffer[3], MASTER);
    fputc(modbus_master_buffer[4], MASTER);
    fputc(modbus_master_buffer[5], MASTER);
    fputc(modbus_master_buffer[6], MASTER);
    fputc(modbus_master_buffer[7], MASTER);
    
    delay_ms(1);
    modbus_master_timeout = 1;
}

// Process master response
unsigned int8 process_master_response(void) {
    unsigned int16 received_crc, calculated_crc;
   
    if (modbus_master_index < 5) {
        return 0;  // Not enough data
    }
    
    // Address verification
    if (modbus_master_buffer[0] != master_slave_address) {
        return 0;
    }
    
    // CRC verification - HIGH byte first, LOW byte second
    received_crc = make16(modbus_master_buffer[modbus_master_index - 2], 
                         modbus_master_buffer[modbus_master_index - 1]);
    calculated_crc = calculate_crc16(modbus_master_buffer, modbus_master_index - 2);
    
    if (received_crc != calculated_crc) {
        return 0;  // CRC error
    }
    
    // Function code verification
    if (modbus_master_buffer[1] != MODBUS_READ_HOLDING_REGS) {
        return 0;
    }
    
    // Data length verification
    if (modbus_master_buffer[2] != master_register_count * 2) {
        return 0;
    }
    
    // Update global variables based on slave type (read directly from buffer)
    if (master_slave_address == EC_SLAVE_ADDRESS) {
        // EC slave sends EC at register 1, Temperature at register 2
        ec_value = make16(modbus_master_buffer[3], modbus_master_buffer[4]);
        temperature = make16(modbus_master_buffer[5], modbus_master_buffer[6]);
    } else if (master_slave_address == PH_SLAVE_ADDRESS) {
        // pH slave sends pH at register 1
        ph_value = make16(modbus_master_buffer[3], modbus_master_buffer[4]);
    }
    
    return 1;  // Success
}

// Optimized master state machine
void modbus_master_task(void) {
    switch (master_sequence_step) {
        case 0:  // Query EC slave
            master_slave_address = EC_SLAVE_ADDRESS;
            master_register_start = 0x00;
            master_register_count = 0x02;
        
            if (master_query_state == 0) {
               build_master_query(master_slave_address,
                                 master_register_start, 
                                 master_register_count);
                master_query_state = 1;
            } else if (master_query_state == 1) {
                if (modbus_master_timeout > MODBUS_MASTER_TIMEOUT) {
                    if (process_master_response()) {
                        // Success - move to next slave
                        master_sequence_step = 1;
                    }
                    clear_frame_buffer(modbus_master_buffer, &modbus_master_index);
                    modbus_master_timeout = 0;
                    master_query_state = 0;
                }
            }
            break;
            
        case 1:  // Query pH slave
            master_slave_address = PH_SLAVE_ADDRESS;
            master_register_start = 0x00;
            master_register_count = 0x01;
            
            if (master_query_state == 0) {
                build_master_query(master_slave_address, 
                                 master_register_start, 
                                 master_register_count);
                master_query_state = 1;
            } else if (master_query_state == 1) {
                if (modbus_master_timeout > MODBUS_MASTER_TIMEOUT) {
                    if (process_master_response()) {
                        // Success - cycle back to EC
                        master_sequence_step = 0;
                    }
                    clear_frame_buffer(modbus_master_buffer, &modbus_master_index);
                    modbus_master_timeout = 0;
                    master_query_state = 0;
                }
            }
            break;
    }
}

// ============================================================================
// MAIN INTERFACE FUNCTIONS
// ============================================================================

void modbus_slave_task(void) {
    if (modbus_slave_timeout > MODBUS_TIMEOUT_CYCLES) {
        modbus_slave_timeout = 0;
        
        if (modbus_slave_index > 0) {
            process_modbus_slave_frame();
            clear_frame_buffer(modbus_slave_buffer, &modbus_slave_index);
        }
    }
}

// ============================================================================
// INTERRUPT HANDLERS - Optimized for Safety
// ============================================================================

#INT_rda
void uart1_receive_interrupt(void) {
    unsigned int8 received_byte;
    if (kbhit(SLAVE)) {
        received_byte = fgetc(SLAVE);
        if(modbus_slave_index < MODBUS_MAX_FRAME_SIZE - 1) {
            modbus_slave_buffer[modbus_slave_index] = received_byte;
            modbus_slave_index++;
        }
        modbus_slave_timeout = 1;
    }
}

#INT_rda2  
void uart2_receive_interrupt(void) {
    unsigned int8 received_byte;
    
    
    // Debug: Increment interrupt counter (separated from alarm registers)
    holding_register[REG_DEBUG_UART2_RX]++;  // 247: UART2 interrupt counter
    
    // Always read to clear interrupt flag even if buffer full
    if (kbhit(MASTER)) {
        received_byte = fgetc(MASTER);
        
        // Debug: Store last received byte (separated from alarm registers)
        holding_register[REG_DEBUG_UART2_TX] = received_byte;  // 248: UART2 last byte
        
        // Only store if buffer has space
        if(modbus_master_index < MODBUS_MAX_FRAME_SIZE - 1) {
            modbus_master_buffer[modbus_master_index] = received_byte;
            modbus_master_index++;
            modbus_master_timeout = 1;
        }
        // If buffer full, data is dropped but interrupt is cleared
        
       
    }
}

// Timer interrupt for timeout management
#define CPU_LED_PIN PIN_G3

// External variables and functions
extern unsigned int16 system_timer;
extern unsigned int16 timer_counter;
extern short program_aktif;
extern void decrement_coil_timers(void);

#INT_timer1
void timer1_interrupt(void) {
    static unsigned int8 cpu_timer = 0;
    static unsigned int8 system_timer_counter = 0;
    static unsigned int8 second_counter = 0;
    
    set_timer1(TIMER1_RELOAD_VALUE);
    
    // Increment timeouts
    if (modbus_slave_timeout) modbus_slave_timeout++;
    if (modbus_master_timeout) modbus_master_timeout++;
    
    // System timer - her 100ms (10 x 10ms interrupt) increment et
    system_timer_counter++;
    if (system_timer_counter >= 10) {  // 100ms = 0.1 saniye
        system_timer_counter = 0;
        system_timer++;  // 0.1 saniye resolution
        
        // Valve/PID timer - her 10 * 100ms = 1 saniye
        second_counter++;
        if (second_counter >= 10) {  // 10 * 0.1s = 1 saniye
            second_counter = 0;
            
            // Program timer - sadece program aktifken artar
            if (program_aktif == 1) {
                timer_counter++;
            }
            
            // Overlap timer decrement
            decrement_coil_timers();
        }
    }
    
    // CPU activity indicator
    if (++cpu_timer > CPU_TIMER_CYCLES) {
        cpu_timer = 0;
        output_toggle(CPU_LED_PIN);
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void modbus_init(void) {
    unsigned int8 i;
    
  
    
    // Clear all buffers and state - CCS C compatible
    for(i = 0; i < MODBUS_MAX_FRAME_SIZE; i++) {
        modbus_slave_buffer[i] = 0;
        modbus_master_buffer[i] = 0;
    }
    
    // Clear master state
    master_slave_address = 0;
    master_register_start = 0;
    master_register_count = 0;
    master_query_state = 0;
    master_sequence_step = 0;
    
    modbus_slave_index = 0;
    modbus_master_index = 0;
    modbus_slave_timeout = 0;
    modbus_master_timeout = 0;
    
    // Initialize debug registers (separated from alarm registers)
    holding_register[REG_DEBUG_UART2_RX] = 0;   // 247: UART2 interrupt counter
    holding_register[REG_DEBUG_UART2_TX] = 0;   // 248: UART2 last received byte
    holding_register[REG_DEBUG_CPU_LOAD] = 0;   // 253: UART2 status
    
    // Initialize timer (but don't enable interrupts here - main.c will do it)
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
    set_timer1(TIMER1_RELOAD_VALUE);
    
    // Note: Interrupts are enabled in main.c in correct order
}

