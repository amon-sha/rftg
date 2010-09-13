/*
 * Race for the Galaxy AI
 * 
 * Copyright (C) 2009 Keldon Jones
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "rftg.h"

/*
 * Card designs.
 */
design library[MAX_DESIGN];

/*
 * Names of card flags.
 */
static char *flag_name[] =
{
	"MILITARY",
	"WINDFALL",
	"START",
	"START_RED",
	"START_BLUE",
	"REBEL",
	"UPLIFT",
	"ALIEN",
	"TERRAFORMING",
	"IMPERIUM",
	"CHROMO",
	"STARTHAND_3",
	"DISCARD_TO_12",
	"GAME_END_14",
	NULL
};

/*
 * Good names (which start at cost/value 2).
 */
static char *good_name[] =
{
	"",
	"",
	"NOVELTY",
	"RARE",
	"GENE",
	"ALIEN",
	NULL
};

/*
 * Special power flag names (by phase).
 */
static char *power_name[6][32] =
{
	/* No phase zero */
	{
		NULL,
	},

	/* Phase one */
	{
		"DRAW",
		"KEEP",
		"DISCARD_ANY",
		NULL,
	},

	/* Phase two */
	{
		"DRAW",
		"REDUCE",
		"DRAW_AFTER",
		"DISCARD_REDUCE",
		NULL,
	},

	/* Phase three */
	{
		"REDUCE",
		"NOVELTY",
		"RARE",
		"GENE",
		"ALIEN",
		"DISCARD_ZERO",
		"DISCARD_MILITARY",
		"MILITARY_HAND",
		"EXTRA_MILITARY",
		"AGAINST_REBEL",
		"PER_MILITARY",
		"PAY_MILITARY",
		"PAY_DISCOUNT",
		"CONQUER_SETTLE",
		"DRAW_AFTER",
		"PLACE_TWO",
		"TAKEOVER_REBEL",
		"TAKEOVER_IMPERIUM",
		"TAKEOVER_MILITARY",
		"TAKEOVER_DEFENSE",
		NULL,
	},

	/* Phase four */
	{
		"TRADE_ANY",
		"TRADE_NOVELTY",
		"TRADE_RARE",
		"TRADE_GENE",
		"TRADE_ALIEN",
		"TRADE_THIS",
		"TRADE_BONUS_CHROMO",
		"TRADE_ACTION",
		"TRADE_NO_BONUS",
		"CONSUME_ANY",
		"CONSUME_NOVELTY",
		"CONSUME_RARE",
		"CONSUME_GENE",
		"CONSUME_ALIEN",
		"CONSUME_THIS",
		"CONSUME_TWO",
		"CONSUME_3_DIFF",
		"CONSUME_N_DIFF",
		"CONSUME_ALL",
		"GET_CARD",
		"GET_2_CARD",
		"GET_VP",
		"DRAW",
		"DRAW_LUCKY",
		"DISCARD_HAND",
		"ANTE_CARD",
		"VP",
		"RECYCLE",
		NULL,
	},

	/* Phase five */
	{
		"PRODUCE",
		"DISCARD_PRODUCE",
		"WINDFALL_ANY",
		"WINDFALL_NOVELTY",
		"WINDFALL_RARE",
		"WINDFALL_GENE",
		"WINDFALL_ALIEN",
		"NOT_THIS",
		"DRAW",
		"DRAW_IF",
		"DRAW_EACH_NOVELTY",
		"DRAW_EACH_RARE",
		"DRAW_EACH_GENE",
		"DRAW_EACH_ALIEN",
		"DRAW_WORLD_GENE",
		"DRAW_MOST_RARE",
		"DRAW_DIFFERENT",
		"DRAW_MILITARY",
		"DRAW_REBEL",
		"DRAW_CHROMO",
		NULL,
	}
};

/*
 * Special victory point flags.
 */
static char *vp_name[] =
{
	"NOVELTY_PRODUCTION",
	"RARE_PRODUCTION",
	"GENE_PRODUCTION",
	"ALIEN_PRODUCTION",
	"NOVELTY_WINDFALL",
	"RARE_WINDFALL",
	"GENE_WINDFALL",
	"ALIEN_WINDFALL",
	"DEVEL_EXPLORE",
	"WORLD_EXPLORE",
	"DEVEL_TRADE",
	"WORLD_TRADE",
	"DEVEL_CONSUME",
	"WORLD_CONSUME",
	"SIX_DEVEL",
	"DEVEL",
	"WORLD",
	"REBEL_FLAG",
	"ALIEN_FLAG",
	"TERRAFORMING_FLAG",
	"UPLIFT_FLAG",
	"IMPERIUM_FLAG",
	"CHROMO_FLAG",
	"MILITARY",
	"TOTAL_MILITARY",
	"REBEL_MILITARY",
	"THREE_VP",
	"KIND_GOOD",
	"NAME",
	NULL
};

/*
 * Lookup a power code.
 */
static int lookup_power(char *ptr, int phase)
{
	int i = 0;

	/* Loop over power names */
	while (power_name[phase][i])
	{
		/* Check this power */
		if (!strcmp(power_name[phase][i], ptr)) return 1 << i;

		/* Next effect */
		i++;
	}

	/* No match */
	printf("No power named '%s'\n", ptr);
	exit(1);
}

/*
 * Read card designs from 'cards.txt' file.
 */
void read_cards(void)
{
	FILE *fff;
	char buf[1024], *ptr;
	int num_design = 0;
	design *d_ptr = NULL;
	power *o_ptr;
	vp_bonus *v_ptr;
	int i, code, phase;

	/* Open card database */
	fff = fopen(DATADIR "/cards.txt", "r");

	/* Check for error */
	if (!fff)
	{
		/* Try reading from current directory instead */
		fff = fopen("cards.txt", "r");
	}

	/* Check for failure */
	if (!fff)
	{
		/* Print error and exit */
		perror("cards.txt");
		exit(1);
	}

	/* Loop over file */
	while (1)
	{
		/* Read a line */
		fgets(buf, 1024, fff);

		/* Check for end of file */
		if (feof(fff)) break;

		/* Strip newline */
		buf[strlen(buf) - 1] = '\0';

		/* Skip comments and blank lines */
		if (!buf[0] || buf[0] == '#') continue;

		/* Switch on type of line */
		switch (buf[0])
		{
			/* New card */
			case 'N':

				/* Current design pointer */
				d_ptr = &library[num_design];

				/* Set index */
				d_ptr->index = num_design++;

				/* Read name */
				d_ptr->name = strdup(buf + 2);
				break;

			/* Type, cost, and value */
			case 'T':

				/* Get type string */
				ptr = strtok(buf + 2, ":");

				/* Read type */
				d_ptr->type = strtol(ptr, NULL, 0);

				/* Get cost string */
				ptr = strtok(NULL, ":");

				/* Read cost */
				d_ptr->cost = strtol(ptr, NULL, 0);

				/* Get VP string */
				ptr = strtok(NULL, ":");

				/* Read VP */
				d_ptr->vp = strtol(ptr, NULL, 0);
				break;

			/* Expansion counts */
			case 'E':

				/* Get first count string */
				ptr = strtok(buf + 2, ":");

				/* Loop over number of expansions */
				for (i = 0; i < MAX_EXPANSION; i++)
				{
					/* Set count */
					d_ptr->expand[i] = strtol(ptr, NULL, 0);

					/* Read next count */
					ptr = strtok(NULL, ":");
				}

				/* Done */
				break;

			/* Flags */
			case 'F':

				/* Get first flag */
				ptr = strtok(buf + 2, " |");

				/* Loop over flags */
				while (ptr)
				{
					/* Check each flag */
					for (i = 0; flag_name[i]; i++)
					{
						/* Check this flag */
						if (!strcmp(ptr, flag_name[i]))
						{
							/* Set flag */
							d_ptr->flags |= 1 << i;
							break;
						}
					}

					/* Check for no match */
					if (!flag_name[i])
					{
						/* Error */
						printf("Unknown flag '%s'!\n",
						       ptr);

						/* Exit */
						exit(1);
					}

					/* Get next flag */
					ptr = strtok(NULL, " |");
				}

				/* Done with flag line */
				break;
			
			/* Good */
			case 'G':

				/* Get good string */
				ptr = buf + 2;

				/* Loop over goods */
				for (i = 0; good_name[i]; i++)
				{
					/* Check this good */
					if (!strcmp(ptr, good_name[i]))
					{
						/* Set good */
						d_ptr->good_type = i;
						break;
					}
				}

				/* Check for no match */
				if (!good_name[i])
				{
					/* Error */
					printf("No good name '%s'!\n", ptr);
					exit(1);
				}

				/* Done with good line */
				break;

			/* Power */
			case 'P':

				/* Get power pointer */
				o_ptr = &d_ptr->powers[d_ptr->num_power++];

				/* Get phase string */
				ptr = strtok(buf + 2, ":");

				/* Read power phase */
				phase = strtol(ptr, NULL, 0);

				/* Save phase */
				o_ptr->phase = phase;

				/* Clear power code */
				code = 0;

				/* Read power flags */
				while ((ptr = strtok(NULL, "|: ")))
				{
					/* Check for end of flags */
					if (isdigit(*ptr) ||
					    *ptr == '-') break;

					/* Lookup effect code */
					code |= lookup_power(ptr, phase);
				}

				/* Store power code */
				o_ptr->code = code;

				/* Read power's value */
				o_ptr->value = strtol(ptr, NULL, 0);

				/* Get times string */
				ptr = strtok(NULL, ":");

				/* Read power's number of times */
				o_ptr->times = strtol(ptr, NULL, 0);
				break;

			/* VP flags */
			case 'V':

				/* Get VP bonus */
				v_ptr = &d_ptr->bonuses[d_ptr->num_vp_bonus++];

				/* Get point string */
				ptr = strtok(buf + 2, ":");

				/* Read point value */
				v_ptr->point = strtol(ptr, NULL, 0);

				/* Get bonus type string */
				ptr = strtok(NULL, ":");

				/* Loop over VP bonus types */
				for (i = 0; vp_name[i]; i++)
				{
					/* Check this type */
					if (!strcmp(ptr, vp_name[i]))
					{
						/* Set type */
						v_ptr->type = i;
						break;
					}
				}

				/* Check for no match */
				if (!vp_name[i])
				{
					/* Error */
					printf("No VP type '%s'!\n", ptr);
					exit(1);
				}

				/* Get name string */
				ptr = strtok(NULL, ":");

				/* Store VP name string */
				v_ptr->name = strdup(ptr);
				break;
		}
	}

	/* Close card design file */
	fclose(fff);
}

/*
 * Rotate players one spot.
 *
 * This will be called until the player with the lowest start world is
 * player number 0.
 */
static void rotate_players(game *g)
{
	player temp, *p_ptr;
	card *c_ptr;
	int i;

	/* Store copy of player 0 */
	temp = g->p[0];

	/* Loop over players */
	for (i = 0; i < g->num_players - 1; i++)
	{
		/* Copy players one space */
		g->p[i] = g->p[i + 1];
	}

	/* Store old player 0 in last spot */
	g->p[i] = temp;

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards owned by no one */
		if (c_ptr->owner == -1) continue;

		/* Adjust owner */
		c_ptr->owner--;

		/* Check for wraparound */
		if (c_ptr->owner < 0) c_ptr->owner = g->num_players - 1;
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Notify player of rotation */
		p_ptr->control->notify_rotation(g);
	}
}

/*
 * Initialize a game.
 */
void init_game(game *g)
{
	player *p_ptr;
	design *d_ptr;
	card *c_ptr;
	int start[MAX_DECK], start_red[MAX_DECK], start_blue[MAX_DECK];
	int goal[MAX_GOAL];
	int start_picks[MAX_PLAYER][2];
	int i, j, k, n;
	int lowest = 99, low_i = -1;
	int num_start = 0, num_start_red = 0, num_start_blue = 0;
	char msg[1024];

	/* Save current random seed */
	g->start_seed = g->random_seed;

#if 0
	sprintf(msg, "start seed: %u\n", g->start_seed);
	message_add(msg);
#endif

	/* Game is not simulated */
	g->simulation = 0;

	/* Set size of VP pool */
	g->vp_pool = g->num_players * 12;

	/* First game round */
	g->round = 1;

	/* No phase or turn */
	g->cur_action = -1;
	g->turn = 0;

	/* Game is not over */
	g->game_over = 0;

	/* No cards in deck yet */
	g->deck_size = 0;

	/* Clear goals */
	for (i = 0; i < MAX_GOAL; i++)
	{
		/* Goal is not active */
		g->goal_active[i] = 0;

		/* Goal is not available */
		g->goal_avail[i] = 0;
	}

	/* Clear number of pending takeovers */
	g->num_takeover = 0;

	/* Loop over card designs */
	for (i = 0; i < MAX_DESIGN; i++)
	{
		/* Get design pointer */
		d_ptr = &library[i];

		/* Get number of cards in use */
		n = d_ptr->expand[g->expanded];

		/* Add cards */
		for (j = 0; j < n; j++)
		{
			/* Get card pointer */
			c_ptr = &g->deck[g->deck_size++];

			/* No owner */
			c_ptr->owner = -1;

			/* Put location in draw deck */
			c_ptr->where = WHERE_DECK;

			/* Card is not just drawn */
			c_ptr->temp = 0;

			/* Card's location is not known */
			c_ptr->known = 0;

			/* Clear used power list */
			for (k = 0; k < MAX_POWER; k++) c_ptr->used[k] = 0;

			/* Card has not produced */
			c_ptr->produced = 0;

			/* Set card's design */
			c_ptr->d_ptr = d_ptr;

			/* Card is not a good nor covered by one */
			c_ptr->good = 0;
			c_ptr->covered = -1;

			/* Check for start world */
			if (d_ptr->flags & FLAG_START)
			{
				/* Add to list */
				start[num_start++] = g->deck_size - 1;
			}

			/* Check for red start world */
			if (d_ptr->flags & FLAG_START_RED)
			{
				/* Add to list */
				start_red[num_start_red++] = g->deck_size - 1;
			}

			/* Check for blue start world */
			if (d_ptr->flags & FLAG_START_BLUE)
			{
				/* Add to list */
				start_blue[num_start_blue++] = g->deck_size - 1;
			}
		}
	}

	/* Add goals when expanded */
	if (g->expanded && !g->goal_disabled)
	{
		/* No goals available yet */
		n = 0;

		/* Use correct "first" goals */
		if (g->expanded == 1)
		{
			/* First expansion only */
			j = GOAL_FIRST_5_VP;
			k = GOAL_FIRST_SIX_DEVEL;
		}
		else
		{
			/* First and second expansion */
			j = GOAL_FIRST_5_VP;
			k = GOAL_FIRST_8_ACTIVE;
		}

		/* Add "first" goals to list */
		for (i = j; i <= k; i++)
		{
			/* Add goal to list */
			goal[n++] = i;
		}

		/* Select four "first" goals at random */
		for (i = 0; i < 4; i++)
		{
			/* Choose goal at random */
			j = myrand(&g->random_seed) % n;

			/* Goal is active */
			g->goal_active[goal[j]] = 1;

			/* Goal is available */
			g->goal_avail[goal[j]] = 1;

			/* Remove chosen goal from list */
			goal[j] = goal[--n];
		}

		/* No goals available yet */
		n = 0;

		/* Use correct "most" goals */
		if (g->expanded == 1)
		{
			/* First expansion only */
			j = GOAL_MOST_MILITARY;
			k = GOAL_MOST_PRODUCTION;
		}
		else
		{
			/* First and second expansion */
			j = GOAL_MOST_MILITARY;
			k = GOAL_MOST_REBEL;
		}

		/* Add "most" goals to list */
		for (i = j; i <= k; i++)
		{
			/* Add goal to list */
			goal[n++] = i;
		}

		/* Select two "most" goals at random */
		for (i = 0; i < 2; i++)
		{
			/* Choose goal at random */
			j = myrand(&g->random_seed) % n;

			/* Goal is active */
			g->goal_active[goal[j]] = 1;

			/* Goal is available */
			g->goal_avail[goal[j]] = 1;

			/* Remove chosen goal from list */
			goal[j] = goal[--n];
		}
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Clear all claimed goals */
		for (j = 0; j < MAX_GOAL; j++)
		{
			/* Goal is unclaimed */
			p_ptr->goal_claimed[j] = 0;

			/* No progress toward goal */
			p_ptr->goal_progress[j] = 0;
		}

		/* Player has no actions chosen */
		p_ptr->action[0] = p_ptr->prev_action[0] = -1;
		p_ptr->action[1] = p_ptr->prev_action[1] = -1;

		/* Player has not used phase bonus */
		p_ptr->phase_bonus_used = 0;

		/* Player has no card to be placed */
		p_ptr->placing = -1;

		/* Player has no bonus military accrued */
		p_ptr->bonus_military = 0;

		/* Player has not discarded any end of turn cards */
		p_ptr->end_discard = 0;

		/* No cards played yet */
		p_ptr->table_order = 0;

		/* Player has no points */
		p_ptr->vp = p_ptr->goal_vp = p_ptr->end_vp = 0;

		/* Player is not the winner */
		p_ptr->winner = 0;

		/* Player has earned no cards/VP this phase */
		p_ptr->phase_cards = p_ptr->phase_vp = 0;

		/* Player has no fake cards */
		p_ptr->fake_hand = p_ptr->total_fake = 0;
		p_ptr->fake_played_dev = p_ptr->fake_played_world = 0;
	       	p_ptr->fake_discards = 0;
	}

	/* Check for second (or later) expansion */
	if (g->expanded >= 2)
	{
		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Choose a Red start world */
			n = myrand(&g->random_seed) % num_start_red;

			/* Add to start world choices */
			start_picks[i][0] = start_red[n];

			/* Collapse list */
			start_red[n] = start_red[--num_start_red];

			/* Choose a Blue start world */
			n = myrand(&g->random_seed) % num_start_blue;

			/* Add to start world choices */
			start_picks[i][1] = start_blue[n];

			/* Collapse list */
			start_blue[n] = start_blue[--num_start_blue];

			/* Get card pointer to first start choice */
			c_ptr = &g->deck[start_picks[i][0]];

			/* XXX Move card to discard */
			c_ptr->where = WHERE_DISCARD;

			/* Get card pointer to second start choice */
			c_ptr = &g->deck[start_picks[i][1]];

			/* XXX Move card to discard */
			c_ptr->where = WHERE_DISCARD;
		}

		/* Loop over players again */
		for (i = 0; i < g->num_players; i++)
		{
			/* Get player pointer */
			p_ptr = &g->p[i];

			/* Give player six cards */
			draw_cards(g, i, 6);

			/* Ask player which start world they want */
			j = p_ptr->control->choose_start(g, i, start_picks[i],
			                                 2);

			/* Check for aborted game */
			if (g->game_over) return;

			/* Remember start world */
			p_ptr->start = j;

			/* XXX Place start world (for now) */
			g->deck[j].where = WHERE_ACTIVE;
			g->deck[j].owner = i;

			/* Assume player gets four cards */
			n = 4;

			/* Get player's start world */
			c_ptr = &g->deck[p_ptr->start];

			/* Check for starting with less */
			if (c_ptr->d_ptr->flags & FLAG_STARTHAND_3) n = 3;

			/* Have player discard down */
			discard_to(g, i, n, 1);

			/* XXX Remove start world so others don't see */
			g->deck[j].where = WHERE_DISCARD;

			/* Check for aborted game */
			if (g->game_over) return;
		}

		/* Loop over players one last time */
		for (i = 0; i < g->num_players; i++)
		{
			/* Place player's chosen start world */
			place_card(g, i, g->p[i].start);
		}
	}
	else
	{
		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Choose a start world number */
			n = myrand(&g->random_seed) % num_start;

			/* Chosen world to player */
			place_card(g, i, start[n]);

			/* Remember start world */
			g->p[i].start = start[n];

			/* Collapse list */
			start[n] = start[--num_start];
		}

		/* Loop over players again */
		for (i = 0; i < g->num_players; i++)
		{
			/* Give player six cards */
			draw_cards(g, i, 6);
		}
	}

	/* Find lowest numbered start world */
	for (i = 0; i < g->num_players; i++)
	{
		/* Check for lower number */
		if (g->p[i].start < lowest)
		{
			/* Remember lowest number */
			lowest = g->p[i].start;
			low_i = i;
		}
	}

	/* Rotate players until player 0 holds lowest start world */
	for (i = 0; i < low_i; i++) rotate_players(g);

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Get player's start world */
		c_ptr = &g->deck[p_ptr->start];

		/* Format message */
		sprintf(msg, "%s starts with %s.\n", p_ptr->name,
		        c_ptr->d_ptr->name);

		/* Send message */
		message_add(msg);
	}

	/* Loop over players */
	for (i = 0; i < g->num_players; i++)
	{
		/* Get player pointer */
		p_ptr = &g->p[i];

		/* Assume player gets four cards */
		n = 4;

		/* Get player's start world */
		c_ptr = &g->deck[p_ptr->start];

		/* Check for starting with less */
		if (c_ptr->d_ptr->flags & FLAG_STARTHAND_3) n = 3;

		/* Have player discard down */
		discard_to(g, i, n, 1);

		/* Check for aborted game */
		if (g->game_over) return;
	}

	/* Clear temporary flags on drawn cards */
	clear_temp(g);

	/* XXX Pretend settle phase to set goal progress properly */
	g->cur_action = ACT_SETTLE;
	check_goals(g);
	g->cur_action = -1;
}