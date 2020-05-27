
/*******************************************************************************
INCLUDED FILES
*******************************************************************************/
#include "Rn42.h"

/*******************************************************************************
PRIVATE CONSTANTS
*******************************************************************************/

/*******************************************************************************
PRIVATE DATA TYPES
*******************************************************************************/

/*******************************************************************************
GLOBAL VARIABLE DEFINITIONS
*******************************************************************************/

/*******************************************************************************
PRIVATE VARIABLE DEFINITIONS
*******************************************************************************/

/*******************************************************************************
PRIVATE FUNCTION DECLARATIONS
*******************************************************************************/

/*******************************************************************************
PRIVATE FUNCTION DEFINITIONS
*******************************************************************************/

/*******************************************************************************
PUBLIC FUNCTION DEFINITIONS
*******************************************************************************/
Rn42::Rn42(ComDriver com_driver, uint32_t (*tick_cb)())
{
    this->tick_cb = tick_cb;

    /* Strings that represent the respective reply from the module */
    static struct Promts command_promts_init[] =
    {
        {"CMD\r\n",             BT_MESSAGE_PROMT_CMD},
        {"END\r\n",             BT_MESSAGE_PROMT_END},
        {"AOK\r\n",             BT_MESSAGE_PROMT_AOK},
        {"RSSI ON\r\n",         BT_MESSAGE_PROMT_RSSION},
        {"RSSI=",               BT_MESSAGE_PROMT_RSSIVALUE},      /* Max is 0xFF, "RSSI=f2,41\r\n" */
        {"RSSI OFF\r\n",        BT_MESSAGE_PROMT_RSSIOFF},
        {"NOT Connected!\r\n",  BT_MESSAGE_PROMT_NOTCONNECTED},
        {"Reboot!\r\n",         BT_MESSAGE_PROMT_REBOOT}
    };

    command_promts = command_promts_init;
    command_promts_n = sizeof(command_promts)/sizeof(struct Promts);
    rssi_value_max = 0xFF;  /** @note This is the max possible RSSI value as per datasheet. */

    this->com_driver_ = com_driver;
}

Rn42::Result Rn42::txCommand(const char* data)
{
    return com_driver_.tx_cb(data, strlen(data));
}

Rn42::Result Rn42::rxPromt(char* data, uint32_t timeout)
{
    return com_driver_.rx_cb(data, timeout);
}

/* TODO:
 * Add a return code for UKNOWN STRING when the function doesn't time out and receives non matching characters
 *
 */
Rn42::Result Rn42::findPromt(Promt_i* promt_i, const char* data)
{
    Result result = RESULT_FAIL;

    unsigned char stringLengthMin = 255, stringLengthCurrent = 255;

    // TODO: this part may be redundant
    // Find the length of the shortest string
    for(uint32_t i=0; i<command_promts_n; i++)
    {
        stringLengthCurrent = strlen(command_promts[i].data);
        if(stringLengthCurrent < stringLengthMin)
            stringLengthMin = stringLengthCurrent;
    }

    uint32_t currCharIndex;
    char oneMatch = 0;
    char oneMatchSave = 0;
    char wholeMatch = 0;

    // Loop through every string response we have stored. Searching for a match. this is how we'll know what message we received
    for(uint32_t i=0; i<command_promts_n; i++)
    {
        currCharIndex = 0;
        oneMatchSave = oneMatch;
        for(uint32_t ii=0; ii<strlen(command_promts[i].data); ii++)
        {
            if(strlen(data) >= (ii+1))
            {
                if(data[currCharIndex] == command_promts[i].data[ii])
                {
                    currCharIndex += 1;
                    // If the characters at the current index of the string match then oneMatch = true
                    oneMatch = 1;
                    // If we matched the whole string then wholeMatch = true
                    if((ii+1) == strlen(command_promts[i].data))
                        wholeMatch = 1;
                }
                else
                {
                    // If the only true oneMatch came from this string and this string failed,
                    // then cancel the oneMatch True
                    if(!oneMatchSave)
                        oneMatch = 0;
                    else
                        oneMatch |= 0;
                    break;
                }
            }
            else
                break;
        }

        // A onematch TRUE is a whole match TRUE at this point
        if(wholeMatch)// && (ii == strlen(command_promts[i].data)))
        {
            currCharIndex += strlen(command_promts[i].data);
            *promt_i = command_promts[i].id;
            break;
        }
    }

    return result;
}

Rn42::Result Rn42::command(const char* data, Promt_i promt_i, uint32_t timeout)
{
    /* Send the command */
    Result result = txCommand(data);
    if(result != RESULT_OK)
    {
        return result;
    }

    /* Receive a newline terminated string back */
    char data_rx[50];
    result = rxPromt(data_rx, timeout);
    if(result != RESULT_OK)
    {
        return result;
    }

    /* Parse the string to determine which prompt we received */
    Promt_i promt_i_rx;
    result = findPromt(&promt_i_rx, data_rx);
    if(result != RESULT_OK)
    {
        return result;
    }
    if(promt_i_rx != promt_i)
    {
        return RESULT_PROMT_MATCH;
    }
    return result;
}

Rn42::Result Rn42::commandMode()
{
    /* Delay to make sure we haven't sent any data to the BT module for
     * 1 [s] before issuing the command string. This is required to enter
     * command mode. */
    delay(1000);

    /* Issue the command mode enter string, and wait for the confirmation */
    Result result = command("$$$", BT_MESSAGE_PROMT_CMD, 1000);
//    char data_rx[50];
//    Promt_i promt_i = com_driver_.rx_cb(data_rx, 1000);
//    if(promt_i != BT_MESSAGE_PROMT_CMD)
//    {
//        result = RESULT_FAIL;
//    }
    return result;
}

Rn42::Result Rn42::normalMode()
{
    /* Issue the command mode enter string, and wait for the confirmation */
    Result result = command("---", BT_MESSAGE_PROMT_END, 1000);
//    char data_rx[50];
//    Promt_i promt_i = com_driver_.rx_cb(data_rx, 1000);
//    if(promt_i != BT_MESSAGE_PROMT_CMD)
//    {
//        result = RESULT_FAIL;
//    }
    return result;
}

/* Issue the reboot command to have the changes take effect */
Rn42::Result Rn42::reboot()
{
    return command("R,1\r\n", BT_MESSAGE_PROMT_REBOOT, 1000);
}

Rn42::Result Rn42::baudRateSet()
{
    Result result = command("SU,92\r\n", BT_MESSAGE_PROMT_AOK, 1000);
    return result;
}

Rn42::Result Rn42::nameSet(char* data)
{
    Result result = RESULT_OK;
    char command_data[50];

    /* This array will hold the incoming string, which is a max of 20
     * characters + 1 null character */
    char name[21];
    uint8_t name_n = strlen(data);

    /* Check to make sure that the parameters have a legal size */
    if((name_n < 1) | (name_n > 20))
    {
        return RESULT_PARAM_SIZE;
    }

    memcpy(name, data, name_n);
    name[name_n] = '\0';

    // Build the change name command string
    strcpy(command_data, "SN,");
    strcat(command_data, name);
    strcat(command_data, "\r\n");
    result = command(command_data, BT_MESSAGE_PROMT_AOK, 1000);
    return result;
}

Rn42::Result Rn42::pinNumberSet(char* data)
{
    Result result = RESULT_OK;
    char command_data[50] = {'\0'};
    char pin[21];
    uint8_t pin_n = strlen(data);

    /* Check to make sure that the parameters have a legal size */
    if((pin_n < 1) | (pin_n > 20))
    {
        return RESULT_PARAM_SIZE;
    }

    memcpy(pin, data, pin_n);
    pin[pin_n] = '\0';

    /* Build the set PIN# command string */
    strcpy(command_data, "SP,");
    strcat(command_data, pin);
    strcat(command_data, "\r\n");
    result = command(command_data, BT_MESSAGE_PROMT_AOK, 1000);
    return result;
}

//Rn42::Result Rn42::rssiGet()
//{
//    // Delay now to make sure we haven't sent any data to the BT module for 1s before issuing the command string
//    delay(1000);
//
//    // Issue the command mode enter string, and wait for the confirmation
//    result = com_driver_.tx_cb("$$$", 1000);
//
//    // If the module confirms we entered command mode
//    if(result == BT_MESSAGE_PROMT_CMD)
//    {
//        // Issue the command that begins the RSSI measurement
//        result = com_driver_.tx_cb("L\r\n", 1000);
//        // If we confirmed that RSSI measurement is enabled, now wait for a measurement
//        if(result == BT_MESSAGE_PROMT_RSSION)
//        {
//            // Wait for the measurement to come in
//            result = BTWaitForMessage(5000);
//            // If we got the RSSI response back then send the results back to the PC
//            if(result == BT_MESSAGE_PROMT_RSSIVALUE)
//            {
//                i = 0;
//                outFuncDataArray.asInt[i++] = BTHandle.RSSIFullValue;
//                outFuncDataArray.asInt[i++] = BTHandle.RSSICurrent;
//                outFuncDataArray.asInt[i++] = BTHandle.RSSIMin;
//                sendDataVIARingBuffer(&commHandle, PROTO_BT_RSSI_ACK, outFuncDataArray.asUChar, i*4);
//                // Now turn off the RSSI measurement. Issue the command again so it turns off the RSSI measurement
//                uint32_t result = com_driver_.tx_cb("L\r\n", 1000);
//                // Parse in any pending measurement responses
//                char waitForRSSIOff = 1;
//                while(waitForRSSIOff)
//                {
//                    result = BTWaitForMessage(1000);
//                    // If we read in an old RSSI measurement from the data
//                    if(result == BT_MESSAGE_PROMT_RSSIVALUE)
//                    {
//                        // Ignore any values after we got the first one
//                    }
//                    else if(result == BT_MESSAGE_PROMT_RSSIOFF)
//                    {
//                        // The module has confirmed our command to stop RSSI measuring
//                        waitForRSSIOff = 0;
//                    }
//                    else
//                    {
//                        // Error - either from timeout or some other source
//                        waitForRSSIOff = 0;
//                    }
//
//                }
//            }
//            else
//            {
//                // Issue L again to turn of the RSSI
//                result = com_driver_.tx_cb("L\r\n", 1000);
//
//                // Send an error
//                outFuncDataArray.asInt[0] = ERROR_BL_BT_FAILTOREADRSSI;
//                outFuncDataArray.asInt[1] = __LINE__;
//                strncpy((char *)&outFuncDataArray.asUChar[8], __FILE__, ERROR_MAXFILELENGTH);
//                sendDataVIARingBuffer(&commHandle, PROTO_BL_ERROR, outFuncDataArray.asUChar, 2*sizeof(uint32_t) + ERROR_MAXFILELENGTH);
//            }
//        }
//        else if(result == BT_MESSAGE_PROMT_NOTCONNECTED)
//        {
//            outFuncDataArray.asInt[0] = ERROR_BL_BT_RSSINOTCONNECTED;
//            outFuncDataArray.asInt[1] = __LINE__;
//            strncpy((char *)&outFuncDataArray.asUChar[8], __FILE__, ERROR_MAXFILELENGTH);
//            sendDataVIARingBuffer(&commHandle, PROTO_BL_ERROR, outFuncDataArray.asUChar, 2*sizeof(uint32_t) + ERROR_MAXFILELENGTH);
//        }
//
//        /* Issue the command mode exit string, and wait for the confirmation */
//        result = com_driver_.tx_cb("---\r\n", 1000);
//        /* If the module confirms we exited command mode */
//        if(result != BT_MESSAGE_PROMT_END)
//        {
//            /* We have an error, but I don't know how to best handle it yet. */
//            break;
//        }
//    }
//}

//Rn42::Result Rn42::waitForMessage(uint32_t timeout)
//{
//    char result;
//    bool waiting = true;
//    uint32_t tick_start = tick_cb();
//
//    struct RingBufferCharType * = BTHandle_in->UARTAppHandle.commRingBufferHandle.RBRx;
//
//    //Identify what kind of message we received based on our list of possible message headers
//    result = findPromt(,
//                                        BTHandle_in->BTMessagePromtArray,
//                                        BTHandle_in->BTMessagePromtNumStrings,
//                                        tick_start,
//                                        timeout);
//
//    //Process the signal strength if its an RSSI response header
//    if(result == BT_MESSAGE_PROMT_RSSIVALUE)
//    {
//        waiting = 1;
//        // This array will hold our hex value
//        char hexNum[3];
//
//        while(waiting)
//        {
//            // Wait until we have the whole response
//            if(ringBuffDataAvailableCount() >= 7)
//            {
//                // Read in the current RSSI hex value
//                // Read in the first of two hex characters
//                hexNum[0] = data[tail];
//                // Increment the tail to the next character
//                tail = ringBuffMoveIndex(, tail, 1);
//                // Read in the second of two hex characters
//                hexNum[1] = data[tail];
//                // Increment the tail to the next character
//                tail = ringBuffMoveIndex(, tail, 1);
//                // Add the null character to terminate the string
//                hexNum[2] = '\0';
//                // Convert the hex string to a decimal number
//                (*BTHandle_in).RSSICurrent = strtol(hexNum, NULL, 16);
//
//                // Skip over the comma
//                // Increment the tail to the next character
//                tail = ringBuffMoveIndex(, tail, 1);
//
//                // Read in the low-watermark RSSI hex value
//                // Read in the first of two hex characters
//                hexNum[0] = data[tail];
//                // Increment the tail to the next character
//                tail = ringBuffMoveIndex(, tail, 1);
//                // Read in the second of two hex characters
//                hexNum[1] = data[tail];
//                // Increment the tail to the next character
//                tail = ringBuffMoveIndex(, tail, 1);
//                // Add the null character to terminate the string
//                hexNum[2] = '\0';
//                // Convert the hex string to a decimal number
//                (*BTHandle_in).RSSIMin = strtol(hexNum, NULL, 16);
//
//                // Skip over the terminating characters \r\n
//                // Increment the tail to the next character
//                tail = ringBuffMoveIndex(, tail, 2);
//
//                // We are now done processing this response
//                waiting = 0;
//            }
//
//            if(util_GetTick() >= (tick_start + timeout))
//            {
//                return result = BT_MESSAGE_PROMT_TIMEOUT;
//            }
//        }
//    }
//    return result;
//}

void Rn42::delay(uint32_t delay)
{
    uint32_t msStart = tick_cb();
    uint32_t msFinish = msStart+delay;
    /* if the counter will overflow while waiting */
    if(msFinish < msStart)
        /* then wait until it overflows and then count up */
        while(tick_cb() >= msStart);
    /* wait until the count is equal to the start+delay */
    while(tick_cb() < msFinish);
}

