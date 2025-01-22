#include <entt/entt.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <tuple>

#include "components.hpp"

static int gVolumeLimit = 50;  // общий лимит
static bool gCanMatch = true;   // флаг «можно ли запускать MatchingSystem»

//=====================================================//
//  2. "СИСТЕМЫ" (функции, работающие с registry)
//    В EnTT мы обычно пишем свободные функции,
//    которые итерируют по необходимым компонентам.
//=====================================================//

inline void QuoteUpdateSystem(entt::registry& registry)
{
    // Здесь предполагаем, что Quotes — это сущности без OrderTag и TradeTag,
    // но у которых есть Side, Price (и, возможно, Quantity).
    auto view = registry.view<Price>(entt::exclude<OrderTag, TradeTag>);

    for (auto entity : view)
    {
        auto &price = view.get<Price>(entity);
        if (price.value < 0.0)
        {
            std::cout << "[QuoteUpdateSystem] Removing quote with negative price: "
                      << price.value << "\n";
            registry.destroy(entity);
        }
    }
}

// Это аналог "OrderBookSystem", где мы хотели собрать все (Side, Price, Quantity),
// исключая TradeTag, и затем построить упрощённый стакан (top-5 buy и sell).
inline void OrderBookSystem(entt::registry& registry)
{
    // Собираем все котировки (Side, Price, Quantity), исключая TradeTag
    auto view = registry.view<Side, Price, Quantity>(entt::exclude<TradeTag, FilledTag>);

    // 1. Сбор всех котировок в вектор
    std::vector<std::tuple<SideT, double, int>> allQuotes;
    allQuotes.reserve(view.size_hint());

    for (auto entity : view)
    {
        auto &side  = view.get<Side>(entity);
        auto &price = view.get<Price>(entity);
        auto &qty   = view.get<Quantity>(entity);

        allQuotes.emplace_back(side.value, price.value, qty.value);
    }

    // 2. Делим на Buy / Sell
    std::vector<std::pair<double,int>> buyQuotes;
    std::vector<std::pair<double,int>> sellQuotes;

    for (auto &q : allQuotes)
    {
        SideT s   = std::get<0>(q);
        double pr = std::get<1>(q);
        int vol   = std::get<2>(q);

        if (s == SideT::Buy)
        {
            buyQuotes.push_back({pr, vol});
        }
        else
        {
            sellQuotes.push_back({pr, vol});
        }
    }

    // 3. Сортировка
    //   Buy -> по убыванию
    std::sort(buyQuotes.begin(), buyQuotes.end(),
              [](auto &a, auto &b)
              {
                  return a.first > b.first;
              });
    //   Sell -> по возрастанию
    std::sort(sellQuotes.begin(), sellQuotes.end(),
              [](auto &a, auto &b)
              {
                  return a.first < b.first;
              });

    // 4. Обрезаем до 5 уровней
    if (buyQuotes.size() > 5)
    {
        buyQuotes.resize(5);
    }
    if (sellQuotes.size() > 5)
    {
        sellQuotes.resize(5);
    }

    // 5. Выводим стакан
    std::cout << "\n[OrderBookSystem] --- CURRENT ORDER BOOK (top 5) ---\n";
    std::cout << "   BUY side:\n";
    for (auto &b : buyQuotes)
    {
        std::cout << "      Price=" << b.first << ", Vol=" << b.second << "\n";
    }
    std::cout << "   SELL side:\n";
    for (auto &s : sellQuotes)
    {
        std::cout << "      Price=" << s.first << ", Vol=" << s.second << "\n";
    }
    std::cout << "------------------------------------------------------\n";
}

//    Если gCanMatch == false, ничего не делаем.
//    Если true, находим пары (Buy, Sell), которые
//    удовлетворяют условию (buy.price >= sell.price).
//    Создаём 2 сделки (2 сущности с TradeTag),
//    "гасим" исходные ордера: ставим FilledTag, убираем ActiveTag.
inline void MatchingSystem(entt::registry& registry)
{
    if (!gCanMatch)
    {
        std::cout << "[MatchingSystem] Skipped due to limit.\n";
        return;
    }

    // Соберём все BUY ордера (OrderTag, ActiveTag, Side=Buy, без FilledTag)
    struct OrderData
    {
        entt::entity entity;
        double price;
        int volume;
    };
    std::vector<OrderData> buyOrders;
    std::vector<OrderData> sellOrders;

    // BUY
    {
        auto view = registry.view<OrderTag, ActiveTag, Side, Price, Quantity>(
            entt::exclude<FilledTag>);
        for (auto entity : view)
        {
            auto &side = view.get<Side>(entity);
            if (side.value == SideT::Buy)
            {
                buyOrders.push_back({
                    entity,
                    view.get<Price>(entity).value,
                    view.get<Quantity>(entity).value
                });
            }
        }
    }

    // SELL
    {
        auto view = registry.view<OrderTag, ActiveTag, Side, Price, Quantity>(
            entt::exclude<FilledTag>);
        for (auto entity : view)
        {
            auto &side = view.get<Side>(entity);
            if (side.value == SideT::Sell)
            {
                sellOrders.push_back({
                    entity,
                    view.get<Price>(entity).value,
                    view.get<Quantity>(entity).value
                });
            }
        }
    }

    // Можно отсортировать buy (по убыванию цены), sell (по возрастанию),
    // если хотим "best match".
    // Для простоты здесь оставим в порядке создания.

    // Для каждого buy попробуем найти любой sell, где buyPrice >= sellPrice
    // и сматчить их (полный fill, без учёта частичного).
    for (auto &b : buyOrders)
    {
        if (b.volume <= 0)
        {
            continue; // вдруг уже поменяли
        }
        for (auto &s : sellOrders)
        {
            if (s.volume <= 0)
            {
                continue;
            }
            // Условие на совпадение
            if (b.price >= s.price)
            {
                // match => создаём 2 сделки: buy side + sell side
                // volume = min(b.volume, s.volume)
                int matchedVol = std::min(b.volume, s.volume);

                std::cout << "[MatchingSystem] Match found: buy("
                          << int(b.entity) << ", price=" << b.price
                          << ", vol=" << b.volume << ") vs sell("
                          << int(s.entity) << ", price=" << s.price
                          << ", vol=" << s.volume << ") => matchedVol="
                          << matchedVol << "\n";

                // 1) Создаём сделку (Trade) для покупателя
                {
                    auto tradeE = registry.create();
                    registry.emplace<TradeTag>(tradeE);
                    registry.emplace<Side>(tradeE, SideT::Buy);
                    registry.emplace<Price>(tradeE, s.price); // обычно deal по sellPrice
                    registry.emplace<Quantity>(tradeE, matchedVol);
                    registry.emplace<IsUploaded>(tradeE, false);
                }
                // 2) Создаём сделку (Trade) для продавца
                {
                    auto tradeE = registry.create();
                    registry.emplace<TradeTag>(tradeE);
                    registry.emplace<Side>(tradeE, SideT::Sell);
                    registry.emplace<Price>(tradeE, s.price);
                    registry.emplace<Quantity>(tradeE, matchedVol);
                    registry.emplace<IsUploaded>(tradeE, false);
                }

                // Считаем, что оба ордера частично/полностью исполнены
                b.volume -= matchedVol;
                s.volume -= matchedVol;

                // Если объём 0 => ставим FilledTag
                if (b.volume <= 0)
                {
                    registry.remove<OrderTag>(b.entity);
                    registry.emplace<FilledTag>(b.entity);
                    registry.remove<ActiveTag>(b.entity);
                    std::cout << "   => Order(" << int(b.entity) << ") filled!\n";
                }
                if (s.volume <= 0)
                {
                    registry.remove<OrderTag>(s.entity);
                    registry.emplace<FilledTag>(s.entity);
                    registry.remove<ActiveTag>(s.entity);
                    std::cout << "   => Order(" << int(s.entity) << ") filled!\n";
                }

                // Здесь можно обрабатывать частичное исполнение (если кто-то ещё >0)
                // Для простоты — match один buy c одним sell,
                // и если b.volume >0, он может ещё match c другим sell.
                // (Цикл продолжится)

                // Если sell полностью исчерпан, переходим к следующему sell
                if (s.volume <= 0)
                {
                    continue;
                }
            }
        }
    }
}

inline void CheckLimitsSystem(entt::registry& registry)
{
    int sumVol = 0;

    // 1) Считаем объём у активных ордеров
    {
        auto view = registry.view<OrderTag, ActiveTag, Quantity>();
        for (auto entity : view)
        {
            sumVol += view.get<Quantity>(entity).value;
        }
    }

    // 2) Считаем объём у сделок (TradeTag), которые НЕ имеют RetiredTradeTag
    {
        auto view = registry.view<TradeTag, Quantity>(entt::exclude<RetiredTradeTag>);
        for (auto entity : view)
        {
            sumVol += view.get<Quantity>(entity).value;
        }
    }

    std::cout << "[CheckLimitsSystem] sumVol=" << sumVol
              << ", gVolumeLimit=" << gVolumeLimit << std::endl;

    // Если sumVol > лимит => запрещаем матчинг
    gCanMatch = (sumVol <= gVolumeLimit);

    if (!gCanMatch)
    {
        std::cout << "[CheckLimitsSystem] Limit exceeded! sumVol=" << sumVol
                  << " > " << gVolumeLimit << "\n";
    }

    // 3) «Если сделка стала RetiredTradeTag», то убавляем лимит на её объём
    {
        auto view = registry.view<TradeTag, RetiredTradeTag, Quantity>();
        for (auto entity : view)
        {
            int vol = view.get<Quantity>(entity).value;

            // Убавляем лимит
            gVolumeLimit -= vol;
            std::cout << "[CheckLimitsSystem] Found retired trade entity=" << int(entity)
                      << ", volume=" << vol
                      << " => new gVolumeLimit=" << gVolumeLimit << "\n";

            // Чтобы не вычитать снова на каждом тике, снимем RetiredTradeTag
            // (или можно хранить флаг, что уже учли).
            registry.destroy(entity);
        }
    }
}
