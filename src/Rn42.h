/*
 * rn42.h
 */
#ifndef RN42_H_
#define RN42_H_

/*******************************************************************************
INCLUDED FILES
*******************************************************************************/
#include <stdint.h>
#include <string>

/*******************************************************************************
EXPORTED CONSTANTS
*******************************************************************************/

/*******************************************************************************
EXPORTED DATA TYPES
*******************************************************************************/
class Rn42
{
public:
    /*******************************************************************************
    PUBLIC TYPES
    *******************************************************************************/
    enum Result
    {
        RESULT_OK = 0,
        RESULT_PARAM_SIZE,
        RESULT_FAIL,
        RESULT_TIMEOUT,
        RESULT_PROMT_MATCH,
    };

    /* PROMT IDs */
    enum Promt_i
    {
        BT_MESSAGE_PROMT_ERRORPARSE,
        BT_MESSAGE_PROMT_CMD,
        BT_MESSAGE_PROMT_END,
        BT_MESSAGE_PROMT_AOK,
        BT_MESSAGE_PROMT_RSSION,
        BT_MESSAGE_PROMT_RSSIVALUE,
        BT_MESSAGE_PROMT_RSSIOFF,
        BT_MESSAGE_PROMT_NOTCONNECTED,
        BT_MESSAGE_PROMT_REBOOT,
    };

    struct ComDriver
    {
        Result (*tx_cb)(const char* data, uint32_t size);
        Result (*rx_cb)(char*  data, uint32_t timeout);
    };

    /* Message promt array - holds the possible responses of the BT module in this case */
    struct Promts
    {
        const char *data;
        enum Promt_i id;
    };

    typedef struct
    {
        uint32_t RSSIFullValue;
        uint32_t RSSICurrent;
        uint32_t RSSIMin;
        /* Promt response parsing */
        uint32_t BTMessagePromtNumStrings;
        struct Promts *BTMessagePromtArray;
    } RN42_handle_t;

    /*******************************************************************************
    PUBLIC VARIABLE DECLARATIONS
    *******************************************************************************/
    uint32_t (*tick_cb)();

    /*******************************************************************************
    PUBLIC FUNCTION DECLARATIONS
    *******************************************************************************/
    Rn42(ComDriver com_driver, uint32_t (*tick_cb)());
    Result commandMode();
    Result normalMode();
    Result reboot();
    Result baudRateSet();
    Result nameSet(char* data);
    Result pinNumberSet(char* data);

private:

    /*******************************************************************************
    PRIVATE VARIABLE DECLARATIONS
    *******************************************************************************/
    ComDriver com_driver_;
    int32_t command_promts_n;
    Promts* command_promts;
    uint8_t rssi_value_max;

    /*******************************************************************************
    PRIVATE FUNCTION DECLARATIONS
    *******************************************************************************/
    Result txCommand(const char* data);
    Result rxPromt(char* data, uint32_t timeout);
    Result findPromt(Promt_i* promt_i, const char* data);
    Result command(const char* data, Promt_i promt_i, uint32_t timeout);
    void delay(uint32_t delay);
};

#endif
