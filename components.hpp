#include <iostream>
#include <string>
#include <vector>

//=====================================================//
//  1. КОМПОНЕНТЫ (COMPONENTS)
//=====================================================//

// только для котировки

struct TradingSessionSubID
{
    int value;
};

// для котировок, ордера и сделки

enum class SideT
{
    Buy,
    Sell
};

struct Side
{
    SideT value;
};

struct Price
{
    double value;
};

struct Quantity
{
    int value;
};

struct ID
{
    int value;
};

// для ордера и сделки

enum class OrderTypeT
{
    Limit,
    Market
};

struct Type
{
    OrderTypeT value;
};

enum class TimeInForceT
{
    GTC,   // Good Till Cancel
    IOC,   // Immediate Or Cancel
    FOK    // Fill Or Kill
};

struct TimeInForce
{
    TimeInForceT value;
};

struct InitiatorLogin
{
    std::string value;
};

struct InitiatorAccount
{
    std::string value;
};

struct InstrumentId
{
    int value;
};

// Специфичные "теги" или флаги
struct ActiveTag {};
struct FilledTag {};
struct OrderTag {};
struct TradeTag {};
struct RetiredTradeTag {};

// Только у Trade
struct IsUploaded
{
    bool value;
};
