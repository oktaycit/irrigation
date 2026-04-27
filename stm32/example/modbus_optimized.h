// ============================================================================
// MODBUS_OPTIMIZED.H - Header file for optimized Modbus RTU implementation
// PIC18F86K22 için optimize edilmiş Modbus RTU kütüphanesi
// Özellikler: Buffer overflow koruması, modular yapı, performans optimizasyonu
// NOT: #case direktifi ile case-sensitive mode aktif
// ============================================================================

#ifndef MODBUS_OPTIMIZED_H
#define MODBUS_OPTIMIZED_H

// ============================================================================
// INCLUDES
// ============================================================================
// CCS C native types used - no stdint.h needed

// ============================================================================
// MODBUS CONFIGURATION CONSTANTS
// ============================================================================

// Modbus protocol constants
#define MODBUS_SLAVE_ADDRESS        1
#define MODBUS_MAX_FRAME_SIZE       42      // Support 16 registers: 7+32+2=41 bytes + 1 safety
#define MODBUS_TIMEOUT_CYCLES       3       // Frame timeout cycles
#define MODBUS_MASTER_TIMEOUT       100     // Master response timeout

// Function codes
#define MODBUS_READ_COILS           0x01
#define MODBUS_READ_INPUTS          0x02
#define MODBUS_READ_HOLDING_REGS    0x03
#define MODBUS_WRITE_SINGLE_COIL    0x05
#define MODBUS_WRITE_SINGLE_REG     0x06

// Slave addresses for sensor communication
#define EC_SLAVE_ADDRESS            0x01    // EC + Temperature sensor
#define PH_SLAVE_ADDRESS            0x02    // pH sensor

// Data structure sizes
#define HOLDING_REGISTER_COUNT      270     // 0-269 all registers
//#define COIL_COUNT                  25      // Digital outputs
#define INPUT_COUNT                 24      // Digital inputs

// Timer configuration
#define TIMER1_RELOAD_VALUE         55535   // ~10ms interrupt period
#define CPU_TIMER_CYCLES            50      // CPU LED toggle period

// Error codes
#define MODBUS_ERROR_ILLEGAL_FUNCTION    0x01
#define MODBUS_ERROR_ILLEGAL_ADDRESS     0x02
#define MODBUS_ERROR_ILLEGAL_DATA_VALUE  0x03
#define MODBUS_ERROR_SLAVE_DEVICE_FAILURE 0x04

// ============================================================================
// DATA TYPES
// ============================================================================
// Structure removed - using separate variables for CCS C compatibility

// ============================================================================
// GLOBAL VARIABLE DECLARATIONS
// ============================================================================

// Register arrays - accessible from other modules
extern unsigned int16 holding_register[HOLDING_REGISTER_COUNT];  // Modbus holding registers
// Note: coil array is defined in variables.h to avoid duplicate declaration
// Note: ec, sicaklik, ph sensor values are defined in modbus_optimized.c

// ============================================================================
// PUBLIC FUNCTION DECLARATIONS
// ============================================================================

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * @brief Initialize the optimized Modbus system
 * @details Sets up buffers, timers, interrupts, UART hardware and initial state
 * @note Must be called once during system initialization
 */
void modbus_init(void);

/**
 * @brief Initialize UART hardware registers for PIC18F86K22
 * @details Configures UART1 and UART2 hardware for reliable transmission
 * @note Called automatically by modbus_init()
 */
void init_uart_hardware(void);

/**
 * @brief Check UART transmission readiness
 * @param is_master 1 for UART2 (MASTER), 0 for UART1 (SLAVE)
 * @return 1 if ready to transmit, 0 if busy
 * @note Use before transmission to prevent blocking
 */
unsigned int8 uart_tx_ready(unsigned int8 is_master);

/**
 * @brief Debug UART register status for PIC18F86K22
 * @param is_master 1 for UART2, 0 for UART1
 * @return Status bits: bit0-2=Normal, bit3=TRIS_ERROR, bit4=PIN_STUCK
 * @note Use for debugging virtual terminal connection and pin direction issues
 */
unsigned int8 debug_uart_status(unsigned int8 is_master);

/**
 * @brief Force correct UART pin directions to prevent blocking
 * @param is_master 1 for UART2, 0 for UART1
 * @return 1 if pins configured correctly, 0 if pin direction error
 * @note Critical for preventing fputc() blocking on PIC18F86K22
 */
unsigned int8 fix_uart_pin_directions(unsigned int8 is_master);

/**
 * @brief Emergency fix for UART blocking issues
 * @details Forces correct TRISC/TRISG pin directions and clears buffers
 * @note Call this function when fputc() starts hanging/blocking
 */
void emergency_uart_fix(void);

/**
 * @brief Test UART responsiveness without blocking
 * @param is_master 1 for UART2, 0 for UART1  
 * @return 1 if UART responsive, 0 if blocked/stuck
 * @note Use to check UART health before transmission
 */
unsigned int8 test_uart_responsive(unsigned int8 is_master);

/**
 * @brief Update holding registers with UART debug information  
 * @details Monitors TRISC/TRISG issues and auto-fixes problems
 * @note Call in main loop to track UART health via HMI
 * @note Uses holding_register[200-204] for debug data
 */
void update_uart_debug_registers(void);

// ============================================================================
// MAIN TASK FUNCTIONS
// ============================================================================

/**
 * @brief Process Modbus slave communication tasks
 * @details Handles incoming Modbus requests and sends responses
 * @note Call this function regularly in main loop (every 10-50ms)
 */
void modbus_slave_task(void);

/**
 * @brief Process Modbus master communication tasks
 * @details Manages sensor queries (EC and pH) with state machine
 * @note Call this function regularly in main loop (every 10-50ms)
 */
void modbus_master_task(void);

// ============================================================================
// INTERRUPT HANDLERS
// ============================================================================

/**
 * @brief UART1 receive interrupt handler
 * @details Handles incoming slave communication (HMI)
 * @note Automatically called by CCS interrupt system
 */
void uart1_receive_interrupt(void);

/**
 * @brief UART2 receive interrupt handler
 * @details Handles incoming master communication (sensor responses)
 * @note Automatically called by CCS interrupt system
 */
void uart2_receive_interrupt(void);

/**
 * @brief Timer1 interrupt handler for timeout management
 * @details Manages frame timeouts and CPU activity indicator
 * @note Automatically called by CCS interrupt system (~20ms period)
 */
void timer1_interrupt(void);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Removed macros - direct access is more efficient for PIC18F

// ============================================================================
// VERSION INFORMATION
// ============================================================================

#define MODBUS_OPTIMIZED_VERSION_MAJOR    1
#define MODBUS_OPTIMIZED_VERSION_MINOR    0
#define MODBUS_OPTIMIZED_VERSION_PATCH    0

// Version string
#define MODBUS_OPTIMIZED_VERSION_STRING   "1.0.0"

// ============================================================================
// COMPILE-TIME CONFIGURATION CHECKS
// ============================================================================

// Ensure buffer sizes are reasonable
#if MODBUS_MAX_FRAME_SIZE < 8
    #error "MODBUS_MAX_FRAME_SIZE must be at least 8 bytes"
#endif

#if MODBUS_MAX_FRAME_SIZE > 256
    #error "MODBUS_MAX_FRAME_SIZE should not exceed 256 bytes"
#endif

// Ensure register counts are reasonable
#if HOLDING_REGISTER_COUNT > 1000
    #warning "HOLDING_REGISTER_COUNT is very large, check memory usage"
#endif

#endif // MODBUS_OPTIMIZED_H

// ============================================================================
// END OF FILE
// ============================================================================ 
