#ifndef MUC_STOCKSERVICE_INC_STOCKSERVICE_H
#define MUC_STOCKSERVICE_INC_STOCKSERVICE_H

#include <cstdint>

namespace muc::stock
{

struct StockQuote
{
    double price;
};

bool fetch_nvda_price(StockQuote& out);

} // namespace muc::stock

#endif // MUC_STOCKSERVICE_INC_STOCKSERVICE_H
