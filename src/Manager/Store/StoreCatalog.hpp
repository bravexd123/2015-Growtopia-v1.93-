#pragma once
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <array>
#include <fmt/core.h>

namespace StoreCatalog {

inline const std::unordered_map<std::string, std::string>& GetCategoryContent() {
    static const std::unordered_map<std::string, std::string> kCategories = {
        { "main", "set_description_text|Welcome to the `2Growtopia Store``!  Tap the item you'd like more info on.  `wThanks for being a supporter of Growtopia!\n"
                  "add_button|iap_menu|Buy Gems|interface/large/store_buttons5.rttex||0|2|0|0||\n"
                  "add_button|itempack_menu|Item Packs|interface/large/store_buttons3.rttex||0|3|0|0||\n"
                  "add_button|locks_menu|Locks And Stuff|interface/large/store_buttons3.rttex||0|4|0|0||\n"
                  "add_button|weather_menu|Weather Machines|interface/large/store_buttons5.rttex|Tired of the same sunny sky?  We offer alternatives within...|0|4|0|0||\n"
                  "add_button|bigitems_menu|Awesome Items|interface/large/store_buttons4.rttex||0|6|0|0||\n"
                  "add_button|token_menu|Growtoken Items|interface/large/store_buttons9.rttex||0|0|0|0||\n" },

        { "itempack", "set_description_text|`2Item Packs!``  Tap the item you'd like more info on, or BACK to go back.|\n"
                  "add_button|door_pack|`wDoor And Sign Hello Pack``|interface/large/store_buttons.rttex|Own your very own door and sign! This pack comes with one of each. Leave cryptic messages and create a door that can open to, well, anywhere.|0|3|15|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|basic_splicing_kit|`wBasic Splicing Kit``|interface/large/store_buttons2.rttex|The basic seeds every farmer needs - 10 Rock Seeds and 10 more random seeds of Rarity 2.|0|3|200|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|small_seed_pack|`wSmall Seed Pack``|interface/large/store_buttons.rttex|You'll get 5 randomly chosen seeds. Who knows what you'll get?!|1|4|50|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|rare_seed_pack|`wRare Seed Pack``|interface/large/store_buttons.rttex|You'll get 5 randomly chosen rare seeds. Expect some wondrous crops with these!|1|7|500|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|clothes_pack|`wClothes Pack``|interface/large/store_buttons2.rttex|Why not look the part? You'll get three randomly chosen wearable items. Some may even have special powers...|0|0|50|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|rare_clothes_pack|`wRare Clothes Pack``|interface/large/store_buttons2.rttex|Enjoy the garb of kings! You'll get three randomly chosen rare wearable items. Some may even have special powers...|0|1|500|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|gangland_style_pack|`wGangland Style``|interface/large/store_buttons2.rttex|Step into the 1920's with a Fedora, a Dame's Fedora, a Pinstripe Suit and Pants, a Flapper Headband and Dress, a Cigar, a Tommygun, a Victrola that plays jazz music, and 10 Art Deco Blocks. It's the whole package!|0|6|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|racing_action_pack|`wRacing Action Pack``|interface/large/store_buttons2.rttex|Get all you need to host races in your worlds! A Race Start Flag, Race End Flag, 2 Checkpoints, 2 Big Old Sideways Arrows, a Big Old Up Arrow, a Big Old Down Arrow, and a complete racing outfit. You'll win the races too, with new Air Robinsons that make you run faster!|0|7|3500|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|composers_pack|`wComposer's Pack``|interface/large/store_buttons3.rttex|With these handy blocks, you'll be able to compose your own music, using your World-Locked world as a Sheet of Music. Requires a World Lock (sold separately!).|0|0|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|fantasy_pack|`wFantasy Pack``|interface/large/store_buttons3.rttex|Hear ye, hear ye! It's a pack of magical wonders! You'll get a mystical Wizard Hat Seed, a Wizard's Robe, a Golden Sword, an Elvish Longbow, 10 Barrels, 3 Tavern Signs, 3 Treasure Chests, and 3 Dragon Gates!|0|6|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|education_pack|`wEducation Pack``|interface/large/store_buttons4.rttex|If you want to build a school in Growtopia, here's what you need! You'll get 10 Chalkboards, 3 School Desks, 20 Red Bricks, a Bulletin Board, 10 Pencils, a Growtopia Lunchbox (Rare), a Grey Hair Bun, an Apple and a random School Uniform clothing.|0|0|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|dungeon_pack|`wDungeon Pack``|interface/large/store_buttons4.rttex|Lock up your enemies in a dark dungeon! Of course they can still leave whenever they want. But they won't want to, because it looks so cool! Pack includes 20 Grimstone Blocks, 20 Blackrock Walls, 20 Iron Bars, 3 Jail Doors, 3 Skeletons, a Headsman's Axe, Worthless Rags, 5 Torches, and a rare Iron Mask which muffles your speech!|0|1|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|zombie_defense_pack|`wZombie Defense Pack``|interface/large/store_buttons4.rttex|The zombie invasion has come. Protect yourself with a Rare Sawed-Off Shotgun, Combat Vest, Zombie-Stompin' Boots, 3 Traffic Barricades, a Military Radio, and Best of all, you get an Antidote! You'll also get 3 Toxic Waste Barrels, 3 Biohazard Signs, 3 Tombstones, and worst of all, the deadly G-Virus itself! (Rare)! Infect your friends!|0|4|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|vegas_pack|`wVegas Pack``|interface/large/store_buttons4.rttex|What happens in Growtopia stays in Growtopia! Buy this pack to get 10 Neon Lights, a Card Block Seed, a Rare Pink Cadillac to drive, 4 Flipping Coins, a Dice Block, a Gambler's Visor, a Slot Machine, a Roulette Wheel, and an entire Showgirl Outfit!|0|5|20000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|farm_pack|`wFarm Pack``|interface/large/store_buttons5.rttex|Put the Grow in Growtopia with this pack, including a Cow you can milk, a Chicken that lays eggs, 10 Wheat, 10 Barn Blocks, 10 Red Wood Walls, a Barn Door, a farmer's outfit including Straw Hat, Overalls, Pitchfork, and Farmgirl Hair. Best of all? You get a Rare Dear John Tractor you can ride that will mow down trees!|0|0|15000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|mad_science_kit|`wMad Science Kit``|interface/large/store_buttons5.rttex|It's SCIENCE! Defy the natural order with a Science Station that produces chemicals, a Laboratory in which to mix them, and a Lab Coat, Combover, and Ze Goggles to do so safely! You'll also get a starter pack of assorted chemicals. Mix them up! Special bonus: a Rare Death Ray to make your science truly mad!|0|3|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|city_pack|`wCity Pack``|interface/large/store_buttons6.rttex|Life in the big city is rough! This pack includes 10 Sidewalks, 3 Street Signs, 3 Streetlamps, 10 Gothic Building tiles, 10 Tenement Building tiles, 10 Fire Escapes, 10 Hedges, a Blue Mailbox, and a Fire Hydrant. Special bonus: A `#Rare `2ATM Machine`` that dishes out gems once a day!|0|0|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|wild_west_pack|`wWild West Pack``|interface/large/store_buttons6.rttex|Yippee-kai-yay! This pack includes a Cowboy Hat and Cowboy Boots (of course), War Paint, Face Bandana, Sheriff's Vest, Layer Cake Dress, Corset, Kansas Curls, 10 Western Building, Saloon Doors, 5 Western Banners, a Buffalo, 10 Rustic Fences, a Campfire that plays cowboy music, and even a Parasol that lets you drift down slowly. Special bonus: A Rare Six Shooter to blast criminals with!|0|2|8000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|astro_pack|`wAstro Pack``|interface/large/store_buttons6.rttex|Boldly go where no Growtopian has gone before with an entire Astronaut outfit, a Rocket Thruster a Solar Panel, 6 Space Connectors, a Porthole, a Compu Panel and a Forcefield. As a special bonus, you can have this Rare Zorbnik DNA we found on a distant planet. It doesn't do anything by itself, but by trading with your friends, you can collect 10 of them, and then... well, who knows?|0|6|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|surgical_kit|`wSurgical Kit``|interface/large/store_buttons7.rttex|Get all the tools you need to become Chief of Surgery at Growtopia General Hospital! You get a `#Rare`` Heart Monitor that lets people know when you are online, a Hospital Bed that lets you perform surgery on anybody laying (or standing) on it, and 10 each of the seven different Surgical Tools you'll need to do that|0|2|8000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|prehistoric_pack|`wPrehistoric Pack``|interface/large/store_buttons8.rttex|Travel way back in time with this pack, including full Caveman and Cavewoman outfits, 10 Cliffside, 5 Rock Platform, a Cave Entrance, 3 Prehistoric Palm, and a Rare Sabertooth Growtopian (that's a mask of sorts). Unleash your inner monster!|0|0|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|shop_pack|`wShop Pack``|interface/large/store_buttons8.rttex|Run a fancy shop with these new items! You'll get 4 Display Boxes to hold items, 4 different kinds of signs to advertise your wares, an Open/Closed Sign you can switch with a punch, a Cash Register, a Mannequin you can dress up to show off clothing, and a Rare Security Camera, which reports when people enter and take items!|0|7|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|home_pack|`wHome Pack``|interface/large/store_buttons9.rttex|Welcome home to Growtopia! Decorate with a Television, 2 Window Curtains, 4 Couch, a Rare Wall Clock that actually tells time, and a Microwave to cook in. Then dress up in a Meaty Apron and Ducky Pajamas to sit down and eat Eggs Benedict, which increases the amount of XP you earn!|0|6|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|cinema_pack|`wCinema Pack``|interface/large/store_buttons10.rttex|It's movie time! This pack includes a Clapboard, a Black Beret, 3D Glasses, 6 Theater Curtains, 6 Marquee Blocks, a Director's Chair, 4 Theater Seats, 6 Movie Screens, a Movie Camera, and a rare GHX Speaker that plays the score from Growtopia: The Movie.|0|2|6000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|rockin_pack|`wRockin' Pack``|interface/large/store_buttons11.rttex|ROCK N' ROLL!!! Play live music in-game with 3 Rare musical instruments, Starchild Makeup, a Rockin' Headband, Leopard Leggings, a Shredded T-Shirt, a Drumkit, 6 Stage Supports, 6 Mega Rock Speakers, and 6 Rock N' Roll Wallpaper.|0|0|9999|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "set_back_target|main\n" },

        { "locks", "set_description_text|`2Locks And Stuff!``  Tap the item you'd like more info on, or BACK to go back.|\n"
                  "add_button|small_lock|`wSmall Lock``|interface/large/store_buttons.rttex|Protect up to `$10`` tiles. Can add friends to the lock so others can edit that area as well. `5It's a perma-item, is never lost when destroyed.``|1|3|50|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|big_lock|`wBig Lock``|interface/large/store_buttons.rttex|Protect up to `$48`` tiles. Can add friends to the lock so others can edit that area as well. `5It's a perma-item, is never lost when destroyed.``|1|1|200|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|huge_lock|`wHuge Lock``|interface/large/store_buttons.rttex|Protect up to `$200`` tiles. Can add friends to the lock so others can edit that area as well. `5It's a perma-item, is never lost when destroyed.``|0|4|500|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|world_lock|`wWorld Lock``|interface/large/store_buttons.rttex|Become the undisputed ruler of your domain with one of these babies. It works like a normal lock except it locks the `$entire world``! Won't work on worlds that other people already have locks on. You can even add additional normal locks to give access to certain areas to friends. `5It's a perma-item, is never lost when destroyed.`` `wRecycles for 200 Gems.``|0|7|2000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|upgrade_backpack|`wUpgrade Backpack`` (`w10 Slots``)|interface/large/store_buttons.rttex|Sewing an extra pocket onto your backpack will allow you to store `$10`` additional item types.  How else are you going to fit all those toilets and doors?|0|1|{}|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|grow_spray|`wGrow Spray Fertilizer``|interface/large/store_buttons.rttex|Why wait?!  Treat yourself to a `$5-pack`` of amazing Grow Spray Fertilizer by GrowTech Corp. Each bottle instantly ages a tree by `$1 hour``.|0|6|200|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|deluxe_grow_spray|`wDeluxe Grow Spray``|interface/large/store_buttons11.rttex|GrowTech's new `$Deluxe`` Grow Spray Fertilizer instantly ages a tree by `$24 hours`` per bottle! That's somewhere around 25 times as much as regular Grow Spray!|0|2|900|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|signal_jammer|`wSignal Jammer``|interface/large/store_buttons.rttex|Get off the grid! Install a `$Signal Jammer``! A single punch will cause it to whir to life, tireless hiding your world and its population from pesky snoopers - only those who know the world name will be able to enter. `5It's a perma-item, is never lost when destroyed.``|1|6|2000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|punch_jammer|`wPunch Jammer``|interface/large/store_buttons7.rttex|Tired of getting bashed around? Set up a Punch Jammer in your world, and people won't be able to punch each other! Can be turned on and off as needed. `5It's a perma-item, is never lost when destroyed.``|0|4|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|zombie_jammer|`wZombie Jammer``|interface/large/store_buttons7.rttex|Got a parkour or race that you don't want slowed down? Turn this on and nobody can be infected by zombie bites in your world. It does not prevent direct infection by the g-Virus itself though. `5It's a perma-item, is never lost when destroyed.``|0|5|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|door_mover|`wDoor Mover``|interface/large/store_buttons8.rttex|Unsatisfied with your world's layout?  This one-use device can be used to move the White Door to any new location in your world, provided there are 2 empty spaces for it to fit in. Disappears when used. `2Only usable on a world you have World Locked.``|0|6|5000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "set_back_target|main\n" },

        { "weather", "set_description_text|`2Weather Machines!``  Tap the item you'd like more info on, or BACK to go back.|\n"
                  "add_button|sunny|`wWeather Machine - Sunny``|interface/large/store_buttons5.rttex|You probably don't need this one... but if you ever have a desire to turn a sunset or desert world back to normal, grab a Sunny Weather Machine to Restore the default Growtopia Sky!|0|5|1000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|night|`wWeather Machine - Night``|interface/large/store_buttons5.rttex|You might not call it weather, but we do! This will turn the background of your world into a lovely night scene with stars and moon. `5It's a perma-item, is never lost when destroyed.``|0|6|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|arid|`wWeather Machine - Arid``|interface/large/store_buttons5.rttex|Want your world to look like a cartoon desert? This will turn the background of your world into a desert scene with all the trimmings. `5It's a perma-item, is never lost when destroyed.``|0|7|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|rainy|`wWeather Machine - Rainy City``|interface/large/store_buttons6.rttex|This will turn the background of your world into a dark, rainy city scene complete with sound effects. `5It's a perma-item, is never lost when destroyed.``|0|5|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|warp|`wWeather Machine - Warp Speed``|interface/large/store_buttons11.rttex|This Weather Machine will launch your world trough space at relativistic speeds, which will cause you to age more slowly, as well as see stars flying by rapidly in the background. `5It's a perma-item, is never lost when destroyed.``|0|3|10000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|mars|`wMars Blast``|interface/large/store_buttons6.rttex|Blast Off to Mars! This powerful rocket ship will launch you to a new world set up like the surface of Mars, with a special martian sky background, and unique terrain not found elsewhere in the Solar System. Mars even has lower gravity than Growtopia normally does! Remember: when using this, you are creating a NEW world by typing in a new name. You can't convert an existing world to Mars, that would be dangerous.|0|7|15000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|thermo|`wThermonuclear Blast``|interface/large/store_buttons8.rttex|This supervillainous device will blast you to a new world that has been scoured completely empty - it contains nothing but Bedrock and a White Door. Remember: when using this, you are creating a NEW World by typing in a new name. It would be irresponsible to let you blow up an entire existing world.|0|5|15000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "set_back_target|main\n" },

        { "bigitems", "set_description_text|`2Awesome Items!``  Tap the item you'd like more info on, or BACK to go back.|\n"
                  "add_button|nyan_hat|`wTurtle Hat``|interface/large/store_buttons3.rttex|It's the greatest hat ever. It bloops out bubbles as you run! `4Not available any other way!``|0|2|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|tiny_horsie|`wTiny Horsie``|interface/large/store_buttons3.rttex|Tired of wearing shoes? Wear a Tiny Horsie instead! Or possibly a large dachshund, we're not sure. Regardless, it lets you run around faster than normal, plus you're on a horse! `4Not available any other way!``|0|5|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|star_ship|`wPleiadian Star Ship``|interface/large/store_buttons4.rttex|Float on, my brother. It's all groovy. This star ship can't fly, but you can still zoom around in it, leaving a trail of energy rings and moving at enhanced speed. Sponsored by Pleiadian. `4Not available any other way!``|0|3|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|red_corvette|`wLittle Red Corvette``|interface/large/store_buttons6.rttex|Cruise around the neighborhood in style with this sweet convertible. It moves at enhanced speed and leaves other Growtopians in your dust. `4Not available any other way!``|0|1|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|motorcycle|`wGrowley Motorcycle``|interface/large/store_buttons11.rttex|The coolest motorcycles available are Growley Dennisons. Get a sporty blue one today! It even moves faster than walking, which is pretty good for a motorcycle. `4Not available any other way!``|0|6|50000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|dragon_hand|`wDragon Hand``|interface/large/store_buttons5.rttex|Call forth the dragons of legend!  With the Dragon Hand, you will command your own pet dragon. Instead of punching blocks or players, you can order your dragon to incinerate them! In addition to just being awesome, this also does increased damage, and pushes other players farther. `4Not available any other way!``|0|1|50000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|stick_horse|`wStick Horse``|interface/large/store_buttons6.rttex|Nobody looks cooler than a person bouncing along on a stick with a fake horse head attached. NOBODY. `4Not available any other way!``|0|3|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|ambulance|`wAmbulance``|interface/large/store_buttons7.rttex|Rush to the scene of an accident while lawyers chase you in this speedy rescue vehicle. `4Not available any other way!``|0|3|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|riding_raptor|`wRiding Raptor``|interface/large/store_buttons7.rttex|Long thought to be extinct, it turns out that these dinosaurs are actually alive and easily tamed. And riding one lets you run around faster than normal! `4Not available any other way!``|0|7|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|mid_owl|`wMid-Pacific Owl``|interface/large/store_buttons10.rttex|This owl is a bit lazy - if you stop moving around, he'll land on your head and fall asleep. Dedicated to the students of the Mid-Pacific Institute. `4Not available any other way!``|0|1|30000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|unicorn|`wUnicorn Garland``|interface/large/store_buttons10.rttex|Prance about in the fields with your very own pet unicorn! It shoots `1R`2A`3I`4N`5B`6O`7W`8S``. `4Not available any other way!``|0|4|50000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|starboard|`wStarBoard``|interface/large/store_buttons11.rttex|Hoverboards are here at last! Zoom around Growtopia on this brand new model, which is powered by fusion energy (that means stars spit out of the bottom). Moves faster than walking. Sponsored by Miwsky, Chudy, and Dawid. `4Not available any other way!``|0|1|30000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|geiger|`wGeiger Counter``|interface/large/store_buttons12.rttex|With this fantabulous device, you can detect radiation around you. It bleeps red, then yellow, then green as you get closer to the source. Who knows what you might find? `4Not available any other way!``|0|1|25000|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "set_back_target|main\n" },

        { "iap", "set_description_text|Tap the gem package you'd like to buy or BACK to go back. Accepted payment methods: PayPal, and Bitcoin.|\n"
                  "add_button|bag_o_gems|`wbag_o_gems``|interface/large/store_buttons.rttex|Real-money purchases aren't available on this server.|1|0|0|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|chest_o_gems|`wchest_o_gems``|interface/large/store_buttons.rttex|Real-money purchases aren't available on this server.|0|5|0|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|gem_fountain|`wgem_fountain``|interface/large/store_buttons2.rttex|Real-money purchases aren't available on this server.|0|2|0|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "add_button|its_rainin_gems|`wits_rainin_gems``|interface/large/store_buttons.rttex|Real-money purchases aren't available on this server.|1|5|0|0|||-1|-1||-1|-1||1||||||0|0|\n"
                  "set_back_target|main\n" },
    };
    return kCategories;
}

inline std::string GetTokenMenuContent(int growtokenCount) {
    return fmt::format(
        "set_description_text|`w`2Spend your Growtokens!`` (You have `5{}``) You earn Growtokens each day based on how many people visit your worlds. Tap the item you'd like more info on, or BACK to go back.|\n"
        "add_button|exp_pot|`wExperience Potion``|interface/large/store_buttons9.rttex|This `#Untradeable`` delicious fizzy drink will make you smarter! 5,000 XP smarter instantly, to be exact.|0|2|-10|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|derpy_block|`wDerpy Star Block``|interface/large/store_buttons10.rttex|DER IM A SUPERSTAR. This is a fairly ordinary block, except for the derpy star on it. Note: It is not permanent, and it doesn't drop seeds. So use it wisely!|0|3|-30|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|nothingness|`wWeather Machine - Nothingness``|interface/large/store_buttons9.rttex|Tired of all that fancy weather? This machine will turn your world completely black. Yup, that's it. Not a single pixel in the background except pure blackness.|0|3|-50|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|spike_juice|`wSpike Juice``|interface/large/store_buttons10.rttex|It's fresh squeezed, with little bits of spikes still in it! Drinking this `#Untradeable`` one-use potion will make you immune to Death Spikes and lava for 5 seconds.|0|5|-60|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|doodad|`wDoodad``|interface/large/store_buttons9.rttex|I have no idea what this thing does. It's something electronic? Maybe?|0|5|-75|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|crystal_cape|`wCrystal Cape``|interface/large/store_buttons11.rttex|This cape is woven of pure crystal, which makes it pretty uncomfortable. But it also makes it magical! It lets you double-jump off of an imaginary Crystal Block in mid-air. Sponsored by Edvoid20, HemeTems, and Aboge.|0|5|-90|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|focused_eyes|`wFocused Eyes``|interface/large/store_buttons9.rttex|This `#Untradeable`` item lets you shoot elecricity from your eyes! Wear them with pride, and creepiness.|0|4|-100|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|muddy_pants|`wMuddy Pants``|interface/large/store_buttons12.rttex|Well, this is just a pair of muddy pants. But it does come with a super secret bonus surprise that is sure to blow your mind!|0|7|-125|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|cuddly_piranha|`wCuddly Piranha``|interface/large/store_buttons10.rttex|This friendly pet piranha won't stay in its bowl! It just wants to snuggle with your face!|0|0|-150|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|puddy_leash|`wPuddy Leash``|interface/large/store_buttons11.rttex|Puddy is a friendly little kitten who will follow you around forever. Sponsored by Fredi, Fynx, and Ludo.|0|7|-180|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|golden_pickaxe|`wGolden Pickaxe``|interface/large/store_buttons9.rttex|Get your own sparkly pickaxe! This `#Untradeable`` item is a status symbol! Oh sure, it isn't any more effective than a normal pickaxe, but it sparkles!|0|1|-200|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "add_button|puppy_leash|`wPuppy Leash``|interface/large/store_buttons11.rttex|Get your own pet puppy! This little dog will follow you around forever, never wavering in her loyalty, thus making her `#Untradeable``.|0|4|-200|0|||-1|-1||-1|-1||1||||||0|0|\n"
        "set_back_target|main\n", growtokenCount);
}

struct StoreItem {
    int32_t price;
    std::vector<std::pair<std::string, uint8_t>> grants;
};

inline const std::unordered_map<std::string, StoreItem>& GetPurchasableItems() {
    static const std::unordered_map<std::string, StoreItem> kItems = {

        { "door_pack", { 15, { { "Door", 1 }, { "Sign", 1 } } } },

        { "small_lock",         { 50,    { { "Small Lock", 1 } } } },
        { "big_lock",           { 200,   { { "Big Lock", 1 } } } },
        { "huge_lock",          { 500,   { { "Huge Lock", 1 } } } },
        { "world_lock",         { 2000,  { { "World Lock", 1 } } } },
        { "grow_spray",         { 200,   { { "Grow Spray Fertilizer", 5 } } } },
        { "deluxe_grow_spray",  { 900,   { { "Deluxe Grow Spray", 1 } } } },
        { "signal_jammer",      { 2000,  { { "Signal Jammer", 1 } } } },
        { "punch_jammer",       { 10000, { { "Punch Jammer", 1 } } } },
        { "zombie_jammer",      { 10000, { { "Zombie Jammer", 1 } } } },
        { "door_mover",         { 5000,  { { "Door Mover", 1 } } } },

        { "sunny",  { 1000,  { { "Weather Machine - Sunny", 1 } } } },
        { "night",  { 10000, { { "Weather Machine - Night", 1 } } } },
        { "arid",   { 10000, { { "Weather Machine - Arid", 1 } } } },
        { "rainy",  { 10000, { { "Weather Machine - Rainy City", 1 } } } },
        { "warp",   { 10000, { { "Weather Machine - Warp Speed", 1 } } } },
        { "mars",   { 15000, { { "Mars Blast", 1 } } } },
        { "thermo", { 15000, { { "Thermonuclear Blast", 1 } } } },

        { "nyan_hat",      { 25000, { { "Turtle Hat", 1 } } } },
        { "tiny_horsie",   { 25000, { { "Tiny Horsie", 1 } } } },
        { "star_ship",     { 25000, { { "Pleiadian Star Ship", 1 } } } },
        { "red_corvette",  { 25000, { { "Little Red Corvette", 1 } } } },
        { "motorcycle",    { 50000, { { "Growley Motorcycle", 1 } } } },
        { "dragon_hand",   { 50000, { { "Dragon Hand", 1 } } } },
        { "stick_horse",   { 25000, { { "Stick Horse", 1 } } } },
        { "ambulance",     { 25000, { { "Ambulance", 1 } } } },
        { "riding_raptor", { 25000, { { "Riding Raptor", 1 } } } },
        { "mid_owl",       { 30000, { { "Mid-Pacific Owl", 1 } } } },
        { "unicorn",       { 50000, { { "Unicorn Garland", 1 } } } },
        { "starboard",     { 30000, { { "StarBoard", 1 } } } },
        { "geiger",        { 25000, { { "Geiger Counter", 1 } } } },

        { "exp_pot",         { -10,  { { "Experience Potion", 1 } } } },
        { "derpy_block",     { -30,  { { "Derpy Star Block", 1 } } } },
        { "nothingness",     { -50,  { { "Weather Machine - Nothingness", 1 } } } },
        { "spike_juice",     { -60,  { { "Spike Juice", 1 } } } },
        { "doodad",          { -75,  { { "Doodad", 1 } } } },
        { "crystal_cape",    { -90,  { { "Crystal Cape", 1 } } } },
        { "focused_eyes",    { -100, { { "Focused Eyes", 1 } } } },
        { "muddy_pants",     { -125, { { "Muddy Pants", 1 } } } },
        { "cuddly_piranha",  { -150, { { "Cuddly Piranha", 1 } } } },
        { "puddy_leash",     { -180, { { "Puddy Leash", 1 } } } },
        { "golden_pickaxe",  { -200, { { "Golden Pickaxe", 1 } } } },
        { "puppy_leash",     { -200, { { "Puppy Leash", 1 } } } },

        { "gangland_style_pack", { 5000, {
            { "Fedora", 1 }, { "Dame's Fedora", 1 }, { "Pinstripe Suit", 1 }, { "Pinstripe Pants", 1 },
            { "Flapper Headband", 1 }, { "Flapper Dress", 1 }, { "Cigar", 1 }, { "Tommygun", 1 },
            { "Victrola", 1 }, { "Art Deco Block", 10 } } } },
        { "racing_action_pack", { 3500, {
            { "Race Start Flag", 1 }, { "Race End Flag", 1 }, { "Checkpoint", 2 },
            { "Big Old Sideways Arrow", 2 }, { "Big Old Up Arrow", 1 }, { "Big Old Down Arrow", 1 },
            { "Air Robinsons", 1 } } } },
        { "composers_pack", { 5000, {
            { "Sheet Music: Blank", 12 }, { "Sheet Music: Bass Note", 12 }, { "Sheet Music: Sharp Bass", 12 },
            { "Sheet Music: Flat Bass", 12 }, { "Sheet Music: Piano Note", 12 }, { "Sheet Music: Sharp Piano", 12 },
            { "Sheet Music: Flat Piano", 12 }, { "Sheet Music: Drums", 12 } } } },
        { "fantasy_pack", { 5000, {
            { "Wizard Hat Seed", 1 }, { "Wizard's Robe", 1 }, { "Golden Sword", 1 }, { "Elvish Longbow", 1 },
            { "Barrel", 10 }, { "Tavern Sign", 3 }, { "Treasure Chest", 3 }, { "Dragon Gate", 3 } } } },
        { "dungeon_pack", { 10000, {
            { "Grimstone", 20 }, { "Blackrock Wall", 20 }, { "Iron Bars", 20 }, { "Jail Door", 3 },
            { "Skeleton", 3 }, { "Headsman's Axe", 1 }, { "Worthless Rags", 1 }, { "Torch", 5 }, { "Iron Mask", 1 } } } },
        { "zombie_defense_pack", { 10000, {
            { "Sawed-Off Shotgun", 1 }, { "Combat Vest", 1 }, { "Zombie-Stompin' Boots", 1 },
            { "Traffic Barricade", 3 }, { "Military Radio", 1 }, { "Antidote", 1 }, { "Toxic Waste Barrel", 3 },
            { "Biohazard Sign", 3 }, { "Tombstone", 3 }, { "g-Virus", 1 } } } },
        { "vegas_pack", { 20000, {
            { "Neon Lights", 10 }, { "Pink Cadillac", 1 }, { "Flipping Coin", 4 }, { "Dice Block", 1 },
            { "Gambler's Visor", 1 }, { "Slot Machine", 1 }, { "Roulette Wheel", 1 },
            { "Showgirl Headdress", 1 }, { "Showgirl Top", 1 }, { "Showgirl Leggings", 1 } } } },
        { "farm_pack", { 15000, {
            { "Cow", 1 }, { "Chicken", 1 }, { "Wheat", 10 }, { "Barn Block", 10 }, { "Red Wood Wall", 10 },
            { "Barn Door", 1 }, { "Straw Hat", 1 }, { "Overalls", 1 }, { "Pitchfork", 1 }, { "Farmgirl Hair", 1 },
            { "Dear John Tractor", 1 } } } },
        { "mad_science_kit", { 5000, {
            { "Science Station", 1 }, { "Laboratory", 1 }, { "Lab Coat", 1 }, { "Combover", 1 },
            { "Ze Goggles", 1 }, { "Death Ray", 1 } } } },
        { "city_pack", { 5000, {
            { "Sidewalk", 10 }, { "Street Sign", 3 }, { "Streetlamp", 3 }, { "Gothic Building", 10 },
            { "Tenement Building", 10 }, { "Fire Escape", 10 }, { "Hedge", 10 }, { "Blue Mailbox", 1 },
            { "Fire Hydrant", 1 }, { "ATM Machine", 1 } } } },
        { "wild_west_pack", { 8000, {
            { "Cowboy Hat", 1 }, { "Cowboy Boots", 1 }, { "War Paint", 1 }, { "Face Bandana", 1 },
            { "Sheriff's Vest", 1 }, { "Layer Cake Dress", 1 }, { "Corset", 1 }, { "Kansas Curls", 1 },
            { "Western Building", 10 }, { "Saloon Doors", 1 }, { "Western Banner", 5 }, { "Buffalo", 1 },
            { "Rustic Fence", 10 }, { "Campfire", 1 }, { "Parasol", 1 }, { "Six Shooter", 1 } } } },
        { "astro_pack", { 5000, {
            { "Astronaut Helmet", 1 }, { "Astronaut Pack", 1 }, { "Rocket Thruster", 1 }, { "Solar Panel", 1 },
            { "Space Connector", 6 }, { "Porthole", 1 }, { "Compu Panel", 1 }, { "Forcefield", 1 },
            { "Zorbnik DNA", 1 } } } },
        { "surgical_kit", { 8000, {
            { "Surgical Sponge", 10 }, { "Surgical Scalpel", 10 }, { "Surgical Anesthetic", 10 },
            { "Surgical Antiseptic", 10 }, { "Surgical Antibiotics", 10 }, { "Surgical Splint", 10 },
            { "Surgical Stitches", 10 }, { "Surgical Gloves", 10 }, { "Heart Monitor", 1 }, { "Hospital Bed", 1 } } } },
        { "prehistoric_pack", { 5000, {
            { "Caveman Hair", 1 }, { "Caveman Club", 1 }, { "Cavewoman Hair", 1 }, { "Cliffside", 10 },
            { "Rock Platform", 5 }, { "Cave Entrance", 1 }, { "Prehistoric Palm", 3 }, { "Sabertooth Growtopian", 1 } } } },
        { "shop_pack", { 10000, {
            { "Display Box", 4 }, { "Open Sign", 1 }, { "For Sale Sign", 1 }, { "Gem Sign", 1 }, { "Street Sign", 1 },
            { "Cash Register", 1 }, { "Mannequin", 1 }, { "Security Camera", 1 } } } },
        { "home_pack", { 5000, {
            { "Television", 1 }, { "Window Curtains", 2 }, { "Couch", 4 }, { "Wall Clock", 1 }, { "Microwave", 1 },
            { "Meaty Apron", 1 }, { "Ducky Pajama Top", 1 }, { "Ducky Pajama Pants", 1 }, { "Eggs Benedict", 1 } } } },
        { "cinema_pack", { 6000, {
            { "Clapboard", 1 }, { "Black Beret", 1 }, { "3D Glasses", 1 }, { "Theater Curtain", 6 },
            { "Marquee Block", 6 }, { "Director's Chair", 1 }, { "Theater Seat", 4 }, { "Movie Screen", 6 },
            { "Movie Camera", 1 }, { "GHX Speaker", 1 } } } },
        { "rockin_pack", { 9999, {
            { "Starchild Makeup", 1 }, { "Rockin' Headband", 1 }, { "Leopard Leggings", 1 },
            { "Shredded T-Shirt", 1 }, { "Drumkit", 1 }, { "Stage Support", 6 }, { "Mega Rock Speaker", 6 },
            { "Rock N' Roll Wallpaper", 6 } } } },
    };
    return kItems;
}

struct RandomPack {
    int32_t price;
    std::vector<std::pair<std::string, uint8_t>> fixedGrants;
    std::vector<std::string> curatedPool;
    bool seedPool = false;
    bool clothingPool = false;
    uint16_t minRarity = 0, maxRarity = 0;
    uint8_t pickCount = 0;
    uint8_t perPickCount = 1;

    bool hasBonusPick = false;
    uint16_t bonusMinRarity = 0, bonusMaxRarity = 0;
};

inline const std::unordered_map<std::string, RandomPack>& GetRandomPacks() {
    static const std::unordered_map<std::string, RandomPack> kPacks = {

        { "basic_splicing_kit", { 200,
            { { "Rock Seed", 10 } },
            { "Cave Column Seed", "Door Seed", "Glass Pane Seed", "Grass Seed", "Lava Rock Seed",
              "Martian Tree Seed", "Sign Seed", "Wood Block Seed" },
            false, false, 0, 0, 1, 10, false, 0, 0 } },

        { "small_seed_pack", { 50, {}, {}, true, false, 2, 12, 4, 1, true, 13, 21 } },

        { "rare_seed_pack", { 500, {}, {}, true, false, 13, 60, 5, 1, false, 0, 0 } },

        { "clothes_pack", { 50, {}, {}, false, true, 0, 10, 3, 1, false, 0, 0 } },

        { "rare_clothes_pack", { 500, {}, {}, false, true, 11, 60, 3, 1, false, 0, 0 } },
    };
    return kPacks;
}

inline bool IsUnimplementedPack(const std::string& key) {
    return key == "education_pack";
}

inline bool IsGemPackage(const std::string& key) {
    return key == "bag_o_gems" || key == "chest_o_gems" || key == "gem_fountain" || key == "its_rainin_gems";
}

inline std::optional<uint32_t> GetBackpackUpgradeCost(uint32_t currentSlots) {
    static constexpr std::array<uint32_t, 38> kPrices = {
        100, 200, 500, 1000, 1700, 2600, 3700, 5000, 6500, 8200,
        10100, 12200, 14500, 17000, 19700, 22600, 25700, 29000, 32500, 36200,
        40100, 44200, 48500, 53000, 57700, 62600, 67700, 73000, 78500, 84200,
        90100, 96200, 102500, 109000, 115700, 122600, 129700, 137000
    };
    if (currentSlots < 16)
        currentSlots = 16;
    size_t index = (currentSlots - 16) / 10;
    if (index >= kPrices.size())
        return std::nullopt;
    return kPrices[index];
}

inline std::string GetLocksMenuContent(uint32_t currentBackpackSpace) {
    auto cost = GetBackpackUpgradeCost(currentBackpackSpace);
    if (cost.has_value())
        return fmt::format(fmt::runtime(GetCategoryContent().at("locks")), *cost);

    std::string content = fmt::format(fmt::runtime(GetCategoryContent().at("locks")), 0);
    static constexpr std::string_view kButtonPrefix = "add_button|upgrade_backpack|";
    size_t start = content.find(kButtonPrefix);
    if (start != std::string::npos) {
        size_t end = content.find('\n', start);
        content.erase(start, end == std::string::npos ? std::string::npos : end - start + 1);
    }
    return content;
}

}
