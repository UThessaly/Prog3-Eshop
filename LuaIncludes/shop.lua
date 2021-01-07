local json = require('json')

local clock = os.clock
local function sleep(n)  -- seconds
  local t0 = clock()
  while clock() - t0 <= n do end
end

local function ItemToTable(item) 
    return {
        id = item.id,
        name = item.name,
        description = item.description,
        count = item.count,
        price = item.stock
    }
end

local function TableToItem(table)
    return Item.new(table.id, table.name, table.description, table.count, table.price)
end


local shop = {}

shop.sleep = sleep
local request_id = 0

local child = get_child()
shop.child = child

local function create_request(action, data)
    local id = request_id
    request_id = request_id + 1

    return {
        id = id,
        action = action,
        data = data
    }
end

local get_data_co = coroutine.create(function ()
    while true do
        local response = child:read()
        if(response == "") then
        else
            coroutine.yield(response)
        end
    end
  end)

local function make_request(action, data)
    local request_table = create_request(action, data)

    child:write(json.encode(request_table))

    local has_data, response_str = coroutine.resume(get_data_co)

    return json.decode(response_str)
end


shop.get_items = function()
    local response = make_request("products:get", {})

    if response.data.items == 0 then
        return 0, {}
    end

    if response.data.items == nil then 
        return 0, {}
    end

    local items = {}
    for index, value in pairs(response.data.items) do
        -- log(json.encode(response.data.items))
        items[index] = TableToItem(value)
    end

    return response.data.item_count, items
end

shop.buy_item = function(item)
    local response = make_request("products:buy", { product_id = item.id })

    if response.error then
        return response.error.message
    else
        return "Bought"
    end
end

return shop