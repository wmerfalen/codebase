/**
* @file structs.h
* Core structures used within the core mud code.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include "protocol.h" /* Kavir Plugin*/
#include "lists.h"

/** Intended use of this macro is to allow external packages to work with a
 * variety of versions without modifications.  For instance, an IS_CORPSE()
 * macro was introduced in pl13.  Any future code add-ons could take into
 * account the version and supply their own definition for the macro if used
 * on an older version. You are supposed to compare this with the macro
 * TBAMUD_VERSION() in utils.h.
 * It is read as Major/Minor/Patchlevel - MMmmPP */
#define _TBAMUD    0x030620

/** If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1 because
 * TRUE/FALSE aren't defined yet. */
#define USE_AUTOEQ	1

/* preamble */
/** As of bpl20, it should be safe to use unsigned data types for the various
 * virtual and real number data types.  There really isn't a reason to use
 * signed anymore so use the unsigned types and get 65,535 objects instead of
 * 32,768. NOTE: This will likely be unconditionally unsigned later.
 * 0 = use signed indexes; 1 = use unsigned indexes */
#define CIRCLE_UNSIGNED_INDEX	1

#if CIRCLE_UNSIGNED_INDEX
# define IDXTYPE	ush_int          /**< Index types are unsigned short ints */
# define IDXTYPE_MAX USHRT_MAX     /**< Used for compatibility checks. */
# define IDXTYPE_MIN 0             /**< Used for compatibility checks. */
# define NOWHERE	((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
# define NOTHING	((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
# define NOBODY		((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
# define NOFLAG   ((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
#else
# define IDXTYPE	sh_int           /**< Index types are unsigned short ints */
# define IDXTYPE_MAX SHRT_MAX      /**< Used for compatibility checks. */
# define IDXTYPE_MIN SHRT_MIN      /**< Used for compatibility checks. */
# define NOWHERE	((IDXTYPE)-1)    /**< nil reference for rooms */
# define NOTHING	((IDXTYPE)-1)    /**< nil reference for objects */
# define NOBODY		((IDXTYPE)-1)	   /**< nil reference for mobiles  */
# define NOFLAG   ((IDXTYPE)-1)    /**< nil reference for flags   */
#endif

/** Function macro for the mob, obj and room special functions */
#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)

/* room-related defines */
/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0    /**< The direction north */
#define EAST           1    /**< The direction east */
#define SOUTH          2    /**< The direction south */
#define WEST           3    /**< The direction west */
#define UP             4    /**< The direction up */
#define DOWN           5    /**< The direction down */
#define NORTHWEST      6 /**< The direction north-west */
#define NORTHEAST      7 /**< The direction north-east */
#define SOUTHEAST      8 /**< The direction south-east */
#define SOUTHWEST      9 /**< The direction south-west */
/** Total number of directions available to move in. BEFORE CHANGING THIS, make
* sure you change every other direction and movement based item that this will
* impact. */

#define NUM_OF_DIRS 10

/* Vnums of progression gear */
#define PROGRESSION_GEAR_X2              30180
#define PROGRESSION_GEAR_X3              2205
#define PROGRESSION_GEAR_X4              2207
#define PROGRESSION_GEAR_PREDATOR        1204
#define PROGRESSION_GEAR_HIGHLANDER      1201
#define PROGRESSION_GEAR_X5              1265
#define PROGRESSION_GEAR_X6              1268
#define PROGRESSION_GEAR_X7              1267
#define PROGRESSION_GEAR_X8              1266
#define PROGRESSION_GEAR_X9              1263
#define PROGRESSION_GEAR_X10             1257
#define PROGRESSION_GEAR_X11             1261
#define PROGRESSION_GEAR_X12             1260
#define PROGRESSION_GEAR_X13             1259
#define PROGRESSION_GEAR_X14             1258
#define PROGRESSION_GEAR_X15             1262
#define PROGRESSION_GEAR_X16             1256 
#define PROGRESSION_GEAR_X17             1255
#define PROGRESSION_GEAR_X18             1254 
#define PROGRESSION_GEAR_X19             1253 
#define PROGRESSION_GEAR_X20             1252 
#define PROGRESSION_GEAR_X21             1251 
#define PROGRESSION_GEAR_X22             1250 

/* Vnums of Crafting Materials */
#define CRAFT_PLASTIC_1             3500
#define CRAFT_PLASTIC_2             3501
#define CRAFT_PLASTIC_3             3502
#define CRAFT_PLASTIC_4             3503
#define CRAFT_PLASTIC_5             3504
#define CRAFT_WOOD_1                3505
#define CRAFT_WOOD_2                3506
#define CRAFT_WOOD_3                3507
#define CRAFT_WOOD_4                3508
#define CRAFT_WOOD_5                3509
#define CRAFT_CLOTH_1               3510
#define CRAFT_CLOTH_2               3511
#define CRAFT_CLOTH_3               3512
#define CRAFT_CLOTH_4               3513
#define CRAFT_CLOTH_5               3514
#define CRAFT_LEATHER_1             3515
#define CRAFT_LEATHER_2             3516
#define CRAFT_LEATHER_3             3517
#define CRAFT_LEATHER_4             3518
#define CRAFT_LEATHER_5             3519
#define CRAFT_ELECTRONICS_1         3520
#define CRAFT_ELECTRONICS_2         3521
#define CRAFT_ELECTRONICS_3         3522
#define CRAFT_ELECTRONICS_4         3523
#define CRAFT_ELECTRONICS_5         3524
#define CRAFT_PAPER_1               3525
#define CRAFT_PAPER_2               3526
#define CRAFT_PAPER_3               3527
#define CRAFT_PAPER_4               3528
#define CRAFT_PAPER_5               3529
#define CRAFT_GLASS_1               3530
#define CRAFT_GLASS_2               3531
#define CRAFT_GLASS_3               3532
#define CRAFT_GLASS_4               3533
#define CRAFT_GLASS_5               3534
#define CRAFT_KEVLAR_1              3535
#define CRAFT_KEVLAR_2              3536
#define CRAFT_KEVLAR_3              3537
#define CRAFT_KEVLAR_4              3538
#define CRAFT_KEVLAR_5              3539
#define CRAFT_CERAMIC_1             3540
#define CRAFT_CERAMIC_2             3541
#define CRAFT_CERAMIC_3             3542
#define CRAFT_CERAMIC_4             3543
#define CRAFT_CERAMIC_5             3544
#define CRAFT_IRON_1                3600
#define CRAFT_IRON_2                3601
#define CRAFT_IRON_3                3602
#define CRAFT_IRON_4                3603
#define CRAFT_IRON_5                3604
#define CRAFT_STEEL_1               3605
#define CRAFT_STEEL_2               3606
#define CRAFT_STEEL_3               3607
#define CRAFT_STEEL_4               3608
#define CRAFT_STEEL_5               3609
#define CRAFT_BRASS_1               3610
#define CRAFT_BRASS_2               3611
#define CRAFT_BRASS_3               3612
#define CRAFT_BRASS_4               3613
#define CRAFT_BRASS_5               3614
#define CRAFT_OSMIUM_1              3615
#define CRAFT_OSMIUM_2              3616
#define CRAFT_OSMIUM_3              3617
#define CRAFT_OSMIUM_4              3618
#define CRAFT_OSMIUM_5              3619
#define CRAFT_ALUMINUM_1            3620
#define CRAFT_ALUMINUM_2            3621
#define CRAFT_ALUMINUM_3            3622
#define CRAFT_ALUMINUM_4            3623
#define CRAFT_ALUMINUM_5            3624
#define CRAFT_TITANIUM_1            3625
#define CRAFT_TITANIUM_2            3626
#define CRAFT_TITANIUM_3            3627
#define CRAFT_TITANIUM_4            3628
#define CRAFT_TITANIUM_5            3629
#define CRAFT_ADAMANTIUM_1          3630
#define CRAFT_ADAMANTIUM_2          3631
#define CRAFT_ADAMANTIUM_3          3632
#define CRAFT_ADAMANTIUM_4          3633
#define CRAFT_ADAMANTIUM_5          3634
#define CRAFT_COPPER_1              3636
#define CRAFT_COPPER_2              3637
#define CRAFT_COPPER_3              3638
#define CRAFT_COPPER_4              3639
#define CRAFT_COPPER_5              3640
#define CRAFT_BRONZE_1              3641
#define CRAFT_BRONZE_2              3642
#define CRAFT_BRONZE_3              3643
#define CRAFT_BRONZE_4              3644
#define CRAFT_BRONZE_5              3645
#define CRAFT_SILVER_1              3646
#define CRAFT_SILVER_2              3647
#define CRAFT_SILVER_3              3648
#define CRAFT_SILVER_4              3649
#define CRAFT_SILVER_5              3650
#define CRAFT_GOLD_1                3651
#define CRAFT_GOLD_2                3652
#define CRAFT_GOLD_3                3653
#define CRAFT_GOLD_4                3654
#define CRAFT_GOLD_5                3655
#define CRAFT_PLATINUM_1            3656
#define CRAFT_PLATINUM_2            3657
#define CRAFT_PLATINUM_3            3658
#define CRAFT_PLATINUM_4            3659
#define CRAFT_PLATINUM_5            3660
#define CRAFT_AMBER_1               3545
#define CRAFT_AMBER_2               3546
#define CRAFT_AMBER_3               3547
#define CRAFT_AMBER_4               3548
#define CRAFT_AMBER_5               3549
#define CRAFT_EMERALD_1             3550
#define CRAFT_EMERALD_2             3551
#define CRAFT_EMERALD_3             3552
#define CRAFT_EMERALD_4             3553
#define CRAFT_EMERALD_5             3554
#define CRAFT_SAPPHIRE_1            3555
#define CRAFT_SAPPHIRE_2            3556
#define CRAFT_SAPPHIRE_3            3557
#define CRAFT_SAPPHIRE_4            3558
#define CRAFT_SAPPHIRE_5            3559
#define CRAFT_RUBY_1                3560
#define CRAFT_RUBY_2                3561
#define CRAFT_RUBY_3                3562
#define CRAFT_RUBY_4                3563
#define CRAFT_RUBY_5                3564
#define CRAFT_PEARL_1               3565
#define CRAFT_PEARL_2               3566
#define CRAFT_PEARL_3               3567
#define CRAFT_PEARL_4               3568
#define CRAFT_PEARL_5               3569
#define CRAFT_DIAMOND_1             3570
#define CRAFT_DIAMOND_2             3571
#define CRAFT_DIAMOND_3             3572
#define CRAFT_DIAMOND_4             3573
#define CRAFT_DIAMOND_5             3574
#define CRAFT_COAL_1                3575
#define CRAFT_COAL_2                3576
#define CRAFT_COAL_3                3577
#define CRAFT_COAL_4                3578
#define CRAFT_COAL_5                3579
#define CRAFT_LEAD_1                3580
#define CRAFT_LEAD_2                3581
#define CRAFT_LEAD_3                3582
#define CRAFT_LEAD_4                3583
#define CRAFT_LEAD_5                3584
#define CRAFT_CONCRETE_1            3585
#define CRAFT_CONCRETE_2            3586
#define CRAFT_CONCRETE_3            3587
#define CRAFT_CONCRETE_4            3588
#define CRAFT_CONCRETE_5            3589
#define CRAFT_GRANITE_1             3590
#define CRAFT_GRANITE_2             3591
#define CRAFT_GRANITE_3             3592
#define CRAFT_GRANITE_4             3593
#define CRAFT_GRANITE_5             3594
#define CRAFT_OBSIDIAN_1            3595
#define CRAFT_OBSIDIAN_2            3596
#define CRAFT_OBSIDIAN_3            3597
#define CRAFT_OBSIDIAN_4            3598
#define CRAFT_OBSIDIAN_5            3599
#define CRAFT_QUARTZ_1              3700
#define CRAFT_QUARTZ_2              3701
#define CRAFT_QUARTZ_3              3702
#define CRAFT_QUARTZ_4              3703
#define CRAFT_QUARTZ_5              3704
#define CRAFT_JADE_1                3705
#define CRAFT_JADE_2                3706
#define CRAFT_JADE_3                3707
#define CRAFT_JADE_4                3708
#define CRAFT_JADE_5                3709
#define CRAFT_BONE_1                3710
#define CRAFT_BONE_2                3711
#define CRAFT_BONE_3                3712
#define CRAFT_BONE_4                3713
#define CRAFT_BONE_5                3714
#define CRAFT_SHELL_1               3715
#define CRAFT_SHELL_2               3716
#define CRAFT_SHELL_3               3717
#define CRAFT_SHELL_4               3718
#define CRAFT_SHELL_5               3719
#define CRAFT_TEETH_1               3720
#define CRAFT_TEETH_2               3721
#define CRAFT_TEETH_3               3722
#define CRAFT_TEETH_4               3723
#define CRAFT_TEETH_5               3724
#define CRAFT_FLESH_1               3725
#define CRAFT_FLESH_2               3726
#define CRAFT_FLESH_3               3727
#define CRAFT_FLESH_4               3728
#define CRAFT_FLESH_5               3729
#define CRAFT_DNA_1                 3730
#define CRAFT_DNA_2                 3731
#define CRAFT_DNA_3                 3732
#define CRAFT_DNA_4                 3733
#define CRAFT_DNA_5                 3734
#define CRAFT_PLANT_1               3735
#define CRAFT_PLANT_2               3736
#define CRAFT_PLANT_3               3737
#define CRAFT_PLANT_4               3738
#define CRAFT_PLANT_5               3739
#define CRAFT_SCALE_1               3740
#define CRAFT_SCALE_2               3741
#define CRAFT_SCALE_3               3742
#define CRAFT_SCALE_4               3743
#define CRAFT_SCALE_5               3744
#define CRAFT_ENERGY_1              3661
#define CRAFT_ENERGY_2              3662
#define CRAFT_ENERGY_3              3663
#define CRAFT_ENERGY_4              3664
#define CRAFT_ENERGY_5              3665
#define CRAFT_FUEL_1                3666
#define CRAFT_FUEL_2                3667
#define CRAFT_FUEL_3                3668
#define CRAFT_FUEL_4                3669
#define CRAFT_FUEL_5                3670
#define CRAFT_ACID_1                3671
#define CRAFT_ACID_2                3672
#define CRAFT_ACID_3                3673
#define CRAFT_ACID_4                3674
#define CRAFT_ACID_5                3675
#define CRAFT_DRUG_1                3676
#define CRAFT_DRUG_2                3677
#define CRAFT_DRUG_3                3678
#define CRAFT_DRUG_4                3679
#define CRAFT_DRUG_5                3680
#define CRAFT_RUBBER_1              3681
#define CRAFT_RUBBER_2              3682
#define CRAFT_RUBBER_3              3683
#define CRAFT_RUBBER_4              3684
#define CRAFT_RUBBER_5              3685
#define CRAFT_POWDER_1              3686
#define CRAFT_POWDER_2              3687
#define CRAFT_POWDER_3              3688
#define CRAFT_POWDER_4              3689
#define CRAFT_POWDER_5              3690
#define CRAFT_PLUTONIUM_1           3745
#define CRAFT_PLUTONIUM_2           3746
#define CRAFT_PLUTONIUM_3           3747
#define CRAFT_PLUTONIUM_4           3748
#define CRAFT_PLUTONIUM_5           3749
#define CRAFT_CLAY_1                3691
#define CRAFT_CLAY_2                3692
#define CRAFT_CLAY_3                3693
#define CRAFT_CLAY_4                3694
#define CRAFT_CLAY_5                3695
#define CRAFT_NOTHING              50049


/* Bionic Vnums */ 
#define FOOTJET_VNUM                114


/* Room flags: used in room_data.room_flags
 WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK           0   /* Dark room, light needed to see                   */
#define ROOM_DEATH          1   /* Death trap, instant death                        */
#define ROOM_NOMOB          2   /* MOBs not allowed in room                         */
#define ROOM_INDOORS        3   /* Indoors, no weather                              */
#define ROOM_PK_OK          4   /* PLAYER_KILLING allowed                           */
#define ROOM_SOUNDPROOF     5   /* Shouts, gossip blocked                           */
#define ROOM_NOTRACK        6   /* Track won't go through                           */
#define ROOM_NO_PSIONICS    7   /* Magic not allowed                                */
#define ROOM_UNDERWATER     8   /* Players cannot breathe                           */
#define ROOM_PRIVATE        9   /* Can't teleport in                                */
#define ROOM_GODROOM       10   /* LVL_GOD+ only allowed                            */
#define ROOM_BFS_MARK      11   /* (R) breath-first srch mark                       */
#define ROOM_HOUSE         12   /* (R) Room is a house                              */
#define ROOM_HOUSE_CRASH   13   /* (R) House needs saving                           */
#define ROOM_ATRIUM        14   /* (R) The door to a house                          */
#define ROOM_SUPER_REGEN   15   /* Players gain psi points back faster              */
#define ROOM_MINUS_REGEN   16   /* Players do not gain psi points as fast           */
#define ROOM_VACUUM        17   /* Players cannot breathe at all                    */
#define ROOM_RADIATION     18   /* Players can get radiation sick                   */
#define ROOM_INTENSE_HEAT  19   /* Players take damage from heat                    */
#define ROOM_INTENSE_COLD  20   /* Players take damage from cold                    */
#define ROOM_PSI_DRAINING  21   /* Players lose psi points from room                */
#define ROOM_MOVE_DRAINING 22   /* Players lose movement from room                  */
#define ROOM_HOLY_GROUND   23   /* Holy Ground - !highlander fight                  */
#define ROOM_PEACEFUL      24   /* Violence not allowed                             */
#define ROOM_TUNNEL        25   /* room for only 1 pers                             */
#define ROOM_NO_TP         26   /* Players cannot teleport in                       */
#define ROOM_NO_SUMPORT    27   /* Players may not summon or teleport in            */
#define ROOM_NO_WHERE      28   /* Where command won't work for people in this room */
#define ROOM_WORLDMAP      29   /* World-map style maps here                        */
#define ROOM_OLC           30   /* (R) Modifyable/!compress                         */
#define ROOM_LOCKER        31   /* players can use lockers here                     */
#define ROOM_NO_RECALL     32   /* Players cannot use recall command here           */
#define ROOM_NO_SUICIDE    33   /* Players cannot commit suicide here               */
#define ROOM_WAREHOUSE	   34   /* Rooms where players can access storage bays		*/
#define ROOM_HOSPITAL	   35   /* Rooms where players can access their corpses     */
#define ROOM_MERC_ONLY	   36   /* No-Merc rooms */
#define ROOM_CRAZY_ONLY    37
#define ROOM_STALKER_ONLY  38
#define ROOM_BORG_ONLY     39
#define ROOM_CALLER_ONLY   40
#define ROOM_HIGHLANDER_ONLY 41
#define ROOM_PREDATOR_ONLY 42
#define ROOM_CUSTOM		   43
#define ROOM_OCCUPIED      44
#define ROOM_JEWELER	   45
#define ROOM_GREENSHOP	   46
#define ROOM_BLUESHOP	   47
#define ROOM_BLACKSHOP	   48

// The total number of Room Flags
#define NUM_ROOM_FLAGS     49

// Zone info: Used in zone_data.zone_flags
#define ZONE_NO_TP          0
#define ZONE_NO_MARBLE      1
#define ZONE_NO_RECALL      2
#define ZONE_NO_SUMPORT     3
#define ZONE_REMORT_ONLY    4
#define ZONE_QUEST          5
#define ZONE_CLOSED         6   /* < Zone is closed - players cannot enter                 */
#define ZONE_NOIMMORT       7   /* < Immortals (below LVL_GRGOD) cannot enter this zone    */
#define ZONE_GRID           8   /* < Zone is 'on the grid', connected, show on 'areas'     */
#define ZONE_NOBUILD        9   /* < Building is not allowed in the zone                   */
#define ZONE_WORLDMAP      10
#define ZONE_NO_EVENT	   11

#define NUM_ZONE_FLAGS     12
/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR           (1 << 0)   // < Exit is a door       
#define EX_CLOSED           (1 << 1)   // < The door is closed   
#define EX_LOCKED           (1 << 2)   // < The door is locked   
#define EX_PICKPROOF        (1 << 3)   // < Lock can't be picked  
#define EX_HIDDEN           (1 << 4)   // < Exit is hidden
#define EX_BLOWPROOF        (1 << 5)   // < Exit can't be blown open

// Sector types: used in room_data.sector_type
#define SECT_INSIDE         0   /* < Indoors, connected to SECT macro.  */
#define SECT_CITY           1   /* < In a city                          */
#define SECT_FIELD          2   /* < In a field                         */
#define SECT_FOREST         3   /* < In a forest                        */
#define SECT_HILLS          4   /* < In the hills                       */
#define SECT_MOUNTAIN       5   /* < On a mountain                      */
#define SECT_WATER_SWIM     6   /* < Swimmable water                    */
#define SECT_WATER_NOSWIM   7   /* < Water - need a boat                */
#define SECT_UNDERWATER     8   /* < Underwater                         */
#define SECT_FLYING         9   /* < Wheee!                             */
#define SECT_PAUSE         10   /* < Each move pauses you for a round   */
#define SECT_DESERT        11   /* < The hot, endless, desert           */
#define SECT_DEATH         12   /* < Deathtrap, red X on ascii map      */
#define SECT_QUEST         13   /* < Quest room, yellow ! on the map    */
#define SECT_SHOP          14   /* < shop room, $ on ascii map   */
#define SECT_UNDERGROUND   15   /* < underground*/
#define SECT_VEHICLE_ONLY   16	/* vehicle only */

// The total number of room Sector Types
#define NUM_ROOM_SECTORS   17

/* char and mob-related defines */

/* History */
#define HIST_ALL       0 /**< Index to history of all channels */
#define HIST_SAY       1 /**< Index to history of all 'say' */
#define HIST_GOSSIP    2 /**< Index to history of all 'gossip' */
#define HIST_WIZNET    3 /**< Index to history of all 'wiznet' */
#define HIST_TELL      4 /**< Index to history of all 'tell' */
#define HIST_SHOUT     5 /**< Index to history of all 'shout' */
#define HIST_GRATS     6 /**< Index to history of all 'grats' */
#define HIST_HOLLER    7 /**< Index to history of all 'holler' */
#define HIST_AUCTION   8 /**< Index to history of all 'auction' */
#define HIST_NEWBIE	   9
#define HIST_RADIO    10
#define HIST_EVENT	  11
#define HIST_GT		  12

#define NUM_HIST      13 /**< Total number of history indexes */

#define HISTORY_SIZE  10 /**< Number of last commands kept in each history */

// PC classes
#define CLASS_UNDEFINED   (-1) /* < PC Class undefined */
#define CLASS_MERCENARY     0
#define CLASS_CRAZY         1
#define CLASS_STALKER       2
#define CLASS_BORG          3
#define CLASS_HIGHLANDER    4
#define CLASS_PREDATOR      5
#define CLASS_CALLER        6
#define CLASS_MARKSMAN		7
#define CLASS_BRAWLER		8
#define CLASS_BOUNTYHUNTER	9
#define CLASS_ENFORCER		10
#define CLASS_COVERT		11
#define CLASS_PRIEST		12
#define CLASS_SNIPER		13
#define CLASS_SABOTEUR		14
#define CLASS_ASSASSIN		15
#define CLASS_STRIKER		16
#define CLASS_TERRORIST		17
#define CLASS_SPY			18
#define CLASS_ANARCHIST		19
#define CLASS_SUMMONER		20
#define CLASS_PSIONICIST	21
#define CLASS_PSYCHOTIC		22
#define CLASS_ELEMENTALIST	23
#define CLASS_TECHNOMANCER	24
#define CLASS_FERAL			25
#define CLASS_SURVIVALIST	26
#define CLASS_HUNTER		27
#define CLASS_BEASTMASTER	28
#define CLASS_VANGUARD		29
#define CLASS_SHAMAN		30
#define CLASS_DRONE			31
#define CLASS_DESTROYER		32
#define CLASS_ASSIMILATOR	33
#define CLASS_GUARDIAN		34
#define CLASS_JUGGERNAUT	35
#define CLASS_PANZER		36
#define CLASS_KNIGHT		37
#define CLASS_BLADESINGER	38
#define CLASS_BLADEMASTER	39
#define CLASS_REAVER		40
#define CLASS_ARCANE		41
#define CLASS_BARD			42
#define CLASS_YOUNGBLOOD	43
#define CLASS_BADBLOOD		44
#define CLASS_WEAPONMASTER	45
#define CLASS_ELDER			46
#define CLASS_PREDALIEN		47
#define CLASS_MECH			48

// Total number of available PC Classes
#define NUM_CLASSES         49  

// NPC classes (currently unused - feel free to implement!) - Thanks, i will - Gahan
#define MOB_CLASS_NONE			0   /* < NPC Class Other (or undefined) */
#define MOB_CLASS_UNDEAD        1   /* < NPC Class Undead */
#define MOB_CLASS_HUMANOID      2   /* < NPC Class Humanoid */
#define MOB_CLASS_MECHANICAL	3
#define MOB_CLASS_HIGHLANDER	4
#define MOB_CLASS_ANIMAL		5
#define MOB_CLASS_CANINEFELINE	6
#define MOB_CLASS_INSECT        7
#define MOB_CLASS_REPTILE		8
#define MOB_CLASS_AQUATIC		9
#define MOB_CLASS_ALIEN			10
#define MOB_CLASS_MUTANT		11
#define MOB_CLASS_PLANT			12
#define MOB_CLASS_ETHEREAL		13
#define MOB_CLASS_SHELL			14
#define MOB_CLASS_BIRD			15

#define NUM_MOB_CLASSES			16

/* Sex */
#define SEX_NEUTRAL   0   /**< Neutral Sex (Hermaphrodite) */
#define SEX_MALE      1   /**< Male Sex (XY Chromosome) */
#define SEX_FEMALE    2   /**< Female Sex (XX Chromosome) */
/** Total number of Genders */
#define NUM_GENDERS   3

// Positions 
#define POS_DEAD            0   /* < Position = dead               */
#define POS_MORTALLYW       1   /* < Position = mortally wounded   */
#define POS_INCAP           2   /* < Position = incapacitated      */
#define POS_STUNNED         3   /* < Position = stunned            */
#define POS_SLEEPING        4   /* < Position = sleeping           */
#define POS_FOCUSING        5   /* < Position = focusing           */
#define POS_MEDITATING      6   /* < Position = meditating         */
#define POS_RESTING         7   /* < Position = resting            */
#define POS_SITTING         8   /* < Position = sitting            */
#define POS_FIGHTING        9   /* < Position = fighting           */
#define POS_STANDING       10   /* < Position = standing           */

//  Total number of positions.
#define NUM_POSITIONS      11

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER        0   /**< Player is a player-killer */
#define PLR_THIEF         1   /**< Player is a player-thief */
#define PLR_FROZEN        2   /**< Player is frozen */
#define PLR_DONTSET       3   /**< Don't EVER set (ISNPC bit, set by mud) */
#define PLR_WRITING       4   /**< Player writing (board/mail/olc) */
#define PLR_MAILING       5   /**< Player is writing mail */
#define PLR_CRASH         6   /**< Player needs to be crash-saved */
#define PLR_SITEOK        7   /**< Player has been site-cleared */
#define PLR_NOSHOUT       8   /**< Player not allowed to shout/goss */
#define PLR_NOTITLE       9   /**< Player not allowed to set title */
#define PLR_DELETED      10   /**< Player deleted - space reusable */
#define PLR_LOADROOM     11   /**< Player uses nonstandard loadroom */
#define PLR_NOWIZLIST    12   /**< Player shouldn't be on wizlist */
#define PLR_NODELETE     13   /**< Player shouldn't be deleted */
#define PLR_INVSTART     14   /**< Player should enter game wizinvis */
#define PLR_CRYO         15   /**< Player is cryo-saved (purge prog) */
#define PLR_NOTDEADYET   16   /**< (R) Player being extracted */
#define PLR_BUG          17   /**< Player is writing a bug */
#define PLR_IDEA         18   /**< Player is writing an idea */
#define PLR_TYPO         19   /**< Player is writing a typo */
#define PLR_PK			 20 
#define PLR_REP          21
#define PLR_MULTI        22

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC            0   /* < Mob has a callable spec-proc                   */
#define MOB_SENTINEL        1   /* < Mob should not move                            */
#define MOB_SCAVENGER       2   /* < Mob picks up stuff on the ground               */
#define MOB_ISNPC           3   /* < (R) Automatically set on all Mobs              */
#define MOB_AWARE           4   /* < Mob can't be backstabbed                       */
#define MOB_AGGRESSIVE      5   /* < Mob auto-attacks everybody nearby              */
#define MOB_STAY_ZONE       6   /* < Mob shouldn't wander out of zone               */
#define MOB_WIMPY           7   /* < Mob flees if severely injured                  */
#define MOB_AGGRO_15        8   /* < Auto-attack any evil PC's                      */
#define MOB_AGGRO_25        9   /* < Auto-attack any good PC's                      */
#define MOB_AGGRO_REMORT   10   /* < Auto-attack any neutral PC's                   */
#define MOB_MEMORY         11   /* < remember attackers if attacked                 */
#define MOB_HELPER         12   /* < attack PCs fighting other NPCs                 */
#define MOB_THIEF          13   /* < Mob can't be charmed                           */
#define MOB_HASTE_2        14   /* < to be replaced by numAttacks ....              */
#define MOB_HASTE_3        15   /* < to be replaced by numAttacks ....              */ 
#define MOB_HASTE_4        16   /* < to be replaced by numAttacks ....              */
#define MOB_PACIFIST       17   /* < Mob doesn't fight                              */
#define MOB_HUNTER         18   /* < Mob hunts attackers till death                 */
#define MOB_JUSTICE        19   /* < not sure                                       */
#define MOB_SANCT          20   /* < Mob has perm sanc                              */
#define MOB_SUPER_SANCT    21   /* < Mob has perm super-sanc                        */
#define MOB_MEGA_SANCT     22   /* < Mob has perm mega_sanct                        */
#define MOB_CHARM        23   /* < Mob can't be charmed                           */
#define MOB_SUMMON         24   /* < Mob can't be summoned                          */
#define MOB_NOSLEEP        25   /* < Mob can't be slept                             */
#define MOB_NOBASH         26   /* < Mob can't be bashed (e.g. trees)               */
#define MOB_NOBLIND        27   /* < Mob can't be blinded                           */
#define MOB_NOTDEADYET     28   /* < (R) Mob being extracted                        */
#define MOB_STAY_WATER	   29
#define MOB_QUESTMASTER	   30
#define MOB_NOSNIPE		   31
#define MOB_NODRAG			32
#define MOB_MERCCORE		33
#define MOB_MERCCALLING		34
#define MOB_MERCFOCUS		35
#define MOB_CRAZYCORE		36
#define MOB_CRAZYCALLING	37
#define MOB_CRAZYFOCUS		38
#define MOB_BORGCORE		39
#define MOB_BORGCALLING		40
#define MOB_BORGFOCUS		41
#define MOB_CALLERCORE		42
#define MOB_CALLERCALLING	43
#define MOB_CALLERFOCUS		44
#define MOB_PREDATORCORE	45
#define MOB_PREDATORCALLING	46
#define MOB_PREDATORFOCUS	47
#define MOB_HIGHLANDERCORE	48
#define MOB_HIGHLANDERCALLING	49
#define MOB_HIGHLANDERFOCUS	50
#define MOB_STALKERCORE		51
#define MOB_STALKERCALLING	52
#define MOB_STALKERFOCUS	53
#define MOB_MECHANIC		54
//  Total number of Mob Flags
#define NUM_MOB_FLAGS      55

// Preference flags: used by char_data.player_specials.pref
#define PRF_BRIEF           0   /* < Room descs won't normally be shown                         */
#define PRF_COMPACT         1   /* < No extra CRLF pair before prompts                          */
#define PRF_NOSHOUT         2   /* < Can't hear shouts                                          */
#define PRF_NOTELL          3   /* < Can't receive tells                                        */
#define PRF_DISPHP          4   /* < Display hit points in prompt                               */
#define PRF_DISPPSI         5   /* < Display psi points in prompt                               */
#define PRF_DISPMOVE        6   /* < Display move points in prompt                              */
#define PRF_DISPAMMO        7   /* < Display the amount of ammo left                            */
#define PRF_NOHASSLE        8   /* < Aggr mobs won't attack                                     */
#define PRF_QUEST           9   /* < On quest                                                   */
#define PRF_NOSUMMON       10   /* < Can be summoned                                            */
#define PRF_NOREPEAT       11   /* < No repetition of comm commands                             */
#define PRF_HOLYLIGHT      12   /* < Can see in dark                                            */
#define PRF_COLOR_1        13   /* < Color (low bit)                                            */
#define PRF_COLOR_2        14   /* < Color (high bit)                                           */
#define PRF_NOWIZ          15   /* < Can't hear wizline                                         */
#define PRF_LOG1           16   /* < On-line System Log (low bit)                               */
#define PRF_LOG2           17   /* < On-line System Log (high bit)                              */
#define PRF_NOAUCT         18   /* < Can't hear auction channel                                 */
#define PRF_NOGOSS         19   /* < Can't hear gossip channel                                  */
#define PRF_NOGRATZ        20   /* < Can't hear grats channel                                   */
#define PRF_SHOWVNUMS      21   /* < Can see VNUMs                                              */
#define PRF_AFK            22   /* < Player has gone AFK                                        */
#define PRF_AUTOCON        23   /* < Player will auto diagnose in fights                        */
#define PRF_AUTOEXIT       24   /* < Display exits in a room                                    */
#define PRF_AUTOEQUIP      25   /* < Player will auto equip upon unrenting                      */
#define PRF_AUTOLOOT       26   /* < Player will auto loot corpses                              */
#define PRF_AUTOSPLIT      27   /* < Player will auto split units                               */
#define PRF_AUTOASSIST     28   /* < this will be weeded out!                                   */
#define PRF_AUTOGAIN       29   /* < Auto-gain when you have enough experience                  */
#define PRF_AUTODAM        30   /* < Displays damage numbers in combat                          */
#define PRF_AUTOUNITS      31   /* < Loot units from a corpse                                   */
#define PRF_AUTOMAP        32   /* < Show map at the side of room descs                         */
#define PRF_LDFLEE         33   /* < Player will flee when link dead                            */
#define PRF_NONEWBIE       34   /* < Player cant hear newbie channel                            */
#define PRF_CLS            35   /* < Clear screen in OLC                                        */
#define PRF_BUILDWALK      36   /* < Build new rooms while walking                              */
#define PRF_RADIOSNOOP     37   /* < Radio broadcast snooping                                   */
#define PRF_SPECTATOR      38   /* < Spectator (invis, can't do any skills/psionics)            */
#define PRF_TEMP_MORTAL    39   /* < Highlander/Immortal acts as normal mortal regarding death  */
#define PRF_FROZEN         40   /* < Player is frozen (freezetag quest, not really frozen)      */
#define PRF_OLDSCORE       41   /* < Player prefers to see old score format                     */
#define PRF_NOWHO          42   /* < Player doesn't show up in who list                         */
#define PRF_SHOWDBG        43   /* < Show player the debug info (this should be a channel)      */
#define PRF_STEALTH        44   /* < Pred stealth, only visible to those with second sight      */
#define PRF_DISPAUTO       45   /* < Show prompt HP, MP, MV when < 25%                          */
#define PRF_NO_TYPO        46   /* < Will not suggest alternative commands for typos            */
#define PRF_AUTOSCAVENGER  47   /* < Automatically scavenge and sacrifice corpse                */
#define PRF_NOFIGHTSPAM    48   /* < Toggles fight spam                                         */
#define PRF_AUTOKEY        49   /* < Automatically unlock locked doors when opening             */
#define PRF_AUTODOOR       50   /* < Use the next available door                                */
#define PRF_NEWWHO         51
#define PRF_OLDWHO         52
#define PRF_OLDEQ          53
#define PRF_NOMARKET	   54
#define PRF_BLIND		   55
#define PRF_SPAM		   56
#define PRF_NOLEAVE	       57

//  Total number of available PRF flags
#define NUM_PRF_FLAGS      58

// Affect bits: used in char_data.char_specials.saved.affected_by
// WARNING: In the world files, NEVER set the bits marked "R" ("Reserved")
#define AFF_DONTUSE           0   /* < DON'T USE!                                                 */
#define AFF_BLIND             1   /* < (R) Char is blind                                          */
#define AFF_NOTRACK           2       
#define AFF_DETECT_PSIONIC    3   /* Char is sensitive to psionics                                */
#define AFF_HOLD              4       
#define AFF_STUN              5       
#define AFF_POISON            6   /* (R) Char is poisoned                                         */ 
#define AFF_GROUP             7   /* (R) Char is grouped                                          */ 
#define AFF_WEAKEN            8   /* Char is weakened                                             */
#define AFF_SLEEP             9   /* (R) Char asleep                                              */
#define AFF_DODGE            10   /* Char is charmed                                              */ 
#define AFF_SNEAK            11   /* Char can move quietly                                        */
#define AFF_HIDE             12   /* Char is hidden                                               */
#define AFF_CHARM            13       
#define AFF_FOLLOW           14
#define AFF_WIMPY            15
#define AFF_COVER_FIRE       16
#define AFF_BODY_BLOCK       17
#define AFF_DISTRACT         18
#define AFF_PARALYZE         19 
#define AFF_PETRIFY          20
#define AFF_WATERWALK        21   /* < Char can walk on water                                     */
#define AFF_FLYING           22   /* < Char is flying                                             */
#define AFF_LEVITATE         22       
#define AFF_SCUBA            23   /* < Room for future expansion                                  */
#define AFF_CURSE            24   /* < Char is cursed                                             */
#define AFF_INFRAVISION      25   /* < Char can see in dark                                       */
#define AFF_SANCT            26   /* < Char protected by sanct                                    */
#define AFF_SUPER_SANCT      27   /* < Char protected by sanct                                    */
#define AFF_MEGA_SANCT       28
#define AFF_INVISIBLE        29   /* < Char is invisible                                          */
#define AFF_DETECT_INVIS     30   /* Char can see invis chars                                     */
#define AFF_ARMORED_AURA     31
#define AFF_PSI_SHIELD       32
#define AFF_HASTE            33
#define AFF_SENSE_LIFE       34       
#define AFF_BIO_TRANCE       35
#define AFF_THERMAL_AURA     36
#define AFF_COOL_AURA        37       
#define AFF_SECOND_SIGHT     38       
#define AFF_DUPLICATES       39
#define AFF_INDESTRUCT_AURA  40
#define AFF_INSPIRE          41
#define AFF_VORPAL_SPEED     42
#define AFF_FANATICAL_FURY   43
#define AFF_LETH_IMMUNE      44
#define AFF_BLIND_IMMUNE     45
#define AFF_COURAGE          46
#define AFF_PSI_MIRROR       47   /* psi mirror affect                                            */
#define AFF_PRIMING          48   /* new affection for priming a bionic weapon					  */
#define AFF_MANIAC           49
#define AFF_BRONZELIFE		 50
#define AFF_SILVERLIFE		 51
#define AFF_GOLDLIFE		 52
#define AFF_PLATINUMLIFE	 53
#define AFF_INFINITYLIFE	 54
#define AFF_BIO_REGEN		 55
#define AFF_COMBAT_MIND		 56
#define AFF_BLEEDING		 57
#define AFF_FLASH			 58
#define AFF_CHIPJACK		 59
#define AFF_EXARMS			 60
#define AFF_FOOTJET			 61
#define AFF_MATRIX			 62
#define AFF_AUTOPILOT		 63
#define AFF_ANIMAL_STATS		 64


//  Total number of affect flags not including the don't use flag.
#define NUM_AFF_FLAGS        65

// Combat Flag Bits: used by char_data.char_specials.saved.combat_flags
#define CMB_DONTUSE			0
#define CMB_COMBO			1
#define CMB_NEXTCOMBO		2

#define NUM_CMB_FLAGS		3

// Modes of connectedness: used by descriptor_data.state
#define CON_PLAYING         0   /* < Playing - Nominal state                                    */
#define CON_CLOSE           1   /* < User disconnect, remove character.                         */
#define CON_GET_NAME        2   /* < Login with name                                            */
#define CON_NAME_CNFRM      3   /* < New character, confirm name                                */
#define CON_PASSWORD        4   /* < Login with password                                        */
#define CON_NEWPASSWD       5   /* < New character, create password                             */
#define CON_CNFPASSWD       6   /* < New character, confirm password                            */
#define CON_QSEX            7   /* < Choose character sex                                       */
#define CON_QCLASS          8   /* < Choose character class                                     */
#define CON_RMOTD           9   /* < Reading the message of the day                             */
#define CON_MENU           10   /* < At the main menu                                           */
#define CON_PLR_DESC       11   /* < Enter a new character description prompt                   */
#define CON_CHPWD_GETOLD   12   /* < Changing passwd: Get old                                   */
#define CON_CHPWD_GETNEW   13   /* < Changing passwd: Get new                                   */
#define CON_CHPWD_VRFY     14   /* < Changing passwd: Verify new password                       */
#define CON_DELCNF1        15   /* < Character Delete: Confirmation 1                           */
#define CON_DELCNF2        16   /* < Character Delete: Confirmation 2                           */
#define CON_QATTRIB        17
#define CON_QEXTRA         18
#define CON_QCONSENT       19
#define CON_IDENT          20   /* < looking up hostname/username                               */
#define CON_DISCONNECT     21   /* < In-game link loss (leave character)                        */
#define CON_OEDIT          22   /* < OLC mode - object editor                                   */
#define CON_REDIT          23   /* < OLC mode - room editor                                     */
#define CON_ZEDIT          24   /* < OLC mode - zone info editor                                */
#define CON_MEDIT          25   /* < OLC mode - mobile editor                                   */
#define CON_SEDIT          26   /* < OLC mode - shop editor                                     */
#define CON_TEDIT          27   /* < OLC mode - text editor                                     */
#define CON_CEDIT          28   /* < OLC mode - conf editor                                     */
#define CON_AEDIT          29   /* < OLC mode - social (action) edit                            */
#define CON_TRIGEDIT       30   /* < OLC mode - trigger edit                                    */
#define CON_HEDIT          31   /* < OLC mode - help edit                                       */
#define CON_QEDIT          32   /* < OLC mode - quest edit                                      */
#define CON_PREFEDIT       33   /* < OLC mode - preference edit                                 */
#define CON_IBTEDIT		   34
#define CON_CUSTOM		   35
#define CON_GET_PROTOCOL   36
#define CON_DEAD		   37   /* < New Death handler											*/
#define CON_RUSURE		   38   /* New menu for class selection - Gahan							*/
/* OLC States range - used by IS_IN_OLC and IS_PLAYING */
#define FIRST_OLC_STATE CON_OEDIT     /**< The first CON_ state that is an OLC */
#define LAST_OLC_STATE  CON_CUSTOM   /**< The last CON_ state that is an OLC  */

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
 which control the valid places you can wear a piece of equipment.
 For example, there are two neck positions on the player, and items
 only get the generic neck type. */
#define WEAR_LIGHT              0
#define WEAR_IMPLANT_R          1
#define WEAR_IMPLANT_L          2
#define WEAR_NECK_1             3
#define WEAR_NECK_2             4
#define WEAR_BODY               5
#define WEAR_HEAD               6
#define WEAR_LEGS               7
#define WEAR_FEET               8
#define WEAR_HANDS              9
#define WEAR_ARMS               10
#define WEAR_SHIELD             11
#define WEAR_ABOUT              12
#define WEAR_WAIST              13
#define WEAR_WRIST_R            14
#define WEAR_WRIST_L            15
#define WEAR_FACE               16
#define WEAR_EARS               17
#define WEAR_WIELD              18
#define WEAR_HOLD               19
#define WEAR_FLOATING_NEARBY    20
#define WEAR_RING_NIP_R         21
#define WEAR_RING_NIP_L         22
#define WEAR_RING_FIN_R         23
#define WEAR_RING_FIN_L         24
#define WEAR_RING_EAR_R         25
#define WEAR_RING_EAR_L         26
#define WEAR_RING_NOSE          27
#define WEAR_EXPANSION          28
#define WEAR_MEDICAL			29

//  Total number of available equipment lcoations
#define NUM_WEARS               30    

// Item types: used by obj_data.obj_flags.type_flag
#define ITEM_LIGHT          1   /* < Item is a light source                                     */
#define ITEM_SCROLL         2   /* < Item is a scroll                                           */
#define ITEM_WAND           3   /* < Item is a wand                                             */
#define ITEM_STAFF          4   /* < Item is a staff                                            */
#define ITEM_HANDHELD       5
#define ITEM_WEAPON         5   /* < Item is a weapon                                           */
#define ITEM_FIREWEAPON     6   /* Unimplemented                                                */
#define ITEM_MISSILE        7   /* Unimplemented                                                */
#define ITEM_TREASURE       8   /* Item is a treasure, not units                                */
#define ITEM_ARMOR          9   /* Item is armor                                                */
#define ITEM_POTION        10   /* Item is a potion                                             */
#define ITEM_WORN          11   /* Unimplemented                                                */
#define ITEM_OTHER         12   /* Misc object                                                  */
#define ITEM_TRASH         13   /* Trash - shopkeeps won't buy                                  */
#define ITEM_TRAP          14   /* Unimplemented                                                */
#define ITEM_CONTAINER     15   /* Item is a container                                          */
#define ITEM_NOTE          16   /* Item is note                                                 */
#define ITEM_DRINKCON      17   /* Item is a drink container                                    */
#define ITEM_KEY           18   /* Item is a key                                                */
#define ITEM_FOOD          19   /* Item is food                                                 */
#define ITEM_MONEY         20   /* Item is money                                                */
#define ITEM_PEN           21   /* Item is a pen                                                */
#define ITEM_BOAT          22   /* Item is a boat                                               */
#define ITEM_FOUNTAIN      23   /* Item is a fountain                                           */
#define ITEM_BUTTON        24
#define ITEM_GUN           25
#define ITEM_AMMO          26
#define ITEM_RECALL        27
#define ITEM_LOTTO         28
#define ITEM_SCRATCH       29
#define ITEM_FLAG          30
#define ITEM_SABER_PIECE   31
#define ITEM_IDENTIFY      32
#define ITEM_MEDKIT        33 
#define ITEM_DRUG          34
#define ITEM_EXPLOSIVE     35
#define ITEM_MARBLE_QUEST  36
#define ITEM_GEIGER        37   /* item is a geiger counter                                     */
#define ITEM_AUTOQUEST     38   /* autoquest token                                              */
#define ITEM_BAROMETER     39   /* tells weather pressure and change                            */
#define ITEM_PSIONIC_BOX   40   /* magical box of lucky charms!                                 */
#define ITEM_QUEST         41   /* qcreate item                                                 */
#define ITEM_EXPANSION     42   /* Expansion Chip for Advanced Skull Bionic - fighting styles   */
#define ITEM_FURNITURE     43   /* < Sittable Furniture                                         */
#define ITEM_IMBUEABLE     44   /* < Imbueable                                                  */
#define ITEM_BODY_PART     45   /* Item was once part of someone                                */
#define ITEM_BIONIC_DEVICE 46   /* Item is an implant                                           */
#define ITEM_WEAPONUPG     47   /* Item is a weapon upgrade                                   */
#define ITEM_MEDICAL	   48
#define ITEM_VEHICLE	   49
#define ITEM_ATTACHMENT	   50
#define ITEM_TABLE	   51

// Total number of item types.
#define NUM_ITEM_TYPES     52

// Take/Wear flags: used by obj_data.obj_flags.wear_flags
#define ITEM_WEAR_TAKE      0   /* Item can be takes                                            */
#define ITEM_WEAR_IMPLANT   1   /* Can be worn inside body                                      */
#define ITEM_WEAR_NECK      2   /* Can be worn around neck                                      */
#define ITEM_WEAR_BODY      3   /* Can be worn on body                                          */
#define ITEM_WEAR_HEAD      4   /* Can be worn on head                                          */
#define ITEM_WEAR_LEGS      5   /* Can be worn on legs                                          */
#define ITEM_WEAR_FEET      6   /* Can be worn on feet                                          */
#define ITEM_WEAR_HANDS     7   /* Can be worn on hands                                         */
#define ITEM_WEAR_ARMS      8   /* Can be worn on arms                                          */
#define ITEM_WEAR_SHIELD    9   /* Can be used as a shield                                      */
#define ITEM_WEAR_ABOUT     10  /* Can be worn about body                                       */
#define ITEM_WEAR_WAIST     11  /* Can be worn around waist                                     */
#define ITEM_WEAR_WRIST     12  /* Can be worn on wrist                                         */
#define ITEM_WEAR_FACE      13
#define ITEM_WEAR_EARS      14
#define ITEM_WEAR_WIELD     15  /* Can be wielded                                               */
#define ITEM_WEAR_HOLD      16  /* Can be held                                                  */
#define ITEM_THROW          17
#define ITEM_FLOAT_NEARBY   18
#define ITEM_WEAR_NIPPLE    19  /* Can be worn on nipple                                        */
#define ITEM_WEAR_FINGER    20  /* hmm what could this be?                                      */
#define ITEM_WEAR_EARRING   21
#define ITEM_WEAR_NOSER     22  /* mmm nose ring? strange                                       */
#define ITEM_WEAR_EXPANSION 23  /* fighting style chip, advanced skull bionic                   */
#define ITEM_WEAR_MEDICAL   24 

//  Total number of item wears
#define NUM_ITEM_WEARS      25

// Extra object flags: used by obj_data.obj_flags.extra_flags
#define ITEM_GLOW              0    /* < Item is glowing                                        */
#define ITEM_HUM               1    /* < Item is humming                                        */
#define ITEM_MINLEV15          2
#define ITEM_MINLEV25          3
#define ITEM_IMM_ONLY          4
#define ITEM_INVISIBLE         5    /* Item is invisible                                        */
#define ITEM_PSIONIC           6    /* Item is psionical (really should be radionic)            */
#define ITEM_NODROP            7    /* Item is cursed: can't drop                               */
#define ITEM_BLESS             8    /* Item is blessed                                          */
#define ITEM_QUEST_ITEM        9    /* quest token? autoquest? hmmm                             */
#define ITEM_ANTI_DT           10   /* will not be lost in a DT -lump                           */
#define ITEM_NOT_USED          11   /* Not usable by neutral people                             */
#define ITEM_NORENT            12   /* Item cannot be rented                                    */
#define ITEM_NODONATE          13   /* Item cannot be donated                                   */
#define ITEM_NOINVIS           14   /* Item cannot be made invis                                */
#define ITEM_BORG_ONLY         15
#define ITEM_CRAZY_ONLY        16
#define ITEM_STALKER_ONLY      17
#define ITEM_MERCENARY_ONLY    18
#define ITEM_HIGHLANDER_ONLY   19
#define ITEM_PREDATOR_ONLY     20
#define ITEM_ANTI_BORG         21
#define ITEM_ANTI_CRAZY        22
#define ITEM_ANTI_STALKER      23
#define ITEM_ANTI_MERCENARY    24
#define ITEM_ANTI_HIGHLANDER   25
#define ITEM_ANTI_PREDATOR     26
#define ITEM_NOSELL            27   /* Shopkeepers won't touch it                               */
#define ITEM_SEVERING          28   /* beheading 268435456                                      */
#define ITEM_NOLOCATE          29   /* item can't be seen with psi 'locate'                     */
#define ITEM_REMORT_ONLY       30   /* item can only be worn/use etc by a remort_char           */
#define ITEM_ANTI_CALLER       31
#define ITEM_CALLER_ONLY       32
#define ITEM_CORPSE_UNDEAD	   33
#define ITEM_CORPSE_HUMANOID   34
#define ITEM_CORPSE_MECHANICAL 35
#define ITEM_CORPSE_HIGHLANDER 36
#define ITEM_CORPSE_ANIMAL	   37
#define ITEM_CORPSE_CANFEL	   38
#define ITEM_CORPSE_INSECT	   39
#define ITEM_CORPSE_REPTILE    40
#define ITEM_CORPSE_AQUATIC    41
#define ITEM_CORPSE_ALIEN	   42
#define ITEM_CORPSE_MUTANT     43
#define ITEM_CORPSE_PLANT	   44
#define ITEM_CORPSE_ETHEREAL   45
#define ITEM_CORPSE_SHELL	   46
#define ITEM_CORPSE_BIRD	   47
#define ITEM_HIDDEN			   48
#define ITEM_BOP			   49				
#define ITEM_UNIQUE			   50				

//  Total number of item flags
#define NUM_ITEM_FLAGS    51

// Modifier constants used with obj affects ('A' fields)
#define APPLY_NONE              0   /* No effect                                                */
#define APPLY_STR               1   /* Apply to strength                                        */
#define APPLY_DEX               2   /* Apply to dexterity                                       */
#define APPLY_INT               3   /* Apply to intelligence                                    */
#define APPLY_PCN               4   /* Apply to perception                                      */
#define APPLY_CON               5   /* Apply to constitution                                    */
#define APPLY_CHA               6   /* Apply to charisma                                        */
#define APPLY_SKILL             7   /* Skillset! -Lump 3/07                                     */
#define APPLY_PSI_MASTERY       8   /* Mastery skill for Psionics, general boost                */
#define APPLY_AGE               9   /* Apply to age                                             */
#define APPLY_CHAR_WEIGHT      10   /* Apply to weight                                          */
#define APPLY_CHAR_HEIGHT      11   /* Apply to height                                          */
#define APPLY_PSI              12   /* Apply to max psi                                         */
#define APPLY_HIT              13   /* Apply to max hit points                                  */
#define APPLY_MOVE             14   /* Apply to max move points                                 */
#define APPLY_AC               15   /* Apply to Armor Class                                     */
#define APPLY_ARMOR            15   /* Apply to Armor Class                                     */
#define APPLY_HITROLL          16   /* Apply to hitroll                                         */
#define APPLY_DAMROLL          17   /* Apply to damage roll                                     */   /* Apply to save throw: psionics                            */
#define APPLY_SAVING_NORMAL	   18	/* UNUSED */
#define APPLY_PSI_REGEN        19   /* Apply to psi regen ability                               */
#define APPLY_HIT_REGEN        20   /* Apply to hit regen ability                               */
#define APPLY_MOVE_REGEN       21   /* Apply to move regen ability                              */
#define APPLY_HITNDAM          22   /* Apply to hitroll and damage roll                         */
#define APPLY_REGEN_ALL        23   /* Apply to all regen ability                               */
#define APPLY_HPV              24   /* Apply to hit and psi points                              */
#define APPLY_PSI2HIT          25   /* Apply to conversion of psi points to hit points          */
#define APPLY_ALL_STATS        26   /* Apply to all player stats                                */
#define APPLY_PSIONIC          27   /* Apply to psionic skill???                                */
#define APPLY_SAVING_POISON	   28
#define APPLY_SAVING_DRUG	   29
#define APPLY_SAVING_FIRE      30
#define APPLY_SAVING_COLD      31
#define APPLY_SAVING_ELECTRICITY 32
#define APPLY_SAVING_EXPLOSIVE 33
#define APPLY_SAVING_PSIONIC   34
#define APPLY_SAVING_NONORM    35
#define APPLY_POISON_FOCUS	   36
#define APPLY_DRUG_FOCUS	   37
#define APPLY_FIRE_FOCUS	   38
#define APPLY_COLD_FOCUS       39
#define APPLY_ELECTRICITY_FOCUS 40
#define APPLY_EXPLOSIVE_FOCUS  41
#define APPLY_PSIONIC_FOCUS    42
#define APPLY_ETHEREAL_FOCUS   43

//  Total number of applies
#define NUM_APPLIES            44

// CyberASSAULT Sepcific Saving Throws - Gahan
#define SAVING_NORMAL				0
#define SAVING_POISON				1
#define SAVING_DRUG					2
#define SAVING_FIRE					3
#define SAVING_COLD					4
#define SAVING_ELECTRICITY			5
#define SAVING_EXPLOSIVE			6
#define SAVING_PSIONIC				7
#define SAVING_NONORM				8

#define MAX_SAVING_THROW			9

// Explosive Types: used by the explosive_update routine
#define EXPLOSIVE_GRENADE   1
#define EXPLOSIVE_MINE      2
#define EXPLOSIVE_DESTRUCT  3
#define EXPLOSIVE_TNT       4
#define EXPLOSIVE_COCKTAIL  5
#define EXPLOSIVE_PLASTIQUE 6

#define NUM_EXPLOSIVE_TYPES 6

// Sub-explosive types (grenades)
#define GRENADE_NORMAL      1
#define GRENADE_GAS         2
#define GRENADE_FLASHBANG   3
#define GRENADE_SONIC       4
#define GRENADE_SMOKE       5
#define GRENADE_FIRE        6
#define GRENADE_CHEMICAL    7
#define GRENADE_PSYCHIC     8
#define GRENADE_LAG         9
#define GRENADE_NUCLEAR    10
#define GRENADE_NAPALM     11

#define NUM_GRENADE_TYPES   11

// Defines used for weapons
#define GUN_NONE        0
#define GUN_ENERGY      1
#define GUN_CLIP        2
#define GUN_SHELL       3
#define GUN_AREA        4
#define GUN_FUEL        5
#define GUN_ROCKET      6
#define GUN_BOLT		7
#define GUN_ARROW		8

#define NUM_GUN_TYPES   9 

#define AMMO_NONE       0
#define AMMO_ENERGY     1
#define AMMO_CLIP       2
#define AMMO_SHELL      3
#define AMMO_AREA       4
#define AMMO_FUEL       5
#define AMMO_ROCKET     6
#define AMMO_BOLT		7
#define AMMO_ARROW		8

#define NUM_WPNPSIONICS          31 //17 spells + 0 for none..

// plastique indicators 
#define PLASTIQUE_ROOM_NORTH    1
#define PLASTIQUE_ROOM_EAST     2 
#define PLASTIQUE_ROOM_SOUTH    3 
#define PLASTIQUE_ROOM_WEST     4 
#define PLASTIQUE_ROOM_UP       5 
#define PLASTIQUE_ROOM_DOWN     6 
#define PLASTIQUE_CHAR          7    // todo: NOT YET IMPLEMENTED 
#define PLASTIQUE_OBJ           8 
#define PLASTIQUE_VEHICLE       9    // todo: NOT YET IMPLEMENTED 
/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/**< Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/**< Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/**< Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/**< Container is locked		*/

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0   /**< Liquid type water */
#define LIQ_BEER       1   /**< Liquid type beer */
#define LIQ_WINE       2   /**< Liquid type wine */
#define LIQ_ALE        3   /**< Liquid type ale */
#define LIQ_DARKALE    4   /**< Liquid type darkale */
#define LIQ_WHISKY     5   /**< Liquid type whisky */
#define LIQ_LEMONADE   6   /**< Liquid type lemonade */
#define LIQ_FIREBRT    7   /**< Liquid type firebrt */
#define LIQ_LOCALSPC   8   /**< Liquid type localspc */
#define LIQ_SLIME      9   /**< Liquid type slime */
#define LIQ_MILK       10  /**< Liquid type milk */
#define LIQ_TEA        11  /**< Liquid type tea */
#define LIQ_COFFE      12  /**< Liquid type coffee */
#define LIQ_BLOOD      13  /**< Liquid type blood */
#define LIQ_SALTWATER  14  /**< Liquid type saltwater */
#define LIQ_CLEARWATER 15  /**< Liquid type clearwater */
/** Total number of liquid types */
#define NUM_LIQ_TYPES     16

/* other miscellaneous defines */
/* Player conditions */
#define DRUNK        0  /**< Player drunk condition */
#define HUNGER       1  /**< Player hunger condition */
#define THIRST       2  /**< Player thirst condition */

/* Sun state for weather_data */
#define SUN_DARK	0  /**< Night time */
#define SUN_RISE	1  /**< Dawn */
#define SUN_LIGHT	2  /**< Day time */
#define SUN_SET		3  /**< Dusk */

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS  0  /**< Weather = No clouds */
#define SKY_CLOUDY     1  /**< Weather = Cloudy */
#define SKY_RAINING    2  /**< Weather = Rain */
#define SKY_LIGHTNING  3  /**< Weather = Lightning storm */
// Age effects
#define AGE_BATCH_0         (1 << 0)
#define AGE_BATCH_1         (1 << 1) // Reduce HnD by 5 in db.c (will this be permanent???)
#define AGE_BATCH_2         (1 << 2) // Reduce Exp Gained, to 75%
#define AGE_BATCH_3         (1 << 3) // Reduce HpV regen rate
#define AGE_BATCH_4         (1 << 4)
#define AGE_BATCH_5         (1 << 5)
#define AGE_BATCH_6         (1 << 6)
#define AGE_BATCH_7         (1 << 7)
#define AGE_BATCH_8         (1 << 8)
#define AGE_BATCH_9         (1 << 9)
/* Rent codes */
#define RENT_UNDEF      0 /**< Character inv save status = undefined */
#define RENT_CRASH      1 /**< Character inv save status = game crash */
#define RENT_RENTED     2 /**< Character inv save status = rented */
#define RENT_CRYO       3 /**< Character inv save status = cryogenics */
#define RENT_FORCED     4 /**< Character inv save status = forced rent */
#define RENT_TIMEDOUT   5 /**< Character inv save status = timed out */
// Teams (GET_TEAM)
#define TEAM_NONE           0
#define TEAM_RED            1
#define TEAM_BLUE           2
#define TEAM_YELLOW         3
#define TEAM_GREEN          4
#define TEAM_CYAN           5
#define TEAM_MAGENTA        6
#define TEAM_WHITE          7

// Bionic core shit
#define BIONIC_NOTYPE		0
#define BIONIC_INTERFACE	1
#define BIONIC_CORE			2
#define BIONIC_STRUCTURE	3
#define BIONIC_ARMOR		4
#define BIONIC_CHIPJACK		5
#define BIONIC_FOOTJET		6
#define BIONIC_MATRIX		7

#define MAX_BIO_TYPES		8

// Beginning of bionics system
#define BIO_INTERFACE           0
#define BIO_CHASSIS             1
#define BIO_SPINE               2
#define BIO_SHOULDER            3
#define BIO_RIBCAGE             4
#define BIO_HIP                 5
#define BIO_ARMS                6
#define BIO_NECK                7
#define BIO_LUNG                8
#define BIO_LEGS                9
#define BIO_SKULL              10
#define BIO_CORE               11
#define BIO_BLASTER            12
#define BIO_BLADES             13
#define BIO_EXARMS             14
#define BIO_EYE                15
#define BIO_JACK               16
#define BIO_VOICE              17
#define BIO_ARMOR              18
#define BIO_JET                19
#define BIO_FOOTJET            20
#define BIO_MATRIX             21 
#define BIO_DESTRUCT           22
#define BIO_CHIPJACK           23

#define BIONIC_NONE             0
#define BIONIC_BASIC            1
#define BIONIC_ADVANCED         2
#define BIONIC_REMORT           3

// body parts which each player has
#define BODY_PART_RESERVED      0
#define BODY_PART_HEAD          1 
#define BODY_PART_LEFT_EYE      2 
#define BODY_PART_RIGHT_EYE     3 
#define BODY_PART_MOUTH         4 
#define BODY_PART_NECK          5 
#define BODY_PART_LEFT_ARM      6 
#define BODY_PART_RIGHT_ARM     7 
#define BODY_PART_LEFT_WRIST    8 
#define BODY_PART_RIGHT_WRIST   9 
#define BODY_PART_LEFT_HAND     10 
#define BODY_PART_RIGHT_HAND    11
#define BODY_PART_LEFT_LEG      12
#define BODY_PART_RIGHT_LEG     13
#define BODY_PART_LEFT_FOOT     14
#define BODY_PART_RIGHT_FOOT    15
#define BODY_PART_CHEST         16
#define BODY_PART_ABDOMAN       17
#define BODY_PART_BACK          18
#define BODY_PART_INTERNAL1     19
#define BODY_PART_INTERNAL2		20
#define BODY_PART_INTERNAL3		21
#define BODY_PART_INTERNAL4		22

#define NUM_BODY_PARTS          23 


// body part conditions
#define NORMAL              (1 << 0)
#define DAMAGED             (1 << 1)
#define BLEEDING            (1 << 2)
#define BROKEN              (1 << 3)
#define RIPPED              (1 << 4)
#define BIONIC_NORMAL       (1 << 5)
#define BIONIC_DAMAGED      (1 << 6)
#define BIONIC_BROKEN       (1 << 7)
#define BIONIC_BUSTED       (1 << 8)
#define BIONIC_MISSING      (1 << 9)

// vehicle defines

#define VTYPE_ROADSTER		0
#define VTYPE_OFFROAD		1
#define VTYPE_HOVERFLY		2
#define VTYPE_BOAT			3
#define VTYPE_SUBMARINE		4
#define VTYPE_LAV			5

#define MAX_VEHICLE_TYPES	6

#define MAX_VEHICLES		10

#define MIN_VEHICLE_ROOM 	2710
#define MAX_VEHICLE_ROOM 	2799
#define LOT_VNUM			3092

/* Settings for Bit Vectors */
#define RF_ARRAY_MAX    8  /**< # Bytes in Bit vector - Room flags */
#define PM_ARRAY_MAX    8  /**< # Bytes in Bit vector - Act and Player flags */
#define PR_ARRAY_MAX    8  /**< # Bytes in Bit vector - Player Pref Flags */
#define AF_ARRAY_MAX    8  /**< # Bytes in Bit vector - Affect flags */
#define CB_ARRAY_MAX	8  /**< # Bytes in Bit vector - Combat flags */
#define TW_ARRAY_MAX    8  /**< # Bytes in Bit vector - Obj Wear Locations */
#define EF_ARRAY_MAX    8  /**< # Bytes in Bit vector - Obj Extra Flags */
#define ZN_ARRAY_MAX    8  /**< # Bytes in Bit vector - Zone Flags */

/* other #defined constants */
/* **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1. */
#define LVL_IMPL    45  /**< Level of Implementors */
#define LVL_CIMP	44
#define LVL_GRGOD   43  /**< Level of Greater Gods */
#define LVL_GOD     42  /**< Level of Gods */
#define LVL_IMMORT	41  /**< Level of Immortals */

/** Minimum level to build and to run the saveall command */
#define LVL_BUILDER	LVL_IMMORT
#define LVL_FREEZE              LVL_GRGOD

/** Arbitrary number that won't be in a string */
#define MAGIC_NUMBER	(0x06)

/** OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 * This will equate to 10 passes per second.
 * @see PASSES_PER_SEC
 * @see RL_SEC
 */
#define OPT_USEC	100000
/** How many heartbeats equate to one real second.
 * @see OPT_USEC
 * @see RL_SEC
 */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
/** Used with other macros to define at how many heartbeats a control loop
 * gets executed. Helps to translate pulse counts to real seconds for
 * human comprehension.
 * @see PASSES_PER_SEC
 */
#define RL_SEC		* PASSES_PER_SEC
#define PULSE_SECOND			(1 RL_SEC)
/** Controls when a zone update will occur. */
#define PULSE_ZONE      (10 RL_SEC)
/** Controls when mobile (NPC) actions and updates will occur. */
#define PULSE_MOBILE    (10 RL_SEC)
// New fight code: NEWFIGHT
#define PULSE_FIGHT			(PASSES_PER_SEC / 3)
//  Controls the time between turns of combat
#define PULSE_VIOLENCE          (2 RL_SEC)

#define PULSE_ROOM              (10 RL_SEC)
#define PULSE_ROOM_FIRE         (2 RL_SEC)
#define PULSE_EXPLOSIVE         (1 RL_SEC) 
#define PULSE_REGEN             (4 RL_SEC)
#define PULSE_ENCUMBRANCE		(4 RL_SEC)
#define PULSE_AUCTION			(20 RL_SEC)
/** Controls when characters and houses (if implemented) will be autosaved.
 * @see CONFIG_AUTO_SAVE
 */
#define PULSE_AUTOSAVE  (60 RL_SEC)
/** Controls when checks are made for idle name and password CON_ states */
#define PULSE_IDLEPWD   (15 RL_SEC)
/** Currently unused. */
#define PULSE_SANITY    (30 RL_SEC)
/** How often to log # connected sockets and # active players.
 * Currently set for 5 minutes.
 */
#define PULSE_USAGE     (5 * 60 RL_SEC)
/** Controls when to save the current ingame MUD time to disk.
 * This should be set >= SECS_PER_MUD_HOUR */
#define PULSE_TIMESAVE	(30 * 60 RL_SEC)
/* Variables for the output buffering system */
#define MAX_SOCK_BUF       (48 * 1024) /**< Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH  500          /**< Max length of prompt        */
#define GARBAGE_SPACE      32          /**< Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE      2048        /**< Static output buffer size   */
/** Max amount of output that can be buffered */
#define LARGE_BUFSIZE      (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define MAX_STRING_LENGTH     49152  /**< Max length of string, as defined */
#define MAX_INPUT_LENGTH      (2 * 4096)    /**< Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH  (24 * 1024)   /**< Max size of *raw* input */
#define MAX_MESSAGES          60     /**< Max Different attack message types */
#define MAX_NAME_LENGTH       20     /**< Max PC/NPC name length */
#define MAX_PWD_LENGTH        30     /**< Max PC password length */
#define MAX_TITLE_LENGTH      80     /**< Max PC title length */
#define HOST_LENGTH           40     /**< Max hostname resolution length */
#define PLR_DESC_LENGTH       4096   /**< Max length for PC description */
#define MAX_PSIONICS          130
#define MAX_SKILLS            200    /**< Max number of skills/spells */
#define MAX_AFFECT            32     /**< Max number of player affections */
#define MAX_OBJ_AFFECT        6      /**< Max object affects */
#define MAX_OBJ_APPLY         6      /**< Max object applies                         */
#define MAX_NOTE_LENGTH       4000   /**< Max length of text on a note obj */
#define MAX_LAST_ENTRIES      6000   /**< Max log entries?? */
#define MAX_HELP_KEYWORDS     256    /**< Max length of help keyword string */
#define MAX_HELP_ENTRY        MAX_STRING_LENGTH /**< Max size of help entry */
#define MAX_COMPLETED_QUESTS  1024   /**< Maximum number of completed quests allowed */
#define MAX_BIONIC              23      /* note: don't go over 40                       */
#define SEVERED_HEAD_VNUM   1712    //used for highlander death
#define MAIL_OBJ_VNUM          0    //mail item
#define SKULL_VNUM         21515    //used for predator PK trophy (skull)
#define MAX_UNITS 2000000000 /**< Maximum possible on hand gold (4 Billion) */
#define MAX_BANK  2000000000 /**< Maximum possible in bank gold (4 Billion) */
#define MAX_GPS_LOCATIONS	10
#define MAX_COMBOLEARNED	10
#define MAX_COMBOLENGTH		5

/** Define the largest set of commands for a trigger.
 * 16k should be plenty and then some. */
#define MAX_CMD_LENGTH (2 * 32468)

/* Type Definitions */
typedef signed char sbyte;          /**< 1 byte; vals = -127 to 127 */
typedef unsigned char ubyte;        /**< 1 byte; vals = 0 to 255 */
typedef signed short int sh_int;    /**< 2 bytes; vals = -32,768 to 32,767 */
typedef unsigned short int ush_int; /**< 2 bytes; vals = 0 to 65,535 */

#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char bool; /**< Technically 1 signed byte; vals should only = TRUE or FALSE. */
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef signed char byte; /**< Technically 1 signed byte; vals should only = TRUE or FALSE. */
#endif

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;  /**< vnum specifically for room */
typedef IDXTYPE obj_vnum;   /**< vnum specifically for object */
typedef IDXTYPE mob_vnum;   /**< vnum specifically for mob (NPC) */
typedef IDXTYPE zone_vnum;  /**< vnum specifically for zone */
typedef IDXTYPE shop_vnum;  /**< vnum specifically for shop */
typedef IDXTYPE trig_vnum;  /**< vnum specifically for triggers */
typedef IDXTYPE qst_vnum;   /**< vnum specifically for quests */

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;  /**< references an instance of a room */
typedef IDXTYPE obj_rnum;   /**< references an instance of a obj */
typedef IDXTYPE mob_rnum;   /**< references an instance of a mob (NPC) */
typedef IDXTYPE zone_rnum;  /**< references an instance of a zone */
typedef IDXTYPE shop_rnum;  /**< references an instance of a shop */
typedef IDXTYPE trig_rnum;  /**< references an instance of a trigger */
typedef IDXTYPE qst_rnum;   /**< references an instance of a quest */

/** Bitvector type for 32 bit unsigned long bitvectors. 'unsigned long long'
 * will give you at least 64 bits if you have GCC. You'll have to search
 * throughout the code for "bitvector_t" and change them yourself if you'd
 * like this extra flexibility. */
typedef unsigned int bitvector_t;
typedef unsigned long long int bitvector_c;

/** Extra description: used in objects, mobiles, and rooms. For example,
 * a 'look hair' might pull up an extra description (if available) for
 * the mob, object or room.
 * Multiple extra descriptions on the same object are implemented as a
 * linked list. */
struct extra_descr_data
{
  char *keyword;      /**< Keyword for look/examine  */
  char *description;  /**< What is shown when this keyword is 'seen' */
  struct extra_descr_data *next; /**< Next description for this mob/obj/room */
};

/* object-related structures */
/**< Number of elements in the object value array. Raising this will provide
 * more configurability per object type, and shouldn't break anything.
 * DO NOT LOWER from the default value of 4. */
#define NUM_OBJ_VAL_POSITIONS 7

/* CyberASSAULT Crafting Struct - Gahan 2013 */
struct player_crafting_data {
	sh_int cclass;
	sh_int subcclass1;
	sh_int subcclass2;
};
/* Cyber assault object stuff */
struct bionic_info_type {
	const char *short_name;
    const char *long_name;
	int device;
    int prereq[3];
    int bio_level;
	int unit_cost;
    int psi_cost;
    int min_level;
	const char *success;
};

struct obj_weapon_data {
    int gun_type;
    int shots_per_round;
    int range;
    struct obj_data *ammo_loaded;
    int bash[2];
};

// I have a better design -- Tails
struct obj_ammo_data {
    int ammo_type;
    int ammo_count;
};

struct obj_wpnpsionic_data {
    int which_psionic;
    int skill_level;
};

struct bionic_data {
    struct obj_data *device;
    struct bionic_data *next;
};
struct affected_rooms {
    sh_int room_number;
    struct affected_rooms *room_next;
};
struct obj_apply_type
{
    byte location;          /* < Which ability to change (APPLY_XXX) */
    sh_int modifier;         /* < How much it changes by              */
};
/** object flags used in obj_data. These represent the instance values for
 * a real object, values that can change during gameplay. */
struct obj_flag_data
{
  int value[NUM_OBJ_VAL_POSITIONS]; /**< Values of the item (see list)    */
  byte type_flag;                   /**< Type of item			    */
  int level;                        /**< Minimum level to use object	    */
  int wear_flags[TW_ARRAY_MAX];     /**< Where you can wear it, if wearable */
  int extra_flags[EF_ARRAY_MAX];    /**< If it hums, glows, etc.	    */
  int weight;                       /**< Weight of the object */
  int cost;                         /**< Value when sold             */
  int cost_per_day;                 /**< Rent cost per real day */
  int timer;                        /**< Timer for object             */
  bool timer_on; 
  int bitvector[AF_ARRAY_MAX];      /**< Affects characters           */
  time_t stamp;
  
  struct obj_apply_type applies[MAX_OBJ_APPLY];

  ubyte remort;
  byte composition;
  byte subcomposition;
};

/** Used in obj_file_elem. DO NOT CHANGE if you are using binary object files
 * and already have a player base and don't want to do a player wipe. */
struct obj_affected_type
{
  byte location;  /**< Which ability to change (APPLY_XXX) */
  sbyte modifier; /**< How much it changes by              */
};
struct attack_rate_type
{
	sh_int fire_rate;
	sh_int num_attacks;
};
/** The Object structure. */
struct obj_data
{
	obj_rnum item_number; /**< The unique id of this object instance. */
	room_rnum in_room;    /**< What room is the object lying in, or -1? */

	struct obj_flag_data obj_flags;  /**< Object information            */
	struct obj_affected_type affected[MAX_OBJ_AFFECT]; /**< affects */

	char *name;        /**< Keyword reference(s) for object. */
	char *description; /**< Shown when the object is lying in a room. */
	char *short_description;  /**< Shown when worn, carried, in a container */
	char *action_description; /**< Displays when (if) the object is used */
	struct extra_descr_data *ex_description; /**< List of extra descriptions */
	struct char_data *carried_by; /**< Points to PC/NPC carrying, or NULL */
	struct char_data *worn_by; /**< Points to PC/NPC wearing, or NULL */
	sh_int worn_on; /**< If the object can be worn, where can it be worn? */

	struct obj_data *in_obj; /**< Points to carrying object, or NULL */
	struct obj_data *contains; /**< List of objects being carried, or NULL */
	long user;
	long id; /**< used by DG triggers - unique id  */
	long bound_id;
	char *bound_name;
	struct trig_proto_list *proto_script; /**< list of default triggers  */
	struct script_data *script;           /**< script info for the object */
	struct bionic_data *bionics;                /* corpse can hold bionics                              */
	struct obj_data *next_content;  /**< For 'contains' lists   */
	struct obj_data *next;          /**< For the object list */
	struct char_data *sitting_here; /**< For furniture, who is sitting in it */
	struct obj_data *plastique;
	struct obj_data *attached;                  /* where the explosive is attached                      */
    struct obj_wpnpsionic_data obj_wpnpsi;
    struct obj_weapon_data obj_weapon;
    struct obj_ammo_data obj_ammo;
	struct char_data *implanted;                /* implanted into someone?                              */
	int gps[MAX_GPS_LOCATIONS];
	sh_int fuel;
	sh_int damage;
	sh_int armor;
};

/** Instance info for an object that gets saved to disk.
 * DO NOT CHANGE if you are using binary object files
 * and already have a player base and don't want to do a player wipe. */
struct obj_file_elem
{
  obj_vnum item_number; /**< The prototype, non-unique info for this object. */

#if USE_AUTOEQ
  sh_int location; /**< If re-equipping objects on load, wear object here */
#endif
  int value[NUM_OBJ_VAL_POSITIONS]; /**< Current object values */
  int extra_flags[EF_ARRAY_MAX];    /**< Object extra flags */
  int weight;                       /**< Object weight */
  int timer;                        /**< Current object timer setting */
  int bitvector[AF_ARRAY_MAX];      /**< Object affects */
  struct obj_affected_type affected[MAX_OBJ_AFFECT]; /**< Affects to mobs */
};

/** Header block for rent files.
 * DO NOT CHANGE the structure if you are using binary object files
 * and already have a player base and don't want to do a player wipe.
 * If you are using binary player files, feel free to turn the spare
 * variables into something more meaningful, as long as you keep the
 * int datatype.
 * NOTE: This is *not* used with the ascii playerfiles.
 * NOTE 2: This structure appears to be unused in this codebase? */
struct rent_info
{
  int time;
  int rentcode;          /**< How this character rented */
  int net_cost_per_diem; /**< ? Appears to be unused ? */
  int gold;              /**< ? Appears to be unused ? */
  int account;           /**< ? Appears to be unused ? */
  int nitems;            /**< ? Appears to be unused ? */
  int spare0;
  int spare1;
  int spare2;
  int spare3;
  int spare4;
  int spare5;
  int spare6;
  int spare7;
};

/* room-related structures */

/** Direction (north, south, east...) information for rooms. */
struct room_direction_data
{
  char *general_description; /**< Show to char looking in this direction. */

  char *keyword; /**< for interacting (open/close) this direction */

  sh_int /*bitvector_t*/ exit_info; /**< Door, and what type? */
  obj_vnum key;      /**< Key's vnum (-1 for no key) */
  room_rnum to_room; /**< Where direction leads, or NOWHERE if not defined */
};

/** The Room Structure. */
struct room_data
{
  room_vnum number;  /**< Rooms number (vnum) */
  zone_rnum zone;    /**< Room zone (for resetting) */
  int sector_type;   /**< sector type (move/hide) */
  int room_flags[RF_ARRAY_MAX]; /**< INDOORS, DARK, etc */
  char *name;        /**< Room name */
  char *description; /**< Shown when entered, looked at */
  struct extra_descr_data *ex_description; /**< Additional things to look at */
  struct room_direction_data *dir_option[NUM_OF_DIRS]; /**< Directions */
  byte light;        /**< Number of lightsources in room */
  SPECIAL(*func);    /**< Points to special function attached to room */
  struct trig_proto_list *proto_script; /**< list of default triggers */
  struct script_data *script; /**< script info for the room */
  struct obj_data *contents;  /**< List of items in room */
  struct affected_type *affected;
  struct char_data *people;   /**< List of NPCs / PCs in room */
  struct obj_data *plastique[NUM_OF_DIRS];

  struct list_data * events;
};

/* char-related structures */
/** Memory structure used by NPCs to remember specific PCs. */
struct memory_rec_struct
{
  long id;  /**< The PC id to remember. */
  struct memory_rec_struct *next; /**< Next PC to remember */
};

/** memory_rec_struct typedef */
typedef struct memory_rec_struct memory_rec;

/** This structure is purely intended to be an easy way to transfer and return
 * information about time (real or mudwise). */
struct time_info_data
{
  int hours;   /**< numeric hour */
  int day;     /**< numeric day */
  int month;   /**< numeric month */
  sh_int year; /**< numeric year */
};

/** Player specific time information. */
struct time_data
{
  time_t birth; /**< Represents the PCs birthday, used to calculate age. */
  time_t logon; /**< Time of the last logon, used to calculate time played */
  int played;   /**< This is the total accumulated time played in secs */
};

/** Player Vehicle Data */
struct vehicle_data
{
	struct obj_data *vehicle;
	int vehicle_room_vnum;
	int vehicle_vnum;
	int vehicle_in_room;
	int vin;	
};

/** The pclean_criteria_data is set up in config.c and used in db.c to determine
 * the conditions which will cause a player character to be deleted from disk
 * if the automagic pwipe system is enabled (see config.c). */
struct pclean_criteria_data
{
  int level; /**< PC level and below to check for deletion */
  int days;  /**< time limit in days, for this level of PC */
};

/** General info used by PC's and NPC's. */
struct char_player_data
{
  char passwd[MAX_PWD_LENGTH+1]; /**< PC's password */
  char *name;                    /**< PC / NPC name */
  char *short_descr;             /**< NPC 'actions' */
  char *long_descr;              /**< PC / NPC look description */
  char *description;             /**< NPC Extra descriptions */
  char *title;                   /**< PC / NPC title */
  byte sex;                      /**< PC / NPC sex */
  byte chclass;                  /**< PC / NPC class */
  byte level;                    /**< PC / NPC level */
  struct time_data time;         /**< PC AGE in days */
  ubyte weight;                  /**< PC / NPC weight */
  ubyte height;                  /**< PC / NPC height */

    int num_deaths;                 /* Number of deaths (death counter)         */
    byte num_remorts;               /* Number of times a player has remorted    */
    long age_modifier;              /* Modifier to age for dying, drugs, etc    */
    int essences;                   /* for higlanders                           */
    int num_dts;                    /* Number of DT's a player has hit          */
	int storagecap;					/* amount of items allowed in storage		*/
	int autopilot;
};

/** Character abilities. Different instances of this structure are used for
 * both inherent and current ability scores (like when poison affects the
 * player strength). */
struct char_ability_data
{
  sbyte str;     /**< Strength.  */
  sbyte str_add; /**< Strength multiplier if str = 18. Usually from 0 to 100 */
  sbyte intel;   /**< Intelligence */
  sbyte pcn;     /**< Perception */
  sbyte dex;     /**< Dexterity */
  sbyte con;     /**< Constitution */
  sbyte cha;     /**< Charisma */
};

struct body_part_info {
    int no_of_hits;
    int condition;
    struct obj_data *bionic_device;
};
// todo: re-implement the percentage.  level alone is boring.
struct knowledge {
    int level;
    int prac_spent;
};

struct combo_knowledge {
	int ident;
};
/** Character 'points', or health statistics. */
struct char_point_data
{
  int psi;     /**< Current mana level  */
  int max_psi; /**< Max mana level */
  int hit;      /**< Curent hit point, or health, level */
  int max_hit;  /**< Max hit point, or health, level */
  int move;     /**< Current move point, or stamina, level */
  int max_move; /**< Max move point, or stamina, level */

  int armor;
  int units;        /**< Current gold carried on character */
  int bank_units;   /**< Gold the char has in a bank account	*/
  int exp;         /**< The experience points, or value, of the character. */
  int max_exp;     /* < The maximum experience value of the character.           */  

  int hitroll;   /**< Any bonus or penalty to the hit roll */
  int damroll;   /**< Any bonus or penalty to the damage roll */
  int rifts;
  long eventmobs;
  int greenshards;
  int blueshards;
  int blackshards;
};

/** char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the players file for PC's. */
struct char_special_data_saved
{
  long idnum;            /**< PC's idnum; -1 for mobiles. */
  int act[PM_ARRAY_MAX]; /**< act flags for NPC's; player flag for PC's */
  int affected_by[AF_ARRAY_MAX]; /**< Bitvector for spells/skills affected by */
  int combat_flags[CB_ARRAY_MAX];
  int apply_saving_throw[MAX_SAVING_THROW];  /**< Saving throw (Bonuses)		*/
  int bionics[40];
  struct knowledge skills[MAX_SKILLS+1];
  struct knowledge psionics[MAX_PSIONICS+1];
  struct combo_knowledge combos[MAX_COMBOLEARNED+1];
  ubyte duplicates;
  ubyte percent_machine;  
};

/** Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data
{
    struct char_data *fighting;  /**< Target of fight; else NULL */
    struct char_data *hunting;   /**< Target of NPC hunt; else NULL */
    struct char_data *funhunting;			/* < Target of Lightnign flash, else NULL */
    struct obj_data *furniture;  /**< Object being sat on/in; else NULL */
    struct char_data *next_in_furniture; /**< Next person sitting, else NULL */
	struct char_data *ranged_fighting;		/* < Active target of ranged combat; else NULL - Gahan */
	struct char_data *locked;				/* Target you've locked onto for ranged combat - Gahan */
	struct char_data *targeted;				/* Player/Mob you've currently been targetted by - Gahan */
    struct body_part_info body_part[NUM_BODY_PARTS];  /* body parts */
	struct vehicle_data vehicle;
    int bionic_percent;
	byte position; /**< Standing, fighting, sleeping, etc. */
    sbyte team;                             /* team the player belongs to           */
    sh_int carry_weight;                       /* < Carried weight                     */
	sh_int worn_weight;						/* < Weight of items worn				*/
	sh_int encumbrance;						/* < Character encumbrance				*/
    byte carry_items;                       /* < Number of items carried            */
    int timer;                              /* < Timer for update                   */
	sbyte attacks;							/* < Player Attacks per round - Gahan   */
	int tick_to_hit;						/* < When the player gets his attack off - Gahan*/
	int combat_tick;						/* amount of ticks on the character - Gahan */
	int ranged_combat_tick;					/* amount of ranged combat ticks on ch - Gahan */
	int trigger_tick;						/* amount of ticks before mob triggers fire - Gahan */
	int circle_tick;						/* New fight code defines for cirlce code - Gahan */
	int core_tick;							/* New fight code defines for power core, borgs - Gahan */
	int psibullets;
	int callerclaws;
	int insurance;							/* temp saved value of ch insurance - Gahan*/
	int tmpdir;								/* Variable used to show the direction of the targetted mob - Gahan */
	long ignoring;
	int storage;
    struct char_data *protecting;           /* Person who you are protecting        */
    struct char_data *protector;            /* Person protecting you                */ 
	struct char_special_data_saved saved; /**< Constants saved for PCs. */
    int body_part_hit_loss;
    int bionic_cha_loss;
    int bionic_psi_loss;
    int bionic_psi_regen_loss;
    int mod_Hit;
    int mod_Psi;
    int mod_HnD;
    int mod_AC;
    int mod_Regen;
    int mod_Unused;
	int totdam;
    int tothits;
	int totalkills;
	int totalrifts;
	bool invehicle;
	int combocounter[MAX_COMBOLENGTH];
};

/** Data only needed by PCs, and needs to be saved to disk. */
struct player_special_data_saved
{
  byte skills[MAX_SKILLS+1]; /**< Character skills. */
  int wimp_level;         /**< Below this # of hit points, flee! */
  byte freeze_level;      /**< Level of god who froze char, if any */
  sh_int invis_level;     /**< level of invisibility */
  room_vnum load_room;    /**< Which room to load PC into */
  room_vnum copyover_room;                /* < Which room to load PC into after copyover  */
  int pref[PR_ARRAY_MAX]; /**< preference flags */
  ubyte bad_pws;          /**< number of bad login attempts */
  sbyte conditions[3];    /**< Drunk, hunger, and thirst */
  struct txt_block *comm_hist[NUM_HIST]; /**< Communication history */
  struct player_crafting_data crafting;
  ubyte page_length;      /**< Max number of rows of text to send at once */
  ubyte screen_width;     /**< How wide the display page is */
  int spells_to_learn;    /**< Remaining number of practice sessions */
  int olc_zone;           /**< Current olc permissions */
  int questpoints;        /**< Number of quest points earned */
  qst_vnum *completed_quests;   /**< Quests completed              */
  int    num_completed_quests;  /**< Number completed              */
  int    current_quest;         /**< vnum of current quest         */
  int    quest_time;            /**< time left on current quest    */
  int    quest_counter;         /**< Count of targets left to get  */
  int attrib_add;
  time_t   lastmotd;            /**< Last time player read motd */
  time_t   lastnews;            /**< Last time player read news */
  int practices;
  long char_psi_regen;
  long char_hit_regen;
  long char_move_regen;
  int char_all_regen;
  long age_effects;
  int frequency;                          /* Radio freq.                                  */
  int fame;
  int progress;
  int tier;
  int tier2;
  int class2;
  ubyte overdose;
  ubyte addiction;
  ubyte psimastery;
};

/** Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the players file. */
struct player_special_data
{
  struct player_special_data_saved saved; /**< Information to be saved. */
  char *prompt_string;
  char *poofin;  /**< Description displayed to room on arrival of a god. */
  char *poofout; /**< Description displayed to room at a god's exit. */
  char *wiztitle;							/* < Description displayed on who screen. */   
  char *exits;                    /** PC Exits Variable for MSDP - Bosstone 2013 **/
  struct alias_data *aliases; /**< Command aliases			*/
  long last_tell;        /**< idnum of PC who last told this PC, used to reply */
  void *last_olc_targ;   /**< ? Currently Unused ? */
  int last_olc_mode;     /**< ? Currently Unused ? */
  char *host;            /**< Resolved hostname, or ip, for player. */
  int buildwalk_sector;  /**< Default sector type for buildwalk */
};

/** Special data used by NPCs, not PCs */
struct mob_special_data
{
  memory_rec *memory; /**< List of PCs to remember */
  byte attack_type;   /**< The primary attack type (bite, sting, hit, etc.) */
  byte default_pos;   /**< Default position (standing, sleeping, etc.) */
  int damnodice;     /**< The number of dice to roll for damage */
  int damsizedice;   /**< The size of each die rolled for damage. */
  int home;				/* Mobile's starting room - Gahan						*/
  int damtype;          /* damage type for mob's attack - Gahan					*/
  sh_int mobclass;			/* Mob classes - Gahan */
  sbyte num_attacks;      /* Number of attacks the mob has (0-20?)                */
};

/** An affect structure. */
struct affected_type
{
  sh_int type; /**< The spell that caused this */
  sh_int duration; /**< For how long its effects will last      */
  sbyte modifier;  /**< Added/subtracted to/from apropriate ability     */
  byte location;   /**< Tells which ability to change(APPLY_XXX). */
  int bitvector[AF_ARRAY_MAX]; /**< Tells which bits to set (AFF_XXX). */

  struct affected_type *next; /**< The next affect in the list of affects. */
};

/** The list element that makes up a list of characters following this
 * character. */
struct follow_type
{
  struct char_data *follower; /**< Character directly following. */
  struct follow_type *next;   /**< Next character following. */
};
struct title_type {
    char *title_m;
    char *title_f;
    int exp;
};

struct combo_type {
	int id;
	char *chsend;
	char *victsend;
	char *roomsend;
	int combo[5];
};

/** Master structure for PCs and NPCs. */
struct char_data
{
  int pfilepos; /**< PC playerfile pos and id number */
  mob_rnum nr;  /**< NPC real instance number */
  room_rnum in_room;     /**< Current location (real room number) */
  room_rnum was_in_room; /**< Previous location for linkdead people  */
  int wait;              /**< wait for how many loops before taking action. */

  struct char_player_data player;       /**< General PC/NPC data */
  struct char_ability_data real_abils;  /**< Abilities without modifiers */
  struct char_ability_data aff_abils;   /**< Abilities with modifiers */
  struct char_point_data points;        /**< Point/statistics */
  struct char_special_data char_specials; /**< PC/NPC specials	  */
  struct player_special_data *player_specials; /**< PC specials		  */
  struct mob_special_data mob_specials; /**< NPC specials		  */
  struct storage_record *storage;					/* storage open */
  struct affected_type *affected;        /**< affected by what spells    */
  struct obj_data *equipment[NUM_WEARS]; /**< Equipment array            */
  struct obj_data *carrying;    /**< List head for objects in inventory */
  struct descriptor_data *desc; /**< Descriptor/connection info; NPCs = NULL */
	int delay_hit_wait;
	struct delay_hit *delay_hit;
  long id; /**< used by DG triggers - unique id */
  struct trig_proto_list *proto_script; /**< list of default triggers */
  struct script_data *script;           /**< script info for the object */
  struct script_memory *memory;         /**< for mob memory triggers */

  struct char_data *next_in_room;  /**< Next PC in the room */
  struct char_data *next;          /**< Next char_data in the room */
  struct char_data *next_fighting; /**< Next in line to fight */
  struct char_data *next_ranged_fighting;			/* < Next in line to ranged fight					*/

  struct follow_type *followers; /**< List of characters following */
  struct char_data *master;      /**< List of character being followed */
  struct affected_type *drugs_used;               /* which drugs are being used                       */
  long pref; /**< unique session id */
  
  struct list_data * events;
};

/** descriptor-related structures */
struct txt_block
{
  char *text;             /**< ? */
  int aliased;            /**< ? */
  struct txt_block *next; /**< ? */
};

/** ? */
struct txt_q
{
  struct txt_block *head; /**< ? */
  struct txt_block *tail; /**< ? */
};

/** Master structure players. Holds the real players connection to the mud.
 * An analogy is the char_data is the body of the character, the descriptor_data
 * is the soul. */
struct descriptor_data
{
  socket_t descriptor;      /**< file descriptor for socket */
  char host[HOST_LENGTH+1]; /**< hostname */
  byte bad_pws;             /**< number of bad pw attemps this login */
  byte idle_tics;           /**< tics idle at password prompt		*/
  int connected;            /**< mode of 'connectedness'		*/
  int desc_num;             /**< unique num assigned to desc		*/
  time_t login_time;        /**< when the person connected		*/
  char *showstr_head;       /**< for keeping track of an internal str	*/
  char **showstr_vector;    /**< for paging through texts		*/
  int showstr_count;        /**< number of pages to page through	*/
  int showstr_page;         /**< which page are we currently showing?	*/
  char **str;               /**< for the modify-str system		*/
  char *backstr;            /**< backup string for modify-str system	*/
  size_t max_str;           /**< maximum size of string in modify-str	*/
  long mail_to;             /**< name for mail system			*/
  int has_prompt;           /**< is the user at a prompt?             */
  char inbuf[MAX_RAW_INPUT_LENGTH];  /**< buffer for raw input		*/
  char last_input[MAX_INPUT_LENGTH]; /**< the last input			*/
  char small_outbuf[SMALL_BUFSIZE];  /**< standard output buffer		*/
  char *output;             /**< ptr to the current output buffer	*/
  char **history;           /**< History of commands, for ! mostly.	*/
  int history_pos;          /**< Circular array position.		*/
  int bufptr;               /**< ptr to end of current output		*/
  int bufspace;             /**< space left in the output buffer	*/
  struct txt_block *large_outbuf; /**< ptr to large buffer, if we need it */
  struct txt_q input;       /**< q of unprocessed input		*/
  struct char_data *character; /**< linked to char			*/
  struct char_data *original;  /**< original char if switched		*/
  struct descriptor_data *snooping; /**< Who is this char snooping	*/
  struct descriptor_data *snoop_by; /**< And who is snooping this char	*/
  struct descriptor_data *next;     /**< link to next descriptor		*/
  struct oasis_olc_data *olc;       /**< OLC info */
  struct custom_editor_data *custom; /**< Custom Equipment Editor - Gahan */
  protocol_t *pProtocol;    /**< Kavir plugin */
  int dead;
  
  struct list_data * events;
};

/* other miscellaneous structures */
/** Fight message display. This structure is used to hold the information to
 * be displayed for every different violent hit type. */
struct msg_type
{
  char *attacker_msg; /**< Message displayed to attecker. */
  char *victim_msg;   /**< Message displayed to victim. */
  char *room_msg;     /**< Message displayed to rest of players in room. */
};

/** An entire message structure for a type of hit or spell or skill. */
struct message_type
{
  struct msg_type die_msg;   /**< Messages for death strikes. */
  struct msg_type miss_msg;  /**< Messages for missed strikes. */
  struct msg_type hit_msg;   /**< Messages for a succesful strike. */
  struct msg_type god_msg;   /**< Messages when trying to hit a god. */
  struct message_type *next; /**< Next set of messages. */
};

/** Head of list of messages for an attack type. */
struct message_list
{
  int a_type;               /**< The id of this attack type. */
  int number_of_attacks;    /**< How many attack messages to chose from. */
  struct message_type *msg; /**< List of messages.			*/
};

/** Social message data structure. */
struct social_messg
{
  int act_nr;    /**< The social id. */
  char *command; /**< The command to activate (smile, wave, etc.) */
  char *sort_as; /**< Priority of social sorted by this. */
  int hide;      /**< If true, and target can't see actor, target doesn't see */
  int min_victim_position; /**< Required Position of victim */
  int min_char_position;   /**< Required Position of char */
  int min_level_char;      /**< Minimum PC level required to use this social. */

  /* No argument was supplied */
  char *char_no_arg;   /**< Displayed to char when no argument is supplied */
  char *others_no_arg; /**< Displayed to others when no arg is supplied */

  /* An argument was there, and a victim was found */
  char *char_found;   /**< Display to char when arg is supplied */
  char *others_found; /**< Display to others when arg is supplied */
  char *vict_found;   /**< Display to target arg */

  /* An argument was there, as well as a body part, and a victim was found */
  char *char_body_found;   /**< Display to actor */
  char *others_body_found; /**< Display to others */
  char *vict_body_found;   /**< Display to target argument */

  /* An argument was there, but no victim was found */
  char *not_found;         /**< Display when no victim is found */

  /* The victim turned out to be the character */
  char *char_auto;   /**< Display when self is supplied */
  char *others_auto; /**< Display to others when self is supplied */

  /* If the char cant be found search the char's inven and do these: */
  char *char_obj_found;   /**< Social performed on object, display to char */
  char *others_obj_found; /**< Social performed on object, display to others */
};

/** Describes bonuses, or negatives, applied to thieves skills. In practice
 * this list is tied to the character's dexterity attribute. */
struct dex_skill_type
{
  sh_int p_pocket; /**< Alters the success rate of pick pockets */
  sh_int p_locks;  /**< Alters the success of pick locks */
  sh_int traps;    /**< Historically alters the success of trap finding. */
  sh_int sneak;    /**< Alters the success of sneaking without being detected */
  sh_int hide;     /**< Alters the success of hiding out of sight */
};

/** Describes the bonuses applied for a specific value of a character's
 * strength attribute. */
struct dex_app_type
{
  sh_int reaction; /**< Historically affects reaction savings throws. */
  sh_int miss_att; /**< Historically affects missile attacks */
  sh_int defensive; /**< Alters character's inherent armor class */
};

/** Describes the bonuses applied for a specific value of a character's
 * strength attribute. */
struct str_app_type
{
  sh_int tohit; /**< To Hit (THAC0) Bonus/Penalty        */
  sh_int todam; /**< Damage Bonus/Penalty                */
  sh_int carry_w; /**< Maximum weight that can be carrried */
  sh_int wield_w; /**< Maximum weight that can be wielded  */
};

/** Describes the bonuses applied for a specific value of a character's
 * wisdom attribute. */
struct pcn_app_type
{
  byte bonus; /**< how many practices player gains per lev */
};

/** Describes the bonuses applied for a specific value of a character's
 * intelligence attribute. */
struct int_app_type
{
  byte learn; /**< how many % a player learns a spell/skill */
};

/** Describes the bonuses applied for a specific value of a
 * character's constitution attribute. */
struct con_app_type
{
  sh_int hitp; /**< Added to a character's new MAXHP at each new level. */
  sh_int shock;
};

struct skill_tree_type {
	const char *display;
	const char *skill_name;
	int tree;
	int practice_cost;
	int tier;
	int	pcclass;
	int psi_or_skill;
	int skill_level;
	int point_cost;
	int skillnum;
    int minpos;
    int targets;
    int violent;
    int routines;
    const char *wearoff;
	const char *rwearoff;
	int prereq;
	int prereqid;
};

struct progression_info_type {
	int questpoints;
	int fame;
	int level;
};

struct class_display_type {
	char *class_name;
	char *dualclass_name;
};

struct exp_cap_type {
	int exp_cap;
};
/** Stores, and used to deliver, the current weather information
 * in the mud world. */
struct weather_data
{
  int pressure; /**< How is the pressure ( Mb )? */
  int change; /**< How fast and what way does it change? */
  int sky; /**< How is the sky? */
  int sunlight; /**< And how much sun? */
};

/** Element in monster and object index-tables.
 NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum) */
struct index_data
{
  mob_vnum vnum; /**< virtual number of this mob/obj   */
  int number; /**< number of existing units of this mob/obj  */
  /** Point to any SPECIAL function assoicated with mob/obj.
   * Note: These are not trigger scripts. They are functions hard coded in
   * the source code. */
  SPECIAL(*func);

  char *farg; /**< String argument for special function. */
  struct trig_data *proto; /**< Points to the trigger prototype. */
};

/** Master linked list for the mob/object prototype trigger lists. */
struct trig_proto_list
{
  int vnum; /**< vnum of the trigger   */
  struct trig_proto_list *next; /**< next trigger          */
};

struct guild_info_type
{
  int pc_class;
  room_vnum guild_room;
  int direction;
};

/** Happy Hour Data */
struct happyhour {
  int qp_rate;
  int exp_rate;
  int gold_rate;
  int ticks_left;
};

/** structure for list of recent players (see 'recent' command) */
struct recent_player
{
   int    vnum;                   /* The ID number for this instance */
   char   name[MAX_NAME_LENGTH];  /* The char name of the player     */
   bool   new_player;             /* Is this a new player?           */
   bool   copyover_player;        /* Is this a player that was on during the last copyover? */
   time_t time;                   /* login time                      */
   char   host[HOST_LENGTH+1];    /* Host IP address                 */
   struct recent_player *next;    /* Pointer to the next instance    */
};

/* Config structs */

/** The game configuration structure used for configurating the game play
 * variables. */
struct game_data
{
  int pk_allowed; /**< Is player killing allowed?    */
  int pt_allowed; /**< Is player thieving allowed?   */
  int level_can_shout; /**< Level player must be to shout.   */
  int holler_move_cost; /**< Cost to holler in move points.    */
  int tunnel_size; /**< Number of people allowed in a tunnel.*/
  int max_exp_gain; /**< Maximum experience gainable per kill.*/
  int max_exp_loss; /**< Maximum experience losable per death.*/
  int max_npc_corpse_time; /**< Num tics before NPC corpses decompose*/
  int max_pc_corpse_time; /**< Num tics before PC corpse decomposes.*/
  int idle_void; /**< Num tics before PC sent to void(idle)*/
  int idle_rent_time; /**< Num tics before PC is autorented.   */
  int idle_max_level; /**< Level of players immune to idle.     */
  int dts_are_dumps; /**< Should items in dt's be junked?   */
  int load_into_inventory; /**< Objects load in immortals inventory. */
  int track_through_doors; /**< Track through doors while closed?    */
  int no_mort_to_immort; /**< Prevent mortals leveling to imms?    */
  int disp_closed_doors; /**< Display closed doors in autoexit?    */
  int diagonal_dirs; /**< Are there 6 or 10 directions? */
  int map_option;         /**< MAP_ON, MAP_OFF or MAP_IMM_ONLY      */
  int map_size;           /**< Default size for map command         */
  int minimap_size;       /**< Default size for mini-map (automap)  */
  int script_players;     /**< Is attaching scripts to players allowed? */

  char *OK; /**< When player receives 'Okay.' text.    */
  char *NOPERSON; /**< 'No one by that name here.'   */
  char *NOEFFECT; /**< 'Nothing seems to happen.'            */
};

/** The rent and crashsave options. */
struct crash_save_data
{
  int free_rent; /**< Should the MUD allow rent for free?   */
  int max_obj_save; /**< Max items players can rent.           */
  int min_rent_cost; /**< surcharge on top of item costs.       */
  int auto_save; /**< Does the game automatically save ppl? */
  int autosave_time; /**< if auto_save=TRUE, how often?         */
  int crash_file_timeout; /**< Life of crashfiles and idlesaves.     */
  int rent_file_timeout; /**< Lifetime of normal rent files in days */
};

/** Important room numbers. This structure stores vnums, not real array
 * numbers. */
struct room_numbers
{
  room_vnum mortal_start_room; /**< vnum of room that mortals enter at.  */
  room_vnum immort_start_room; /**< vnum of room that immorts enter at.  */
  room_vnum frozen_start_room; /**< vnum of room that frozen ppl enter.  */
  room_vnum donation_room_1; /**< vnum of donation room #1.            */
  room_vnum donation_room_2; /**< vnum of donation room #2.            */
  room_vnum donation_room_3; /**< vnum of donation room #3.            */
};

/** Operational game variables. */
struct game_operation
{
  ush_int DFLT_PORT; /**< The default port to run the game.  */
  char *DFLT_IP; /**< Bind to all interfaces.     */
  char *DFLT_DIR; /**< The default directory (lib).    */
  char *LOGNAME; /**< The file to log messages to.    */
  int max_playing; /**< Maximum number of players allowed. */
  int max_filesize; /**< Maximum size of misc files.   */
  int max_bad_pws; /**< Maximum number of pword attempts.  */
  int siteok_everyone; /**< Everyone from all sites are SITEOK.*/
  int nameserver_is_slow; /**< Is the nameserver slow or fast?   */
  int use_new_socials; /**< Use new or old socials file ?      */
  int auto_save_olc; /**< Does OLC save to disk right away ? */
  char *MENU; /**< The MAIN MENU.        */
  char *WELC_MESSG; /**< The welcome message.      */
  char *START_MESSG; /**< The start msg for new characters.  */
  int medit_advanced; /**< Does the medit OLC show the advanced stats menu ? */
  int ibt_autosave; /**< Does "bug resolve" autosave ? */
};

/** The Autowizard options. */
struct autowiz_data
{
  int use_autowiz; /**< Use the autowiz feature?   */
  int min_wizlist_lev; /**< Minimun level to show on wizlist.  */
};

struct delay_hit
{
	struct char_data *ch;
	char *victim; 
	struct msg_type hit_msg;
	struct msg_type af_msg;
	struct msg_type echo_msg;
	int psi_num;
	int psi_level;
	bool psi_ack;
	int dam_type;
	int dam;
};

// CyberASSAULT Port Structs

#define AUCTION_LENGTH	6
typedef struct auction_data AUCTION_DATA;
typedef struct auction_data auction_data;
struct auction_data {
	struct obj_data *item;
	struct char_data *owner;
	struct char_data *high_bidder;

	sh_int status;
	int current_bid;
	int units_held;
	int minimum_bid;
};

extern AUCTION_DATA auction_info;

/**
 Main Game Configuration Structure.
 Global variables that can be changed within the game are held within this
 structure. During gameplay, elements within this structure can be altered,
 thus affecting the gameplay immediately, and avoiding the need to recompile
 the code.
 If changes are made to values of the elements of this structure during game
 play, the information will be saved to disk.
 */
struct config_data
{
  /** Path to on-disk file where the config_data structure gets written. */
  char *CONFFILE;
  /** In-game specific global settings, such as allowing player killing. */
  struct game_data play;
  /** How is renting, crash files, and object saving handled? */
  struct crash_save_data csd;
  /** Special designated rooms, like start rooms, and donation rooms. */
  struct room_numbers room_nums;
  /** Basic operational settings, like max file sizes and max players. */
  struct game_operation operation;
  /** Autowiz specific settings, like turning it on and minimum level */
  struct autowiz_data autowiz;
};

#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

#endif /* _STRUCTS_H_ */
