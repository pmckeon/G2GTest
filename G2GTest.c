#include <stdint.h>
#include <stdio.h>
#include "SMSlib.h"
#include "assets2banks.h"

// define GG Link stuff
volatile __sfr __at 0x01 G2G_IOPort;
volatile __sfr __at 0x02 G2G_NMIPort;
volatile __sfr __at 0x03 G2G_TxPort;
volatile __sfr __at 0x04 G2G_RxPort;
volatile __sfr __at 0x05 G2G_StatusPort;

#define G2G_BYTE_SENT 0x01
#define G2G_BYTE_RECV 0x02
#define G2G_ERROR 0x04
#define G2G_ENABLE_NMI_ON_RECV 0x08
#define G2G_ENABLE_SEND 0x10
#define G2G_ENABLE_RECV 0x20

#define STRING_OFFSET (-32)

const uint8_t baudString[][5] = {"300", "1200", "2400", "4800"};
const uint8_t baudValue[] = {0xF0, 0xB0, 0x70, 0x30};

enum screens
{
    PARALLEL,
    SERIAL,
    MAX_SCREENS
};

unsigned int screen;

unsigned int keyPress;
uint8_t select_x, select_y, selected_output;
uint8_t sendByte, recByte, baud, status, portstate;
uint8_t str[3];

void hextostring(uint8_t hex, uint8_t *str)
{
    uint8_t upper;
    uint8_t lower;

    upper = (hex >> 4);
    lower = (hex & 0x0F);

    if (upper > 9)
        str[0] = upper + 55;
    else
        str[0] = upper + 48;

    if (lower > 9)
        str[1] = lower + 55;
    else
        str[1] = lower + 48;

    str[2] = '\0';
}

void putstring(uint8_t x, uint8_t y, const char *string)
{
    SMS_setNextTileatXY(x, y);
    while (*string)
    {
        SMS_setTile(*string++ + STRING_OFFSET);
    }
}

void GG_loadTileMapArea (unsigned char x, unsigned char y, unsigned char *src, unsigned char width, unsigned char height) {
  unsigned char cur_y;
  for (cur_y=y;cur_y<y+height;cur_y++) {
      SMS_loadTileMap(x, cur_y, src, width*2);
      src+=(width*2);
  }
}

void Clear_Screen(void);
void Setup_Parallel_Screen(void);
void Parallel_Screen(void);
void Setup_Serial_Screen(void);
void Serial_Screen(void);

void main(void)
{
    SMS_autoSetUpTextRenderer();

    SMS_loadTiles(switch__tiles__bin, 128, switch__tiles__bin_size);
    GG_loadBGPalette(switch__palette__bin);

    SMS_loadTiles(select__tiles__bin, 256, select__tiles__bin_size);
    GG_loadSpritePalette(select__palette__bin);

    screen = PARALLEL;
    Setup_Parallel_Screen();

    for (;;)
    {
        SMS_waitForVBlank();

        switch (screen)
        {
        case PARALLEL:
            Parallel_Screen();
            break;

        case SERIAL:
            Serial_Screen();
            break;
        }
    }
}

void Clear_Screen(void)
{
    SMS_VRAMmemsetW(XYtoADDR(0, 0), 0, 32 * 28 * 2); // Initialise VRAM.
}

void Setup_Parallel_Screen(void)
{
    Clear_Screen();

    select_x = 112;
    select_y = 40;
    selected_output = 0;
    sendByte = 0; // To store the IO status
    status = 0xFF; // To store the IO port mode
    G2G_IOPort = 0x00; // Set all IO to LOW
    G2G_NMIPort = 0xFF; // Set all IO to input
    G2G_StatusPort = 0x00; // Disabled serial communication.
    portstate = 0;

    putstring(8, 5, "1:IN");
    putstring(8, 7, "2:IN");
    putstring(8, 9, "3:IN");
    putstring(8, 11, "4:IN");
    putstring(8, 13, "5:IN");
    putstring(8, 15, "6:IN");
    putstring(8, 17, "7:IN");

    SMS_loadTileMapArea(14, 5, switch_off__tilemap__bin, 4, 2);
    SMS_loadTileMapArea(14, 7, switch_off__tilemap__bin, 4, 2);
    SMS_loadTileMapArea(14, 9, switch_off__tilemap__bin, 4, 2);
    SMS_loadTileMapArea(14, 11, switch_off__tilemap__bin, 4, 2);
    SMS_loadTileMapArea(14, 13, switch_off__tilemap__bin, 4, 2);
    SMS_loadTileMapArea(14, 15, switch_off__tilemap__bin, 4, 2);
    SMS_loadTileMapArea(14, 17, switch_off__tilemap__bin, 4, 2);

    putstring(21, 5, "Off");
    putstring(21, 7, "Off");
    putstring(21, 9, "Off");
    putstring(21, 11, "Off");
    putstring(21, 13, "Off");
    putstring(21, 15, "Off");
    putstring(21, 17, "Off");

    SMS_setBackdropColor (15);
}

void Parallel_Screen(void)
{
    SMS_initSprites();

    keyPress = SMS_getKeysPressed();
    if (keyPress & PORT_A_KEY_UP)
    {
        select_y -= 16;
        selected_output--;
        if(select_y < 40)
        {
            select_y = 136;
            selected_output = 6;
        }
    }
    else if (keyPress & PORT_A_KEY_DOWN)
    {
        select_y += 16;
        selected_output++;
        if(select_y > 136)
        {
            select_y = 40;
            selected_output = 0;
        }
    }
    else if (keyPress & PORT_A_KEY_1)
    {
        //sendByte ^= 1 << selected_output;
        G2G_IOPort ^= 1 << selected_output;
        //G2G_IOPort = sendByte;
    }
    else if(keyPress & PORT_A_KEY_2)
    {
        G2G_NMIPort ^= 1 << selected_output;
        //G2G_NMIPort = status;

        if(G2G_NMIPort & (1 << selected_output))
        {
            putstring(10, (selected_output*2) + 5, "IN ");
        }
        else
        {
            putstring(10, (selected_output*2) + 5, "OUT");
        }
    }
    else if (keyPress & GG_KEY_START)
    {
        screen = SERIAL;
        Setup_Serial_Screen();
        return;
    }

    if (G2G_IOPort != portstate)
    {
        for (int i = 0; i < 7; i++)
        {
            if (G2G_IOPort & (1 << i))
            {
                GG_loadTileMapArea(14, (i * 2) + 5, switch_on__tilemap__bin, 4, 2);
                putstring(21, (i * 2) + 5, "On ");
            }
            else
            {
                GG_loadTileMapArea(14, (i * 2) + 5, switch_off__tilemap__bin, 4, 2);
                putstring(21, (i * 2) + 5, "Off");
            }
        }
        portstate = G2G_IOPort;
    }

    SMS_addSprite(select_x, select_y, 0);
    SMS_addSprite(select_x + 8, select_y, 1);
    SMS_addSprite(select_x + 16, select_y, 1);
    SMS_addSprite(select_x + 24, select_y, 2);
    SMS_addSprite (select_x, select_y + 8, 3);
    SMS_addSprite (select_x + 8, select_y + 8, 4);
    SMS_addSprite (select_x + 16, select_y + 8, 4);
    SMS_addSprite (select_x + 24, select_y + 8, 5);

    SMS_copySpritestoSAT();
}

void Setup_Serial_Screen(void)
{
    Clear_Screen();
    SMS_copySpritestoSAT();
    
    select_x = 8;
    select_y = 5;
    baud = 0;
    sendByte = 0x00;
    recByte = 0x00;
    G2G_IOPort = 0x00;
    G2G_NMIPort = 0xFF;
    G2G_StatusPort = baudValue[0];

    SMS_setNextTileatXY(select_x, select_y);
    SMS_setTile(30);

    putstring(10, 5, "Baud: 300");
    putstring(10, 6, "Send Flag: 0");
    putstring(10, 7, "Send Byte:");
    hextostring(sendByte, str);
    putstring(21, 7, str);
    putstring(10, 8, "Recv Flag: 0");
    putstring(10, 9, "Recv Byte:");
}

void Serial_Screen(void)
{
    keyPress = SMS_getKeysPressed();
    if (keyPress & PORT_A_KEY_UP)
    {
        SMS_setNextTileatXY(select_x, select_y);
        SMS_setTile(0);
        select_y--;
        if (select_y < 5)
            select_y = 9;
        SMS_setNextTileatXY(select_x, select_y);
        SMS_setTile(30);
    }
    else if (keyPress & PORT_A_KEY_DOWN)
    {
        SMS_setNextTileatXY(select_x, select_y);
        SMS_setTile(0);
        select_y++;
        if (select_y > 9)
            select_y = 5;
        SMS_setNextTileatXY(select_x, select_y);
        SMS_setTile(30);
    }
    else if (keyPress & PORT_A_KEY_LEFT)
    {
        switch (select_y)
        {
        case 5:
            if (baud == 0)
                break;
            baud--;
            putstring(16, 5, "    ");
            putstring(16, 5, baudString[baud]);
            G2G_StatusPort = baudValue[baud];
            break;

        case 7:
            sendByte--;
            hextostring(sendByte, str);
            putstring(21, 7, str);
            break;
        }
    }
    else if (keyPress & PORT_A_KEY_RIGHT)
    {
        switch (select_y)
        {
        case 5:
            baud++;
            if (baud > 3)
                baud = 3;
            putstring(16, 5, "    ");
            putstring(16, 5, baudString[baud]);
            G2G_StatusPort = baudValue[baud];
            break;

        case 7:
            sendByte++;
            hextostring(sendByte, str);
            putstring(21, 7, str);
            break;
        }
    }
    else if (keyPress & PORT_A_KEY_1)
    {
        G2G_TxPort = sendByte; // there's a bug here somewhere causing multiple bytes to be sent
    }
    else if (keyPress & PORT_A_KEY_2)
    {
        recByte = G2G_RxPort;
        hextostring(recByte, str);
        putstring(21, 9, str);
    }
    else if (keyPress & GG_KEY_START)
    {
        screen = PARALLEL;
        Setup_Parallel_Screen();
        return;
    }

    status = G2G_StatusPort;
    str[0] = (status & G2G_BYTE_SENT) + 48;
    str[1] = '\0';
    putstring(21, 6, str);

    status = G2G_StatusPort;
    str[0] = (status & G2G_BYTE_RECV) + 48;
    str[1] = '\0';
    putstring(21, 8, str);
}

SMS_EMBED_SEGA_ROM_HEADER(1, 0);
SMS_EMBED_SDSC_HEADER_AUTO_DATE(1, 0, "thatawesomeguy", "G2GTest", "2021");