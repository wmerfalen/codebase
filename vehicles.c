//////////////////////////////////////////////////////////////////////////////////
// This vehicles.c file ha been written from scratch by Gahan.					//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the player vehicle system for CyberASSAULT							//
// Anything that has to do with it will be found here or within act.movement.c	//
//////////////////////////////////////////////////////////////////////////////////

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "ban.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "act.h"
#include "genzon.h"
#include "genwld.h"
#include "genolc.h"
#include "config.h"
#include "custom.h"
#include "graph.h"

#define MIN_VEHICLE_ROOM 	2710
#define MAX_VEHICLE_ROOM 	2799
#define LOT_VNUM			3092

static void drive_vehicle(struct char_data *ch, int dir, bool autopilot);
static int vehicle_handle_objs(struct obj_data *obj);
static void vehicle_death(struct char_data *ch, struct obj_data *vehicle);
FILE *popen( const char *command, const char *type );
int pclose( FILE *stream );
char *fgetf( char *s, int n, register FILE *iop );
void save_vehicles(void) {
    FILE *fl;
    char filename[MAX_STRING_LENGTH];
	struct obj_data *obj;
	struct obj_data *next_obj;
	int result;

    snprintf(filename, sizeof(filename), LIB_VEHICLE);

    if (!(fl = fopen(filename, "w"))) {
        log("SYSERR: VEHICLE FILE FAILED TO SAVE.\r\n"); 
        return;
    }
	
    for (obj = object_list; obj; obj = next_obj) {
        next_obj = obj->next;
		if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE)
			result = objsave_save_obj_record(obj, fl, 0);  // 0: this object is not worn
	}

    fclose(fl);
    return;	
}

void Vehicle_save(void) {
	struct char_data *ch;
	struct char_data *next_ch;
	// check to see if they're in a vehicle, if so, dump them out.
	// We will have automatic vehicle saving using another function
	for (ch = character_list; ch; ch = next_ch) {
		next_ch = ch->next;
		if (IS_MOB(ch))
			continue;
		if (IN_VEHICLE(ch)) {
			ch->char_specials.invehicle = FALSE;
			char_from_room(ch);
			char_to_room(ch, real_room(LOT_VNUM));
		}
	}
	save_vehicles();
}

static int vehicle_handle_objs(struct obj_data *obj)
{
	obj_to_room(obj, real_room(LOT_VNUM));
	obj->fuel = GET_OBJ_VAL(obj, 3);
	obj->damage = 0;
	obj->armor = GET_OBJ_VAL(obj, 4);
	return TRUE;
}

void Vehicle_load(void) {
  FILE *fl;
  char filename[MAX_STRING_LENGTH];
  obj_save_data *loaded, *current;
  int num_objs;

  snprintf(filename, sizeof(filename), LIB_VEHICLE);
  if (!(fl = fopen(filename, "r")))	/* no file found */
    return;
  //for (i = 0; i < MAX_BAG_ROWS; i++)
  //	  cont_row[i] = NULL;
  loaded = objsave_parse_objects(fl);

  for (current = loaded; current != NULL; current=current->next)
    num_objs += vehicle_handle_objs(current->obj);

  while (loaded != NULL) {
	current = loaded;
	loaded = loaded->next;
	free(current);
  }

  fclose(fl);

  return;
}

void enter_vehicle(struct char_data *ch, struct obj_data *vehicle) {
	int i;
	int room_num = 0;
	struct room_data *source_room;
	struct room_data *target_room;
	char buf[MAX_INPUT_LENGTH];
	char buf2[MAX_INPUT_LENGTH];
	bool found = FALSE;
	bool success = TRUE;
	
    int vehicle_rooms[] = {
	2700,
	2701,
	2702,
	2703,
	2704,
	2705,
	2706,
	2707,
	2708,
	2709
    };
	
	if (!ch || !vehicle)
		return;
		
	// first, save all information regarding the vehicle object and room that they were in.
	VEHICLE(ch) = vehicle;	
	
	// lets figure out if the player already has a vehicle room
	if (vehicle->bound_id == 0) {
		vehicle->bound_id = GET_IDNUM(ch);
		vehicle->bound_name = GET_NAME(ch);
		vehicle->fuel = GET_OBJ_VAL(vehicle, 3);
	}
			
	if (vehicle->bound_id != GET_IDNUM(ch)) {
		send_to_char(ch, "That is not your vehicle.\r\n");
		return;
	}
	
	// first, we restring the vehicle
	sprintf(buf, "%s belonging to %s is here.", vehicle->short_description, GET_NAME(ch));
	sprintf(buf2, "%s %s", vehicle->name, GET_NAME(ch));
	success = restring_name(ch, vehicle, buf2);
	success = oset_long_description(vehicle, buf);
	// second, figure out what vnums are available.
	for (i = MIN_VEHICLE_ROOM; i <= MAX_VEHICLE_ROOM; i++)
		if (!(ROOM_FLAGGED(real_room(i), ROOM_OCCUPIED))) {
			room_num = i;
			found = TRUE;
			break;
		}
	// if none are available, lets just give them an error message for now to keep it stable
	if (found == FALSE) {
		send_to_char(ch, "[ERROR 2]: You've failed to enter your vehicle.  Report this error code to Gahan.\r\n");
		return;
	}
	// okay, so whats the first room available? Did we find the correct zone?
	if (room_num > MAX_VEHICLE_ROOM || room_num < MIN_VEHICLE_ROOM) {
		send_to_char(ch, "[ERROR 3]: You've failed to enter your vehicle.  Report this error code to Gahan.\r\n");
		return;
	}	
	// lets get the desriptions set
	VEHICLE_VNUM(ch) = room_num;
	WINDOW_VNUM(ch) = IN_ROOM(ch);
	ch->char_specials.invehicle = TRUE;
	source_room = &world[real_room(vehicle_rooms[GET_OBJ_VAL(vehicle, 0)])];
	target_room = &world[real_room(room_num)];
	copy_room_strings(target_room, source_room);
	// GET_OBJ_VAL_0(vehicle) to get descriptions
	// lets flag the room occupied so no one else can get in...
	SET_BIT_AR(ROOM_FLAGS(real_room(room_num)), ROOM_OCCUPIED);
	// keep the vehicle on the ground
	// transfer the player to the room.
	char_from_room(ch);
	char_to_room(ch, real_room(room_num));
	send_to_char(ch, "You hop into a %s.\r\n", VEHICLE_NAME(ch));
	look_at_room(ch, real_room(room_num));
	send_to_room(WINDOW_VNUM(ch), "%s jumps into a %s.\r\n", GET_NAME(ch), VEHICLE_NAME(ch));
}

void vehicle_look(struct char_data *ch) {
	send_to_char(ch, "@C%s@n\r\n", world[IN_ROOM(ch)].name);
	if (!PRF_FLAGGED(ch, PRF_BRIEF))
		send_to_char(ch, "%s", world[IN_ROOM(ch)].description);
	list_obj_to_char(world[IN_ROOM(ch)].contents, ch, 0, FALSE);
	list_char_to_char(world[real_room(VEHICLE_VNUM(ch))].people, ch);
	send_to_char(ch, "@YLooking outside the vehicle you can see...\r\n\r\n");
	send_to_char(ch, "@R%s@n\r\n", world[WINDOW_VNUM(ch)].name);
	if (!PRF_FLAGGED(ch, PRF_BRIEF))
		send_to_char(ch, "%s", world[WINDOW_VNUM(ch)].description);
	list_char_to_char(world[WINDOW_VNUM(ch)].people, ch);
	do_auto_exits(ch);
}

int vehicle_terrain_check(struct obj_data *vehicle, int going_to)
{
	if (!vehicle)
		return (0);

	if (GET_OBJ_VAL(vehicle, 1) == VTYPE_ROADSTER) {
		if (SECT(going_to) == 15 ||
			SECT(going_to) == 16 ||
			SECT(going_to) == 1 ||
			SECT(going_to) == 2 ||
			SECT(going_to) == 3 ||
			SECT(going_to) == 4 ||
			SECT(going_to) == 12 ||
			SECT(going_to) == 11)
			return (1);
		else
			return (0);
	}

	else if (GET_OBJ_VAL(vehicle, 1) == VTYPE_OFFROAD) {
		if (SECT(going_to) == 15 ||
			SECT(going_to) == 16 ||
			SECT(going_to) == 1 ||
			SECT(going_to) == 2 ||
			SECT(going_to) == 3 ||
			SECT(going_to) == 4 ||
			SECT(going_to) == 5 ||
			SECT(going_to) == 6 ||
			SECT(going_to) == 11 ||
			SECT(going_to) == 12)
			return (1);
		else
			return (0);
	}

	else if (GET_OBJ_VAL(vehicle, 1) == VTYPE_HOVERFLY) {
		if (SECT(going_to) == 2 ||
			SECT(going_to) == 1 ||
			SECT(going_to) == 16 ||
			SECT(going_to) == 3 ||
			SECT(going_to) == 4 ||
			SECT(going_to) == 5 ||
			SECT(going_to) == 6 ||
			SECT(going_to) == 7 ||
			SECT(going_to) == 8 ||
			SECT(going_to) == 9 ||
			SECT(going_to) == 10 ||
			SECT(going_to) == 11 ||
			SECT(going_to) == 12)
			return (1);
		else
			return (0);
		}

	else if (GET_OBJ_VAL(vehicle, 1) == VTYPE_BOAT) {
		if (SECT(going_to) == 7 ||
			SECT(going_to) == 16 ||
			SECT(going_to) == 8 ||
			SECT(going_to) == 12 ||
			SECT(going_to) == 6)
			return (1);
		else
			return (0);
	}

	else if (GET_OBJ_VAL(vehicle, 1) == VTYPE_SUBMARINE) {
		if (SECT(going_to) == 7 ||
			SECT(going_to) == 16 ||
			SECT(going_to) == 8 ||
			SECT(going_to) == 9 ||
			SECT(going_to) == 12 ||
			SECT(going_to) == 6)
			return (1);
		else
			return (0);
	}

	else if (GET_OBJ_VAL(vehicle, 1) == VTYPE_LAV) {
		if (SECT(going_to) == 15 ||
			SECT(going_to) == 16 ||
			SECT(going_to) == 1 ||
			SECT(going_to) == 2 ||
			SECT(going_to) == 3 ||
			SECT(going_to) == 4 ||
			SECT(going_to) == 5 ||
			SECT(going_to) == 6 ||
			SECT(going_to) == 7 ||
			SECT(going_to) == 8 ||
			SECT(going_to) == 9 ||
			SECT(going_to) == 10 ||
			SECT(going_to) == 11 ||
			SECT(going_to) == 12)
			return (1);
		else
			return (0);
	}
	else
		return (0);
}

static void vehicle_death(struct char_data *ch, struct obj_data *vehicle)
{
	int going_to;

	send_to_room(WINDOW_VNUM(ch), "@Y%s is ejected from %s, landing awkwardly on their back!@n\r\n", GET_NAME(ch), VEHICLE_NAME(ch));
	send_to_room(WINDOW_VNUM(ch), "@Y%s explodes violently as the fuel compartment ignites!!@n\r\n", VEHICLE_NAME(ch));
	send_to_char(ch, "\r\n@YYou are ejected from your vehicle, landing awkwardly on your back!@n\r\n");
	send_to_char(ch, "@YYour vehicle explodes violently as its fuel compartment ignites!!@n\r\n\r\n");
	extract_obj(vehicle);
	going_to = WINDOW_VNUM(ch);
	ch->char_specials.invehicle = FALSE;
	ch->char_specials.vehicle.vehicle_room_vnum = 0;
	char_from_room(ch);
	char_to_room(ch, WINDOW_VNUM(ch));
	look_at_room(ch, IN_ROOM(ch));
	entry_memory_mtrigger(ch);
	greet_mtrigger(ch, -1);
	greet_memory_mtrigger(ch);
	if (!IS_NPC(ch) && ROOM_FLAGGED(going_to, ROOM_DEATH)) {
		mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(going_to), world[going_to].name);

		send_to_char(ch, "ZAP!! You have just hit a Death Trap.\r\n"
						 "The smell of your own flesh kind of reminds you of overcooked sausages you used to make in college.\r\n"
						 "Luckily for you, this doesn't count as a death, nor cost you any of your money or valuables.\r\n"
						 "Punishments for Death Traps of this sort force you to Motown armory, and lags you for a slightly\r\n"
						 "rediculous amount of time.  Be careful though, not all Death Traps save you from real death,\r\n"
						 "and cause you to pay for it.  In other words, look before you leap!\r\n");

       char_from_room(ch);
       char_to_room(ch, real_room(3001));
       GET_WAIT_STATE(ch) = 20 * PULSE_VIOLENCE;
	}
}

static void drive_vehicle(struct char_data *ch, int dir, bool autopilot)
{
	struct obj_data *vehicle;
	char leave_message[MAX_INPUT_LENGTH];
	char arrive_message[MAX_INPUT_LENGTH];
	vehicle = VEHICLE(ch);
	
	if (!vehicle) {
		send_to_char(ch, "Your car has disappeared, notify Gahan immediately.\r\n");
		return;
	}
	
	if (dir >= 0) {
		if (!VEHICLE_CAN_GO(ch, dir)) {
			if (autopilot == FALSE) {
				send_to_char(ch, "Your vehicle cannot go that way.\r\n");
				return;
			}
			if (autopilot == TRUE) {
				send_to_char(ch, "The GPS notifies you of an exit that is blocked.  Your vehicle slows down to a stop.\r\n");
				REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AUTOPILOT);
				return;
			}
		}
		
		if (vehicle->fuel < 1) {
			send_to_char(ch, "Your vehicle is out of fuel, you will need to refuel before you can continue.\r\n");
			if (autopilot == TRUE)
				REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AUTOPILOT);
			return;
		}
		
		snprintf(leave_message, sizeof(leave_message), "%s leaves %s.", vehicle->short_description, dirs[dir]);
		send_to_room(WINDOW_VNUM(ch), "%s", leave_message);
		room_rnum going_to = VEHICLE_EXIT(ch, dir)->to_room;
		if (!(vehicle_terrain_check(vehicle, going_to))) {
			if (autopilot == FALSE) {
				send_to_char(ch, "Your vehicle doesn't have the ability to move across that type of terrain.\r\n");
				return;
			}
		}
		WINDOW_VNUM(ch) = going_to;
		obj_from_room(vehicle);
		obj_to_room(vehicle, WINDOW_VNUM(ch));

		vehicle->fuel -= 1;
        snprintf(arrive_message, sizeof(arrive_message), "%s arrives from %s%s.", 
			vehicle->short_description,
            (dir < UP ? "the " : ""),
            (dir == UP ? "below": dir == DOWN ? "above" : dirs[rev_dir[dir]]));
        send_to_room(WINDOW_VNUM(ch), "%s", arrive_message);
		
		if (autopilot == FALSE)
			vehicle_look(ch);
		else if (autopilot == TRUE && !PRF_FLAGGED(ch, PRF_BLIND))
			vehicle_look(ch);

		if (ROOM_FLAGGED(going_to, ROOM_DEATH) || SECT(going_to) == 12) {
			vehicle->damage += 50;
			if ((GET_OBJ_VAL(VEHICLE(ch), 2) - VEHICLE(ch)->damage) <= 0)
				vehicle_death(ch, vehicle);
			return;
		}
	}
	
	else {
		send_to_char(ch, "You drive your vehicle right into a brick wall. Damnit I think it left a dent too!\r\n");
		return;
	}
}

void autopilot_move(struct char_data *ch) {
	int dir;
	
	if ((dir = find_first_step(WINDOW_VNUM(ch), real_room(AUTOPILOT_ROOM(ch)), ch, TRUE)) < 0) {
		send_to_char(ch, "Your GPS cannot determine a logical path to your destination from here.\r\n");
		REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AUTOPILOT);
	}
	else {
		drive_vehicle(ch, dir, TRUE);
	}
}

ACMD(do_stop) {
	if (!IN_VEHICLE(ch))  {
		send_to_char(ch, "You need to be inside of a vehicle before you can stop it.\r\n");
		return;
	}
	else if (AFF_FLAGGED(ch, AFF_AUTOPILOT)) {
		send_to_char(ch, "Your press the abort button on your GPS and the vehicle slows to a halt.\r\n");
		REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AUTOPILOT);
		return;
	}
	else
		send_to_char(ch, "Your vehicle is not currently in auto-pilot mode.\r\n");
}

ACMD(do_drive) {
	char arg1[MAX_INPUT_LENGTH];
	int dir = 0;
	
	one_argument(argument, arg1);
	
	if (!IN_VEHICLE(ch))  {
		send_to_char(ch, "You need to be inside of a vehicle before you can drive it.\r\n");
		return;
	}
	if (!strcmp(arg1, "nw"))
		dir = 6;
	else if (!strcmp(arg1, "ne"))
		dir = 7;
	else if (!strcmp(arg1, "se"))
		dir = 8;
	else if (!strcmp(arg1, "sw"))
		dir = 9;
	else
		dir = search_block(arg1, dirs, FALSE);
	drive_vehicle(ch, dir, FALSE);
}

ACMD(do_refuel) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
    struct char_data *tmp_char;
	struct obj_data *vehicle, *fuel;
	int arg1dotmode, arg2dotmode;

	two_arguments(argument, arg1, arg2);

	arg1dotmode = find_all_dots(arg1);
	arg2dotmode = find_all_dots(arg2);

	if (!*arg1) {
		send_to_char(ch, "What vehicle would you like to refuel?\r\n");
		send_to_char(ch, "Type: refuel <vehicle> <fuel>\r\n");
		return;
	}
	else {
		generic_find(arg1, FIND_OBJ_ROOM, ch, &tmp_char, &vehicle);

		if (!vehicle) {
			send_to_char(ch, "You don't see that vehicle here.\r\n");
			return;
		}
		else if (GET_OBJ_TYPE(vehicle) != ITEM_VEHICLE) {
			send_to_char(ch, "You can only refuel vehicles.\r\n");
			return;
		}
		else if (!*arg2) {
			send_to_char(ch, "What fuel would you like to use to reful that vehicle?\r\n");
			return;
		}
		else {
			if (!(fuel = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
				send_to_char(ch, "You don't seem to be carrying that type of fuel.\r\n");
				return;
			}
			else if (!IS_AMMO(fuel) || GET_AMMO_TYPE(fuel) != AMMO_FUEL) {
				send_to_char(ch, "You cannot refuel a vehicle with that!\r\n");
				return;
			}
			else {
				if (vehicle->fuel >= GET_OBJ_VAL(vehicle, 3)) {
					send_to_char(ch, "That vehice is already full of fuel.\r\n");
					return;
				}
				int fueladd = GET_OBJ_VAL(fuel, 1);
				if ((vehicle->fuel + fueladd) > GET_OBJ_VAL(vehicle, 3))
					vehicle->fuel = GET_OBJ_VAL(vehicle, 3);
				else
					vehicle->fuel += fueladd;
				send_to_char(ch, "You refuel %s with %s, the fuel gage now reads: %d / %d.\r\n", vehicle->short_description, fuel->short_description, vehicle->fuel, GET_OBJ_VAL(vehicle, 3));
				extract_obj(fuel);
			}
		}
	}
}

ACMD(do_repair) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct char_data *tch;
	struct obj_data *vehicle;
	int costs;
	bool found = FALSE;
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1) {
		send_to_char(ch, "Syntax:\r\n");
		send_to_char(ch, "repair <vehicle>           - to assess costs\r\n");
		send_to_char(ch, "repair <vehicle> confirm   - to confirm and repair\r\n");
		return;
	}
	
	for (tch = world[IN_ROOM(ch)].people; tch;tch = tch->next_in_room)
		if (IS_NPC(tch) && MOB_FLAGGED(tch, MOB_MECHANIC))
			found = TRUE;

	if (found == FALSE) {
		send_to_char(ch, "You don't see anyone here that can repair a vehicle for you.\r\n");
		return;
	}
	
	else {
		vehicle = get_obj_in_list_vis(ch, arg1, NULL, world[ch->in_room].contents);
		if (!vehicle) {
			send_to_char(ch, "You don't see anything by that name to repair.\r\n");
			return;
		}
		else {
			if (vehicle->damage > 0) {
				costs = (GET_OBJ_VAL(vehicle, 2) - vehicle->damage) * 200000;

				if (!*arg2) {
					send_to_char(ch, "To repair that vehicle it will cost you %d resource units.\r\n", costs);
					send_to_char(ch, "Please type repair <vehicle> confirm to confirm and repair.\r\n");
					return;
				}
				if (!strcmp(arg2, "confirm")) {
					if (GET_UNITS(ch) >= costs) {
						send_to_char(ch, "You spend %d resource units and repair your vehicle.\r\n", costs);
						vehicle->damage = 0;
						GET_UNITS(ch) -= costs;
						return;
					}
					else {
						send_to_char(ch, "You cannot afford to repair that vehicle for %d resource units.\r\n", costs);
						return;
					}
				}
				else {
					send_to_char(ch, "Please type repair <vehicle> confirm to confirm and repair your vehicle.\r\n");
					return;
				}
			}
			else {
				send_to_char(ch, "That vehicle is in pristine condition, and does not need any repairs.\r\n");
				return;
			}
		}
	}
}

ACMD(do_gps) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct obj_data *vehicle;
	bool found = FALSE;
	int i;
	int num;
	
	two_arguments(argument, arg1, arg2);
	vehicle = VEHICLE(ch);
	
	if (!IN_VEHICLE(ch)) {
		send_to_char(ch, "You need to be inside of a vehicle to use its on board navigation system.\r\n");
		return;
	}

	if (!*arg1) {
		send_to_char(ch, "The onboard navigation system of a %s reads:\r\n\r\n", VEHICLE_NAME(ch));
		for (i = 0; i < GET_OBJ_VAL(vehicle, 5); i++)
			if (vehicle->gps[i] != 0) {
				send_to_char(ch, "%2d)  %s \r\n", i+1, world[real_room(vehicle->gps[i])].name);
				found = TRUE;
			}
			else
				send_to_char(ch, "%2d)  Empty.\r\n", i+1);
		return;
	}
	
	if (!strcmp(arg1, "program")) {
		if (!*arg2) {
			send_to_char(ch, "Which slot would you like to set for this location? (1 - %d)\r\n",GET_OBJ_VAL(vehicle, 5));
			return;
		}
		if (is_number(arg2)) {
			num = atoi(arg2);
			if (num < 1 || num > GET_OBJ_VAL(vehicle, 5)) {
				send_to_char(ch, "You must select a programming slot between 1 and %d\r\n", GET_OBJ_VAL(vehicle, 5));
				return;
			}
			else {
				send_to_char(ch, "Slot %d programmed to this location.\r\n", num);
				vehicle->gps[num-1] = GET_ROOM_VNUM(WINDOW_VNUM(ch));
				return;
			}
		}
		else {
			send_to_char(ch, "You must select a programming slot between 1 and %d\r\n", GET_OBJ_VAL(vehicle, 5));
			return;
		}
	}
	
	else if (!strcmp(arg1, "destination")) {
		if (!*arg2) {
			send_to_char(ch, "You need to specify which GPS location to set your destination to. (1 - %d)\r\n",GET_OBJ_VAL(vehicle, 5));
			return;
		}
		if (is_number(arg2)) {
			num = atoi(arg2);
			if (num < 1 || num > GET_OBJ_VAL(vehicle, 5)) {
				send_to_char(ch, "You must specify a valid GPS location to set your destination to. (1 - %d)\r\n",GET_OBJ_VAL(vehicle, 5));
				return;
			}
			else {
				if (vehicle->gps[num-1] == 0) {
					send_to_char(ch, "That GPS location has not been programmed yet, please select another.\r\n");
					return;
				}
				else {
					SET_BIT_AR(AFF_FLAGS(ch), AFF_AUTOPILOT);
					AUTOPILOT_ROOM(ch) = vehicle->gps[num-1];
					send_to_char(ch, "You hear the voice of the GPS operator say 'Confirmed' as the engine begins to rev up.\r\n");
					return;
				}
			}
		}
	}
	else {
		send_to_char(ch, " Syntax:\r\n");
		send_to_char(ch, " gps            - list all gps locations in your vehicle\r\n");
		send_to_char(ch, " gps program <number>     - assign a gps location to the room you're currently in.\r\n");
		send_to_char(ch, " gps destination <number> - Autopilot to desired destination\r\n");
	}
}

#define RESTOREPATH "/home/cyber/backups/home/cyber/CA.conversion/lib/vehicles"
ACMD(do_vrestore) {
  FILE *fl;
  char filename[MAX_STRING_LENGTH];
  obj_save_data *loaded, *current;
  int num_objs;

  snprintf(filename, sizeof(filename), RESTOREPATH);
  if (!(fl = fopen(filename, "r"))) {	/* no file found */
	  send_to_char(ch, "Vehicle restore failed.  Please extract a vehicle restore point in the shell in ~/backups/\r\n");
      return;
  }
  //for (i = 0; i < MAX_BAG_ROWS; i++)
  //	  cont_row[i] = NULL;
  loaded = objsave_parse_objects(fl);

  for (current = loaded; current != NULL; current=current->next)
    num_objs += vehicle_handle_objs(current->obj);

  while (loaded != NULL) {
	current = loaded;
	loaded = loaded->next;
	free(current);
  }

  fclose(fl);

  return;
}

char *fgetf( char *s, int n, register FILE *iop ) {    
	register int c;    
	register char *cs;     
	c = '\0';    
	cs = s;    
	while( --n > 0 && (c = getc(iop)) != EOF)	
		if ((*cs++ = c) == '\0')	    
			break;    *cs = '\0';    
	return((c == EOF && cs == s) ? NULL : s);
}

ACMD(do_mudbackup) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_RAW_INPUT_LENGTH];

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "To back up the mud, please type mudbackup -force to complete the process.\r\n");
		return;
	}
	if (!strcmp(arg, "-force")) {
	    FILE *fp;     
		fp = popen( "ls ~/backups/", "r" );     
		fgetf( buf, 5000, fp );     
		send_to_char(ch, "%s", buf );     
		pclose( fp );     
		return;
	}
	else
		send_to_char(ch, "Unrecognized argument.\r\n");
}