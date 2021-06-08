#include <stdint.h>
#include <stdio.h>
#include "SMSlib.h"

// define GG Link stuff
__sfr __at 0x01 G2G_IOPinPort;
__sfr __at 0x02 G2G_NMIPort;
__sfr __at 0x03 G2G_TxPort;
__sfr __at 0x04 G2G_RxPort;
__sfr __at 0x05 G2G_StatusPort;

#define G2G_BYTE_SENT           0x01
#define G2G_BYTE_RECV           0x02
#define G2G_ERROR               0x04
#define G2G_ENABLE_NMI_ON_RECV  0x08
#define G2G_ENABLE_SEND         0x10
#define G2G_ENABLE_RECV         0x20

#define STRING_OFFSET (-32)

const uint8_t baudString[][5] = {"300", "1200", "2400", "4800"};
const uint8_t baudValue[] = {0xF0, 0xB0, 0x70, 0x30};

void hextostring(uint8_t hex, uint8_t *str)
{
    uint8_t upper;
    uint8_t lower;

    upper = (hex >> 4);
    lower = (hex & 0x0F);
    
    if(upper > 9)
        str[0] = upper + 55;
    else
        str[0] = upper + 48;

    if(lower > 9)
        str[1] = lower + 55;
    else
        str[1] = lower + 48;

    str[2] = '\0';
}

void putstring(uint8_t x, uint8_t y, const char* string)
{
    SMS_setNextTileatXY(x, y);
    while (*string)
    {
        SMS_setTile(*string++ + STRING_OFFSET);
    }
}

/*void NMI_InterruptHandler (void) __critical __interrupt
{
    //
}*/

void main(void)
{
    uint8_t keyPress, char_x, char_y;
    uint8_t sendByte, recByte, baud, status;
    uint8_t str[3];

    SMS_VRAMmemsetW(XYtoADDR(0,0), 0, 32*28*2); // Initialise VRAM

    SMS_autoSetUpTextRenderer();

    G2G_IOPinPort = 0x00;
    G2G_NMIPort = 0x80;
    G2G_StatusPort = baudValue[0];
    //G2G_StatusPort = G2G_ENABLE_SEND|G2G_ENABLE_RECV;

    char_x = 6;
    char_y = 3;
    baud = 0;
    sendByte = 0x2F;
    recByte = 0x00;

    SMS_setNextTileatXY(char_x, char_y);
    SMS_setTile(30);

    putstring(7, 3, "Baud: 300");
    putstring(7, 4, "Send Flag: 0");
    putstring(7, 5, "Send Byte:");
    hextostring(sendByte, str);
    putstring(18, 5, str);
    putstring(7, 6, "Recv Flag: 0");
    putstring(7, 7, "Recv Byte:");

    for (;;)
	{
        keyPress=SMS_getKeysPressed();
        if (keyPress & PORT_A_KEY_UP)
        {
            SMS_setNextTileatXY(char_x, char_y);
            SMS_setTile(0);
            char_y--;
            if(char_y < 3) char_y = 3;
            SMS_setNextTileatXY(char_x, char_y);
            SMS_setTile(30);
        }
        else if (keyPress & PORT_A_KEY_DOWN)
        {
            SMS_setNextTileatXY(char_x, char_y);
            SMS_setTile(0);
            char_y++;
            if(char_y > 7) char_y = 7;
            SMS_setNextTileatXY(char_x, char_y);
            SMS_setTile(30);
        }
        else if (keyPress & PORT_A_KEY_LEFT)
        {
            switch(char_y)
            {
                case 3:
                if(baud == 0) break;
                baud--;
                putstring(13, 3, "    ");
                putstring(13, 3, baudString[baud]);
                G2G_StatusPort = baudValue[baud];
                break;

                case 4:
                break;

                case 5:
                sendByte--;
                hextostring(sendByte, str);
                putstring(18, 5, str);
                break;

                case 6:
                break;

                case 7:
                break;
            }
        }
        else if (keyPress & PORT_A_KEY_RIGHT)
        {
            switch(char_y)
            {
                case 3:
                baud++;
                if(baud > 3) baud = 3;
                putstring(13, 3, "    ");
                putstring(13, 3, baudString[baud]);
                G2G_StatusPort = baudValue[baud];
                break;

                case 4:
                break;

                case 5:
                sendByte++;
                hextostring(sendByte, str);
                putstring(18, 5, str);
                break;

                case 6:
                break;

                case 7:
                break;
            }
        }
        else if (keyPress & PORT_A_KEY_1)
        {
            G2G_TxPort = sendByte;
        }
        else if (keyPress & PORT_A_KEY_2)
        {
            recByte=G2G_RxPort;
            hextostring(recByte, str);
            putstring(18, 7, str);
        }

        SMS_waitForVBlank();

        status = G2G_StatusPort;
        str[0] = (status & G2G_BYTE_SENT) + 48;
        str[1] = '\0';
        putstring(18, 4, str);

        status = G2G_StatusPort;
        str[0] = (status & G2G_BYTE_RECV) + 48;
        str[1] = '\0';
        putstring(18, 6, str);
	}
}

SMS_EMBED_SEGA_ROM_HEADER(1,0);
SMS_EMBED_SDSC_HEADER_AUTO_DATE(1,0,"thatawesomeguy","G2GTest","2021");