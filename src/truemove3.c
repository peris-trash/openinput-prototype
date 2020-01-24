#include "truemove3.h"

typedef struct {
    uint8_t motion;
    uint8_t observation;
    delta_xy_t delta;
    uint8_t squal;
} motion_burst_t;

volatile delta_xy_t truemove3Delta = {};

uint8_t truemove3_read(uint8_t address)
{
    uint8_t value;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        TRUEMOVE3_SELECT();

        spi_transfer_byte(address & 0x7F); // 7 bit address + read bit(0)
        delay_us(120); // Tsrad
        value = spi_transfer_byte(0x00);

        TRUEMOVE3_UNSELECT();
    }
    delay_us(160);

    return value;
}
void truemove3_write(uint8_t address, uint8_t value)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        TRUEMOVE3_SELECT();

        spi_transfer_byte((address & 0x7F) | 0x80); // 7 bit address + write bit(1)
        delay_us(120); // Tsrad
        spi_transfer_byte(value);

        TRUEMOVE3_UNSELECT();
    }
    delay_us(160);
}
motion_burst_t truemove3_read_motion_burst()
{
    motion_burst_t values = {};

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        uint8_t buf[6];

        TRUEMOVE3_SELECT();

        spi_transfer_byte(TRUEMOVE3_REG_MOT_BURST); // 7 bit address + read bit(0)
        delay_us(35); // Tsrad_motbr
        spi_read(buf, 6);

        TRUEMOVE3_UNSELECT();

        values.motion = buf[0];
        values.observation = buf[1];
        values.delta.x |= buf[2];
        values.delta.x |= (buf[3] << 8);
        values.delta.y |= buf[4];
        values.delta.y |= (buf[5] << 8);
    }
    delay_us(160);

    return values;
}

uint8_t truemove3_init(uint8_t* firmware)
{
    IRQ_DISABLE(EXTI3_IRQn);

    if(!firmware)
        return 0;

    TRUEMOVE3_UNSELECT();
    delay_ms(1);

    // software reset
    truemove3_write(TRUEMOVE3_REG_PWR_UP_RST, TRUEMOVE3_RESET_CMD);
    delay_ms(50);
    
    truemove3_read(TRUEMOVE3_REG_MOTION);
    truemove3_read(TRUEMOVE3_REG_DELTA_X_L);
    truemove3_read(TRUEMOVE3_REG_DELTA_X_H);
    truemove3_read(TRUEMOVE3_REG_DELTA_Y_L);
    truemove3_read(TRUEMOVE3_REG_DELTA_Y_H);

    if(truemove3_read(TRUEMOVE3_REG_PID) != 0x42) // confirm comm link
        return 0;

    truemove3_write(TRUEMOVE3_REG_CONFIG2, 0x00); // clear REST enable bit

    truemove3_write(TRUEMOVE3_REG_SROM_EN, TRUEMOVE3_SROM_DWNLD_CMD); // initialize SROM download
    delay_ms(15);

    truemove3_write(TRUEMOVE3_REG_SROM_EN, TRUEMOVE3_SROM_DWNLD_START_CMD); // start SROM download

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        TRUEMOVE3_SELECT();

        spi_transfer_byte(TRUEMOVE3_REG_SROM_BURST | 0x80); // 7 bit srom_burst address + write bit(1)
        delay_us(15);

        for(uint16_t b = 0; b < 4094; b++) // write firmware image (4094 bytes)
        {
            spi_transfer_byte(*firmware++);
            delay_us(15);
        }

        TRUEMOVE3_UNSELECT();
    }
    delay_ms(2); // Tbexit

    uint8_t sromId = truemove3_read(TRUEMOVE3_REG_SROM_ID); // read srom firmware id

    if(!sromId)
        return 0;

    truemove3_write(TRUEMOVE3_REG_SROM_EN, TRUEMOVE3_SROM_CRC_CMD); // initialize CRC check
    delay_ms(15);

    volatile uint16_t sromCrc = 0;

    sromCrc |= (truemove3_read(TRUEMOVE3_REG_DOUT_H) << 8);
    sromCrc |= truemove3_read(TRUEMOVE3_REG_DOUT_L);

    if(!sromCrc) //TODO: check CRC
        return 0;

    if(!(truemove3_read(TRUEMOVE3_REG_OBSERVATION) & TRUEMOVE3_SROM_RUN)) // check SROM running bit
        return 0;

    truemove3_write(TRUEMOVE3_REG_CONFIG2, 0x00); // clear REST enable bit

    truemove3_write(TRUEMOVE3_REG_MOT_BURST, 0x01);

    delay_ms(500);

    truemove3_read_motion_burst();

    truemove3_write(TRUEMOVE3_REG_OBSERVATION, 0x00); // clear observation register

    truemove3_write(TRUEMOVE3_REG_CONFIG1, 0x01);

    truemove3_read_motion_burst();

    IRQ_ENABLE(EXTI3_IRQn);

    return sromId;
}

void truemove3_motion_event()
{
    motion_burst_t burst = truemove3_read_motion_burst();
    truemove3Delta.x += burst.delta.x;
    truemove3Delta.y += burst.delta.y;
}

delta_xy_t truemove3_get_deltas()
{
    delta_xy_t values;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        values.x = truemove3Delta.x;
        values.y = truemove3Delta.y;
        truemove3Delta.x = 0;
        truemove3Delta.y = 0;
    }

    return values;
}