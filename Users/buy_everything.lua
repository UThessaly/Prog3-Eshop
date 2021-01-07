local shop = require('shop')

while true do
    local amount, items = shop.get_items()

    if amount == 0 then
        break
    end

    local item_to_buy = math.random(1, amount)

    local bought = shop.buy_item(items[item_to_buy])

    if bought == "Bought" then
        log("Bought Product " .. items[item_to_buy].name)
    else
        log("Error: " .. bought .. " when trying to buy product " .. items[item_to_buy].name)
    end

    shop.sleep(1)
end