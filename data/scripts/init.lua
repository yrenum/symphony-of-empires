require("api")

-- Culture
local japanesse = Culture:create{ ref_name = "japanesse" }
japanesse.name = _("Japanesse")
Culture:register(japanesse)
local russian = Culture:create{ ref_name = "russian" }
russian.name = _("Russian")
Culture:register(russian)

-- Religion
local good_bois = Religion:create{ ref_name = "good_bois" }
good_bois.name = _("Good bois")
Religion:register(good_bois)
local bad_bois = Religion:create{ ref_name = "bad_bois" }
bad_bois.name = _("Bad bois")
Religion:register(bad_bois)

-- Pop types
function entrepreneur_on_tick()
	--print("hello entrepreneur!")
end
local entrepreneur = PopType:create{ ref_name = "entrepreneur", on_tick_fn = "entrepreneur_on_tick" }
entrepreneur.name = _("Entrepreneur")
PopType:register(entrepreneur)

function artisan_on_tick()
	--print("hello artisan!")
end
local artisan = PopType:create{ ref_name = "artisan", on_tick_fn = "artisan_on_tick" }
artisan.name = _("Artisan")
PopType:register(artisan)

function craftsmen_on_tick()
	--print("hello craftsmen!")
end
local craftsmen = PopType:create{ ref_name = "craftsmen", on_tick_fn = "craftsmen_on_tick" }
craftsmen.name = _("Craftsmen")
PopType:register(craftsmen)

function bureaucrat_on_tick()
	--print("hello bureaucrat!")
end
local bureaucrat = PopType:create{ ref_name = "bureaucrat", on_tick_fn = "bureaucrat_on_tick" }
bureaucrat.name = _("Bureaucrat")
PopType:register(bureaucrat)

function aristocrat_on_tick()
	--print("hello aristocrat!")
end
local aristocrat = PopType:create{ ref_name = "aristocrat", on_tick_fn = "aristocrat_on_tick" }
aristocrat.name = _("Aristocrat")
PopType:register(aristocrat)

function clergymen_on_tick()
	--print("hello clergymen!")
end
local clergymen = PopType:create{ ref_name = "clergymen", on_tick_fn = "clergymen_on_tick" }
clergymen.name = _("Clergymen")
PopType:register(clergymen)

function farmer_on_tick()
	--print("hello farmer!")
end
local farmer = PopType:create{ ref_name = "farmer", on_tick_fn = "farmer_on_tick" }
farmer.name = _("Farmer")
PopType:register(farmer)

function soldier_on_tick()
	--print("hello soldier!")
end
local soldier = PopType:create{ ref_name = "soldier", on_tick_fn = "soldier_on_tick" }
soldier.name = _("Soldier")
PopType:register(soldier)

function laborer_on_tick()
	--print("hello laborer!")
end
local laborer = PopType:create{ ref_name = "laborer", on_tick_fn = "laborer_on_tick" }
laborer.name = _("Laborer")
PopType:register(laborer)

-- Good types

-- Farmable stuff
local wheat = Good:create{ ref_name = "wheat" }
wheat.name = _("Wheat")
Good:register(wheat)
local grapes = Good:create{ ref_name = "grapes" }
grapes.name = _("Grapes")
Good:register(grapes)
local rice = Good:create{ ref_name = "rice" }
rice.name = _("Rice")
Good:register(rice)
local cotton = Good:create{ ref_name = "cotton" }
cotton.name = _("Cotton")
Good:register(cotton)
local coffe_bean = Good:create{ ref_name = "coffe_bean" }
coffe_bean.name = _("Coffe bean")
Good:register(coffe_bean)
local salt_grain = Good:create{ ref_name = "salt_grain" }
salt_grain.name = _("Salt grain")
Good:register(salt_grain)
local silk = Good:create{ ref_name = "silk" }
silk.name = _("Silk")
Good:register(silk)

-- Industrial materials
local fabric = Good:create{ ref_name = "fabric" }
fabric.name = _("Fabric")
Good:register(fabric)
local dye = Good:create{ ref_name = "dye" }
dye.name = _("Dye")
Good:register(dye)
local textile = Good:create{ ref_name = "textile" }
textile.name = _("Textile")
Good:register(textile)
local plastic = Good:create{ ref_name = "plastic" }
plastic.name = _("Plastic")
Good:register(plastic)

-- Minerals
local copper = Good:create{ ref_name = "copper" }
copper.name = _("Copper")
Good:register(copper)
local silica = Good:create{ ref_name = "silica" }
silica.name = _("Silica")
Good:register(silica)
local uranium = Good:create{ ref_name = "uranium" }
uranium.name = _("Uranium")
Good:register(uranium)
local gold = Good:create{ ref_name = "gold" }
gold.name = _("Gold")
Good:register(gold)
local iron = Good:create{ ref_name = "iron" }
iron.name = _("Iron")
Good:register(iron)
local diamond = Good:create{ ref_name = "diamond" }
diamond.name = _("Diamond")
Good:register(diamond)
local titanium = Good:create{ ref_name = "titanium" }
titanium.name = _("Titanium")
Good:register(titanium)
local oil = Good:create{ ref_name = "oil" }
oil.name = _("Oil")
Good:register(oil)
local timber = Good:create{ ref_name = "timber" }
timber.name = _("Timber")
Good:register(timber)
local coal = Good:create{ ref_name = "coal" }
coal.name = _("Coal")
Good:register(coal)

-- Food
local glass = Good:create{ ref_name = "glass" }
glass.name = _("Glass")
Good:register(glass)
local bread = Good:create{ ref_name = "bread" }
bread.name = _("Bread")
Good:register(bread)
local salt = Good:create{ ref_name = "salt" }
salt.name = _("Salt")
Good:register(salt)

-- Commodities
local window = Good:create{ ref_name = "window" }
window.name = _("Window")
Good:register(window)
local wine = Good:create{ ref_name = "wine" }
wine.name = _("Wine")
Good:register(wine)
local gasoline = Good:create{ ref_name = "gasoline" }
gasoline.name = _("Gasoline")
Good:register(gasoline)
local chair = Good:create{ ref_name = "chair" }
chair.name = _("Chair")
Good:register(chair)
local bed = Good:create{ ref_name = "bed" }
bed.name = _("Bed")
Good:register(bed)
print("good types")

-- Industry types

-- Primary sector industries (the most essential)
local sand_smelter = IndustryType:create{ ref_name = "sand_smelter" }
sand_smelter.name = _("Sand smelter")
IndustryType:register(sand_smelter)
-- glass factories can create glass from thin air
IndustryType:add_output(sand_smelter, glass)

local oil_refinery = IndustryType:create{ ref_name = "oil_refinery" }
oil_refinery.name = _("Oil refinery")
IndustryType:register(oil_refinery)
-- thin air oil, i think the air needs some democracy!
IndustryType:add_output(oil_refinery, oil)

local lumberjack = IndustryType:create{ ref_name = "lumberjack" }
lumberjack.name = _("Lumberjack")
IndustryType:register(lumberjack)
-- same logic here
IndustryType:add_output(lumberjack, timber)

local vineyard = IndustryType:create{ ref_name = "vineyard" }
vineyard.name = _("Vineyard")
IndustryType:register(vineyard)
IndustryType:add_output(vineyard, grapes)

local wheat_farm = IndustryType:create{ ref_name = "wheat_farm" }
wheat_farm.name = _("Wheat farm")
IndustryType:register(wheat_farm)
IndustryType:add_output(wheat_farm, wheat)

local rice_farm = IndustryType:create{ ref_name = "rice_farm" }
rice_farm.name = _("Rice farm")
IndustryType:register(rice_farm)
IndustryType:add_output(rice_farm, wheat)

local cotton_farm = IndustryType:create{ ref_name = "cotton_farm" }
cotton_farm.name = _("Cotton farm")
IndustryType:register(cotton_farm)
IndustryType:add_output(cotton_farm, wheat)

-- Mines can create stuff from thin air, but don't worry because this is
-- not abuse-ble
local diamond_mine = IndustryType:create{ ref_name = "diamond_mine" }
diamond_mine.name = _("Diamond mine")
IndustryType:register(diamond_mine)
IndustryType:add_output(diamond_mine, diamond)

local iron_mine = IndustryType:create{ ref_name = "iron_mine" }
iron_mine.name = _("Iron mine")
IndustryType:register(iron_mine)
IndustryType:add_output(iron_mine, iron)

local gold_mine = IndustryType:create{ ref_name = "gold_mine" }
gold_mine.name = _("Gold mine")
IndustryType:register(gold_mine)
IndustryType:add_output(gold_mine, gold)

local titanium_mine = IndustryType:create{ ref_name = "titanium_mine" }
titanium_mine.name = _("Titanium mine")
IndustryType:register(titanium_mine)
IndustryType:add_output(titanium_mine, titanium)
print("primary sector industries")

-- Secondary sector industries (now the fun begins)
local chair_manufacturer = IndustryType:create{ ref_name = "chair_manufacturer" }
chair_manufacturer.name = _("Chair manufacturer")
IndustryType:register(chair_manufacturer)
IndustryType:add_input(chair_manufacturer, timber)
IndustryType:add_output(chair_manufacturer, chair)

local bed_manufacturer = IndustryType:create{ ref_name = "bed_manufacturer" }
bed_manufacturer.name = _("Bed manufacturer")
IndustryType:register(bed_manufacturer)
IndustryType:add_input(bed_manufacturer, timber)
IndustryType:add_output(bed_manufacturer, bed)
print("secondary sector industries")

-- Nations
local nation = Nation:create{ ref_name = "argentina", color = 0x00a9ff, default_flag = "flags/argentina_republic.png" }
nation.name = _("Argentina")
Nation:register(nation)

local nation = Nation:create{ ref_name = "brazil", color = 0x4CFF00, default_flag = "flags/brazil_monarchy.png" }
nation.name = _("Brazil")
Nation:register(nation)

local nation = Nation:create{ ref_name = "gran_colombia", color = 0xE1FF91, default_flag = "flags/gran_colombia_republic.png" }
nation.name = _("Gran colombia")
Nation:register(nation)

local nation = Nation:create{ ref_name = "peru", color = 0xFF2158, default_flag = "flags/peru_republic.png" }
nation.name = _("Peru")
Nation:register(nation)

local nation = Nation:create{ ref_name = "bolivia", color = 0xffc4ee, default_flag = "flags/bolivia_republic.png" }
nation.name = _("Bolivia")
Nation:register(nation)

local nation = Nation:create{ ref_name = "paraguay", color = 0xb71cff, default_flag = "flags/paraguay_republic.png" }
nation.name = _("Paraguay")
Nation:register(nation)

local nation = Nation:create{ ref_name = "chile", color = 0x241f3d, default_flag = "flags/chile_republic.png" }
nation.name = _("Chile")
Nation:register(nation)

local nation = Nation:create{ ref_name = "federated_republic_central_america", color = 0x5199ff, default_flag = "flags/federated_republic_central_america_republic.png" }
nation.name = _("Federated Republic of Central America")
Nation:register(nation)

local nation = Nation:create{ ref_name = "mexico", color = 0x92FF60, default_flag = "flags/mexico_republic.png" }
nation.name = _("Mexico")
Nation:register(nation)

local nation = Nation:create{ ref_name = "united_states", color = 0x639FFF, default_flag = "flags/united_states_democracy.png" }
nation.name = _("United states")
Nation:register(nation)
print("nations")

-- Provinces

--------------------------------------------------------------
-- Continent: America
-- Subcontinent: South America
-- Country: Brazil

local name_of_province = Province:create{ ref_name = "ceara", color = 0x00FFFF }
name_of_province.name = _("Ceará")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "rio_grande_do_norte", color = 0xFF006E }
name_of_province.name = _("Rio Grande do Norte")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "paraiba", color = 0x4CFF00 }
name_of_province.name = _("Paraíba")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "pernambuco", color = 0x7F7F }
name_of_province.name = _("Pernambuco")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "alagoas", color = 0xB6FF00 }
name_of_province.name = _("Alagoas")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "sergipe", color = 0xFF6A00 }
name_of_province.name = _("Sergipe")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "bahia", color = 0xA0A0A0 }
name_of_province.name = _("Bahia")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "sao_francisco", color = 0xB200FF }
name_of_province.name = _("São Francisco")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "piaui", color = 0x49B0FF }
name_of_province.name = _("Piauí")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "maranhao", color = 0xC289FF }
name_of_province.name = _("Maranhão")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "grao_para", color = 0xFF6DDD }
name_of_province.name = _("Grão-Pará")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "roraima", color = 0xFFFD8E }
name_of_province.name = _("Roraima")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "mato_grosso", color = 0x9DFF35 }
name_of_province.name = _("Mato Grosso")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "goias", color = 0x49FF62 }
name_of_province.name = _("Goiás")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "minas_gerais", color = 0x9E0049 }
name_of_province.name = _("Minas Gerais")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "sao_paulo", color = 0x7FFF8E }
name_of_province.name = _("São Paulo")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "santa_catarina", color = 0xFF545F }
name_of_province.name = _("Santa Catarina")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "oeste_catarinense", color = 0xFF26A4 }
name_of_province.name = _("Oeste Catarinense")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "porto_alegre", color = 0x96FFF8 }
name_of_province.name = _("Porto Alegre")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "bage", color = 0xD089FF }
name_of_province.name = _("Bagé")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "cisplatina", color = 0xFFEA84 }
name_of_province.name = _("Cisplatina")
Province:register(name_of_province)
--------------------------------------------------------------
-- Continent: Americas
-- Subcontinent: South America
-- Country: Paraguay

local name_of_province = Province:create{ ref_name = "asuncion", color = 0x00C7FF }
name_of_province.name = _("Asunción")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "vila_sacaron", color = 0xB71CFF }
name_of_province.name = _("Vila Sacarón")
Province:register(name_of_province)

local name_of_province = Province:create{ ref_name = "nioaque", color = 0x5174FF }
name_of_province.name = _("Nioaque")
Province:register(name_of_province)


-- THIS IS NOT PART OF THE FINAL MODDING API

-- Companies

-- local rival_transport_company = Company:create{ name = "Rival Transport Corporate", money = 10000000, is_transport = true }
-- Company:register(rival_transport_company)
-- add_op_province_to_company(1, hokkaido_tohoku.ref_name)

print("companies")

print("loaded mod.lua")
