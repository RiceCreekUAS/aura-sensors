#include <EEPROM.h>

const int CONFIG_OFFSET = 2;  // starting point for writing big eeprom struct

// global definitions
uint16_t serial_number;

uint16_t read_serial_number() {
    uint8_t lo = EEPROM.read(0);
    uint8_t hi = EEPROM.read(1);
    // Serial.printf(" raw serial number read %d %d\n", hi, lo);
    serial_number = hi * 256 + lo;
    return serial_number;
};

int set_serial_number(uint16_t value) {
    serial_number = value;
    uint16_t hi = serial_number / 256;
    uint16_t lo = serial_number - (hi * 256);
    // Serial.printf(" set serial number raw: %d %d\n", hi, lo);
    EEPROM.update(0, byte(lo));
    EEPROM.update(1, byte(hi));
    return serial_number;
};

void master_defaults() {
    config_master.board = 0;
}

void power_defaults() {
     config_power.have_attopilot = false;
}

void led_defaults() {
     config_led.pin = 0;
}

void config_load_defaults() {
    Serial.println("Setting default config ...");
    master_defaults();
    imu_setup_defaults();
    pwm_defaults();
    act_gain_defaults();
    mixing_defaults();
    sas_defaults();
    power_defaults();
    led_defaults();
}

int extract_config_buf(uint8_t config_buf[], int pos, uint8_t *buf, int len) {
    for ( int i = 0; i < len; i++ ) {
        buf[i] = config_buf[pos + i];
    }
    return len;
}

int config_read_eeprom() {
    uint8_t config_buf[config_size];
    int status = 0;
    if ( config_size + CONFIG_OFFSET <= E2END - 2 /* checksum */ + 1 ) {
        Serial.print("Loading EEPROM, bytes: ");
        Serial.println(config_size);
        noInterrupts();
        for ( int i = 0; i < config_size; i++ ) {
            config_buf[i] = EEPROM.read(CONFIG_OFFSET + i);
        }
        byte read_cksum0 = EEPROM.read(CONFIG_OFFSET + config_size);
        byte read_cksum1 = EEPROM.read(CONFIG_OFFSET + config_size+1);
        interrupts()
        byte calc_cksum0 = 0;
        byte calc_cksum1 = 0;
        ugear_cksum( START_OF_MSG0 /* arbitrary magic # */, START_OF_MSG1 /* arbitrary magic # */, (byte *)&config_buf, config_size, &calc_cksum0, &calc_cksum1 );
        if ( read_cksum0 != calc_cksum0 || read_cksum1 != calc_cksum1 ) {
            Serial.println("Check sum error!");
        } else {
            status = 1;
            // assemble packed config buffer
            int pos = 0;
            pos += extract_config_buf( config_buf, pos, (uint8_t *)&config_master, config_master.len );
            pos += extract_config_buf( config_buf, pos, (uint8_t *)&config_imu, config_imu.len );
            pos += extract_config_buf( config_buf, pos, (uint8_t *)&config_actuators, config_actuators.len );
            pos += extract_config_buf( config_buf, pos, (uint8_t *)&config_airdata, config_airdata.len );
            pos += extract_config_buf( config_buf, pos, (uint8_t *)&config_power, config_power.len );
            pos += extract_config_buf( config_buf, pos, (uint8_t *)&config_led, config_led.len );    
        }
    } else {
        Serial.println("ERROR: config structure too large for EEPROM hardware!");
    }
    return status;
}

int build_config_buf(uint8_t config_buf[], int pos, uint8_t *buf, int len) {
    for ( int i = 0; i < len; i++ ) {
        config_buf[pos + i] = buf[i];
    }
    return len;
}

int config_write_eeprom() {
    // assemble packed config buffer
    uint8_t config_buf[config_size];
    int pos = 0;
    pos += build_config_buf( config_buf, pos, (uint8_t *)&config_master, config_master.len );
    pos += build_config_buf( config_buf, pos, (uint8_t *)&config_imu, config_imu.len );
    pos += build_config_buf( config_buf, pos, (uint8_t *)&config_actuators, config_actuators.len );
    pos += build_config_buf( config_buf, pos, (uint8_t *)&config_airdata, config_airdata.len );
    pos += build_config_buf( config_buf, pos, (uint8_t *)&config_power, config_power.len );
    pos += build_config_buf( config_buf, pos, (uint8_t *)&config_led, config_led.len );
    
    Serial.println("Write EEPROM (any changed bytes) ...");
    int status = 0;
    if ( config_size + CONFIG_OFFSET <= E2END - 2 /* checksum */ + 1 ) {
        byte calc_cksum0 = 0;
        byte calc_cksum1 = 0;
        ugear_cksum( START_OF_MSG0 /* arbitrary magic # */, START_OF_MSG1 /* arbitrary magic # */, (byte *)&config_buf, config_size, &calc_cksum0, &calc_cksum1 );
        noInterrupts();
        for ( int i = 0; i < config_size; i++ ) {
            EEPROM.update(CONFIG_OFFSET + i, config_buf[i]);
        }
        EEPROM.update(CONFIG_OFFSET + config_size, calc_cksum0);
        EEPROM.update(CONFIG_OFFSET + config_size+1, calc_cksum1);
        interrupts();
        status = 1;
    } else {
        Serial.println("ERROR: config structure too large for EEPROM hardware!");
    }
    return status;
}
