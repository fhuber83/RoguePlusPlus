#pragma once

/*
 * What the daemons and fuses do (see rules/Scheduler.hpp for when they run):
 * healing, hunger, wandering monsters, and the end of confusion, see
 * invisible, blindness and haste.
 */

namespace rogue::rules {

/*
 * doctor:
 *	A healing daemon that restores hit points after rest.
 */
void doctor();

/*
 * swander:
 *	Called when it is time to start rolling for wandering monsters.
 */
void swander();

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up.
 */
void rollwand();

/*
 * unconfuse:
 *	Release the poor player from his confusion.
 */
void unconfuse();

/*
 * unsee:
 *	Turn off the ability to see invisible.
 */
void unsee();

/*
 * sight:
 *	He gets his sight back.
 */
void sight();

/*
 * nohaste:
 *	End the hasting.
 */
void nohaste();

/*
 * stomach:
 *	Digest the hero's food.
 */
void stomach();

}  // namespace rogue::rules
