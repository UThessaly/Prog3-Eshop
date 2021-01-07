local shop = require('shop')

local money = math.random(50, 500) or 1000

local function find_item_to_buy(items)
    for index, value in pairs(items) do
        if money > value.price then return true, index end
    end
    return false, -1
end

while true do
    local amount, items = shop.get_items()

    if amount == 0 then
        break
    end

    local found, item_to_buy = find_item_to_buy(items)

    if not found then
        log("Not enough money to buy anything...")
        break
    end

    local bought = shop.buy_item(items[item_to_buy])

    if bought == "Bought" then
        log("Bought Product " .. items[item_to_buy].name)
        money = money - items[item_to_buy].price
    else
        log("Error: " .. bought .. " when trying to buy product " .. items[item_to_buy].name)
    end
end