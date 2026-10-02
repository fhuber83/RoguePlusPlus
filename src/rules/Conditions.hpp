#pragma once

/*
 * The fuses that end a condition (see rules/Scheduler.hpp for when they
 * run): confusion, seeing invisible, blindness and haste.
 */

namespace rogue::rules {

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

}  // namespace rogue::rules
