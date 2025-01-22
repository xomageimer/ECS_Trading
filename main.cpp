#include <entt/entt.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <tuple>

#include "systems.hpp"

//=====================================================//
//  3. СОЗДАНИЕ СУЩНОСТЕЙ (ENTITIES) В EnTT
//=====================================================//

// Пример создания «Order»
entt::entity CreateOrder(entt::registry& reg, int id, double price, int vol, SideT side) {
    auto e = reg.create();
    reg.emplace<ID>(e, id);
    reg.emplace<Side>(e, side);
    reg.emplace<Price>(e, price);
    reg.emplace<Quantity>(e, vol);
    reg.emplace<OrderTag>(e);
    reg.emplace<ActiveTag>(e);
    return e;
}

// Пример создания «Trade»
entt::entity CreateTradeEntity(entt::registry& registry)
{
    entt::entity e = registry.create();
    registry.emplace<ID>(e, 456);
    registry.emplace<Side>(e, SideT::Sell);
    registry.emplace<Price>(e, 99.5);
    registry.emplace<Quantity>(e, 10);
    registry.emplace<Type>(e, OrderTypeT::Market);
    registry.emplace<InitiatorLogin>(e, "user555");
    registry.emplace<InitiatorAccount>(e, "ACC-0010");
    registry.emplace<InstrumentId>(e, 999);
    registry.emplace<TimeInForce>(e, TimeInForceT::IOC);
    registry.emplace<IsUploaded>(e, false);
    // Тег TradeTag
    registry.emplace<TradeTag>(e);
    return e;
}

// Дополнительная функция для примера: создание нескольких «Quote»
void CreateSampleQuotes(entt::registry& registry)
{
    // "Buy" #1
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Buy);
        registry.emplace<Price>(e, 99.60);
        registry.emplace<Quantity>(e, 8);
        registry.emplace<ID>(e, 2);
    }
    // "Buy" #2
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Buy);
        registry.emplace<Price>(e, 97.56);
        registry.emplace<Quantity>(e, 54);
        registry.emplace<ID>(e, 3);
    }
    // "Buy" #3
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Buy);
        registry.emplace<Price>(e, 98.333);
        registry.emplace<Quantity>(e, 15);
        registry.emplace<ID>(e, 4);
    }
    // "Buy" #4
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Buy);
        registry.emplace<Price>(e, 105.f);
        registry.emplace<Quantity>(e, 23);
        registry.emplace<ID>(e, 5);
    }
    // "Buy" #5
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Buy);
        registry.emplace<Price>(e, 100.f);
        registry.emplace<Quantity>(e, 56);
        registry.emplace<ID>(e, 6);
    }
    // "Sell" #1
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Sell);
        registry.emplace<Price>(e, 99.70);
        registry.emplace<Quantity>(e, 5);
        registry.emplace<ID>(e, 7);
    }
    // "Sell" #2
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Sell);
        registry.emplace<Price>(e, 105.70);
        registry.emplace<Quantity>(e, 54);
        registry.emplace<ID>(e, 8);
    }
    // "Sell" #3
    {
        auto e = registry.create();
        registry.emplace<Side>(e, SideT::Sell);
        registry.emplace<Price>(e, -13.70);
        registry.emplace<Quantity>(e, 100);
        registry.emplace<ID>(e, 8);
    }

}


//=====================================================//
//  4. main: ИНИЦИАЛИЗАЦИЯ, ЗАПУСК "СИСТЕМ"
//=====================================================//

int main()
{
    entt::registry registry;

    // Пример: создаём несколько ордеров
    // - Buy(1, price=100, vol=10)
    // - Buy(2, price=99, vol=5)
    // - Sell(3, price=99.5, vol=5)
    // - Sell(4, price=98, vol=10)
    CreateOrder(registry, 1, 100.0, 10, SideT::Buy);
    CreateOrder(registry, 2, 99.0, 5, SideT::Buy);
    CreateOrder(registry, 3, 99.5, 5, SideT::Sell);
    CreateOrder(registry, 4, 98.0, 10, SideT::Sell);

    // Создаём примеры
    auto e = CreateTradeEntity(registry);
    CreateSampleQuotes(registry);

    // Имитируем несколько "тиков" (итераций), где мы вызываем наши "системы"
    int i = 0;
    {
        std::cout << "\n--- TICK " << i++ << " ---\n";

        // "Системы" для Quote
        QuoteUpdateSystem(registry);
        OrderBookSystem(registry);

        // Система для проверки лимитов
        CheckLimitsSystem(registry);

        // "Системы" для Order
        MatchingSystem(registry);
    }

    CreateOrder(registry, 2, 99.0, 5, SideT::Buy);
    CreateOrder(registry, 4, 98.0, 10, SideT::Sell);

    {
        std::cout << "\n--- TICK " << i++ << " ---\n";

        // "Системы" для Quote
        QuoteUpdateSystem(registry);
        OrderBookSystem(registry);

        // Система для проверки лимитов
        CheckLimitsSystem(registry);

        // "Системы" для Order
        MatchingSystem(registry);
    }

    registry.emplace<RetiredTradeTag>(e);

    {
        std::cout << "\n--- TICK " << i++ << " ---\n";

        // "Системы" для Quote
        QuoteUpdateSystem(registry);
        OrderBookSystem(registry);

        // Система для проверки лимитов
        CheckLimitsSystem(registry);

        // "Системы" для Order
        MatchingSystem(registry);
    }

    return 0;
}
